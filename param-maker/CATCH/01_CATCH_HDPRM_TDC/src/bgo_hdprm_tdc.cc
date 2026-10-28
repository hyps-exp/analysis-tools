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


const Int_t NumOfSegBGO = 24;
const Int_t CId  = 32;
const Int_t PlId = 0;
const Int_t AorT = 1;
const Int_t UorD = 0;
const Double_t p1 = -0.8333;


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
  std::cout << "Start ./bgo_hdprm_tdc runnumber:" << runnumber << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;

  Double_t hdprm_new[NumOfSegBGO];

  TString rootfile, txtfile, pdffile;

  rootfile.Form("../../rootfile/run%05d_BGO.root", runnumber);
  txtfile.Form("../../output/run%05d/CATCH_bgo_hdprm_tdc_%05d.txt", runnumber, runnumber);
  pdffile.Form("../../output/run%05d/CATCH_bgo_hdprm_tdc_%05d.pdf", runnumber, runnumber);
  TFile *file = new TFile(rootfile);

  std::ofstream outputfile(txtfile);
  outputfile << "####################################################" << std::endl;
  outputfile << "# BGO HDPRM TDC" << std::endl;
  outputfile << "####################################################" << std::endl;

  std::cout << "#D Start calculating" << std::endl;
  TH1D *h_bgo[NumOfSegBGO];
  TF1 *f[NumOfSegBGO];
  TString title, hist;
  TCanvas *c1 = new TCanvas("c1", "c1", 1200, 800);
  c1->Divide(6, 4);
  c1->Print(pdffile + "[", "pdf");

  for(Int_t j=0; j<NumOfSegBGO; j++){
    c1->cd(j+1);

    hist.Form("h%d100", j+1);
    h_bgo[j] = (TH1D*)gROOT->FindObject(hist);
    h_bgo[j]->Draw();

    Int_t max_bin = h_bgo[j]->GetMaximumBin();
    Double_t max  = h_bgo[j]->GetBinCenter(max_bin);
    h_bgo[j]->Fit("gaus", "", "", max-10, max+10);

    f[j] = h_bgo[j]->GetFunction("gaus");
    if(!f[j])
      hdprm_new[j] = 0;
    else
      hdprm_new[j] = f[j]->GetParameter(1);

    TLatex tex;
    tex.SetTextSize(0.1);
    tex.SetTextColor(kBlack);
    TString write = Form("mean : %f", hdprm_new[j]);
    tex.DrawLatex(800, h_bgo[j]->GetMaximum()*0.65, write);

    std::cout << j << "\t" << std::fixed << std::setprecision(3) << hdprm_new[j] << std::endl;

    outputfile << CId << "\t" << PlId << "\t" << j << "\t" << AorT << "\t" << UorD << "\t"
	       << std::fixed << std::setprecision(3) << hdprm_new[j] << "\t"
	       << std::fixed << std::setprecision(4) << p1 << std::endl;

    if(j == NumOfSegBGO - 1){
      c1->Print(pdffile, "pdf");
    }
  }

  c1->Print(pdffile + "]", "pdf");

}
