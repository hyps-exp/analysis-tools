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


const Int_t NumOfPlaneCFT = 8;
const Int_t NumOfSegPHI4 = 910;
const Int_t NumOfSegCFT[8] = {426,584,472,692,510,800,538,910};
const Int_t CId  = 31;
const Int_t AorT = 1;
const Int_t UorD = 0;
const Double_t p1 = -1;


int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usage : " << argv[0] << "  <runnumber> <layer>"
	      << std::endl;
    return 0;
  }

  Int_t runnumber, layer;
  runnumber = std::stoi(argv[1]);
  layer = std::stoi(argv[2]);

  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << "Start ./cft_hdprm_tdc runnumber:" << runnumber << " layer:" << layer << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;

  Double_t hdprm_new[NumOfSegCFT[layer]];

  TString rootfile, txtfile, pdffile;

  rootfile.Form("../../rootfile/run%05d_CFT.root", runnumber);
  txtfile.Form("../../output/run%05d/CATCH_cft_hdprm_tdc_%05d_%d.txt", runnumber, runnumber, layer);
  pdffile.Form("../../output/run%05d/CATCH_cft_hdprm_tdc_%05d_%d.pdf", runnumber, runnumber, layer);
  TFile *file = new TFile(rootfile);

  std::ofstream outputfile(txtfile);
  outputfile << "####################################################" << std::endl;
  outputfile << "CFT HDPRM TDC" << std::endl;
  outputfile << "####################################################" << std::endl;

  std::cout << "#D Start calculating" << std::endl;
  TH2D *h;
  TH1D *h_cft[NumOfSegCFT[layer]];
  TF1 *f[NumOfSegCFT[layer]];
  TString title, hist;
  TCanvas *c1 = new TCanvas("c1", "c1", 1000, 800);
  c1->Divide(4, 8);
  c1->Print(pdffile + "[", "pdf");

  hist.Form("h%d201", layer+1);
  h = (TH2D*)gROOT->FindObject(hist);
  Int_t num;
  for(Int_t j=0; j<NumOfSegCFT[layer]; j++){
    num = j%32;
    c1->cd(num+1);

    title.Form("h_cft_%d%03d", layer, j);
    Int_t segment = h->GetXaxis()->FindBin(j);

    h_cft[j] = h->ProjectionY(title, segment, segment);
    h_cft[j]->Draw();

    Int_t max_bin = h_cft[j]->GetMaximumBin();
    Double_t max  = h_cft[j]->GetBinCenter(max_bin);
    h_cft[j]->Fit("gaus", "", "", max-10, max+10);

    f[j] = h_cft[j]->GetFunction("gaus");
    if(!f[j])
      hdprm_new[j] = 0;
    else
      hdprm_new[j] = f[j]->GetParameter(1);

    TLatex tex;
    tex.SetTextSize(0.1);
    tex.SetTextColor(kBlack);
    TString write = Form("mean : %f", hdprm_new[j]);
    tex.DrawLatex(100, h_cft[j]->GetMaximum()*0.65, write);

    std::cout << layer << "\t" << j << "\t" << std::fixed << std::setprecision(3) << hdprm_new[j] << std::endl;
    std::cout << write << std::endl;

    outputfile << CId << "\t" << layer << "\t" << j << "\t" << AorT << "\t" << UorD << "\t"
	       << std::fixed << std::setprecision(3) << hdprm_new[j] << "\t"
	       << std::fixed << std::setprecision(1) << p1 << std::endl;

    if(num + 1 == 32 || j == NumOfSegCFT[layer] - 1){
      c1->Print(pdffile, "pdf");
    }
  }

  c1->Print(pdffile + "]", "pdf");

}
