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
#include <TChain.h>
#include <TChainElement.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TApplication.h>
#include <TSystem.h>

#include "DetectorID.hh"
#include "PathID.hh"
#include "../../../../include/hyps_constant.hh"

// ref : cft_lowmipnew

const Int_t MaxDepth = 16;
const Int_t AorT = 0;
const Int_t UorD = 1;
const Double_t p1 = -1;

struct Event{
  Int_t ntCFT;
  Int_t MaxSegment[NumOfPlaneCFT][MaxDepth];
  Double_t MaxMipLow[NumOfPlaneCFT][MaxDepth];
  Double_t MaxAdcLow[NumOfPlaneCFT][MaxDepth];
  Double_t theta_cft[MaxDepth];
  Int_t segBGOt[NumOfSegBGO+1];
  Double_t energyBGO[NumOfSegBGO+1];
  Int_t segPiIDt[MaxDepth];
};
static Event event;


int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return 0;
  }

  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << "Start cft_lowmip_hand" << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;
  time_t start_time, end_time;
  start_time = time(NULL);

  int runnumber = std::atoi(argv[1]);

  TApplication app("app", &argc, argv);

  // data setup
  TString filename;
  filename.Form("%s/run%05d/cft_proton_mip_%05d.root", root_path.Data(), runnumber, runnumber);
  TFile *fin = TFile::Open(filename, "READ");
  if( !fin || fin->IsZombie() ){
    std::cerr << "#E : Cannot open " << filename << std::endl;
    return -1;
  }
  fin->cd();

  // filename.Form("%s/run%05d/catch_error_cft_lowmip_%05d.txt", txt_path.Data(), runnumber, runnumber);
  filename.Form("%s/run%05d/catch_error_cft_lowmip_load_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ifstream ifs(filename);
  if(!ifs.is_open()){
    std::cerr << "#E cannot open file : " << filename << std::endl;
    return -1;
  }

  filename.Form("%s/run%05d/catch_cft_lowmip_hand_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream ofs(filename);
  ofs << "### CFT LowGain MIP" << std::endl;

  // analysis (search MIP)

  TString hist, title, cond, cut;
  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Divide(4, 8);
  TCanvas *c2 = new TCanvas("c2", "c2");
  filename.Form("%s/run%05d/cft_lowgain_mip_hand_%05d.pdf", pdf_path.Data(), runnumber, runnumber);
  c1->Print(filename + "[", "pdf");
  TH1D *h1[NumOfPlaneCFT][NumOfSegCFT[7]];
  TF1 *f1[NumOfPlaneCFT][NumOfSegCFT[7]];

  Int_t layer, seg;
  Int_t count = 0;
  std::cout << "#D Start drawing and fitting hists" << std::endl;
  std::string line;
  while(ifs.good() && std::getline(ifs, line)){
    if(line[0] != '#'){
      std::istringstream input_line( line );
      Int_t PlId, SegId;
      if(input_line >> PlId >> SegId){
	layer = PlId;
	seg   = SegId;
      }
    }else{
      continue;
    }

    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "layer : " << layer << ", seg : " << seg << std::endl;
    std::cout << "" << std::endl;
    c2->cd();

    hist.Form("h%d%03d", layer+1, seg);
    h1[layer][seg] = (TH1D *)fin->Get(hist);
    h1[layer][seg]->Rebin(10);

    h1[layer][seg]->Draw();
    c2->Modified();
    c2->Update();

    gSystem->ProcessEvents();

    // Fit : 1st time
    hist.Form("f%d%03d", layer+1, seg);
    f1[layer][seg] = new TF1(hist, "gaus");

    TString correct;
    TString hoge;
    Double_t N0, mean0, sigma0, N1, mean1, sigma1, min, max;
    while(1){

      std::cout << "range" << std::endl;
      std::cout << "min : " << std::endl;
      std::cin >> min;
      std::cout << "max : " << std::endl;
      std::cin >> max;


      h1[layer][seg]->Fit(hist, "", "", min, max);
      c2->Modified();
      c2->Update();

      gSystem->ProcessEvents();

      std::cout << "correct (y/-) : " << std::endl;
      std::cin >> correct;

      if(correct == "y")
	break;
    }

    Double_t N = 0;
    Double_t mean = 0;
    Double_t sig  = 0;
    if(h1[layer][seg]->GetFunction(hist)){
      N    = f1[layer][seg]->GetParameter(0);
      mean = f1[layer][seg]->GetParameter(1);
      sig  = f1[layer][seg]->GetParameter(2);
    }


    ofs << det_id::kCFT << "\t" << layer << "\t" << seg << "\t" << AorT << "\t" << UorD << "\t"
	<< 0 << "\t"
	<< std::fixed << std::setprecision(3) << mean << std::endl;

    c1->cd(count+1);
    h1[layer][seg]->Draw();

    TString write = Form("mean : %03f", mean);
    TLatex tex;
    tex.SetTextSize(0.1);
    tex.SetTextColor(kBlack);
    tex.DrawLatex(500, N, write);

    count++;
    if(count == 32){
      c1->Print(filename, "pdf");
      c1->Clear();
      c1->Divide(4, 8);
      count = 0;
    }
  }
  if(count!=0)
    c1->Print(filename, "pdf");

  c1->Print(filename + "]", "pdf");
  // ofs.Close();
  // outfile->Close();

  end_time = time(NULL);

  std::cout << "#D End this program" << std::endl;
  std::cout << "   Start time   : " << start_time << std::endl;
  std::cout << "   End time     : " << end_time << std::endl;
  std::cout << "   Process time : " << end_time - start_time << std::endl;

  return 0;

}
