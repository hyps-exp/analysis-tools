#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <math.h>

#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TChain.h>
#include <TChainElement.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TGraphErrors.h>

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t CId  = 31;
const Int_t AorT = 0;
const Int_t UorD = 3;

int main(int argc, char **argv)
{
  if (argc != 5) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <run_sim> <run_start> <run_end>"
	      << std::endl;
    return 0;
  }

  int runnumber = std::atoi(argv[1]);
  int run_sim   = std::atoi(argv[2]);
  int run_start = std::atoi(argv[3]);
  int run_end   = std::atoi(argv[4]);

  std::cout << "" << std::endl;
  std::cout << "#D makeConParam.C" << std::endl;
  std::cout << "" << std::endl;

  TString rootfile, pdffile, txtfile;
  rootfile.Form("%s/run%05d/cft_ana_geant4_%05d_%05d_%d.root", root_path.Data(), runnumber, run_start, run_end, run_sim);
  std::cout << "   file name : " << rootfile << std::endl;

  TChain *chain = new TChain("tree");
  chain->Add(rootfile);

  pdffile.Form("%s/run%05d/catch_cft_calib_%05d.pdf", pdf_path.Data(), runnumber, runnumber);
  txtfile.Form("%s/run%05d/catch_cft_calib.txt", txt_path.Data(), runnumber);
  std::ofstream ofs(txtfile);
  ofs << "## CFT Energy Calibration Parameter" << std::endl;

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Divide(2, 4);
  TCanvas *c2 = new TCanvas("c2", "c2", 800, 1200);
  c2->Divide(2, 4);
  TCanvas *c3 = new TCanvas("c3", "c3", 800, 1200);
  c3->Divide(3, 7);
  TCanvas *c4 = new TCanvas("c4", "c4", 800, 1200);
  c4->Divide(3, 7);
  c1->Print(pdffile + "[", "pdf");

  // 1 : simulation, 2 : calculation
  Int_t bin = 20;
  Double_t first = 0.7;
  Double_t last  = 1.2;
  Double_t first_x = 0.5;
  Double_t last_x = 3;
  Double_t first_y = 0.2;
  Double_t last_y = 1.6;
  TH2D *h1[NumOfPlaneCFT];
  TH2D *h2[NumOfPlaneCFT];
  // 0: projectionY, 1: projectionX, 2: combination
  TGraphErrors *gr1[3][NumOfPlaneCFT];
  TGraphErrors *gr2[3][NumOfPlaneCFT];
  TF1 *f1[NumOfPlaneCFT];
  TF1 *f2[NumOfPlaneCFT];


  TString hist, title, cond;
  ofs << "## Simulation" << std::endl;
  ofs << "layer \t par0 \t par1" << std::endl;
  for(Int_t i=0; i<NumOfPlaneCFT; i++){
    if(i%2 ==  1){
      first = 0.6;
      last  = 1.3;
      first_x = 0.3;
      last_x  = 2.8;
      first_y = 0.3;
      last_y  = 1.7;
    }

    c1->cd(i+1);
    hist.Form("h1%d", i);
    title.Form("CFT MIP vs Sim ( layer : %d )", i);
    cond.Form("MaxMipLow_%d[track_proton]:CFT_MaxFiberEdep%d_proton", i, i);
    h1[i] = new TH2D(hist, title, 100, 0, 4, 100, 0, 2);

    chain->Project(hist, cond, "", "colz");
    h1[i]->SetStats(0);
    h1[i]->SetLabelSize(0.05, "x");
    h1[i]->SetLabelSize(0.05, "y");
    h1[i]->Draw();

    // 0: projectionY, 1: projectionX, 2: combination
    Double_t ene_x[3][bin];
    Double_t mip_y[3][bin];
    Double_t error_x[3][bin];
    Double_t error_y[3][bin];
    for(Int_t j=0; j<bin; j++){
      c3->cd(j+1);
      hist.Form("f_%d_%d", i, j);
      TH1D *f = new TH1D(hist, "", 100, 0, 2);
      TF1 *ff;

      Double_t unit = (last_x - first_x)/bin;
      Int_t firstBin = h1[i]->GetXaxis()->FindBin(first_x+unit*j);
      Int_t lastBin  = h1[i]->GetXaxis()->FindBin(first_x+unit*(j+1));
      h1[i]->ProjectionY(hist, firstBin, lastBin);

      f->Fit("gaus");
      ff = f->GetFunction("gaus");
      ene_x[0][j]   = first_x+unit*(j+first_x);
      error_x[0][j] = unit/2;
      mip_y[0][j]   = ff->GetParameter(1);
      error_y[0][j] = ff->GetParError(1);

      unit     = (last_y - first_y)/bin;
      firstBin = h1[i]->GetYaxis()->FindBin(first_y+unit*j);
      lastBin  = h1[i]->GetYaxis()->FindBin(first_y+unit*(j+1));
      h1[i]->ProjectionX(hist, firstBin, lastBin);

      f->Fit("gaus");
      ff = f->GetFunction("gaus");
      ene_x[1][j]   = ff->GetParameter(1);
      error_x[1][j] = ff->GetParError(1);
      mip_y[1][j]   = first_y+unit*(j+first_y);
      error_y[1][j] = unit/2;

    }
    // c3->Print(pdffile);

    Int_t num_x = bin;
    Int_t num_y = bin;
    for(Int_t a=0; a<bin; a++){
      for(Int_t b=0; b<bin; b++){
	if(mip_y[0][a]>mip_y[1][b]){
	  continue;
	}else if(mip_y[0][a]<mip_y[1][b]){
	  if(ene_x[0][a]<ene_x[1][b]){
	    break;
	  }else if(ene_x[0][a]>ene_x[1][b]){
	    num_x = a;
	    num_y = b;
	  }
	}
      }
      if(num_x<bin)
	break;
    }

    Int_t count = 0;
    gr1[2][i] = new TGraphErrors();
    for(Int_t a=0; a<num_x; a++){
      if(mip_y[0][a]<0) continue;
      gr1[2][i]->SetPoint(count, ene_x[0][a], mip_y[0][a]);
      gr1[2][i]->SetPointError(count, error_x[0][a], error_y[0][a]);
      count++;
    }
    for(Int_t b=num_y; b<bin; b++){
      if(ene_x[1][b]<0) continue;
      gr1[2][i]->SetPoint(count, ene_x[1][b], mip_y[1][b]);
      gr1[2][i]->SetPointError(count, error_x[1][b], error_y[1][b]);
      count++;
    }

    hist.Form("f1%d", i);
    f1[i] = new TF1(hist, "[0]*(1 - exp(-[1]*x/[0]))", first, last);
    f1[i]->SetParameters(2.4, 1);
    gr1[0][i] = new TGraphErrors(bin, ene_x[0], mip_y[0], error_x[0], error_y[0]);
    gr1[0][i]->SetMarkerStyle(20);
    gr1[0][i]->SetMarkerColor(kRed);

    gr1[1][i] = new TGraphErrors(bin, ene_x[1], mip_y[1], error_x[1], error_y[1]);
    gr1[1][i]->SetMarkerStyle(20);
    gr1[1][i]->SetMarkerColor(kBlue);

    // gr1[2][i] = new TGraphErrors(bin, ene_x[2], mip_y[2], error_x[2], error_y[2]);
    gr1[2][i]->SetMarkerStyle(20);
    gr1[2][i]->SetMarkerColor(kGreen);

    std::cout << "#D Fit (sim)" << std::endl;
    std::cout << "   layer : " << i << std::endl;
    c1->cd(i+1);
    gr1[0][i]->Draw("P");
    gr1[1][i]->Draw("P");
    gr1[2][i]->Draw("P");
    c2->cd(i+1);
    gr1[0][i]->Draw("AP");
    gr1[1][i]->Draw("P");
    gr1[2][i]->Draw("P");
    gr1[2][i]->Fit(hist);
    Double_t par0 = f1[i]->GetParameter(0);
    Double_t par1 = f1[i]->GetParameter(1);

    c1->cd(i+1);
    f1[i]->SetRange(0, 4);
    f1[i]->Draw("same");

    ofs << CId << "\t" << i << "\t" << 0 << "\t" << AorT << "\t" << UorD << "\t"
	<< std::fixed << std::setprecision(6) << par0 << "\t"
	<< std::fixed << std::setprecision(6) << par1 << std::endl;

  }
  c1->Print(pdffile);
  c2->Print(pdffile);
  c1->Print(pdffile + "]", "pdf");

}
