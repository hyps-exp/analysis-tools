/**
 * @file    make_TOF_ADC_param.c
 * @brief   Analysis TOF ADC calibration
 * @author  Akao Ryogo
 * @date    2026-10-29
 *
 * Change history:
 *   2026-10-29  Akao Ryogo  First version
 */
//
// Usage (from analysis_tools/):
//   root -l -b -q param-maker/TOF/make_TOF_ADC_param.C

#include <TFile.h>
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TFitResult.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

static const int    RUN            = 7013;
static const int    NSEG           = 48;
static const int    CID_TOF        = 14;
static const int    AT_ADC         = 0;     // kAdcHigh
static const double PED_HALF_WIDTH = 100.0; 
static const double MIP_MARGIN_LO  = 300.0; 
static const double MIP_MARGIN_HI  = 600.0; 
static const double MIP_ABOVE_PED  = 200.0; 


static const double TMPL_P0 = 100.;
static const double TMPL_P1 = 1000.;

// analysis_tools/ root directory: two levels above this macro's own directory
// (param-maker/TOF/make_TOF_ADC_param.C -> param-maker/TOF -> param-maker -> analysis_tools).
static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

struct ADCPar {
    double p0, p1;
    bool p0_ok, p1_ok;
};


struct MIPConf { double lo, hi; };

// pedestal fit
static double fit_pedestal(TH1* h, double fallback, bool& ok)
{
    ok = false;
    if (!h) return fallback;
    int mbin = h->GetMaximumBin();
    double xpk = h->GetBinCenter(mbin);
    TFitResultPtr r = h->Fit("gaus", "QRS", "", xpk - PED_HALF_WIDTH, xpk + PED_HALF_WIDTH);
    if (r.Get() && r->Status() == 0) { ok = true; return r->Parameter(1); }
    return xpk;  
}

// MIP fit (Landau)
static double fit_mip(TH1* h, double pedestal, double fallback, bool& ok,
                      double ovr_lo = 0., double ovr_hi = 0.)
{
    ok = false;
    if (!h) return fallback;

    double fit_lo, fit_hi, xpeak;

    if (ovr_lo > 0. && ovr_hi > 0.) {
        int b0 = h->FindBin(ovr_lo), b1 = h->FindBin(ovr_hi);
        int mbin = b0; double max_val = 0;
        for (int b = b0; b <= b1; b++) {
            if (h->GetBinContent(b) > max_val) { max_val = h->GetBinContent(b); mbin = b; }
        }
        if (max_val <= 0) return fallback;
        xpeak  = h->GetBinCenter(mbin);
        fit_lo = ovr_lo;
        fit_hi = ovr_hi;
    } else {
        double xlo_search = pedestal + MIP_ABOVE_PED;
        int bin_lo = h->FindBin(xlo_search);
        int mbin = bin_lo; double max_val = 0;
        for (int b = bin_lo; b <= h->GetNbinsX(); b++) {
            if (h->GetBinContent(b) > max_val) { max_val = h->GetBinContent(b); mbin = b; }
        }
        if (max_val <= 0) return fallback;
        xpeak  = h->GetBinCenter(mbin);
        fit_lo = std::max(xpeak - MIP_MARGIN_LO, xlo_search);
        fit_hi = xpeak + MIP_MARGIN_HI;
    }

    TF1* fl = new TF1("_landau_tmp", "landau", fit_lo, fit_hi);
    fl->SetParameter(1, xpeak);
    fl->SetParameter(2, xpeak * 0.08);

    TFitResultPtr r = h->Fit(fl, "QRS", "", fit_lo, fit_hi);
    double mpv = fl->GetParameter(1);
    delete fl;

    if (r.Get() && r->Status() == 0 && mpv > pedestal + 100. && mpv < 4096.) {
        ok = true;
        return mpv;
    }
    return fallback;
}

void make_TOF_ADC_param(
    const char* root_file = nullptr,
    const char* param_out = nullptr,
    const char* fit_conf  = nullptr)   
                                        
{
    TString rf  = root_file ? TString(root_file)
                            : base_path(Form("root/run%05d/run%05d_Hodoscope.root", RUN, RUN));

    
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString po  = param_out ? TString(param_out)
                            : base_path(Form("output/run%05d/TOF_ADC_param_run%05d.txt", run_num, run_num));
    TString pdf = base_path(Form("output/run%05d/make_TOF_ADC_param_%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);
    gSystem->mkdir(gSystem->DirName(pdf), kTRUE);

    TFile* f = TFile::Open(rf);
    if (!f || f->IsZombie()) {
        fprintf(stderr, "[ERROR] Cannot open %s\n", rf.Data());
        return;
    }
    printf("=== make_TOF_ADC_param  run %05d ===\n", run_num);
    printf("Root  : %s\n",   rf.Data());
    printf("Out   : %s\n\n", po.Data());

    gStyle->SetOptStat(1);
    gStyle->SetOptFit(1);

    
    ADCPar res_up[NSEG], res_dn[NSEG];
    for (int s = 0; s < NSEG; s++) {
        res_up[s] = {TMPL_P0, TMPL_P1, false, false};
        res_dn[s] = {TMPL_P0, TMPL_P1, false, false};
    }

    MIPConf mip_up[NSEG], mip_dn[NSEG];
    for (int s = 0; s < NSEG; s++) mip_up[s] = mip_dn[s] = {0., 0.};
    {
        bool    fc_is_default = (fit_conf == nullptr || strlen(fit_conf) == 0);
        TString fc_path = fc_is_default ? base_path("fit_range/TOF/tof_mip_fit_range.txt")
                                         : TString(fit_conf);
        std::ifstream fc(fc_path.Data());
        if (!fc) {
            if (fc_is_default)
                printf("[INFO] No MIP fit range conf at %s; using auto-detection\n", fc_path.Data());
            else
                fprintf(stderr, "[WARN] Cannot open fit_conf: %s  (ignored)\n", fc_path.Data());
        } else {
            std::string ln;
            int n_override = 0;
            while (std::getline(fc, ln)) {
                if (ln.empty() || ln[0] == '#') continue;
                std::istringstream iss(ln);
                int seg, ud; double lo, hi;
                if (!(iss >> seg >> ud >> lo >> hi)) continue;
                if (seg < 0 || seg >= NSEG) continue;
                if (ud == 0) mip_up[seg] = {lo, hi};
                else         mip_dn[seg] = {lo, hi};
                n_override++;
            }
            printf("[INFO] MIP fit conf: %s  (%d overrides)\n", fc_path.Data(), n_override);
        }
    }

    { TCanvas* c = new TCanvas("ctmp","",1,1); c->Print(pdf+"[","pdf"); delete c; }

    const int SEGS_PER_PAGE = 8;
    int nPages = (NSEG + SEGS_PER_PAGE - 1) / SEGS_PER_PAGE;


    printf("--- pedestal fit (id=7/8) ---\n");
    printf("%-5s  %-4s  %10s  %s\n", "Seg","U/D","pedestal","status");
    printf("%s\n", std::string(32, '-').c_str());

    for (int page = 0; page < nPages; page++) {
        int seg0 = page * SEGS_PER_PAGE;
        int seg1 = std::min(seg0 + SEGS_PER_PAGE, NSEG);
        int nseg = seg1 - seg0;

        TCanvas* c = new TCanvas(Form("c_ped_%d", page),
                                 Form("Pedestal seg%d-%d", seg0, seg1-1),
                                 1600, 800);
        c->Divide(nseg, 2, 0.005, 0.005);

        for (int k = 0; k < nseg; k++) {
            int seg   = seg0 + k;
            int hbase = 80000 + (seg + 1) * 100;

            // Up (id=7: w/o TDC — pure pedestal events)
            c->cd(k + 1);
            gPad->SetLogy(); gPad->SetLeftMargin(0.15); gPad->SetBottomMargin(0.14);
            TH1* h = (TH1*)f->Get(Form("h%d", hbase + 7));
            bool ok = false;
            res_up[seg].p0 = fit_pedestal(h, res_up[seg].p0, ok);
            res_up[seg].p0_ok = ok;
            if (h) { h->SetTitle(Form("seg%d Up ped=%.1f", seg, res_up[seg].p0)); h->Draw(); }
            printf("%-5d  Up    %10.3f  %s\n", seg, res_up[seg].p0, ok ? "fit" : "max");

            // Down (id=8: w/o TDC — pure pedestal events)
            c->cd(nseg + k + 1);
            gPad->SetLogy(); gPad->SetLeftMargin(0.15); gPad->SetBottomMargin(0.14);
            h = (TH1*)f->Get(Form("h%d", hbase + 8));
            ok = false;
            res_dn[seg].p0 = fit_pedestal(h, res_dn[seg].p0, ok);
            res_dn[seg].p0_ok = ok;
            if (h) { h->SetTitle(Form("seg%d Down ped=%.1f", seg, res_dn[seg].p0)); h->Draw(); }
            printf("%-5d  Down  %10.3f  %s\n", seg, res_dn[seg].p0, ok ? "fit" : "max");
        }
        c->Print(pdf, "pdf");
        delete c;
    }

  
    printf("\n--- MIPfit (id=5/6, Landau) ---\n");
    printf("%-5s  %-4s  %10s  %s\n", "Seg","U/D","MIP peak","status");
    printf("%s\n", std::string(32, '-').c_str());

    for (int page = 0; page < nPages; page++) {
        int seg0 = page * SEGS_PER_PAGE;
        int seg1 = std::min(seg0 + SEGS_PER_PAGE, NSEG);
        int nseg = seg1 - seg0;

        TCanvas* c = new TCanvas(Form("c_mip_%d", page),
                                 Form("MIP seg%d-%d", seg0, seg1-1),
                                 1600, 800);
        c->Divide(nseg, 2, 0.005, 0.005);

        for (int k = 0; k < nseg; k++) {
            int seg   = seg0 + k;
            int hbase = 80000 + (seg + 1) * 100;

            // Up (id=5)
            c->cd(k + 1);
            gPad->SetLogy(); gPad->SetLeftMargin(0.15); gPad->SetBottomMargin(0.14);
            TH1* h = (TH1*)f->Get(Form("h%d", hbase + 5));
            bool ok = false;
            res_up[seg].p1 = fit_mip(h, res_up[seg].p0, res_up[seg].p1, ok,
                                     mip_up[seg].lo, mip_up[seg].hi);
            res_up[seg].p1_ok = ok;
            if (h) { h->SetTitle(Form("seg%d Up MIP=%.1f", seg, res_up[seg].p1)); h->Draw(); }
            printf("%-5d  Up    %10.3f  %s\n", seg, res_up[seg].p1, ok ? "fit" : "template");

            // Down (id=6)
            c->cd(nseg + k + 1);
            gPad->SetLogy(); gPad->SetLeftMargin(0.15); gPad->SetBottomMargin(0.14);
            h = (TH1*)f->Get(Form("h%d", hbase + 6));
            ok = false;
            res_dn[seg].p1 = fit_mip(h, res_dn[seg].p0, res_dn[seg].p1, ok,
                                     mip_dn[seg].lo, mip_dn[seg].hi);
            res_dn[seg].p1_ok = ok;
            if (h) { h->SetTitle(Form("seg%d Down MIP=%.1f", seg, res_dn[seg].p1)); h->Draw(); }
            printf("%-5d  Down  %10.3f  %s\n", seg, res_dn[seg].p1, ok ? "fit" : "template");
        }
        c->Print(pdf, "pdf");
        delete c;
    }

    // Summary
    int n_p0 = 0, n_p1 = 0;
    for (int s = 0; s < NSEG; s++) {
        if (res_up[s].p0_ok) n_p0++;
        if (res_dn[s].p0_ok) n_p0++;
        if (res_up[s].p1_ok) n_p1++;
        if (res_dn[s].p1_ok) n_p1++;
    }
    printf("\nSummary: pedestal %d/%d fitted, MIP %d/%d fitted\n",
           n_p0, 2*NSEG, n_p1, 2*NSEG);

    // TOF ADC param output
    {
        po.ReplaceAll("//", "/");
        std::ofstream fout(po.Data());
        if (!fout) {
            fprintf(stderr, "[ERROR] Cannot write %s\n", po.Data());
        } else {
            fout << "# TOF ADC Up  (p0=pedestal Gaussian fit, p1=MIP Landau fit)\n";
            for (int s = 0; s < NSEG; s++)
                fout << Form("14\t0\t%d\t0\t0\t%.6f\t%.6f\n",
                             s, res_up[s].p0, res_up[s].p1);
            fout << "# TOF ADC Down  (p0=pedestal Gaussian fit, p1=MIP Landau fit)\n";
            for (int s = 0; s < NSEG; s++)
                fout << Form("14\t0\t%d\t0\t1\t%.6f\t%.6f\n",
                             s, res_dn[s].p0, res_dn[s].p1);
            fout.close();
            printf("[INFO] Wrote %s\n", po.Data());
        }
    }

    { TCanvas* c = new TCanvas("cend","",1,1); c->Print(pdf+"]","pdf"); delete c; }
    f->Close();
    printf("[INFO] -> %s\n", pdf.Data());
    printf("=== Done ===\n");
}
