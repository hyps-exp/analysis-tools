// update_T0_param.C
// Fit T0 detector (CId=11) calibration parameters from Hodoscope.root and
// dump the result to a PDF (fit plots) and a txt file (T0-only HDPRM lines).
//
// Ported from hyps-analyzer/dev/macro/DC_param_make/update_T0_param.C for the
// analysis_tools/param-maker/T0 layout. Unlike the original, this version does
// NOT read/merge an existing HDPRM file: it only ever writes the T0 (CId=11)
// lines it just fit, seeded from hardcoded fallback values below.
//
// T0 histograms (Hodoscope.root, T0Hid=50000, seg=0):
//   h50101  T0 ADC Up  (Left,  all events)
//   h50102  T0 ADC Down(Right, all events)
//   h50103  T0 TDC Up  (Left)
//   h50104  T0 TDC Down(Right)
//
// ADC p0 (pedestal) is fit from a dedicated pedestal-run file (ped_root_file);
// ADC p1 (MIP) and TDC p0 are fit from the physics-run file (root_file).
//
// TDC: by default the peak is found automatically (global maximum bin), then
//      fit with a two-pass Gaussian (rough FWHM-based window, then refined to
//      mean +/- N*sigma). Optionally pass tdc_range_str to fix the fit range
//      instead, as a comma-separated list of 4 values (no spaces):
//        fit_lo_up,fit_lo_dn,fit_hi_up,fit_hi_dn
//      The Gaussian is then fit once within [lo,hi] (seeded from the maximum
//      bin inside that window).
// ADC: pedestal/MIP fit ranges are NOT hardcoded; pass them via adc_range_str
//      as a comma-separated list of 8 values (no spaces):
//        ped_lo_up,ped_lo_dn,ped_hi_up,ped_hi_dn,mip_lo_up,mip_lo_dn,mip_hi_up,mip_hi_dn
//      If omitted, defaults matching the previous hardcoded values are used.
//
// Output txt lines (CId=11, PlId=0, Seg=0):
//   11  0  0  0  ud  p0_ped  p1_mip    (ADC: at=0)
//   11  0  0  1  ud  p0_tdc  p1_slope  (TDC: at=1, p1 slope is fixed hardware value)
//
// Usage (from analysis_tools/):
//   root -l -b -q param-maker/T0/update_T0_param.C
//   root -l -b -q 'param-maker/T0/update_T0_param.C("root/run07013/run07013_Hodoscope.root")'

#include <TFile.h>
#include <TH1.h>
#include <TF1.h>
#include <TList.h>
#include <TObject.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TFitResult.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const int RUN         = 7013;
static const int CID_T0      = 11;
static const int T0_HID_BASE = 50100;  // 50000 + (seg+1)*100 for seg=0

// Default ADC fit ranges [0]=Up(Left) [1]=Down(Right), used when
// adc_range_str is not given.
static const double DEFAULT_ADC_PED_LO[2] = {  150.,  180. };
static const double DEFAULT_ADC_PED_HI[2] = {  250.,  300. };
static const double DEFAULT_ADC_MIP_LO[2] = {  330.,  430. };
static const double DEFAULT_ADC_MIP_HI[2] = {  430.,  530. };

// Fallback template values, used only when a fit fails.
static const double TMPL_ADC_P0[2] = {       100.,          100. };
static const double TMPL_ADC_P1[2] = {       500.,          500. };
static const double TMPL_TDC_P0[2] = {    671537.,       671331. };
static const double TMPL_TDC_P1[2] = { -0.00097691419, -0.00097691419 }; // fixed hardware slope

// ----------------------------------------------------------------
// Automatic peak-find + two-pass Gaussian fit (used for TDC, where the
// peak position can shift run to run and a fixed range is brittle).
// Pass 1: fit within peak +/- FWHM (estimated from the raw histogram).
// Pass 2: refine within fitted_mean +/- nsigma*fitted_sigma.
// ----------------------------------------------------------------
struct PeakFitResult {
    bool   ok       = false;
    double mean     = 0.;
    double sigma    = 0.;
    double range_lo = 0.;
    double range_hi = 0.;
};

static PeakFitResult AutoPeakGausFit(TH1* h, double nsigma = 2.5)
{
    PeakFitResult res;
    if (!h) return res;

    int    nbins   = h->GetNbinsX();
    int    max_bin = h->GetMaximumBin();
    double peak    = h->GetBinCenter(max_bin);
    double max_val = h->GetBinContent(max_bin);
    res.mean = peak;   // fallback if everything below fails
    if (max_val <= 0) return res;

    double half   = max_val * 0.5;
    int    lo_bin = max_bin;
    while (lo_bin > 1     && h->GetBinContent(lo_bin) > half) lo_bin--;
    int    hi_bin = max_bin;
    while (hi_bin < nbins && h->GetBinContent(hi_bin) > half) hi_bin++;
    double fwhm = h->GetBinCenter(hi_bin) - h->GetBinCenter(lo_bin);
    if (fwhm <= 0) fwhm = 10. * h->GetBinWidth(max_bin);

    double lo1 = peak - fwhm, hi1 = peak + fwhm;
    h->GetListOfFunctions()->Clear();
    TFitResultPtr r1 = h->Fit("gaus", "R+S+Q", "", lo1, hi1);
    res.range_lo = lo1; res.range_hi = hi1;
    if (!r1.Get() || r1->Status() != 0) return res;   // keep raw peak as fallback

    double mean1 = r1->Parameter(1), sigma1 = r1->Parameter(2);
    res.ok = true; res.mean = mean1; res.sigma = sigma1;

    double lo2 = mean1 - nsigma * sigma1, hi2 = mean1 + nsigma * sigma1;
    TFitResultPtr r2 = h->Fit("gaus", "R+S", "", lo2, hi2);
    if (r2.Get() && r2->Status() == 0) {
        res.mean = r2->Parameter(1); res.sigma = r2->Parameter(2);
        res.range_lo = lo2; res.range_hi = hi2;
    }
    return res;
}

// ----------------------------------------------------------------
// Single-pass Gaussian fit within a user-specified fixed range [lo,hi]
// (used for TDC when the fit range is given via tdc_range_str).
// The peak seed is the maximum bin inside the window.
// ----------------------------------------------------------------
static PeakFitResult FixedRangeGausFit(TH1* h, double lo, double hi)
{
    PeakFitResult res;
    if (!h || !(lo < hi)) return res;
    res.range_lo = lo; res.range_hi = hi;

    int lo_bin = h->GetXaxis()->FindBin(lo);
    int hi_bin = h->GetXaxis()->FindBin(hi);
    int max_bin = lo_bin;
    for (int b = lo_bin; b <= hi_bin; b++)
        if (h->GetBinContent(b) > h->GetBinContent(max_bin)) max_bin = b;
    res.mean = h->GetBinCenter(max_bin);   // fallback if the fit fails
    if (h->GetBinContent(max_bin) <= 0) return res;

    h->GetListOfFunctions()->Clear();
    TFitResultPtr r = h->Fit("gaus", "R+S+Q", "", lo, hi);
    if (r.Get() && r->Status() == 0) {
        res.ok = true; res.mean = r->Parameter(1); res.sigma = r->Parameter(2);
    }
    return res;
}

// analysis_tools/ root directory: two levels above this macro's own directory
// (param-maker/T0/update_T0_param.C -> param-maker/T0 -> param-maker -> analysis_tools).
static TString base_dir()
{
    TString fn = __FILE__;
    if (!gSystem->IsAbsoluteFileName(fn))
        fn = TString(gSystem->WorkingDirectory()) + "/" + fn;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(fn)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

void update_T0_param(
    const char* root_file     = nullptr,
    const char* ped_root_file = nullptr,
    const char* param_out     = nullptr,
    const char* adc_range_str = nullptr,
    const char* tdc_range_str = nullptr)
    // adc_range_str: "ped_lo_up,ped_lo_dn,ped_hi_up,ped_hi_dn,mip_lo_up,mip_lo_dn,mip_hi_up,mip_hi_dn"
    // tdc_range_str: "fit_lo_up,fit_lo_dn,fit_hi_up,fit_hi_dn" (empty = auto peak-find)
{
    TString rf  = root_file     ? TString(root_file)
                                 : base_path(Form("root/run%05d/run%05d_Hodoscope.root", RUN, RUN));
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString prf = ped_root_file ? TString(ped_root_file)
                                 : base_path(Form("root/run%05d/run%05d_Hodoscope_pedrun.root", run_num, run_num));
    TString po  = param_out     ? TString(param_out)
                                 : base_path(Form("output/run%05d/T0_param_run%05d.txt", run_num, run_num));
    TString pdf = base_path(Form("output/run%05d/update_T0_param_%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);
    gSystem->mkdir(gSystem->DirName(pdf), kTRUE);

    // ADC fit ranges: parse "8 comma-separated values", else use defaults
    double adc_ped_lo[2], adc_ped_hi[2], adc_mip_lo[2], adc_mip_hi[2];
    for (int ud = 0; ud < 2; ud++) {
        adc_ped_lo[ud] = DEFAULT_ADC_PED_LO[ud];
        adc_ped_hi[ud] = DEFAULT_ADC_PED_HI[ud];
        adc_mip_lo[ud] = DEFAULT_ADC_MIP_LO[ud];
        adc_mip_hi[ud] = DEFAULT_ADC_MIP_HI[ud];
    }
    if (adc_range_str && adc_range_str[0] != '\0') {
        double v[8]; int k = 0;
        std::istringstream ss(adc_range_str);
        std::string tok;
        while (std::getline(ss, tok, ',') && k < 8) v[k++] = std::stod(tok);
        if (k == 8) {
            adc_ped_lo[0] = v[0]; adc_ped_lo[1] = v[1];
            adc_ped_hi[0] = v[2]; adc_ped_hi[1] = v[3];
            adc_mip_lo[0] = v[4]; adc_mip_lo[1] = v[5];
            adc_mip_hi[0] = v[6]; adc_mip_hi[1] = v[7];
        } else {
            fprintf(stderr, "[WARN] adc_range_str expected 8 values, got %d; using defaults\n", k);
        }
    }
    // Display x-range for ADC pads: a bit above the MIP fit window
    double adc_xmax[2] = { adc_mip_hi[0] * 1.3, adc_mip_hi[1] * 1.3 };

    // TDC fit range: parse "4 comma-separated values", else auto peak-find
    bool   tdc_fixed_range = false;
    double tdc_fit_lo[2] = {0., 0.}, tdc_fit_hi[2] = {0., 0.};
    if (tdc_range_str && tdc_range_str[0] != '\0') {
        double v[4]; int k = 0;
        std::istringstream ss(tdc_range_str);
        std::string tok;
        while (std::getline(ss, tok, ',') && k < 4) v[k++] = std::stod(tok);
        if (k == 4) {
            tdc_fit_lo[0] = v[0]; tdc_fit_lo[1] = v[1];
            tdc_fit_hi[0] = v[2]; tdc_fit_hi[1] = v[3];
            tdc_fixed_range = true;
        } else {
            fprintf(stderr, "[WARN] tdc_range_str expected 4 values, got %d; using auto peak-find\n", k);
        }
    }

    printf("=== update_T0_param  run %05d ===\n", run_num);
    printf("Root      (TDC + ADC MIP): %s\n", rf.Data());
    printf("Ped root  (ADC pedestal) : %s\n", prf.Data());
    printf("Param out (T0 lines, txt): %s\n\n", po.Data());
    printf("Fallback TDC: Up=%.1f  Down=%.1f\n", TMPL_TDC_P0[0], TMPL_TDC_P0[1]);
    printf("Fallback ADC: Up ped=%.1f MIP=%.1f  Down ped=%.1f MIP=%.1f\n",
           TMPL_ADC_P0[0], TMPL_ADC_P1[0], TMPL_ADC_P0[1], TMPL_ADC_P1[1]);
    if (tdc_fixed_range)
        printf("TDC fit range (fixed): Up=[%.1f, %.1f]  Down=[%.1f, %.1f]\n\n",
               tdc_fit_lo[0], tdc_fit_hi[0], tdc_fit_lo[1], tdc_fit_hi[1]);
    else
        printf("TDC fit range: auto peak-find\n\n");

    // ----------------------------------------------------------------
    // Load histograms (SetDirectory keeps them alive after file close)
    // htdc/hadc (TDC + ADC MIP) come from the physics run (rf);
    // hadc_ped (ADC pedestal) comes from the dedicated pedestal run (prf).
    // ----------------------------------------------------------------
    TFile* f = TFile::Open(rf);
    if (!f || f->IsZombie()) { fprintf(stderr, "[ERROR] Cannot open %s\n", rf.Data()); return; }

    TH1* htdc[2], *hadc[2];
    for (int ud = 0; ud < 2; ud++) {
        htdc[ud] = (TH1*)f->Get(Form("h%d", T0_HID_BASE + 3 + ud));
        if (htdc[ud]) htdc[ud]->SetDirectory(gROOT);
        else fprintf(stderr, "[WARN] h%d not found\n", T0_HID_BASE + 3 + ud);

        hadc[ud] = (TH1*)f->Get(Form("h%d", T0_HID_BASE + 1 + ud));
        if (hadc[ud]) hadc[ud]->SetDirectory(gROOT);
        else fprintf(stderr, "[WARN] h%d not found\n", T0_HID_BASE + 1 + ud);
    }
    f->Close();

    TH1* hadc_ped[2];
    if (prf == rf) {
        // same file as the physics run: clone so the pedestal fit doesn't
        // clear the MIP fit's function list off the same histogram object
        for (int ud = 0; ud < 2; ud++) {
            hadc_ped[ud] = hadc[ud] ? (TH1*)hadc[ud]->Clone(Form("%s_ped", hadc[ud]->GetName())) : nullptr;
            if (hadc_ped[ud]) hadc_ped[ud]->SetDirectory(gROOT);
        }
    } else {
        TFile* fp = TFile::Open(prf);
        if (!fp || fp->IsZombie()) { fprintf(stderr, "[ERROR] Cannot open %s\n", prf.Data()); return; }
        for (int ud = 0; ud < 2; ud++) {
            hadc_ped[ud] = (TH1*)fp->Get(Form("h%d", T0_HID_BASE + 1 + ud));
            if (hadc_ped[ud]) {
                hadc_ped[ud]->SetDirectory(gROOT);
                hadc_ped[ud]->SetName(Form("%s_ped", hadc_ped[ud]->GetName()));
            } else {
                fprintf(stderr, "[WARN] h%d not found in ped_root_file\n", T0_HID_BASE + 1 + ud);
            }
        }
        fp->Close();
    }

    // ----------------------------------------------------------------
    // Fit all histograms BEFORE creating the canvas
    // ----------------------------------------------------------------
    gStyle->SetOptStat(1);
    gStyle->SetOptFit(111);

    bool   adc_ped_ok[2]={}, adc_mip_ok[2]={}, tdc_ok[2]={};
    double new_adc_p0[2], new_adc_p1[2], new_tdc_p0[2];
    double tdc_range_lo[2], tdc_range_hi[2];
    for (int ud = 0; ud < 2; ud++) {
        new_adc_p0[ud] = TMPL_ADC_P0[ud];
        new_adc_p1[ud] = TMPL_ADC_P1[ud];
        new_tdc_p0[ud] = TMPL_TDC_P0[ud];
    }

    for (int ud = 0; ud < 2; ud++) {
        // --- TDC: fixed-range fit if given, else auto peak-find + two-pass fit ---
        if (htdc[ud]) {
            PeakFitResult pr = tdc_fixed_range
                ? FixedRangeGausFit(htdc[ud], tdc_fit_lo[ud], tdc_fit_hi[ud])
                : AutoPeakGausFit(htdc[ud]);
            tdc_ok[ud]       = pr.ok;
            new_tdc_p0[ud]   = pr.mean;
            tdc_range_lo[ud] = pr.range_lo;
            tdc_range_hi[ud] = pr.range_hi;
        }

        // --- ADC pedestal: fit on the dedicated pedestal-run histogram ---
        if (hadc_ped[ud]) {
            hadc_ped[ud]->GetListOfFunctions()->Clear();
            TFitResultPtr rp = hadc_ped[ud]->Fit("gaus", "R+S", "", adc_ped_lo[ud], adc_ped_hi[ud]);
            if (rp.Get() && rp->Status() == 0) {
                adc_ped_ok[ud] = true;
                new_adc_p0[ud] = rp->Parameter(1);
            }
        }

        // --- ADC MIP: fit on the physics-run histogram ---
        if (hadc[ud]) {
            hadc[ud]->GetListOfFunctions()->Clear();
            TFitResultPtr rm = hadc[ud]->Fit("gaus", "R+S", "", adc_mip_lo[ud], adc_mip_hi[ud]);
            if (rm.Get() && rm->Status() == 0) {
                adc_mip_ok[ud] = true;
                new_adc_p1[ud] = rm->Parameter(1);
            }
        }
    }

    // ----------------------------------------------------------------
    // Print results
    // ----------------------------------------------------------------
    printf("%-6s  %-12s  %-14s  %s\n", "Side", "TDC T0 peak", "ADC pedestal", "ADC MIP");
    printf("%s\n", std::string(52,'-').c_str());
    for (int ud = 0; ud < 2; ud++) {
        const char* side = ud == 0 ? "Up  " : "Down";
        printf("%-6s  %8.1f (%s)  %8.1f (%s)   %8.1f (%s)\n",
               side,
               new_tdc_p0[ud], tdc_ok[ud]    ? "fit " : "max ",
               new_adc_p0[ud], adc_ped_ok[ud] ? "fit " : "tmpl",
               new_adc_p1[ud], adc_mip_ok[ud] ? "fit " : "tmpl");
        double tdc_shift_ns = (new_tdc_p0[ud] - TMPL_TDC_P0[ud]) * std::abs(TMPL_TDC_P1[ud]);
        printf("       TDC shift vs fallback: %.1f counts = %.4f ns\n",
               new_tdc_p0[ud] - TMPL_TDC_P0[ud], tdc_shift_ns);
    }

    // ----------------------------------------------------------------
    // Draw canvas: row1=TDC (physics run), row2=ADC pedestal (ped run),
    //              row3=ADC MIP (physics run)
    // ----------------------------------------------------------------
    TCanvas* c = new TCanvas("c_t0", "T0 calibration", 400*2, 350*3);
    c->Print(pdf+"[", "pdf");
    c->Divide(2, 3, 0.005, 0.005);

    for (int ud = 0; ud < 2; ud++) {
        const char* side = ud == 0 ? "Up" : "Down";

        c->cd(1 + ud);
        if (htdc[ud]) {
            double half = tdc_range_hi[ud] - tdc_range_lo[ud];
            htdc[ud]->GetXaxis()->SetRangeUser(tdc_range_lo[ud] - half, tdc_range_hi[ud] + half);
            htdc[ud]->SetTitle(Form("T0 TDC %s: T0=%.1f", side, new_tdc_p0[ud]));
            htdc[ud]->Draw("HIST");
            TList* funcs = htdc[ud]->GetListOfFunctions();
            if (funcs) { TIter next(funcs); TObject* obj;
                         while ((obj = next())) obj->Draw("SAME"); }
        }

        c->cd(3 + ud);
        if (hadc_ped[ud]) {
            hadc_ped[ud]->GetXaxis()->SetRangeUser(0., adc_xmax[ud]);
            hadc_ped[ud]->SetTitle(Form("T0 ADC %s (ped run): ped=%.1f",
                                    side, new_adc_p0[ud]));
            hadc_ped[ud]->Draw("HIST");
            TList* funcs = hadc_ped[ud]->GetListOfFunctions();
            if (funcs) { TIter next(funcs); TObject* obj;
                         while ((obj = next())) obj->Draw("SAME"); }
        }

        c->cd(5 + ud);
        if (hadc[ud]) {
            hadc[ud]->GetXaxis()->SetRangeUser(0., adc_xmax[ud]);
            hadc[ud]->SetTitle(Form("T0 ADC %s (physics run): MIP=%.1f",
                                    side, new_adc_p1[ud]));
            hadc[ud]->Draw("HIST");
            TList* funcs = hadc[ud]->GetListOfFunctions();
            if (funcs) { TIter next(funcs); TObject* obj;
                         while ((obj = next())) obj->Draw("SAME"); }
        }
    }

    c->Print(pdf, "pdf");
    c->Print(pdf+"]", "pdf");
    delete c;

    // ----------------------------------------------------------------
    // Write T0-only HDPRM lines to txt
    // ----------------------------------------------------------------
    {
        po.ReplaceAll("//", "/");
        std::ofstream fout(po.Data());
        if (!fout) {
            fprintf(stderr, "[ERROR] Cannot write %s\n", po.Data());
        } else {
            fout << "# T0 (CId=11) ADC: p0=pedestal, p1=MIP\n";
            for (int ud = 0; ud < 2; ud++)
                fout << Form("11\t0\t0\t0\t%d\t%.6f\t%.6f\n", ud, new_adc_p0[ud], new_adc_p1[ud]);
            fout << "# T0 (CId=11) TDC: p0=T0_peak (updated), p1=slope (fixed)\n";
            for (int ud = 0; ud < 2; ud++)
                fout << Form("11\t0\t0\t1\t%d\t%.6f\t%.15f\n", ud, new_tdc_p0[ud], TMPL_TDC_P1[ud]);
            fout.close();
            printf("\n[INFO] Wrote %s\n", po.Data());
        }
    }

    printf("[INFO] -> %s\n=== Done ===\n", pdf.Data());
}
