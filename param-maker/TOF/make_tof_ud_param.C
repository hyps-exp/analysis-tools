/**
 * @file    make_tof_ud_param.c
 * @brief   Analysis TOF U-D (Up/Down TDC) calibration
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
#include <TStyle.h>
#include <TLine.h>
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

// Change the PATH to be referenced later
static const char* DEFAULT_INPUT_PARAM =
    "/home/had/ryogo/work/HYPS/param/dev/HDPRM/My_HodoParam_hyps_run07013";

static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

// fit
bool fitPeak(TH1D* h, int seg, const char* tag,
             double fit_half, double hist_lo, double hist_hi, int min_ent,
             double& mean_out)
{
    mean_out = 0.0;

    if (h->GetEntries() < min_ent) {
        printf("[WARN] Seg %2d %s: entries=%6.0f < %d  -> skip (要データ増)\n",
               seg, tag, h->GetEntries(), min_ent);
        int mbin = h->GetMaximumBin();
        mean_out = h->GetBinCenter(mbin);
        return false;
    }

    int    mbin = h->GetMaximumBin();
    double mpx  = h->GetBinCenter(mbin);
    double flo  = TMath::Max(mpx - fit_half, hist_lo + 0.5);
    double fhi  = TMath::Min(mpx + fit_half, hist_hi - 0.5);

    TF1* gf = new TF1(Form("gf_%s_seg%d", tag, seg), "gaus", flo, fhi);
    gf->SetParameter(1, mpx);
    TFitResultPtr r = h->Fit(gf, "RSQ");

    bool ok = (r.Get() != nullptr) && r->IsValid() && (r->Status() == 0)
              && (gf->GetParameter(2) > 0.1)
              && (gf->GetParameter(2) < 10.0);

    if (ok) {
        mean_out = gf->GetParameter(1);
        printf("[OK  ] Seg %2d %s: mean=%+7.2f ns  sigma=%5.2f ns\n",
               seg, tag, gf->GetParameter(1), gf->GetParameter(2));
    } else {
        mean_out = mpx;
        printf("[WARN] Seg %2d %s: fit failed -> max-bin=%.2f ns  (要確認)\n",
               seg, tag, mpx);
    }

    gf->Draw("same");
    TLine* zero = new TLine(0, 0, 0, h->GetMaximum() * 0.3);
    zero->SetLineColor(kBlue); zero->SetLineStyle(2);
    zero->Draw("same");

    return ok;
}

//main
void make_tof_ud_param(
    const char* root_file    = nullptr,
    const char* input_param  = nullptr,   
    const char* param_out    = nullptr,
    int    min_ent  = 100,
    double fit_half = 3.0,     // Gaussian 
    double hist_lo  = -100.0,
    double hist_hi  =  100.0,
    int    hist_bin = 2000     
) {
    const int    N_SEG    = 48;
    const int    DET_ID   = 14;
    const int    MAXDEPTH = 16;
    const int    MIN_ENT  = min_ent;
    const double FIT_HALF = fit_half;
    const double HIST_LO  = hist_lo;
    const double HIST_HI  = hist_hi;
    const int    HIST_BIN = hist_bin;

    TString rf = root_file ? TString(root_file)
                           : base_path(Form("root/run%05d/run%05d_Hodoscope.root", RUN, RUN));
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(rf);
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString ip = input_param ? TString(input_param) : TString(DEFAULT_INPUT_PARAM);
    TString po = param_out ? TString(param_out)
                           : base_path(Form("output/run%05d/TOF_TDC_param_run%05d.txt", run_num, run_num));
    TString pdf_up = base_path(Form("output/run%05d/tof_ud_calib_up_run%05d.pdf", run_num, run_num));
    TString pdf_dn = base_path(Form("output/run%05d/tof_ud_calib_dn_run%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);

    printf("=== make_tof_ud_param  run %05d ===\n", run_num);
    printf("Root  : %s\n", rf.Data());
    printf("In    : %s\n", ip.Data());
    printf("Out   : %s\n\n", po.Data());


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
    printf("[INFO] Loaded %zu UP + %zu DOWN TOF TDC params from %s\n",
           p_up.size(), p_dn.size(), ip.Data());

 
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

    TH1D* h_up[N_SEG];
    TH1D* h_dn[N_SEG];
    for (int seg = 0; seg < N_SEG; seg++) {
        h_up[seg] = new TH1D(Form("h_up_seg%d", seg),
                             Form("TOF seg%d UP;tofut [ns];Counts", seg),
                             HIST_BIN, HIST_LO, HIST_HI);
        h_dn[seg] = new TH1D(Form("h_dn_seg%d", seg),
                             Form("TOF seg%d DOWN;tofdt [ns];Counts", seg),
                             HIST_BIN, HIST_LO, HIST_HI);
    }

    Long64_t nev = tr->GetEntries();
    printf("[INFO] Reading %lld events from TTree ...\n", nev);
    for (Long64_t ev = 0; ev < nev; ev++) {
        tr->GetEntry(ev);
        for (int seg = 0; seg < N_SEG; seg++) {
            if (!p_up.count(seg) || !p_dn.count(seg)) continue;
            for (int d = 0; d < MAXDEPTH; d++) {
                double raw_u = tofut[seg][d];
                double raw_d = tofdt[seg][d];
                // 生TDCカウントをnsに変換: t_ns = (T_raw - p0) * p1
                if (TMath::Finite(raw_u) && raw_u > 0) {
                    double vu = (raw_u - p_up[seg].p0) * p_up[seg].p1;
                    if (vu > HIST_LO && vu < HIST_HI) h_up[seg]->Fill(vu);
                }
                if (TMath::Finite(raw_d) && raw_d > 0) {
                    double vd = (raw_d - p_dn[seg].p0) * p_dn[seg].p1;
                    if (vd > HIST_LO && vd < HIST_HI) h_dn[seg]->Fill(vd);
                }
            }
        }
    }

    // peak fit
    gStyle->SetOptFit(111);
    gStyle->SetOptStat(1000010);

    TCanvas* c_up = new TCanvas("c_up", "TOF UP TDC Fit",   1800, 2400);
    c_up->Divide(6, 8);
    TCanvas* c_dn = new TCanvas("c_dn", "TOF DOWN TDC Fit", 1800, 2400);
    c_dn->Divide(6, 8);

    double T_up[N_SEG], T_dn[N_SEG];
    bool   ok_up[N_SEG], ok_dn[N_SEG];

    printf("\n--- UP channel fits ---\n");
    for (int seg = 0; seg < N_SEG; seg++) {
        c_up->cd(seg + 1);
        h_up[seg]->Draw();
        ok_up[seg] = fitPeak(h_up[seg], seg, "UP", FIT_HALF, HIST_LO, HIST_HI, MIN_ENT, T_up[seg]);
    }

    printf("\n--- DOWN channel fits ---\n");
    for (int seg = 0; seg < N_SEG; seg++) {
        c_dn->cd(seg + 1);
        h_dn[seg]->Draw();
        ok_dn[seg] = fitPeak(h_dn[seg], seg, "DN", FIT_HALF, HIST_LO, HIST_HI, MIN_ENT, T_dn[seg]);
    }

    c_up->SaveAs(pdf_up.Data());
    c_dn->SaveAs(pdf_dn.Data());
    printf("\n[INFO] Saved: %s\n", pdf_up.Data());
    printf("[INFO] Saved: %s\n",   pdf_dn.Data());

    // ----------------------------------------------------------------
    //  p0_U, p0_D 
    //   T_avg = (T_up + T_dn) / 2  
    //   p0_U_new = p0_U + (T_up - T_avg) / p1_U
    //   p0_D_new = p0_D + (T_dn - T_avg) / p1_D
    // ----------------------------------------------------------------
    std::map<int, double> p0u_new, p0d_new;

    printf("\n%-4s %-9s %-9s %-9s  %-13s %-13s  %-13s %-13s  %s\n",
           "seg", "T_up[ns]", "T_dn[ns]", "T_avg[ns]",
           "p0_U_old", "p0_U_new", "p0_D_old", "p0_D_new", "src");
    printf("%s\n", std::string(100, '-').c_str());

    for (int seg = 0; seg < N_SEG; seg++) {
        if (!p_up.count(seg) || !p_dn.count(seg)) continue;

        double T_avg = (T_up[seg] + T_dn[seg]) / 2.0;

        p0u_new[seg] = p_up[seg].p0 + (T_up[seg] - T_avg) / p_up[seg].p1;
        p0d_new[seg] = p_dn[seg].p0 + (T_dn[seg] - T_avg) / p_dn[seg].p1;

        printf("%3d  %+8.2f  %+8.2f  %+8.2f   %11.3f->%11.3f   %11.3f->%11.3f  [%s/%s]\n",
               seg, T_up[seg], T_dn[seg], T_avg,
               p_up[seg].p0, p0u_new[seg],
               p_dn[seg].p0, p0d_new[seg],
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

    fout << "# TOF TDC Up/Down (CId=14, aort=1): p0=U-D aligned T0, p1=slope (unchanged)\n";
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
}
