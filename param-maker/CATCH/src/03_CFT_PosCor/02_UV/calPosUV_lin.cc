#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <cmath>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TNtuple.h>
#include <TRandom.h>
#include <TTree.h>
#include <TBranch.h>
#include <TChain.h>
#include <TChainElement.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TAxis.h>

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t MaxHits2 = 60;
const auto qnan = TMath::QuietNaN();

struct Event{
  Int_t nhU_16layer[4];
  Double_t hitMeanSegU_16layer[4][MaxHits2];
  Double_t phiU_16layer[4][MaxHits2];
  Double_t calZU_16layer[4][MaxHits2];
  Double_t seg[4][MaxHits2];
  Double_t phi[4][MaxHits2];
  Double_t phi1[4][MaxHits2];
  Double_t phi2[4][MaxHits2];
  Double_t calz1[4][MaxHits2];
};
static Event event;


void SetBranchAddress(TChain *chain);

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return -1;
  }

  int runnumber = std::atoi(argv[1]);

  TString filename = Form("%s/run%05d/run%05d_CFTTrackingCosmic.root", root_path.Data(), runnumber, runnumber);
  TChain *chain = new TChain("tree");
  chain->Add(filename);

  SetBranchAddress(chain);
  Int_t n_entries = chain->GetEntries();

  filename.Form("%s/run%05d/run%05d_fiberlinear.root", root_path.Data(), runnumber, runnumber);
  TFile *fout = new TFile(filename, "recreate");

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1000);
  c1->Divide(2, 8);
  TH2F *h[4][NumOfSegCFT_UV4];
  TString title, set;
  Double_t par_pol1[2];

  TString pdfname1, pdfname2, pdfname3, pdfname4, txtname;
  pdfname1.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_phi-z1.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname2.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_phi-z2.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname3.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_phi-z3.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname4.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_phi-z4.pdf", pdf_path.Data(), runnumber, runnumber);
  txtname.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_u_pos.txt", txt_path.Data(), runnumber, runnumber);

  for(Int_t i=0; i<4; i++){
    Int_t uv = 2*i;
    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      title.Form("h%d_%d", i, j);
      set.Form("PHI-Z (layer:%d seg:%d)", i, j);
      h[i][j] = new TH2F(title, set, 1080, -360, 720, 400, 0, 400);
    }
  }

  // fill event (one by one)
  for(Int_t i=0; i<n_entries; i++){
    if(i%10000==0){
      std::cout<<i<<std::endl;
    }
    chain->GetEntry(i);
    for(Int_t j=0; j<4; j++){
      Int_t nhU = event.nhU_16layer[j];
      for(Int_t nh=0; nh<nhU; nh++){
	// Clear();
	event.seg[j][nh] = event.hitMeanSegU_16layer[j][nh];
	Double_t ss = event.seg[j][nh];
	if(fabs(ss-(Int_t)ss)<0.001){
	  Double_t calz = event.calZU_16layer[j][nh];
	  event.phi[j][nh] = event.phiU_16layer[j][nh];

	  event.phi1[j][nh] = event.phi[j][nh] + 360;
	  event.phi2[j][nh] = event.phi[j][nh] - 360;
	  event.calz1[j][nh] = calz + offsetCATCH;

	  h[j][(Int_t)ss]->Fill(event.phi[j][nh], event.calz1[j][nh]);
	  h[j][(Int_t)ss]->Fill(event.phi1[j][nh], event.calz1[j][nh]);
	  h[j][(Int_t)ss]->Fill(event.phi2[j][nh], event.calz1[j][nh]);
	}
      }
    }
    // tree->Fill();
  }

  // tree->Write();

  // input pdf an txt file
  std::ofstream ofs(txtname);
  ofs << "### CFT UV phi-z linear par" <<std::endl;

  for(Int_t i=0; i<4; i++){

    ofs << "### UV" << i+1 << std::endl;
    ofs << "### layer \t seg \t par0 \t par1" << std::endl;

    TString pdfname;
    if(i==0)   pdfname = pdfname1;
    if(i==1)   pdfname = pdfname2;
    if(i==2)   pdfname = pdfname3;
    if(i==3)   pdfname = pdfname4;
    c1->Print(pdfname + "[", "pdf");
    Int_t uv = 2*i;

    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      if(j%10==0){
	std::cout<< i << "\t" << j <<std::endl;
      }

      Double_t fit_range1 = 91.;
      Double_t fit_range2 = 449.;
      if (i==0 || i==2) {
        fit_range1 -= 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
        fit_range2 -= 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      } else {
        fit_range1 = -269. + 360*(Double_t)j/(Double_t)NumOfSegCFT[uv];
        fit_range2 = 89. + 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      }
      Int_t num = j%16;
      TF1 *func;
      c1->cd(num+1);
      h[i][j]->Draw();
      h[i][j]->Fit("pol1", "", "", fit_range1, fit_range2);
      func = h[i][j]->GetFunction("pol1");
      if(num==15 || j==NumOfSegCFT[uv]-1){
	c1->Print(pdfname, "pdf");
      }

      if (!func){
	ofs << i << " \t " << j << " \t error \t "  << std::endl;
      }else{
	par_pol1[0] = func->GetParameter(0);
	par_pol1[1] = func->GetParameter(1);
	ofs << i << " \t " << j << " \t " << par_pol1[0] << " \t " << par_pol1[1] <<std::endl;
      }

    }
    c1->Print(pdfname + "]", "pdf");
  }

  ofs.close();
  fout->Write();
  fout->Close();

}

void Clear()
{
  for(Int_t i=0; i<4; i++){
    for(Int_t j=0; j<MaxHits2; j++){
      event.seg[i][j] = qnan;
      event.phi[i][j] = qnan;
      event.phi1[i][j] = qnan;
      event.phi2[i][j] = qnan;
      event.calz1[i][j] = qnan;
    }
  }
}

void SetBranchAddress(TChain *chain)
{
  chain->SetBranchAddress("nhU1_16layer", &event.nhU_16layer[0]);
  chain->SetBranchAddress("nhU2_16layer", &event.nhU_16layer[1]);
  chain->SetBranchAddress("nhU3_16layer", &event.nhU_16layer[2]);
  chain->SetBranchAddress("nhU4_16layer", &event.nhU_16layer[3]);

  chain->SetBranchAddress("hitMeanSegU1_16layer", &event.hitMeanSegU_16layer[0]);
  chain->SetBranchAddress("hitMeanSegU2_16layer", &event.hitMeanSegU_16layer[1]);
  chain->SetBranchAddress("hitMeanSegU3_16layer", &event.hitMeanSegU_16layer[2]);
  chain->SetBranchAddress("hitMeanSegU4_16layer", &event.hitMeanSegU_16layer[3]);

  chain->SetBranchAddress("phiU1_16layer", &event.phiU_16layer[0]);
  chain->SetBranchAddress("phiU2_16layer", &event.phiU_16layer[1]);
  chain->SetBranchAddress("phiU3_16layer", &event.phiU_16layer[2]);
  chain->SetBranchAddress("phiU4_16layer", &event.phiU_16layer[3]);

  chain->SetBranchAddress("calZU1_16layer", &event.calZU_16layer[0]);
  chain->SetBranchAddress("calZU2_16layer", &event.calZU_16layer[1]);
  chain->SetBranchAddress("calZU3_16layer", &event.calZU_16layer[2]);
  chain->SetBranchAddress("calZU4_16layer", &event.calZU_16layer[3]);

}
