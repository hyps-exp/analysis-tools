/**
 * @file    update_PHC_TOF.c
 * @brief   Analysis TOF time-walk (PHC) calibration
 * @author  Akao Ryogo
 * @date    2026-10-30
 *
 * Change history:
 *   2026-10-30  Akao Ryogo  First version
 */
#include <TFile.h>
#include <TChain.h>
#include <TTree.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TF1.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLine.h>
#include <TSystem.h>
#include <TMath.h>
#include <TFitResult.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const int RUN  = 7013;
static const int NSEG = 48;


static const double TMPL_P0 = 2.0;
static const double TMPL_P1 = 0.0;
static const double TMPL_P2 = 2.0;

static TString base_dir()
{
    TString f = __FILE__;
    if (!gSystem->IsAbsoluteFileName(f))
        f = TString(gSystem->WorkingDirectory()) + "/" + f;
    return gSystem->DirName(gSystem->DirName(gSystem->DirName(f)));
}
static TString base_path(const char* rel) { return base_dir() + "/" + rel; }

struct PHCPar { double p0, p1, p2; bool ok; };

// x-axis: 1/sqrt(dE) in [0,2], y-axis: dt [ns]
static TGraphErrors* make_walk_graph(TH2D* h2, double half_win = 0.5)
{
    if (!h2) return nullptr;
    std::vector<double> gx, gy, gex, gey;

    int n_bins_x = h2->GetNbinsX();
    for (int ib = 1; ib <= n_bins_x; ib++) {
        TH1D* proj = h2->ProjectionY(Form("_pw_%s_%d", h2->GetName(), ib), ib, ib);
        if (proj->GetEntries() > 1) {
            int max_bin = proj->GetMaximumBin();
            double peak = proj->GetBinCenter(max_bin);
            TF1* gfit = new TF1(Form("_gf_%s_%d", h2->GetName(), ib),
                                "gaus", peak - half_win, peak + half_win);
            proj->Fit(gfit, "QR");
            double mean = gfit->GetParameter(1);
            double err  = gfit->GetParError(1);
            if (err > 0 && std::fabs(mean) < 2.0) {
                gx.push_back(h2->GetXaxis()->GetBinCenter(ib));
                gy.push_back(mean);
                gex.push_back(h2->GetXaxis()->GetBinWidth(ib) / 2.0);
                gey.push_back(err);
            }
            delete gfit;
        }
        delete proj;
    }

    if (!gx.empty())
        return new TGraphErrors((int)gx.size(),
                                gx.data(), gy.data(), gex.data(), gey.data());
    return nullptr;
}

void update_PHC_TOF(
    const char* root_file = nullptr,  
    const char* param_out = nullptr)
{
    TString rf = root_file ? TString(root_file)
                           : base_path(Form("root/run%05d/run%05d_HypsTracking.root", RUN, RUN));

    // Build TChain from comma-separated file list
    TChain* tree = new TChain("hyps");
    std::vector<TString> added_files;
    {
        std::string tok;
        std::istringstream ss(rf.Data());
        while (std::getline(ss, tok, ',')) {
            TString fn = TString(tok.c_str()).Strip(TString::kBoth);
            if (fn.IsNull()) continue;
            if (!gSystem->AccessPathName(fn)) {
                tree->Add(fn);
                added_files.push_back(fn);
            } else {
                fprintf(stderr, "[WARN] File not found (skipped): %s\n", fn.Data());
            }
        }
    }
    if (added_files.empty()) {
        fprintf(stderr, "[ERROR] No valid ROOT files loaded\n");
        delete tree; return;
    }

    // Run number for the output names comes from the first input file
    int run_num = RUN;
    {
        TString bn = gSystem->BaseName(added_files.front());
        Ssiz_t idx = bn.Index("run");
        if (idx != kNPOS) sscanf(bn.Data() + idx, "run%d", &run_num);
    }
    TString po  = param_out ? TString(param_out)
                            : base_path(Form("output/run%05d/TOF_PHC_param_run%05d.txt", run_num, run_num));
    TString pdf = base_path(Form("output/run%05d/update_PHC_TOF_%05d.pdf", run_num, run_num));

    gSystem->mkdir(gSystem->DirName(po), kTRUE);
    gSystem->mkdir(gSystem->DirName(pdf), kTRUE);

    printf("=== update_PHC_TOF  run %05d ===\n", run_num);
    for (int i = 0; i < (int)added_files.size(); i++)
        printf("Root[%d]: %s\n", i, added_files[i].Data());
    printf("  entries : %lld\n", tree->GetEntries());
    printf("Out   : %s\n\n", po.Data());

    PHCPar tmpl_u[NSEG], tmpl_d[NSEG];
    for (int s = 0; s < NSEG; s++) {
        tmpl_u[s] = {TMPL_P0, TMPL_P1, TMPL_P2, false};
        tmpl_d[s] = {TMPL_P0, TMPL_P1, TMPL_P2, false};
    }

    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);
    { TCanvas* c = new TCanvas("ctmp","",1,1); c->Print(pdf+"[","pdf"); delete c; }

       TString cut_base =
        "ntHyps==1&&chisqrHyps[0]<50"
        "&&xtgtHyps[0]<50&&xtgtHyps[0]>-60"
        "&&ytgtHyps[0]>-50&&ytgtHyps[0]<50"
        "&&(utgtHyps[0]<-0.05||utgtHyps[0]>0.05)"
        "&&(vtgtHyps[0]<-0.05||vtgtHyps[0]>0.05)"
        "&&vtz_Hyps[0]<150&&vtz_Hyps[0]>-200"
        "&&fabs(ytofHyps[0])>65";

    PHCPar res_u[NSEG], res_d[NSEG];

    printf("%-5s  %-4s  %8s  %5s  %8s  %s\n","Seg","U/D","p[0]","p[1]","p[2]","status");
    printf("%s\n", std::string(46, '-').c_str());

    for (int i = 0; i < NSEG; i++) {
        int seg = i + 1;  

        TCanvas* c = new TCanvas(Form("c_phc_%d", i),
                                 Form("PHC seg%d", i), 1600, 800);
        c->Divide(2, 2, 0.005, 0.005);

        // ---- Up side ----
        // y = utTofSeg[i] - tTofCalc[1] + ytofHyps*0.5/77.3511
        {
            TH2D* h2 = new TH2D(Form("h2_up_%d", i),
                                Form("Up Seg%d;1/#sqrt{dE_{up}} [MeV^{-1/2}];t_{meas}-t_{calc} [ns]", i),
                                50, 0., 2.0, 200, -10., 10.);
            TString cut = Form("%s&&udeTofSeg[%d]>0", cut_base.Data(), i);
            tree->Draw(Form("utTofSeg[%d]-tTofCalc[1]+ytofHyps[0]*0.5/77.3511:1.0/sqrt(udeTofSeg[%d])>>h2_up_%d", i, i, i),
                       cut, "goff");

            c->cd(1);
            gPad->SetLeftMargin(0.12); gPad->SetRightMargin(0.15); gPad->SetBottomMargin(0.13);
            if (h2->GetEntries() > 0) h2->Draw("COLZ");

            TGraphErrors* gr = make_walk_graph(h2, 1.0);
            c->cd(1);
            if (h2->GetEntries() > 0) h2->Draw("COLZ");
            if (gr) { gr->SetMarkerStyle(20); gr->Draw("P SAME"); }

            res_u[i] = tmpl_u[i]; res_u[i].ok = false;

            c->cd(3);
            gPad->SetLeftMargin(0.12); gPad->SetBottomMargin(0.13);
            if (gr) {
                gr->SetTitle(Form("Up Seg%d fit;1/#sqrt{dE};t_{meas}-t_{calc}", i));
                gr->Draw("AP");
                gr->Fit("pol1", "Q", "", 0.4, 1.8);
                TF1* fit = gr->GetFunction("pol1");
                if (fit) {
                    double p0f =  fit->GetParameter(1);  // slope → PHC p[0]
                    double p2f = -fit->GetParameter(0);  // -intercept → PHC p[2]
                    if (p0f > 0.1 && p0f < 5.0 && p2f > -5.0 && p2f < 5.0)
                        res_u[i] = {p0f, 0., p2f, true};
                }
            }
            printf("%-5d  Up    %8.4f  %5.2f  %8.4f  %s\n",
                   i, res_u[i].p0, res_u[i].p1, res_u[i].p2,
                   res_u[i].ok ? "fit" : "template");
        }

        // ---- Down side ----
        // y = dtTofSeg[i] - tTofCalc[1] - ytofHyps*0.5/77.3511  (pion + propagation correction)
        {
            TH2D* h2 = new TH2D(Form("h2_dn_%d", i),
                                Form("Down Seg%d;1/#sqrt{dE_{down}} [MeV^{-1/2}];t_{meas}-t_{calc} [ns]", i),
                                50, 0., 2.0, 200, -10., 10.);
            TString cut = Form("%s&&ddeTofSeg[%d]>0", cut_base.Data(), i);
            tree->Draw(Form("dtTofSeg[%d]-tTofCalc[1]-ytofHyps[0]*0.5/77.3511:1.0/sqrt(ddeTofSeg[%d])>>h2_dn_%d", i, i, i),
                       cut, "goff");

            c->cd(2);
            gPad->SetLeftMargin(0.12); gPad->SetRightMargin(0.15); gPad->SetBottomMargin(0.13);
            if (h2->GetEntries() > 0) h2->Draw("COLZ");

            TGraphErrors* gr = make_walk_graph(h2, 1.0);
            c->cd(2);
            if (h2->GetEntries() > 0) h2->Draw("COLZ");
            if (gr) { gr->SetMarkerStyle(20); gr->Draw("P SAME"); }

            res_d[i] = tmpl_d[i]; res_d[i].ok = false;

            c->cd(4);
            gPad->SetLeftMargin(0.12); gPad->SetBottomMargin(0.13);
            if (gr) {
                gr->SetTitle(Form("Down Seg%d fit;1/#sqrt{dE};t_{meas}-t_{calc}", i));
                gr->Draw("AP");
                gr->Fit("pol1", "Q", "", 0.4, 1.8);
                TF1* fit = gr->GetFunction("pol1");
                if (fit) {
                    double p0f =  fit->GetParameter(1);
                    double p2f = -fit->GetParameter(0);
                    if (p0f > 0.1 && p0f < 5.0 && p2f > -5.0 && p2f < 5.0)
                        res_d[i] = {p0f, 0., p2f, true};
                }
            }
            printf("%-5d  Down  %8.4f  %5.2f  %8.4f  %s\n",
                   i, res_d[i].p0, res_d[i].p1, res_d[i].p2,
                   res_d[i].ok ? "fit" : "template");
        }

        c->Print(pdf, "pdf");
        delete c;
    }

   
    po.ReplaceAll("//", "/");
    std::ofstream fout(po.Data());
    if (!fout) {
        fprintf(stderr, "[ERROR] Cannot write %s\n", po.Data());
    } else {
        fout << "# TOF Up  (TGraph Gaussian-slice fit: p[0]=slope, p[2]=intercept)\n";
        for (int s = 0; s < NSEG; s++)
            fout << Form("14\t0\t%d\t0\t1\t3\t%.6f\t%.6f\t%.6f\n",
                         s, res_u[s].p0, res_u[s].p1, res_u[s].p2);
        fout << "# TOF Down  (TGraph Gaussian-slice fit: p[0]=slope, p[2]=intercept)\n";
        for (int s = 0; s < NSEG; s++)
            fout << Form("14\t0\t%d\t1\t1\t3\t%.6f\t%.6f\t%.6f\n",
                         s, res_d[s].p0, res_d[s].p1, res_d[s].p2);
        fout.close();
        printf("\n[INFO] Wrote %s\n", po.Data());
    }

    int n_fit = 0;
    for (int s = 0; s < NSEG; s++) {
        if (res_u[s].ok) n_fit++;
        if (res_d[s].ok) n_fit++;
    }
    printf("Summary: %d/%d fitted, %d used template\n", n_fit, 2*NSEG, 2*NSEG - n_fit);

    { TCanvas* c = new TCanvas("cend","",1,1); c->Print(pdf+"]","pdf"); delete c; }
    delete tree;
    printf("[INFO] -> %s\n", pdf.Data());
    printf("=== Done ===\n");
}
