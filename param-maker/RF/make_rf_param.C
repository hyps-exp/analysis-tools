/**
 * @file    make_rf_param.c
 * @brief   Analysis RF timing calibration
 * @author  Akao Ryogo
 * @date    2026-10-30
 *
 * Change history:
 *   2026-10-30  Akao Ryogo  First version
 */

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TSpectrum.h>
#include <TLine.h>
#include <TLatex.h>
#include <TSystem.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>

static const int RUN = 7013;

static const int    kMaxDepth = 16;


static const double kDefaultBunchInterval   = 23.6;              // ns
static const int    kDefaultPeakRank        = 1;
static const double kDefaultBinWidthCounts  = 10.0;               // TDC counts
static const double kDefaultEdgeMarginFrac  = 0.55;
static const double kDefaultFitHalfWinFrac  = 0.45;
static const double kDefaultPlotHalfWinFrac = 1.5;   
static const double kDefaultTdcCountsPerNs  = 12080.0 / 11.8;


static const double TMPL_P1 = -0.00097691419;


static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }


struct RFFitConf {
    double bunch_interval, bin_width_counts;
    int    peak_rank;
    double edge_margin_frac, fit_half_win_frac, plot_half_win_frac, tdc_counts_per_ns;
};

static RFFitConf LoadRFFitConf()
{
    RFFitConf c = { kDefaultBunchInterval, kDefaultBinWidthCounts, kDefaultPeakRank,
                     kDefaultEdgeMarginFrac, kDefaultFitHalfWinFrac, kDefaultPlotHalfWinFrac,
                     kDefaultTdcCountsPerNs };

    TString path = base_path("fit_range/RF/rf_fit_conf.txt");
    std::ifstream fin(path.Data());
    if (!fin.is_open()) {
        printf("[INFO] No RF fit conf at %s; using hardcoded defaults\n", path.Data());
        return c;
    }

    std::string line;
    int n_set = 0;
    while (std::getline(fin, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream iss(line);
        std::string key; double val;
        if (!(iss >> key >> val)) continue;
        if      (key == "bunch_interval")     { c.bunch_interval     = val; n_set++; }
        else if (key == "peak_rank")          { c.peak_rank          = (int)val; n_set++; }
        else if (key == "bin_width_counts")   { c.bin_width_counts   = val; n_set++; }
        else if (key == "edge_margin_frac")   { c.edge_margin_frac   = val; n_set++; }
        else if (key == "fit_half_win_frac")  { c.fit_half_win_frac  = val; n_set++; }
        else if (key == "plot_half_win_frac") { c.plot_half_win_frac = val; n_set++; }
        else if (key == "tdc_counts_per_ns")  { c.tdc_counts_per_ns  = val; n_set++; }
        else fprintf(stderr, "[WARN] Unknown key in %s: %s (ignored)\n", path.Data(), key.c_str());
    }
    printf("[INFO] RF fit conf: %s  (%d keys set)\n", path.Data(), n_set);
    return c;
}

// main
void make_rf_param(
    const char* root_file    = nullptr,
    const char* param_out    = nullptr,   
    const char* user_out     = nullptr,  
    double bunch_interval    = -1.,       
    int peak_rank            = 0,         
    double bin_width_counts  = -1.       
)
{
    TString rf = root_file ? TString(root_file)
                           : base_path(Form("root/run%05d/run%05d_Hodoscope.root", RUN, RUN));
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString po  = param_out ? TString(param_out)
                            : base_path(Form("output/run%05d/RF_param_run%05d.txt", run_num, run_num));
    TString uo  = user_out  ? TString(user_out)
                            : base_path(Form("output/run%05d/RF_user_param_run%05d.txt", run_num, run_num));
    TString png = base_path(Form("output/run%05d/rf_full_run%05d.png", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);

    RFFitConf conf = LoadRFFitConf();
    if (bunch_interval   < 0.) bunch_interval   = conf.bunch_interval;
    if (peak_rank        == 0) peak_rank        = conf.peak_rank;
    if (bin_width_counts < 0.) bin_width_counts = conf.bin_width_counts;
    const double kEdgeMarginFrac  = conf.edge_margin_frac;
    const double kFitHalfWinFrac  = conf.fit_half_win_frac;
    const double kPlotHalfWinFrac = conf.plot_half_win_frac;
    const double kTdcCountsPerNs  = conf.tdc_counts_per_ns;

    std::printf("=== RF Parameter Maker  run%05d ===\n", run_num);
    std::printf("ROOT file    : %s\n", rf.Data());
    std::printf("BunchInterval: %.6g ns\n", bunch_interval);
    std::printf("Peak rank    : %d (from right)\n", peak_rank);
    std::printf("Bin width    : %.1f TDC counts\n", bin_width_counts);
    std::printf("Param out    : %s\n", po.Data());
    std::printf("User  out    : %s\n\n", uo.Data());

    // RF バンチ間隔を TDC counts に換算（mode ごとに異なるため bunch_interval から動的に算出）
    const double kBunchCounts = bunch_interval * kTdcCountsPerNs;
    std::printf("Bunch spacing: %.1f TDC counts (%.6g ns)\n\n", kBunchCounts, bunch_interval);


    TFile* f = TFile::Open(rf, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "[ERROR] Cannot open ROOT file: " << rf.Data() << std::endl;
        return;
    }
    TTree* t = (TTree*)f->Get("tree");
    if (!t) {
        std::cerr << "[ERROR] TTree 'tree' not found in " << rf.Data() << std::endl;
        f->Close();
        return;
    }


    double rft[1][kMaxDepth];
    t->SetBranchStatus("*", 0);
    t->SetBranchStatus("rft", 1);
    t->SetBranchAddress("rft", rft);

   
    std::printf("[0/3] Scanning tree for rft range ...\n");
    Long64_t nev    = t->GetEntries();
    Long64_t nscan  = nev < 200000 ? nev : 200000;
    double vmin =  1e18, vmax = -1e18;
    for (Long64_t i = 0; i < nscan; ++i) {
        t->GetEntry(i);
        for (int j = 0; j < kMaxDepth; ++j) {
            double v = rft[0][j];
            if (v != v || v > 1e17) continue;  // NaN or sentinel skip
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
        }
    }
    if (vmin > vmax) {
        std::cerr << "[ERROR] No valid rft values found in first " << nscan << " events." << std::endl;
        f->Close();
        return;
    }
   
    double margin = 2.0 * kBunchCounts;
    double hmin = std::max(0.0, vmin - margin);
    double hmax = vmax + margin;
    
    int nbins = (int)((hmax - hmin) / bin_width_counts);
    if (nbins < 200) nbins = 200;
    std::printf("  range: %.0f - %.0f  nbins: %d\n", hmin, hmax, nbins);


    TH1D* h = new TH1D("h_rft", Form("RF TDC from tree (run%05d);TDC count;Entries / bin", run_num),
                        nbins, hmin, hmax);
    h->SetDirectory(nullptr);
    for (Long64_t i = 0; i < nev; ++i) {
        t->GetEntry(i);
        for (int j = 0; j < kMaxDepth; ++j) {
            double v = rft[0][j];
            if (v != v || v > 1e17) continue;
            h->Fill(v);
        }
    }
    f->Close();
    delete f;
    std::printf("  Total entries filled: %.0f\n", h->GetEntries());

    double bin_width = h->GetBinWidth(1);
    double xmax      = h->GetXaxis()->GetXmax();
    double edge_cut  = xmax - kEdgeMarginFrac * kBunchCounts;

    // peak detection
    std::printf("[1/3] Fitting rightmost RF peak ...\n");

    TH1D* h_spec = (TH1D*)h->Clone("h_rf_spec");
    h_spec->SetDirectory(nullptr);

    double ts_sigma = std::max(1.0, 300.0 / bin_width);

    TSpectrum sp(100);
    int npeaks = sp.Search(h_spec, ts_sigma, "goff nodraw", 0.05);
    delete h_spec;

    if (npeaks == 0) {
        std::cerr << "[ERROR] No peaks found by TSpectrum." << std::endl;
        delete h;
        return;
    }

    Double_t* px = sp.GetPositionX();
    std::vector<double> valid_peaks;
    for (int i = 0; i < npeaks; ++i) {
        if (px[i] < edge_cut)
            valid_peaks.push_back(px[i]);
    }
    std::sort(valid_peaks.begin(), valid_peaks.end(), std::greater<double>());

    int valid_count = (int)valid_peaks.size();
    if (valid_count == 0) {
        std::cerr << "[ERROR] All peaks within edge margin (edge_cut=" << edge_cut << ")." << std::endl;
        delete h;
        return;
    }
    if (peak_rank < 1 || peak_rank > valid_count) {
        std::cerr << "[ERROR] peak_rank=" << peak_rank << " is out of range (found "
                  << valid_count << " valid peaks)." << std::endl;
        delete h;
        return;
    }
    double rightmost_x = valid_peaks[peak_rank - 1];
    std::printf("  Found %d valid peaks, selected rank %d from right: x=%.0f\n",
                valid_count, peak_rank, rightmost_x);

    // gaussian fit
    double fit_lo = rightmost_x - kFitHalfWinFrac * kBunchCounts;
    double fit_hi = rightmost_x + kFitHalfWinFrac * kBunchCounts;
    double plot_lo = rightmost_x - kPlotHalfWinFrac * kBunchCounts;
    double plot_hi = rightmost_x + kPlotHalfWinFrac * kBunchCounts;

    TF1* gf = new TF1("gf_rf", "gaus", fit_lo, fit_hi);
    gf->SetParameter(0, h->GetMaximum());
    gf->SetParameter(1, rightmost_x);
    gf->SetParameter(2, 0.1 * kBunchCounts);
    gf->SetParLimits(2, 0.0, 0.3 * kBunchCounts);

    h->Fit(gf, "RQ0");

    double mean  = gf->GetParameter(1);
    double sigma = std::abs(gf->GetParameter(2));
    double amp   = gf->GetParameter(0);
    std::printf("  Gaussian fit: mean=%.2f, sigma=%.2f, amplitude=%.1f\n", mean, sigma, amp);


    TCanvas* c = new TCanvas("c_rf", "RF peak fit", 1400, 500);
    c->Divide(2, 1);

    c->cd(1);
    gPad->SetLeftMargin(0.13);
    TH1D* h_left = (TH1D*)h->Clone("h_rf_left");
    h_left->SetDirectory(nullptr);
    h_left->GetXaxis()->SetRangeUser(plot_lo, plot_hi);
    h_left->SetFillColor(38);
    h_left->SetLineColor(kBlue+1);
    h_left->SetTitle(Form("Fit region (run%05d);TDC count;Entries / bin", run_num));
    h_left->Draw("HIST");
    gf->SetLineColor(kRed);
    gf->SetLineWidth(2);
    gf->Draw("same");
    TLine* l1 = new TLine(mean, 0, mean, h_left->GetMaximum());
    l1->SetLineColor(kRed); l1->SetLineStyle(2); l1->SetLineWidth(2);
    l1->Draw();
    TLatex tex1;
    tex1.SetNDC(); tex1.SetTextSize(0.04);
    tex1.DrawLatex(0.16, 0.86, Form("mean = %.1f", mean));
    tex1.DrawLatex(0.16, 0.79, Form("#sigma  = %.1f", sigma));

    c->cd(2);
    gPad->SetLeftMargin(0.13);
    TH1D* h_right = (TH1D*)h->Clone("h_rf_right");
    h_right->SetDirectory(nullptr);
    h_right->SetFillColor(38);
    h_right->SetLineColor(kBlue+1);
    h_right->SetTitle(Form("Full range (run%05d);TDC count;Entries / bin", run_num));
    h_right->Draw("HIST");
    TLine* l2 = new TLine(mean, 0, mean, h_right->GetMaximum() * 0.8);
    l2->SetLineColor(kRed); l2->SetLineStyle(2); l2->SetLineWidth(2);
    l2->Draw();

    c->SaveAs(png.Data());
    std::printf("  Plot saved: %s\n", png.Data());

    delete l1; delete l2;
    delete h_left; delete h_right;
    delete c;


    std::printf("[2/3] Writing RF param -> %s\n", po.Data());
    {
        po.ReplaceAll("//", "/");
        std::ofstream fout(po.Data());
        if (!fout.is_open()) {
            std::cerr << "[ERROR] Cannot write " << po.Data() << std::endl;
        } else {
            fout << "# RF (CId=1): p0=TDC peak (updated), p1=slope (fixed)\n";
            fout << Form("1\t0\t0\t1\t0\t%.0f\t%.11f\n", mean, TMPL_P1);
            fout.close();
            std::printf("  RF p0: %.0f\n", mean);
        }
    }

   
    std::printf("[3/3] Writing BunchInterval -> %s\n", uo.Data());
    {
        uo.ReplaceAll("//", "/");
        std::ofstream fout(uo.Data());
        if (!fout.is_open()) {
            std::cerr << "[ERROR] Cannot write " << uo.Data() << std::endl;
        } else {
            fout << Form("BunchInterval %.6g\n", bunch_interval);
            fout.close();
            std::printf("  BunchInterval: %.6g\n", bunch_interval);
        }
    }

    delete h; delete gf;

    std::printf("\n=== Done ===\n");
    std::printf("Created: %s\n", po.Data());
    std::printf("Created: %s\n", uo.Data());
}
