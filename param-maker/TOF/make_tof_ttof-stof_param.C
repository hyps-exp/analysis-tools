/**
 * @file    make_tof_ttof-stof_param.c
 * @brief   Analysis TOF absolute timing calibration (tTofCalc - stof)
 * @author  Akao Ryogo
 * @date    2026-10-29
 *
 * Change history:
 *   2026-10-29  Akao Ryogo  First version
 */
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TLine.h>
#include <TFitResult.h>
#include <TMath.h>
#include <TSystem.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>

static const int RUN = 7013;

static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

void make_tof_ttof_stof_param(
    const char* root_file    = nullptr,
    const char* input_param  = nullptr,   
                                           
    const char* param_out    = nullptr,
    int    tof_calc_idx      = 1,       // tTofCalc[idx]: 0=e, 1=π, 2=K, 3=p
    double vtz_lo            = -200.0,  
    double vtz_hi            =  150.0,
    double utgt_lo           = -0.08,   
    double utgt_hi           =  0.08,
    double vtgt_lo           = -0.05,
    double vtgt_hi           =  0.05,
    double hist_lo           = -5.0,    
    double hist_hi           = 15.0,   
    double rms_half          =  1.5,    
    double fit_nsig          =  2.0,    
    double fit_half_min      =  0.2    
) {
    const int    N_SEG    = 48;
    const int    DET_ID   = 14;
    const int    MAX_HITS = 64;
    const int    TCALC_N  = 4;
    const int    MIN_ENT  = 30;
    const double RMS_HALF     = rms_half;
    const double FIT_NSIG     = fit_nsig;
    const double FIT_HALF_MIN = fit_half_min;
    const double HIST_LO  = hist_lo;
    const double HIST_HI  = hist_hi;
    const int    HIST_BIN = (int)((hist_hi - hist_lo) / 0.05 + 0.5);  // 0.05 ns/bin

    TString rf = root_file ? TString(root_file)
                           : base_path(Form("root/run%05d/run%05d_HypsTracking.root", RUN, RUN));
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString ip = input_param ? TString(input_param)
                             : base_path(Form("output/run%05d/TOF_TDC_param_run%05d.txt", run_num, run_num));
    TString po = param_out ? TString(param_out)
                           : base_path(Form("output/run%05d/TOF_ttof-stof_param_run%05d.txt", run_num, run_num));
    TString pdf = base_path(Form("output/run%05d/tof_abs_fit_stof_run%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);

    printf("=== make_tof_ttof-stof_param  run %05d ===\n", run_num);
    printf("ROOT file   : %s\n", rf.Data());
    printf("Input param : %s\n", ip.Data());
    printf("Output param: %s\n", po.Data());
    printf("tTofCalc idx: %d\n", tof_calc_idx);
    printf("vtz  cut    : [%.1f, %.1f] mm\n", vtz_lo, vtz_hi);
    printf("beam veto   : utgt [%.3f, %.3f]  vtgt [%.3f, %.3f]  (outside)\n",
           utgt_lo, utgt_hi, vtgt_lo, vtgt_hi);
    printf("\n");

    // input param file: read HDPRM TOF TDC params (p0, p1)
    struct Param { double p0, p1; };
    std::map<int, Param> p_up, p_dn;

    std::ifstream fin(ip.Data());
    if (!fin.is_open()) {
        std::cerr << "[ERROR] Cannot open " << ip.Data() << std::endl;
        return;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        int cid, plid, seg, aort, ud;
        double p0v, p1v;
        if (!(ss >> cid >> plid >> seg >> aort >> ud >> p0v >> p1v)) continue;
        if (cid != DET_ID || aort != 1) continue;
        if (ud == 0) p_up[seg] = {p0v, p1v};
        if (ud == 1) p_dn[seg] = {p0v, p1v};
    }
    fin.close();
    printf("[INFO] Loaded %zu UP + %zu DOWN TOF TDC params (HDPRM)\n",
           p_up.size(), p_dn.size());


    TFile* f = TFile::Open(rf);
    if (!f || f->IsZombie()) {
        std::cerr << "[ERROR] Cannot open " << rf.Data() << std::endl;
        return;
    }
    TTree* tr = (TTree*)f->Get("hyps");
    if (!tr) {
        std::cerr << "[ERROR] TTree 'hyps' not found in " << rf.Data() << std::endl;
        f->Close();
        return;
    }

    Int_t    ntHyps;
    Double_t tofsegHyps[MAX_HITS];
    Double_t stof[MAX_HITS];
    Double_t tTofCalc[TCALC_N];
    Double_t vtz_Hyps[MAX_HITS];
    Double_t utgtHyps[MAX_HITS];
    Double_t vtgtHyps[MAX_HITS];

    tr->SetBranchStatus("*", 0);
    tr->SetBranchStatus("ntHyps",     1);  tr->SetBranchAddress("ntHyps",     &ntHyps);
    tr->SetBranchStatus("tofsegHyps", 1);  tr->SetBranchAddress("tofsegHyps",  tofsegHyps);
    tr->SetBranchStatus("stof",       1);  tr->SetBranchAddress("stof",         stof);
    tr->SetBranchStatus("tTofCalc",   1);  tr->SetBranchAddress("tTofCalc",     tTofCalc);
    tr->SetBranchStatus("vtz_Hyps",   1);  tr->SetBranchAddress("vtz_Hyps",     vtz_Hyps);
    tr->SetBranchStatus("utgtHyps",   1);  tr->SetBranchAddress("utgtHyps",     utgtHyps);
    tr->SetBranchStatus("vtgtHyps",   1);  tr->SetBranchAddress("vtgtHyps",     vtgtHyps);

    int calc_idx = tof_calc_idx;

    TH1D* h[N_SEG];
    for (int seg = 0; seg < N_SEG; seg++) {
        h[seg] = new TH1D(Form("h_abs_seg%d", seg),
                          Form("TOF abs seg%d;tTofCalc[%d]-stof [ns];Counts", seg, calc_idx),
                          HIST_BIN, HIST_LO, HIST_HI);
    }

    Long64_t nev = tr->GetEntries();
    printf("[INFO] Reading %lld events ...\n", nev);
    for (Long64_t ev = 0; ev < nev; ev++) {
        tr->GetEntry(ev);
        if (calc_idx < 0 || calc_idx >= TCALC_N) continue;
        double t_calc = tTofCalc[calc_idx];
        if (!TMath::Finite(t_calc)) continue;

        for (int j = 0; j < ntHyps; j++) {
            if (TMath::IsNaN(stof[j]) || TMath::IsNaN(vtz_Hyps[j])
                || TMath::IsNaN(utgtHyps[j]) || TMath::IsNaN(vtgtHyps[j])) continue;

            bool vtx_ok    = (vtz_Hyps[j] > vtz_lo && vtz_Hyps[j] < vtz_hi);
            bool beam_veto = (utgtHyps[j] > utgt_lo && utgtHyps[j] < utgt_hi
                           && vtgtHyps[j] > vtgt_lo && vtgtHyps[j] < vtgt_hi);
            if (!vtx_ok || beam_veto) continue;

            int seg = (int)tofsegHyps[j] - 1;  // 1-indexed → 0-indexed
            if (seg < 0 || seg >= N_SEG) continue;
            double diff = t_calc - stof[j];
            if (diff > HIST_LO && diff < HIST_HI) h[seg]->Fill(diff);
        }
    }

    // fit 
    gStyle->SetOptFit(111);
    gStyle->SetOptStat(1000010);

    TCanvas* c = new TCanvas("c_abs", "TOF Abs Timing Fit", 1800, 2500);

    // ヘッダーパッド: 使用した条件を表示
    TPad* pad_hdr = new TPad("pad_hdr", "", 0.0, 0.96, 1.0, 1.0);
    pad_hdr->SetMargin(0.01, 0.01, 0.05, 0.05);
    pad_hdr->Draw();
    pad_hdr->cd();
    TPaveText* pt = new TPaveText(0.0, 0.0, 1.0, 1.0, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(1);
    pt->SetTextAlign(12);
    pt->SetTextSize(0.45);
    pt->AddText(Form("run%05d  |  tTofCalc[%d]-stof  |  "
                     "vtz [%.0f, %.0f] mm  |  "
                     "beam veto: utgt [%.3f, %.3f]  vtgt [%.3f, %.3f]",
                     run_num, tof_calc_idx,
                     vtz_lo, vtz_hi,
                     utgt_lo, utgt_hi, vtgt_lo, vtgt_hi));
    pt->Draw();

    
    c->cd();
    TPad* pad_main = new TPad("pad_main", "", 0.0, 0.0, 1.0, 0.96);
    pad_main->Draw();
    pad_main->cd();
    pad_main->Divide(6, 8);

    double t_peak[N_SEG] = {};
    bool   ok[N_SEG]     = {};

    printf("\n--- TOF absolute timing fits ---\n");
    for (int seg = 0; seg < N_SEG; seg++) {
        pad_main->cd(seg + 1);
        h[seg]->Draw();

        if (h[seg]->GetEntries() < MIN_ENT) {
            printf("[WARN] Seg %2d: entries=%.0f < %d -> max-bin\n",
                   seg, h[seg]->GetEntries(), MIN_ENT);
            t_peak[seg] = h[seg]->GetBinCenter(h[seg]->GetMaximumBin());
            ok[seg] = false;
            continue;
        }

        int    mbin = h[seg]->GetMaximumBin();
        double mpx  = h[seg]->GetBinCenter(mbin);

      
        h[seg]->SetAxisRange(mpx - RMS_HALF, mpx + RMS_HALF);
        double sigma_est = TMath::Max(h[seg]->GetRMS(), FIT_HALF_MIN / FIT_NSIG);
        h[seg]->SetAxisRange(HIST_LO, HIST_HI);

        double fit_half = TMath::Max(FIT_NSIG * sigma_est, FIT_HALF_MIN);
        double flo  = TMath::Max(mpx - fit_half, HIST_LO + 0.1);
        double fhi  = TMath::Min(mpx + fit_half, HIST_HI - 0.1);

        TF1* gf = new TF1(Form("gf_abs_seg%d", seg), "gaus", flo, fhi);
        gf->SetParameter(0, h[seg]->GetMaximum());
        gf->SetParameter(1, mpx);
        gf->SetParameter(2, sigma_est);
        TFitResultPtr r = h[seg]->Fit(gf, "RSQ");

        bool fit_ok = (r.Get() != nullptr) && r->IsValid() && (r->Status() == 0)
                   && (gf->GetParameter(2) > 0.05) && (gf->GetParameter(2) < 2.0)
                   && (TMath::Abs(gf->GetParameter(1) - mpx) < fit_half);

        if (fit_ok) {
            t_peak[seg] = gf->GetParameter(1);
            ok[seg] = true;
            printf("[OK  ] Seg %2d: t_peak=%+7.3f ns  sigma=%.3f ns  (fit_half=%.2f ns)\n",
                   seg, gf->GetParameter(1), gf->GetParameter(2), fit_half);
        } else {
            t_peak[seg] = mpx;
            ok[seg] = false;
            printf("[WARN] Seg %2d: fit failed -> max-bin=%.3f ns  (sigma_est=%.3f fit_half=%.2f)\n",
                   seg, mpx, sigma_est, fit_half);
        }

        TLine* zero = new TLine(0, 0, 0, h[seg]->GetMaximum() * 0.3);
        zero->SetLineColor(kBlue); zero->SetLineStyle(2);
        zero->Draw("same");
    }

    c->SaveAs(pdf.Data());
    printf("\n[INFO] Plot saved: %s\n", pdf.Data());


    printf("\n%-4s %-10s  %-13s %-13s  %-13s %-13s  %s\n",
           "seg", "t_peak[ns]",
           "p0_U_old", "p0_U_new",
           "p0_D_old", "p0_D_new", "src");
    printf("%s\n", std::string(90, '-').c_str());

    std::map<int, double> p0u_new, p0d_new;
    for (int seg = 0; seg < N_SEG; seg++) {
        if (!p_up.count(seg) || !p_dn.count(seg)) continue;
        p0u_new[seg] = p_up[seg].p0 - t_peak[seg] / p_up[seg].p1;
        p0d_new[seg] = p_dn[seg].p0 - t_peak[seg] / p_dn[seg].p1;
        printf("%3d  %+9.3f   %11.3f->%11.3f   %11.3f->%11.3f  [%s]\n",
               seg, t_peak[seg],
               p_up[seg].p0, p0u_new[seg],
               p_dn[seg].p0, p0d_new[seg],
               ok[seg] ? "fit" : "est");
    }

    po.ReplaceAll("//", "/");
    std::ofstream fout(po.Data());
    if (!fout.is_open()) {
        std::cerr << "[ERROR] Cannot write " << po.Data() << std::endl;
        f->Close();
        return;
    }

    fout << "# TOF TDC Up/Down (CId=14, aort=1): absolute timing shift (tTofCalc[" << calc_idx << "]-stof)\n";
    for (int seg = 0; seg < N_SEG; seg++) {
        if (!p0u_new.count(seg)) continue;
        fout << Form("14\t0\t%d\t1\t0\t%.6f\t%.11f\n", seg, p0u_new[seg], p_up[seg].p1);
    }
    for (int seg = 0; seg < N_SEG; seg++) {
        if (!p0d_new.count(seg)) continue;
        fout << Form("14\t0\t%d\t1\t1\t%.6f\t%.11f\n", seg, p0d_new[seg], p_dn[seg].p1);
    }
    fout.close();

    printf("\n[INFO] -> Output param: %s\n", po.Data());
    f->Close();
    printf("\n=== Done ===\n");
}
