/**
 * @file    make_rf_phc_param.c
 * @brief   Analysis RF PHC  
 * @author  Akao Ryogo
 * @date    2026-10-30
 *
 * Change history:
 *   2026-10-30  Akao Ryogo  First version
 */

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TProfile.h>
#include <TLatex.h>
#include <TSystem.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <limits>

static const int RUN = 7013;


static const double kUnsetVal = -9999.0;
static bool IsUnset(double v) { return v < kUnsetVal + 1.0; }

static const double kDefaultBunchInterval = 1.966;
static const double kDefaultDeLo          = 1.2;
static const double kDefaultDeHi          = 7.5;
static const int    kDefaultNSlice        = 45;
static const double kDefaultDeHistMax     = 8.0;
static const int    kDefaultRfNy          = 5000;
static const double kDefaultRfPeriod      = 1.966;
static const double kDefaultRfYlo         = -5.0;
static const double kDefaultRfYhi         = 5.0;


static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

struct RFPHCConf {
    TString root_file;  // empty = not set
    double  bunch_interval, de_lo, de_hi, de_hist_max, rf_period, rf_ylo, rf_yhi;
    int     nslice, rf_ny;
};


static RFPHCConf LoadRFPHCConf()
{
    RFPHCConf c = { "", kDefaultBunchInterval, kDefaultDeLo, kDefaultDeHi, kDefaultDeHistMax,
                     kDefaultRfPeriod, kDefaultRfYlo, kDefaultRfYhi,
                     kDefaultNSlice, kDefaultRfNy };

    TString path = base_path("fit_range/RF/RF_PHC_param.txt");
    std::ifstream fin(path.Data());
    if (!fin.is_open()) {
        printf("[INFO] No RF PHC fit conf at %s; using hardcoded defaults\n", path.Data());
        return c;
    }

    std::string line;
    int n_set = 0;
    while (std::getline(fin, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream iss(line);
        std::string key, sval;
        if (!(iss >> key >> sval)) continue;
        if (key == "root_file") { c.root_file = sval.c_str(); n_set++; continue; }
        double val = atof(sval.c_str());
        if      (key == "bunch_interval") { c.bunch_interval = val; n_set++; }
        else if (key == "de_lo")          { c.de_lo          = val; n_set++; }
        else if (key == "de_hi")          { c.de_hi          = val; n_set++; }
        else if (key == "nslice")         { c.nslice         = (int)val; n_set++; }
        else if (key == "de_hist_max")    { c.de_hist_max    = val; n_set++; }
        else if (key == "rf_ny")          { c.rf_ny          = (int)val; n_set++; }
        else if (key == "rf_period")      { c.rf_period      = val; n_set++; }
        else if (key == "rf_ylo")         { c.rf_ylo         = val; n_set++; }
        else if (key == "rf_yhi")         { c.rf_yhi         = val; n_set++; }
        else fprintf(stderr, "[WARN] Unknown key in %s: %s (ignored)\n", path.Data(), key.c_str());
    }
    printf("[INFO] RF PHC fit conf: %s  (%d keys set)\n", path.Data(), n_set);
    return c;
}


static TString ResolveRootFileList(const TString& list_str, int run_for_dir)
{
    TString dir = base_path(Form("root/run%05d", run_for_dir));
    std::string out;
    std::string tok;
    std::istringstream ss(list_str.Data());
    bool first = true;
    while (std::getline(ss, tok, ',')) {
        size_t s = tok.find_first_not_of(" \t");
        size_t e = tok.find_last_not_of(" \t");
        if (s == std::string::npos) continue;
        std::string fn = tok.substr(s, e - s + 1);
        bool is_path = (fn.find('/') != std::string::npos);
        std::string resolved = is_path ? fn : (std::string(dir.Data()) + "/" + fn);
        if (!first) out += ",";
        out += resolved;
        first = false;
    }
    return TString(out.c_str());
}


static bool FitOneSide(TH2D* h2, const char* label,
                       double fit_lo, double fit_hi,
                       double gauss_range,
                       double rf_half,
                       double parm[3],
                       const double* parm_rough = nullptr,
                       double cut_win = 0.0,
                       TCanvas* c_hist = nullptr,
                       TCanvas* c_fit  = nullptr)
{
    int nslice = h2->GetNbinsX();
    const double bunch_period = rf_half * 2.0;
    const bool pass2 = (parm_rough != nullptr && cut_win > 0.0
                        && std::abs(parm_rough[0]) > 1e-6);

   
    double y_global_seed = 0.0;
    if (!pass2) {
        int xbin_seed_lo = h2->GetXaxis()->FindBin(fit_lo + (fit_hi - fit_lo) * 0.4);
        int xbin_seed_hi = h2->GetXaxis()->FindBin(fit_hi);
        TH1D* h_seed = h2->ProjectionY(Form("_hseed_%s", label), xbin_seed_lo, xbin_seed_hi);
        h_seed->SetDirectory(nullptr);
        y_global_seed = (h_seed->GetEntries() > 0)
                        ? h_seed->GetBinCenter(h_seed->GetMaximumBin()) : 0.0;
        delete h_seed;
        std::printf("    [%s] global seed (y from high-de proj): %.3f ns\n", label, y_global_seed);
    } else {
        std::printf("    [%s] pass-2 with cut window ±%.3f ns\n", label, cut_win);
    }

    std::vector<double> xc, yc, xe, ye;
    double y_prev = y_global_seed;

    for (int j = 0; j < nslice; ++j) {
        double xj = h2->GetXaxis()->GetBinCenter(j+1);
        if (xj < fit_lo || xj > fit_hi) continue;

        TH1D* hproj = h2->ProjectionY(Form("_hproj_%s_%d", label, j), j+1, j+1);
        hproj->SetDirectory(nullptr);
        if (hproj->GetEntries() < 5) { delete hproj; continue; }

        double peak_seed;

        if (pass2 && xj > parm_rough[1]) {
            double y_expect = parm_rough[0] / std::sqrt(xj - parm_rough[1]) - parm_rough[2];
            double ylo_cut = y_expect - cut_win;
            double yhi_cut = y_expect + cut_win;
            int ybin_lo = hproj->FindBin(ylo_cut);
            int ybin_hi = hproj->FindBin(yhi_cut);
            ybin_lo = std::max(ybin_lo, 1);
            ybin_hi = std::min(ybin_hi, hproj->GetNbinsX());
            double max_cnt = -1.0;
            int max_bin = -1;
            for (int ib = ybin_lo; ib <= ybin_hi; ++ib) {
                if (hproj->GetBinContent(ib) > max_cnt) {
                    max_cnt = hproj->GetBinContent(ib);
                    max_bin = ib;
                }
            }
            if (max_bin < 0 || max_cnt < 1.0) { delete hproj; continue; }
            peak_seed = hproj->GetBinCenter(max_bin);
        } else {
            
            peak_seed = hproj->GetBinCenter(hproj->GetMaximumBin());
            if (y_prev < 1e17) {
                double best_seed = peak_seed;
                double best_dist = std::abs(peak_seed - y_prev);
                for (int k = -3; k <= 3; ++k) {
                    double cand = y_prev + k * bunch_period;
                    if (cand < h2->GetYaxis()->GetXmin() || cand > h2->GetYaxis()->GetXmax()) continue;
                    double dist = std::abs(peak_seed - cand);
                    if (dist < best_dist) { best_dist = dist; best_seed = cand; }
                }
                if (std::abs(best_seed - y_prev) > bunch_period * 0.55) {
                    delete hproj; continue;
                }
                peak_seed = best_seed;
            }
        }

        hproj->Fit("gaus", "Q", "", peak_seed - gauss_range, peak_seed + gauss_range);
        TF1* gf = hproj->GetFunction("gaus");

        if (!gf) { delete hproj; continue; }
        double peak  = gf->GetParameter(1);
        double sigma = gf->GetParameter(2);
        double norm  = gf->GetParameter(0);

        delete hproj;

        if (std::isnan(peak) || std::isnan(sigma) || sigma <= 0.0 || norm <= 0.0) continue;
        if (std::abs(peak - peak_seed) > gauss_range) continue;

        // 誤差 = sigma / sqrt(N) (最小 0.005 ns)
        double neff = std::max(norm * sigma * std::sqrt(2.0 * M_PI)
                               / (h2->GetYaxis()->GetBinWidth(1)), 5.0);
        double yerr = std::max(sigma / std::sqrt(neff), 0.005);

        xc.push_back(xj);
        yc.push_back(peak);
        xe.push_back(0.0);
        ye.push_back(yerr);
        y_prev = peak;  
    }

    if ((int)xc.size() < 5) {
        std::cerr << "  [" << label << "] Too few valid Gaussian peaks ("
                  << xc.size() << ")." << std::endl;
        parm[0] = parm[1] = parm[2] = 0.0;
        return false;
    }

    // TGraphErrors → slewing fit: [0]/(sqrt(x-[1]))-[2]
    TGraphErrors* gr = new TGraphErrors((int)xc.size(),
                                        xc.data(), yc.data(),
                                        xe.data(), ye.data());
    gr->SetName(Form("gr_%s", label));
    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(kRed);
    gr->SetMarkerSize(0.7);

    TF1* slew = new TF1(Form("slew_%s", label),
                        "[0]/(sqrt(x-[1]))-[2]", fit_lo, fit_hi);


    double y_lo = yc.front(), x_lo = xc.front();
    double y_hi = yc.back(),  x_hi = xc.back();
    double inv_lo = 1.0 / std::sqrt(std::max(x_lo, 0.01));
    double inv_hi = 1.0 / std::sqrt(x_hi);
    double p0_init = (std::abs(inv_lo - inv_hi) > 1e-6)
                     ? (y_lo - y_hi) / (inv_lo - inv_hi) : -1.0;
    if (p0_init > 0.0) p0_init = -1.0;
    double p2_init = p0_init * inv_hi - y_hi;  // [2] = [0]/sqrt(x_hi) - y_hi


    slew->SetParameter(0, p0_init);
    slew->SetParameter(1, 0.0);
    slew->SetParameter(2, p2_init);
    slew->SetParLimits(0, -200.0, 0.0);
    slew->SetParLimits(1, -1.0, fit_lo * 0.9);

    gr->Fit(slew, "RQ", "", fit_lo, fit_hi);

    parm[0] = slew->GetParameter(0);
    parm[1] = slew->GetParameter(1);
    parm[2] = slew->GetParameter(2);

    std::printf("    [%s] Gauss-peak: n=%d y_lo=%.3g y_hi=%.3g"
                " slew: [0]=%.4g [1]=%.4g [2]=%.4g\n",
                label, (int)xc.size(), y_lo, y_hi, parm[0], parm[1], parm[2]);

    
    if (c_hist) {
        c_hist->cd();
        gPad->SetLeftMargin(0.13);
        h2->Draw("col");
        TGraphErrors* gr_ov = new TGraphErrors(*gr);
        gr_ov->SetMarkerStyle(7);
        gr_ov->SetMarkerColor(kRed);
        gr_ov->SetLineColor(kRed);
        gr_ov->Draw("P same");
        TF1* slew_ov = new TF1(Form("slew_ov_%s_%s", label, c_hist->GetName()),
                               "[0]/(sqrt(x-[1]))-[2]", fit_lo, fit_hi);
        slew_ov->SetParameters(parm[0], parm[1], parm[2]);
        slew_ov->SetLineColor(kOrange+1);
        slew_ov->SetLineWidth(2);
        slew_ov->Draw("same");
    }

  
    if (c_fit) {
        c_fit->cd();
        gPad->SetLeftMargin(0.15);
        gPad->SetBottomMargin(0.15);

        double ymin_d = 1e18, ymax_d = -1e18;
        for (int j = 0; j < gr->GetN(); ++j) {
            double yi = gr->GetY()[j];
            if (yi < ymin_d) ymin_d = yi;
            if (yi > ymax_d) ymax_d = yi;
        }
        double margin = (ymax_d > ymin_d) ? (ymax_d - ymin_d) * 0.3 : 1.0;
        if (ymin_d > ymax_d) { ymin_d = -rf_half; ymax_d = rf_half; }

        TGraphErrors* gr_fit = new TGraphErrors(*gr);
        gr_fit->SetTitle(Form("Gauss peak vs deT0-%s;deT0 [MIP];RF1st peak [ns]", label));
        gr_fit->SetMarkerStyle(20);
        gr_fit->SetMarkerColor(kBlue+1);
        gr_fit->SetMarkerSize(0.7);
        gr_fit->SetMinimum(ymin_d - margin);
        gr_fit->SetMaximum(ymax_d + margin);
        gr_fit->Draw("AP");
        gr_fit->GetXaxis()->SetLimits(fit_lo, fit_hi);
        gr_fit->GetXaxis()->SetRangeUser(fit_lo, fit_hi);
        gPad->Update();

        slew->SetLineColor(kRed);
        slew->SetLineWidth(2);
        slew->Draw("same");

        TLatex tex;
        tex.SetNDC(); tex.SetTextSize(0.05);
        tex.DrawLatex(0.18, 0.86, Form("[0]=%.4g", parm[0]));
        tex.DrawLatex(0.18, 0.79, Form("[1]=%.4g", parm[1]));
        tex.DrawLatex(0.18, 0.72, Form("[2]=%.4g", parm[2]));
    }

    if (!c_hist && !c_fit) {
        delete gr;
        delete slew;
    }
    return true;
}

// main
void make_rf_phc_param(
    const char* root_file     = nullptr,
    const char* param_out     = nullptr,  
    double bunch_interval     = kUnsetVal,    
    double de_lo              = kUnsetVal,    
    double de_hi              = kUnsetVal,    
    int    nslice             = 0,        
    double de_hist_max        = kUnsetVal,    
    int    rf_ny              = 0,        
    double rf_period          = kUnsetVal,    
    double rf_ylo             = kUnsetVal,    
    double rf_yhi             = kUnsetVal     
)
{
    
    RFPHCConf conf = LoadRFPHCConf();
    if (IsUnset(bunch_interval)) bunch_interval = conf.bunch_interval;
    if (IsUnset(de_lo))          de_lo          = conf.de_lo;
    if (IsUnset(de_hi))          de_hi          = conf.de_hi;
    if (nslice == 0)             nslice         = conf.nslice;
    if (IsUnset(de_hist_max))    de_hist_max    = conf.de_hist_max;
    if (rf_ny == 0)               rf_ny          = conf.rf_ny;
    if (IsUnset(rf_period))      rf_period      = conf.rf_period;
    if (IsUnset(rf_ylo))         rf_ylo         = conf.rf_ylo;
    if (IsUnset(rf_yhi))         rf_yhi         = conf.rf_yhi;

    TString rf_list_raw = root_file ? TString(root_file)
                        : (!conf.root_file.IsNull() ? conf.root_file
                           : TString(Form("run%05d_HypsTracking.root", RUN)));
    TString rf_list = ResolveRootFileList(rf_list_raw, RUN);

    const double fold_period = bunch_interval;
    const double rf_half     = 0.5 * fold_period;

    TChain* t = new TChain("hyps");
    std::vector<TString> added_files;
    {
        std::string tok;
        std::istringstream ss_chain(rf_list.Data());
        while (std::getline(ss_chain, tok, ',')) {
            size_t s = tok.find_first_not_of(" \t");
            size_t e = tok.find_last_not_of(" \t");
            if (s == std::string::npos) continue;
            std::string fn = tok.substr(s, e - s + 1);
            if (gSystem->AccessPathName(fn.c_str())) {
                fprintf(stderr, "[WARN] File not found (skipped): %s\n", fn.c_str());
                continue;
            }
            t->Add(fn.c_str());
            added_files.push_back(fn.c_str());
        }
        if (added_files.empty()) {
            std::cerr << "[ERROR] No valid ROOT files loaded." << std::endl;
            delete t; return;
        }
    }

    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(added_files.front());
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString po = param_out ? TString(param_out)
                          : base_path(Form("output/run%05d/RF_PHC_param_run%05d.txt", run_num, run_num));
    TString outdir = base_path(Form("output/run%05d", run_num));
    gSystem->mkdir(outdir, kTRUE);

    std::printf("=== RF PHC Parameter Maker (HypsTracking)  run%05d ===\n", run_num);
    for (int i = 0; i < (int)added_files.size(); i++)
        std::printf("Root[%d]: %s\n", i, added_files[i].Data());
    std::printf("Output param : %s\n", po.Data());
    std::printf("BunchInterval: %.4g ns  (fold period)\n", bunch_interval);
    std::printf("RF pre-filter: [%.3g, %.3g] ns\n", rf_ylo, rf_yhi);
    std::printf("Fit range    : [%.3g, %.3g] MIP\n\n", de_lo, de_hi);

    const int kMaxHits = 50;
    int    nhT0_v = 0;
    double RF1st_v = 999.0;
    double udeT0_v[kMaxHits], ddeT0_v[kMaxHits];
    t->SetBranchStatus("*", 0);
    t->SetBranchStatus("nhT0",  1);
    t->SetBranchStatus("RF1st", 1);
    t->SetBranchStatus("udeT0", 1);
    t->SetBranchStatus("ddeT0", 1);
    t->SetBranchAddress("nhT0",  &nhT0_v);
    t->SetBranchAddress("RF1st", &RF1st_v);
    t->SetBranchAddress("udeT0", udeT0_v);
    t->SetBranchAddress("ddeT0", ddeT0_v);

    std::printf("[1/3] Building 2D histograms (unfolded RF1st: [%.3g, %.3g] ns) ...\n",
                rf_ylo, rf_yhi);

    TH2D* h2u = new TH2D("h_rfphc_u",
        Form("RF1st vs deT0-U (run%05d);deT0-U [MIP];RF1st [ns]", run_num),
        nslice, 0.0, de_hist_max, rf_ny, rf_ylo, rf_yhi);
    TH2D* h2d = new TH2D("h_rfphc_d",
        Form("RF1st vs deT0-D (run%05d);deT0-D [MIP];RF1st [ns]", run_num),
        nslice, 0.0, de_hist_max, rf_ny, rf_ylo, rf_yhi);
    h2u->SetDirectory(nullptr);
    h2d->SetDirectory(nullptr);

    Long64_t nev = t->GetEntries();
    Long64_t cnt = 0;

    for (Long64_t i = 0; i < nev; ++i) {
        t->GetEntry(i);

        if (RF1st_v > 900.0) continue;
        double rf1st = RF1st_v;

    
        if (rf1st < rf_ylo || rf1st >= rf_yhi) continue;

        if (nhT0_v < 1) continue;
        double det0_u = udeT0_v[0];
        double det0_d = ddeT0_v[0];
        bool u_ok = (det0_u == det0_u && det0_u > 0.0);
        bool d_ok = (det0_d == det0_d && det0_d > 0.0);

        if (u_ok) h2u->Fill(det0_u, rf1st);
        if (d_ok) h2d->Fill(det0_d, rf1st);
        if (u_ok || d_ok) ++cnt;
    }
    delete t;

    std::printf("  Events with valid RF+T0: %lld / %lld\n", cnt, nev);
    std::printf("  U entries: %.0f\n", h2u->GetEntries());
    std::printf("  D entries: %.0f\n", h2d->GetEntries());

    if (h2u->GetEntries() < 100 || h2d->GetEntries() < 100) {
        std::cerr << "[ERROR] Too few entries for fitting." << std::endl;
        delete h2u; delete h2d; return;
    }

    std::printf("[2/3] Slicing and fitting ...\n");

    std::string plot_dir = outdir.Data();
    std::string pdf_path  = plot_dir + Form("/rf_phc_run%05d.pdf", run_num);
    std::string png_p1    = plot_dir + Form("/rf_phc_run%05d_p1.png", run_num);
    std::string png_p2    = plot_dir + Form("/rf_phc_run%05d_p2.png", run_num);
    std::string png_p3    = plot_dir + Form("/rf_phc_run%05d_p3.png", run_num);

    double gauss_half = 0.10;


    auto make_c = [](const char* name, const char* title) -> TCanvas* {
        return new TCanvas(name, title, 700, 500);
    };

    auto save_page = [&](const char* tag,
                         double parmu_out[3], double parmd_out[3],
                         const double* pu_r, const double* pd_r,
                         double cwin, const std::string& png_out) {
        std::string s_hu = plot_dir+"/"+tag+"_hu.png";
        std::string s_fu = plot_dir+"/"+tag+"_fu.png";
        std::string s_hd = plot_dir+"/"+tag+"_hd.png";
        std::string s_fd = plot_dir+"/"+tag+"_fd.png";
        TCanvas *ch_u = make_c((std::string(tag)+"_chu").c_str(),"2D U");
        TCanvas *cf_u = make_c((std::string(tag)+"_cfu").c_str(),"fit U");
        TCanvas *ch_d = make_c((std::string(tag)+"_chd").c_str(),"2D D");
        TCanvas *cf_d = make_c((std::string(tag)+"_cfd").c_str(),"fit D");
        FitOneSide(h2u,"u",de_lo,de_hi,gauss_half,rf_half,parmu_out,pu_r,cwin,ch_u,cf_u);

        ch_u->cd(); ch_u->Update(); ch_u->SaveAs(s_hu.c_str());
        cf_u->cd(); cf_u->Update(); cf_u->SaveAs(s_fu.c_str());

        FitOneSide(h2d,"d",de_lo,de_hi,gauss_half,rf_half,parmd_out,pd_r,cwin,ch_d,cf_d);
        ch_d->cd(); ch_d->Update(); ch_d->SaveAs(s_hd.c_str());
        cf_d->cd(); cf_d->Update(); cf_d->SaveAs(s_fd.c_str());
        delete ch_u; delete cf_u; delete ch_d; delete cf_d;

        std::string cmd = "montage " + s_hu + " " + s_hd + " " + s_fu + " " + s_fd
            + " -tile 2x2 -geometry +0+0 " + png_out + " > /dev/null 2>&1";
        gSystem->Exec(cmd.c_str());
        gSystem->Exec(("rm -f "+s_hu+" "+s_fu+" "+s_hd+" "+s_fd).c_str());
        std::printf("  %s saved: %s\n", tag, png_out.c_str());
    };

    // pass-1
    std::printf("  [pass-1] ...\n");
    double parmu1[3] = {0.0}, parmd1[3] = {0.0};
    save_page("p1", parmu1, parmd1, nullptr, nullptr, 0.0, png_p1);

    // pass-2
    std::printf("  [pass-2] ...\n");
    const double cut_win = 0.5;
    double parmu[3] = {0.0}, parmd[3] = {0.0};
    double* pu_rough = (std::abs(parmu1[0]) > 1e-6) ? parmu1 : nullptr;
    double* pd_rough = (std::abs(parmd1[0]) > 1e-6) ? parmd1 : nullptr;
    save_page("p2", parmu, parmd, pu_rough, pd_rough, cut_win, png_p2);
    delete h2u;
    delete h2d;


    std::printf("[2b/3] Verification ...\n");

    double rf_period_check = (rf_period > 0) ? rf_period : 1.966;
    double rf_half_check   = 0.5 * rf_period_check;
    TH2D* h2u_corr = new TH2D("h_rfphc_u_corr",
        Form("RF1st (corrected) vs deT0-U (run%05d);deT0-U [MIP];CRF1st fold [ns]", run_num),
        nslice, 0.0, de_hist_max, rf_ny, -rf_half_check, rf_half_check);
    TH2D* h2d_corr = new TH2D("h_rfphc_d_corr",
        Form("RF1st (corrected) vs deT0-D (run%05d);deT0-D [MIP];CRF1st fold [ns]", run_num),
        nslice, 0.0, de_hist_max, rf_ny, -rf_half_check, rf_half_check);
    h2u_corr->SetDirectory(nullptr);
    h2d_corr->SetDirectory(nullptr);

    {
        TChain* tv = new TChain("hyps");
        for (auto& fn : added_files) tv->Add(fn.Data());

        int    nhT0_vv = 0;
        double RF1st_vv = 999.0;
        double udeT0_vv[50], ddeT0_vv[50];
        tv->SetBranchStatus("*", 0);
        tv->SetBranchStatus("nhT0",  1);
        tv->SetBranchStatus("RF1st", 1);
        tv->SetBranchStatus("udeT0", 1);
        tv->SetBranchStatus("ddeT0", 1);
        tv->SetBranchAddress("nhT0",  &nhT0_vv);
        tv->SetBranchAddress("RF1st", &RF1st_vv);
        tv->SetBranchAddress("udeT0", udeT0_vv);
        tv->SetBranchAddress("ddeT0", ddeT0_vv);

        Long64_t nev_v = tv->GetEntries();
        for (Long64_t i = 0; i < nev_v; ++i) {
            tv->GetEntry(i);

            if (RF1st_vv > 900.0) continue;
            double rf1st_v = RF1st_vv;
            if (rf1st_v < rf_ylo || rf1st_v >= rf_yhi) continue;

            if (nhT0_vv < 1) continue;
            double det0_u_v = udeT0_vv[0];
            double det0_d_v = ddeT0_vv[0];
            bool u_ok_v = (det0_u_v == det0_u_v && det0_u_v > 0.0);
            bool d_ok_v = (det0_d_v == det0_d_v && det0_d_v > 0.0);
            if (!u_ok_v || !d_ok_v) continue;

            double cor_u = 0.0, cor_d = 0.0;
            if (det0_u_v > parmu[1] && det0_u_v > de_lo)
                cor_u = parmu[0] / std::sqrt(det0_u_v - parmu[1]) - parmu[2];
            if (det0_d_v > parmd[1] && det0_d_v > de_lo)
                cor_d = parmd[0] / std::sqrt(det0_d_v - parmd[1]) - parmd[2];

            double rf_corr = rf1st_v - (cor_u + cor_d) * 0.5;
            while (rf_corr >  rf_half_check)  rf_corr -= rf_period_check;
            while (rf_corr < -rf_half_check)  rf_corr += rf_period_check;

            h2u_corr->Fill(det0_u_v, rf_corr);
            h2d_corr->Fill(det0_d_v, rf_corr);
        }
        delete tv;
    }

    {
        TCanvas* cc = new TCanvas("c_corr", "RF PHC verification", 1400, 500);
        cc->Divide(2, 1);
        cc->cd(1); gPad->SetLeftMargin(0.13); h2u_corr->Draw("colz");
        cc->cd(2); gPad->SetLeftMargin(0.13); h2d_corr->Draw("colz");
        cc->SaveAs(png_p3.c_str());
        std::printf("  Page 3 saved: %s\n", png_p3.c_str());
        delete cc;
    }

    {
        std::string cmd = std::string("convert ")
            + png_p1 + " " + png_p2 + " " + png_p3 + " " + pdf_path
            + " > /dev/null 2>&1";
        int ret = gSystem->Exec(cmd.c_str());
        if (ret == 0) {
            std::printf("  PDF saved : %s\n", pdf_path.c_str());
            gSystem->Exec(("rm -f " + png_p1 + " " + png_p2 + " " + png_p3).c_str());
        } else {
            std::printf("  [WARN] convert failed; PNG files kept.\n");
        }
    }
    delete h2u_corr;
    delete h2d_corr;


    std::printf("[3/3] Writing RF PHC param -> %s\n", po.Data());

    double p0u = parmu[0] * 0.5, p1u = parmu[1], p2u = parmu[2] * 0.5;
    double p0d = parmd[0] * 0.5, p1d = parmd[1], p2d = parmd[2] * 0.5;

    po.ReplaceAll("//", "/");
    std::ofstream fout(po.Data());
    if (!fout.is_open()) {
        std::cerr << "[ERROR] Cannot write " << po.Data() << std::endl;
        return;
    }
    fout << "# RF PHC (CId=1, PlId=1): Type1 slewing p0=slope*0.5, p1=offset, p2=intercept*0.5\n";
    fout << Form("1\t1\t0\t0\t1\t3\t%.8g\t%.8g\t%.8g\n", p0u, p1u, p2u);
    fout << Form("1\t1\t0\t1\t1\t3\t%.8g\t%.8g\t%.8g\n", p0d, p1d, p2d);
    fout.close();
    std::printf("  RF-U: p0=%.6g  p1=%.6g  p2=%.6g\n", p0u, p1u, p2u);
    std::printf("  RF-D: p0=%.6g  p1=%.6g  p2=%.6g\n", p0d, p1d, p2d);

    std::printf("\n=== Done ===\n");
    std::printf("Created: %s\n", po.Data());
}
