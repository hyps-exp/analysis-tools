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

struct Event{
  Int_t nhU_16layer[4];
  Double_t hitMeanSegU_16layer[4][MaxHits2];
  Double_t phiU_16layer[4][MaxHits2];
  Double_t calZU_16layer[4][MaxHits2];
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

  filename.Form("%s/run%05d/run%05d_fiberpol5_1.root", root_path.Data(), runnumber, runnumber);
  TFile *fout = new TFile(filename, "recreate");

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1000);
  c1->Divide(2, 8);

  TH2F *h[4][NumOfSegCFT_UV4];
  TString title, set;
  Double_t par_pol1[4][NumOfSegCFT[6]][2];
  Double_t par_pol5[4][NumOfSegCFT[6]][6];

  TString pdfname1, pdfname2, pdfname3, pdfname4, txtname;
  pdfname1.Form("%s/run%05d/CATCH_calPosUV_res1_%05d_phi-z1.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname2.Form("%s/run%05d/CATCH_calPosUV_res1_%05d_phi-z2.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname3.Form("%s/run%05d/CATCH_calPosUV_res1_%05d_phi-z3.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname4.Form("%s/run%05d/CATCH_calPosUV_res1_%05d_phi-z4.pdf", pdf_path.Data(), runnumber, runnumber);
  txtname.Form("%s/run%05d/CATCH_calPosUV_res1_%05d_u_pos_pol5.txt", txt_path.Data(), runnumber, runnumber);

  filename.Form("%s/run%05d/CATCH_calPosUV_lin_%05d_u_pos.txt", txt_path.Data(), runnumber, runnumber);
  std::ifstream txtfile(filename);
  if(!txtfile.is_open()){
    std::cerr << "#E " << "file open fail : " << filename << std::endl;
    return -1;
  }

  std::string line;
  while(txtfile.good() && std::getline(txtfile, line)){
    if(line[0] == '#') continue;
    std::istringstream input_line( line );
    Int_t layer, seg;
    Double_t pol0, pol1;
    if(input_line >> layer >> seg >> pol0 >> pol1){
      par_pol1[layer][seg][0] = pol0;
      par_pol1[layer][seg][1] = pol1;
    }
  }

  for(Int_t i=0; i<4; i++){
    Int_t uv = 2*i;
    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      title.Form("h%d", (i+1)*1000+j);
      set.Form("ResPHI-Z (layer:%d seg:%d)", i, j);
      h[i][j] = new TH2F(title, set, 1080, -360, 720, 40, -5, 5);
    }
  }

  Int_t ll1=0;
  Int_t ll2=0;
  Int_t ll3=0;
  // fill event (one by one)
  for(Int_t i=0; i<n_entries; i++){
    if(i%10000==0){
      std::cout<<i<<std::endl;
    }
    chain->GetEntry(i);
    for(Int_t j=0; j<4; j++){
      Int_t uv = 2*j;
      Int_t nhU = event.nhU_16layer[j];
      for(Int_t nh=0; nh<nhU; nh++){
	Double_t seg = event.hitMeanSegU_16layer[j][nh];
	Double_t calz = event.calZU_16layer[j][nh] + offsetCATCH;
	Double_t phi = event.phiU_16layer[j][nh];
	Int_t seg_n = (Int_t)seg;

	if(fabs(seg-seg_n)<0.001){
	  TF1 *f1 = new TF1("f1", "pol1");
	  f1->SetParameters(par_pol1[j][seg_n][0], par_pol1[j][seg_n][1]);

	  Double_t phi2 = phi + 360;
	  Double_t phi3 = phi - 360;

	  Double_t cor_calz1 = f1->Eval(phi);
	  Double_t cor_calz2 = f1->Eval(phi2);
	  Double_t cor_calz3 = f1->Eval(phi3);

	  Double_t res_calz1 = calz - cor_calz1;
	  Double_t res_calz2 = calz - cor_calz2;
	  Double_t res_calz3 = calz - cor_calz3;

	  Double_t fit_range1 = 91.;
	  Double_t fit_range2 = 449.;
	  if (j==0 || j==2) {
	    fit_range1 -= 360.*(Double_t)seg_n/(Double_t)NumOfSegCFT[uv];
	    fit_range2 -= 360.*(Double_t)seg_n/(Double_t)NumOfSegCFT[uv];
	  } else {
	    fit_range1 = -269. + 360*(Double_t)seg_n/(Double_t)NumOfSegCFT[uv];
	    fit_range2 = 89. + 360.*(Double_t)seg_n/(Double_t)NumOfSegCFT[uv];
	  }

	  if(phi>=fit_range1 && phi<=fit_range2){

	    ll1 = seg_n;
	    h[j][seg_n]->Fill(phi, res_calz1);
	    h[j][seg_n]->Fill(phi2, res_calz1);
	    h[j][seg_n]->Fill(phi3, res_calz1);
	  }else if(phi<fit_range1){

	    ll2 = seg_n;
	    h[j][seg_n]->Fill(phi, res_calz2);
	    h[j][seg_n]->Fill(phi2, res_calz2);
	    h[j][seg_n]->Fill(phi3, res_calz2);
	  }else if(phi>fit_range2){

	    ll3 = seg_n;
	    h[j][seg_n]->Fill(phi, res_calz3);
	    h[j][seg_n]->Fill(phi2, res_calz3);
	    h[j][seg_n]->Fill(phi3, res_calz3);
	  }else{
	    std::cout << "#E cannot fill event : layer = " << j << ", seg = " << seg_n << std::endl;
	  }
	}
      }
    }
  }

  //fit, write text, and save pdf

  std::ofstream ofs(txtname);
  ofs << "### CFT UV phi-z pol5 par vol.1" <<std::endl;

  for(Int_t i=0; i<4; i++){

    std::cout << "UV" << i+1 << std::endl;

    ofs << "### UV" << i+1 << std::endl;
    ofs << "### typeid \t layer \t seg \t par0 \t par1 \t par2 \t par3 \t par4 \t par5" << std::endl;

    TString pdfname ;
    if(i==0)   pdfname = pdfname1;
    if(i==1)   pdfname = pdfname2;
    if(i==2)   pdfname = pdfname3;
    if(i==3)   pdfname = pdfname4;
    c1->Print(pdfname + "[", "pdf");
    Int_t uv = 2*i;

    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
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
      TF1 *f2;

      c1->cd(num+1);
      h[i][j]->Draw();
      h[i][j]->Fit("pol5", "", "", fit_range1, fit_range2);
      f2 = h[i][j]->GetFunction("pol5");

      if(num==15 || j==NumOfSegCFT[uv]-1){
	c1->Print(pdfname, "pdf");
      }

      if (!f2){
	ofs << 1 << " " << i << " " << j << " error"  << std::endl;
      }else{
	par_pol5[i][j][0] = f2->GetParameter(0);
	par_pol5[i][j][1] = f2->GetParameter(1);
	par_pol5[i][j][2] = f2->GetParameter(2);
	par_pol5[i][j][3] = f2->GetParameter(3);
	par_pol5[i][j][4] = f2->GetParameter(4);
	par_pol5[i][j][5] = f2->GetParameter(5);
	ofs << 1 << " " << i << " " << j << " "
	    << par_pol5[i][j][0] + par_pol1[i][j][0] << " "
	    << par_pol5[i][j][1] + par_pol1[i][j][1] << " "
	    << par_pol5[i][j][2] << " "
	    << par_pol5[i][j][3] << " "
	    << par_pol5[i][j][4] << " "
	    << par_pol5[i][j][5] << std::endl;
      }

    }
    c1->Print(pdfname + "]", "pdf");
  }


  ofs.close();
  fout->Write();
  fout->Close();

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
