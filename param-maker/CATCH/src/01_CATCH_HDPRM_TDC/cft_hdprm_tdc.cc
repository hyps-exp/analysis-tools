// Author : Miyu Yoshida
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

#include "DetectorID.hh"
#include "PathID.hh"
#include "../../../../include/hyps_constant.hh"


const Int_t AorT = static_cast<Int_t>(readout::AdcOrTdc::kTdc);
const Int_t UorD = 0;
const Double_t p1 = -module_lsb::kVeasirocTdc;


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

  rootfile.Form("%s/run%05d/run%05d_CFT.root", root_path.Data(), runnumber, runnumber);
  txtfile.Form("%s/run%05d/run%05d_CATCH_CFT_HDPRM_TDC_layer%d.txt", pdf_path.Data(), runnumber, runnumber, layer);
  pdffile.Form("%s/run%05d/run%05d_CATCH_CFT_HDPRM_TDC_layer%d.pdf", txt_path.Data(), runnumber, runnumber, layer);
  TFile *file = new TFile(rootfile);
  if (file->IsZombie()) {
    std::cerr << "#E Cannot open " << rootfile << std::endl;
    return 1;
  }

  std::ofstream outputfile(txtfile);
  outputfile << "####################################################" << std::endl;
  outputfile << "# CFT HDPRM TDC" << std::endl;
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
  if (!h) {
    std::cerr << "#E Histogram " << hist << " not found in " << rootfile << std::endl;
    return 1;
  }
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

    outputfile << det_id::kCFT << "\t" << layer << "\t" << j << "\t" << AorT << "\t" << UorD << "\t"
	       << std::fixed << std::setprecision(3) << hdprm_new[j] << "\t"
	       << std::fixed << std::setprecision(1) << p1 << std::endl;

    if(num + 1 == 32 || j == NumOfSegCFT[layer] - 1){
      c1->Print(pdffile, "pdf");
    }
  }

  c1->Print(pdffile + "]", "pdf");

}
