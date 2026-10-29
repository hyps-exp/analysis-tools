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

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return -1;
  }

  int runnumber = std::atoi(argv[1]);

  TString filename = Form("%s/run%05d/run%05d_fiberpol5_3.root", root_path.Data(), runnumber, runnumber);
  TFile *fin = new TFile(filename);

  filename.Form("%s/run%05d/run%05d_fiberpoint.root", root_path.Data(), runnumber, runnumber);
  TFile *fout = new TFile(filename, "recreate");
  fin->cd();

  TH2F *h1[4][NumOfSegCFT_UV4];
  TH1D *h2[4][NumOfSegCFT_UV4][36];
  TString title, set;
  Double_t par_pol1[4][NumOfSegCFT[6]][2];
  Double_t par_pol5[4][NumOfSegCFT[6]][6];
  Double_t par_point[4][NumOfSegCFT[6]][36];

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1000);
  c1->Divide(2, 8);

  TString pdfname1, pdfname2, pdfname3, pdfname4, txtname;
  pdfname1.Form("%s/run%05d/CATCH_calPosUV_point_%05d_phi-z1.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname2.Form("%s/run%05d/CATCH_calPosUV_point_%05d_phi-z2.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname3.Form("%s/run%05d/CATCH_calPosUV_point_%05d_phi-z3.pdf", pdf_path.Data(), runnumber, runnumber);
  pdfname4.Form("%s/run%05d/CATCH_calPosUV_point_%05d_phi-z4.pdf", pdf_path.Data(), runnumber, runnumber);
  txtname.Form("%s/run%05d/CATCH_calPosUV_point_%05d_u_pos.txt", txt_path.Data(), runnumber, runnumber);

  for(Int_t i=0; i<4; i++){
    Int_t uv = 2*i;
    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      title.Form("h%d", (i+1)*1000000+j*100);
      set.Form("ResPHI-Z (layer:%d seg:%d)", i, j);
      h1[i][j] = new TH2F(title, set, 1080, -360, 720, 40, -5, 5);
      for(Int_t k=0; k<36; k++){
	title.Form("h%d", (i+1)*1000000+100000+j*100+k);
	set.Form("ResPHI-Z (layer:%d seg:%d angle:%d - %d)", i, j, k*10, (k+1)*10);
	h2[i][j][k] = new TH1D(title, set, 40, -5, 5);
      }
    }
  }

  // ProjectionY and GetMean
  for(Int_t i=0; i<4; i++){
    std::cout << "UV" << i+1 << std::endl;
    Int_t uv = i*2;
    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      TString name;
      name.Form("h%d", (i+1)*1000+j);
      h1[i][j] = (TH2F *)fin->Get(name);

      Double_t fit_range1 = 91.;
      Double_t fit_range2 = 449.;
      if (i==0 || i==2) {
	fit_range1 -= 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
	fit_range2 -= 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      } else {
	fit_range1 = -269. + 360*(Double_t)j/(Double_t)NumOfSegCFT[uv];
	fit_range2 = 89. + 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      }
      for(Int_t k=0; k<36; k++){
	Double_t point_range1 = fit_range1 + k*10;
	Double_t point_range2 = point_range1 + 10;
	if(!h1[i][j]){
	  par_point[i][j][k] = 0;
	}else{
	  h2[i][j][k] = h1[i][j]->ProjectionY("", point_range1, point_range2);
	  par_point[i][j][k] = h2[i][j][k]->GetMean();
	  // if(par_point[i][j][k] == 0){
	  //   std::cout << i << " " << j << " " << k << std::endl;
	  // }
	}
      }
    }
  }

  //fit, write text, and save pdf

  std::ofstream ofs(txtname);
  ofs << "### CFT UV phi-z pol5 point par vol.1" <<std::endl;

  for(Int_t i=0; i<4; i++){

    std::cout << "UV" << i+1 << std::endl;

    ofs << "### UV" << i+1 << std::endl;
    ofs << "### typeID \t layer \t seg \t angle0 \t data point * 36" << std::endl;

    Int_t uv = 2*i;

    TString pdfname;
    if(i==0) pdfname = pdfname1;
    if(i==1) pdfname = pdfname2;
    if(i==2) pdfname = pdfname3;
    if(i==3) pdfname = pdfname4;
    c1->Print(pdfname + "[", "pdf");

    for(Int_t j=0; j<NumOfSegCFT[uv]; j++){
      Double_t fit_range1 = 91.;
      Double_t fit_range2 = 449.;
      if (i==0 || i==2) {
	fit_range1 -= 360.*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      } else {
	fit_range1 = -269. + 360*(Double_t)j/(Double_t)NumOfSegCFT[uv];
      }

      Double_t x[36], y[36];
      for(Int_t k=0; k<36; k++){
	x[k] = fit_range1 + k*10;
	y[k] = par_point[i][j][k];
      }

      Int_t num = j%16;
      c1->cd(num+1);
      h1[i][j]->Draw();
      TGraph *gr = new TGraph(36, x, y);
      gr->SetMarkerColor(2);
      gr->SetMarkerStyle(1);
      gr->Draw("P");

      if(num==15){
	c1->Print(pdfname, "pdf");
      }

      ofs << 2 << " " << i << " " << j << " " << fit_range1 << " "
	  << par_point[i][j][0] << " "
	  << par_point[i][j][1] << " "
	  << par_point[i][j][2] << " "
	  << par_point[i][j][3] << " "
	  << par_point[i][j][4] << " "
	  << par_point[i][j][5] << " "
	  << par_point[i][j][6] << " "
	  << par_point[i][j][7] << " "
	  << par_point[i][j][8] << " "
	  << par_point[i][j][9] << " "
	  << par_point[i][j][10] << " "
	  << par_point[i][j][11] << " "
	  << par_point[i][j][12] << " "
	  << par_point[i][j][13] << " "
	  << par_point[i][j][14] << " "
	  << par_point[i][j][15] << " "
	  << par_point[i][j][16] << " "
	  << par_point[i][j][17] << " "
	  << par_point[i][j][18] << " "
	  << par_point[i][j][19] << " "
	  << par_point[i][j][20] << " "
	  << par_point[i][j][21] << " "
	  << par_point[i][j][22] << " "
	  << par_point[i][j][23] << " "
	  << par_point[i][j][24] << " "
	  << par_point[i][j][25] << " "
	  << par_point[i][j][26] << " "
	  << par_point[i][j][27] << " "
	  << par_point[i][j][28] << " "
	  << par_point[i][j][29] << " "
	  << par_point[i][j][30] << " "
	  << par_point[i][j][31] << " "
	  << par_point[i][j][32] << " "
	  << par_point[i][j][33] << " "
	  << par_point[i][j][34] << " "
	  << par_point[i][j][35] << std::endl;
    }
    c1->Print(pdfname + "]", "pdf");
  }

  ofs.close();
  fout->Write();
  fout->Close();

}
