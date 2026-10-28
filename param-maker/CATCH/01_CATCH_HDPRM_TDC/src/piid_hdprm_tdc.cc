#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <string>
#include <iomanip>
#include <math.h>

#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TTree.h>
#include <TBranch.h>
#include <TChain.h>
#include <TChainElement.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TString.h>
#include <TLatex.h>


const Int_t NumOfSegPiID = 32;
const Int_t CId  = 33;
const Int_t PlId = 0;
const Int_t AorT = 1;
const Int_t UorD = 0;
const Double_t p1 = -1.0;


int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << "  <runnumber>"
	      << std::endl;
    return 0;
  }

  Int_t runnumber;
  runnumber = std::stoi(argv[1]);

  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << "Start ./cft_hdprm_tdc runnumber:" << runnumber << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;

  Double_t hdprm_new[NumOfSegPiID];

  TString rootfile, txtfile, pdffile;

  rootfile.Form("../../rootfile/run%05d_CFT.root", runnumber);
  txtfile.Form("../../output/run%05d/CATCH_piid_hdprm_tdc_%05d.txt", runnumber, runnumber);
  pdffile.Form("../../output/run%05d/CATCH_piid_hdprm_tdc_%05d.pdf", runnumber, runnumber);

  TFile *file = new TFile(rootfile);

  std::ofstream outputfile(txtfile);
  outputfile << "####################################################" << std::endl;
  outputfile << "# PiID HDPRM TDC" << std::endl;
  outputfile << "####################################################" << std::endl;


  std::cout << "#D Start calculating" << std::endl;
  TH2D *h;
  TH1D *h_piid[NumOfSegPiID];
  TF1 *f[NumOfSegPiID];
  TString title, hist;
  TCanvas *c1 = new TCanvas("c1", "c1", 1000, 800);
  c1->Divide(4, 8);
  c1->Print(pdffile + "[", "pdf");

  hist.Form("h8100");
  h = (TH2D*)gROOT->FindObject(hist);
  Int_t num;
  for(Int_t j=0; j<NumOfSegPiID; j++){
    num = j%32;
    c1->cd(num+1);

    title.Form("h_piid_%02d", j);
    Int_t segment = h->GetXaxis()->FindBin(j);

    h_piid[j] = (TH1D*)h->ProjectionY(title, segment, segment);


    h_piid[j]->Draw();
    Int_t max_bin = h_piid[j]->GetMaximumBin();
    Double_t max  = h_piid[j]->GetBinCenter(max_bin);
    h_piid[j]->Fit("gaus", "", "", max-10, max+10);

    f[j] = h_piid[j]->GetFunction("gaus");
    if(!f[j])
      hdprm_new[j] = 0;
    else
      hdprm_new[j] = f[j]->GetParameter(1);

    TLatex tex;
    tex.SetTextSize(0.1);
    tex.SetTextColor(kBlack);
    TString write = Form("mean : %f", hdprm_new[j]);
    tex.DrawLatex(100, h_piid[j]->GetMaximum()*0.65, write);

    std::cout << 0 << "\t" << j << "\t" << std::fixed << std::setprecision(3) << hdprm_new[j] << std::endl;
    std::cout << write << std::endl;

    outputfile << CId << "\t" << PlId << "\t" << j << "\t" << AorT << "\t" << UorD << "\t"
	       << std::fixed << std::setprecision(3) << hdprm_new[j] << "\t"
	       << std::fixed << std::setprecision(1) << p1 << std::endl;
    if(num + 1 == 32 || j == NumOfSegPiID - 1){
      c1->Print(pdffile, "pdf");
    }
  }

  c1->Print(pdffile + "]", "pdf");

}
