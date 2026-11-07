/**
 * @file    make_tof_ud_param.c
 * @brief   Analysis TOF U-D (Up/Down TDC) calibration
 * @author  Akao Ryogo
 * @date    2026-10-29
 *
 * Change history:
 *   2026-10-29  Akao Ryogo  First version
 *   2026-11-08  Akao Ryogo  Fit the raw TDC peak directly and write it as
 *                           p0; no longer depends on an existing HodoParam
 *                           file (same convention as make_rf_param.C /
 *                           update_T0_param.C).
 */
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLine.h>
#include <TMath.h>
#include <TSystem.h>
#include <TFitResult.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

static const int RUN = 7013;


static const double TDC_P1 = -0.0009765625; 

static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }


bool fitPeak(TH1D* h, int seg, const char* tag, int min_ent, double nsigma,
             double& mean_out, double& sigma_out)
{
    mean_out = 0.0;
    sigma_out = 0.0;

    if (h->GetEntries() < min_ent) {
        int mbin = h->GetMaximumBin();
        mean_out = h->GetBinCenter(mbin);
        printf("[WARN] Seg %2d %s: entries=%6.0f < %d  -> skip, use max-bin=%.0f counts\n",
               seg, tag, h->GetEntries(), min_ent, mean_out);
        return false;
    }

    int    max_bin = h->GetMaximumBin();
    double peak     = h->GetBinCenter(max_bin);
    double max_val  = h->GetBinContent(max_bin);
    mean_out = peak;
    if (max_val <= 0) return false;

    double half   = max_val * 0.5;
    int    nbins  = h->GetNbinsX();
    int    lo_bin = max_bin;
    while (lo_bin > 1     && h->GetBinContent(lo_bin) > half) lo_bin--;
    int    hi_bin = max_bin;
    while (hi_bin < nbins && h->GetBinContent(hi_bin) > half) hi_bin++;
    double fwhm = h->GetBinCenter(hi_bin) - h->GetBinCenter(lo_bin);
    if (fwhm <= 0) fwhm = 10.0 * h->GetBinWidth(max_bin);

    h->GetListOfFunctions()->Clear();
    TF1* gf1 = new TF1(Form("gf1_%s_seg%d", tag, seg), "gaus", peak - fwhm, peak + fwhm);
    gf1->SetParameter(1, peak);
    TFitResultPtr r1 = h->Fit(gf1, "RSQ+");
    if (!r1.Get() || r1->Status() != 0) {
        printf("[WARN] Seg %2d %s: pass1 fit failed -> max-bin=%.0f counts\n", seg, tag, peak);
        return false;
    }

    double mean1  = r1->Parameter(1);
    double sigma1 = TMath::Abs(r1->Parameter(2));
    mean_out  = mean1;
    sigma_out = sigma1;

    TF1* gf2 = new TF1(Form("gf2_%s_seg%d", tag, seg), "gaus",
                        mean1 - nsigma * sigma1, mean1 + nsigma * sigma1);
    gf2->SetParameter(1, mean1);
    TFitResultPtr r2 = h->Fit(gf2, "RSQ+");
    if (r2.Get() && r2->IsValid() && r2->Status() == 0) {
        mean_out  = r2->Parameter(1);
        sigma_out = TMath::Abs(r2->Parameter(2));
    }

    printf("[OK  ] Seg %2d %s: mean=%10.1f counts  sigma=%6.1f counts  (%.3f ns)\n",
           seg, tag, mean_out, sigma_out, sigma_out * TMath::Abs(TDC_P1));

    TLine* zero = new TLine(mean_out, 0, mean_out, h->GetMaximum() * 0.3);
    zero->SetLineColor(kBlue); zero->SetLineStyle(2);
    zero->Draw("same");

    return true;
}

//main
void make_tof_ud_param(
    const char* root_file  = nullptr,
    const char* param_out  = nullptr,
    int    min_ent    = 100,
    double fit_nsigma = 2.5,     
    double range_margin_ns = 20.0,  
    int    hist_bin    = 2000
) {
    const int    N_SEG    = 48;
    const int    DET_ID   = 14;
    const int    MAXDEPTH = 16;
    const int    MIN_ENT  = min_ent;
    const double NSIGMA   = fit_nsigma;
    const int    HIST_BIN = hist_bin;
    const double RANGE_MARGIN = range_margin_ns / TMath::Abs(TDC_P1);  // -> raw TDC counts

    TString rf = root_file ? TString(root_file)
                           : base_path(Form("root/run%05d/run%05d_Hodoscope.root", RUN, RUN));
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString po = param_out ? TString(param_out)
                           : base_path(Form("output/txt/run%05d/TOF_TDC_param_run%05d.txt", run_num, run_num));
    TString pdf_up = base_path(Form("output/pdf/run%05d/tof_ud_calib_up_run%05d.pdf", run_num, run_num));
    TString pdf_dn = base_path(Form("output/pdf/run%05d/tof_ud_calib_dn_run%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);
    gSystem->mkdir(gSystem->DirName(pdf_up), kTRUE);

    printf("=== make_tof_ud_param  run %05d ===\n", run_num);
    printf("Root  : %s\n", rf.Data());
    printf("Out   : %s\n\n", po.Data());

    TFile* f = TFile::Open(rf);
    if (!f || f->IsZombie()) {
        std::cerr << "[ERROR] Cannot open " << rf.Data() << std::endl;
        return;
    }
    TTree* tr = (TTree*)f->Get("tree");
    if (!tr) {
        std::cerr << "[ERROR] TTree 'tree' not found in " << rf.Data() << std::endl;
        f->Close();
        return;
    }

    Double_t tofut[N_SEG][MAXDEPTH];
    Double_t tofdt[N_SEG][MAXDEPTH];
    tr->SetBranchStatus("*", 0);
    tr->SetBranchStatus("tofut", 1);
    tr->SetBranchStatus("tofdt", 1);
    tr->SetBranchAddress("tofut", tofut);
    tr->SetBranchAddress("tofdt", tofdt);

    Long64_t nev   = tr->GetEntries();
    Long64_t nscan = nev < 200000 ? nev : 200000;
    double vmin_up =  1e18, vmax_up = -1e18;
    double vmin_dn =  1e18, vmax_dn = -1e18;
    printf("[0/2] Scanning %lld events for raw-TDC range ...\n", nscan);
    for (Long64_t ev = 0; ev < nscan; ev++) {
        tr->GetEntry(ev);
        for (int seg = 0; seg < N_SEG; seg++) {
            for (int d = 0; d < MAXDEPTH; d++) {
                double u = tofut[seg][d];
                if (TMath::Finite(u) && u > 0) {
                    if (u < vmin_up) vmin_up = u;
                    if (u > vmax_up) vmax_up = u;
                }
                double dn = tofdt[seg][d];
                if (TMath::Finite(dn) && dn > 0) {
                    if (dn < vmin_dn) vmin_dn = dn;
                    if (dn > vmax_dn) vmax_dn = dn;
                }
            }
        }
    }
    if (vmin_up > vmax_up || vmin_dn > vmax_dn) {
        std::cerr << "[ERROR] No valid tofut/tofdt values found in first " << nscan << " events." << std::endl;
        f->Close();
        return;
    }
    double hlo_up = TMath::Max(0.0, vmin_up - RANGE_MARGIN), hhi_up = vmax_up + RANGE_MARGIN;
    double hlo_dn = TMath::Max(0.0, vmin_dn - RANGE_MARGIN), hhi_dn = vmax_dn + RANGE_MARGIN;
    printf("  UP   range: %.0f - %.0f\n", hlo_up, hhi_up);
    printf("  DOWN range: %.0f - %.0f\n\n", hlo_dn, hhi_dn);

    TH1D* h_up[N_SEG];
    TH1D* h_dn[N_SEG];
    for (int seg = 0; seg < N_SEG; seg++) {
        h_up[seg] = new TH1D(Form("h_up_seg%d", seg),
                             Form("TOF seg%d UP;raw TDC [count];Counts", seg),
                             HIST_BIN, hlo_up, hhi_up);
        h_dn[seg] = new TH1D(Form("h_dn_seg%d", seg),
                             Form("TOF seg%d DOWN;raw TDC [count];Counts", seg),
                             HIST_BIN, hlo_dn, hhi_dn);
    }

    printf("[1/2] Reading %lld events from TTree ...\n", nev);
    for (Long64_t ev = 0; ev < nev; ev++) {
        tr->GetEntry(ev);
        for (int seg = 0; seg < N_SEG; seg++) {
            for (int d = 0; d < MAXDEPTH; d++) {
                double raw_u = tofut[seg][d];
                double raw_d = tofdt[seg][d];
                if (TMath::Finite(raw_u) && raw_u > 0) h_up[seg]->Fill(raw_u);
                if (TMath::Finite(raw_d) && raw_d > 0) h_dn[seg]->Fill(raw_d);
            }
        }
    }


    gStyle->SetOptFit(111);
    gStyle->SetOptStat(1000010);

    TCanvas* c_up = new TCanvas("c_up", "TOF UP TDC Fit",   1800, 2400);
    c_up->Divide(6, 8);
    TCanvas* c_dn = new TCanvas("c_dn", "TOF DOWN TDC Fit", 1800, 2400);
    c_dn->Divide(6, 8);

    double p0_up[N_SEG], p0_dn[N_SEG];
    double sg_up[N_SEG], sg_dn[N_SEG];
    bool   ok_up[N_SEG], ok_dn[N_SEG];

    printf("\n[2/2] --- UP channel fits ---\n");
    for (int seg = 0; seg < N_SEG; seg++) {
        c_up->cd(seg + 1);
        h_up[seg]->Draw();
        ok_up[seg] = fitPeak(h_up[seg], seg, "UP", MIN_ENT, NSIGMA, p0_up[seg], sg_up[seg]);
    }

    printf("\n--- DOWN channel fits ---\n");
    for (int seg = 0; seg < N_SEG; seg++) {
        c_dn->cd(seg + 1);
        h_dn[seg]->Draw();
        ok_dn[seg] = fitPeak(h_dn[seg], seg, "DN", MIN_ENT, NSIGMA, p0_dn[seg], sg_dn[seg]);
    }

    c_up->SaveAs(pdf_up.Data());
    c_dn->SaveAs(pdf_dn.Data());
    printf("\n[INFO] Saved: %s\n", pdf_up.Data());
    printf("[INFO] Saved: %s\n",   pdf_dn.Data());

    printf("\n%-4s %-13s %-9s  %-13s %-9s  %s\n",
           "seg", "p0_U[count]", "sig[ns]", "p0_D[count]", "sig[ns]", "src");
    printf("%s\n", std::string(70, '-').c_str());
    for (int seg = 0; seg < N_SEG; seg++) {
        printf("%3d  %11.1f  %7.3f   %11.1f  %7.3f  [%s/%s]\n",
               seg, p0_up[seg], sg_up[seg] * TMath::Abs(TDC_P1),
               p0_dn[seg], sg_dn[seg] * TMath::Abs(TDC_P1),
               ok_up[seg] ? "fit" : "est",
               ok_dn[seg] ? "fit" : "est");
    }

    // output param file
    po.ReplaceAll("//", "/");
    std::ofstream fout(po.Data());
    if (!fout.is_open()) {
        std::cerr << "[ERROR] Cannot write " << po.Data() << std::endl;
        f->Close();
        return;
    }

    fout << "# TOF TDC Up/Down (CId=14, aort=1): p0=fitted raw TDC peak, p1=fixed TDC slope\n";
    for (int seg = 0; seg < N_SEG; seg++)
        fout << Form("14\t0\t%d\t1\t0\t%.6f\t%.11f\n", seg, p0_up[seg], TDC_P1);
    for (int seg = 0; seg < N_SEG; seg++)
        fout << Form("14\t0\t%d\t1\t1\t%.6f\t%.11f\n", seg, p0_dn[seg], TDC_P1);
    fout.close();

    printf("\n[INFO] -> Output param: %s\n", po.Data());

    f->Close();
}
