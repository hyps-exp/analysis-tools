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

// ref : cft_lowmipnew

const Int_t MaxDepth = 16;
const Int_t CId  = 31;
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

bool IsPhysicalFit(TH1D *h, Double_t mean, Double_t sig);

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

  // data setup
  TString filename;
  filename.Form("%s/run%05d/cft_proton_mip_%05d.root", root_path.Data(), runnumber, runnumber);
  TFile *fin = TFile::Open(filename, "READ");
  if( !fin || fin->IsZombie() ){
    std::cerr << "#E : Cannot open " << filename << std::endl;
    return -1;
  }
  fin->cd();

  filename.Form("../../param/HDPRM/yoshida_HodoParam_hyps_%05d_05", runnumber);
  std::ifstream ifs(filename);
  if(!ifs.is_open()){
    std::cerr << "#E cannot open file : " << filename << std::endl;
    return -1;
  }

  filename.Form("%s/run%05d/catch_cft_lowmip_load_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream ofs(filename);
  ofs << "### CFT LowGain MIP" << std::endl;

  filename.Form("%s/run%05d/catch_error_cft_lowmip_load_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream output(filename);
  output << "### CFT LowGain MIP" << std::endl;

  // analysis (search MIP)

  TString hist, title, cond, cut;
  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Divide(4, 8);
  TCanvas *c2 = new TCanvas("c2", "c2");
  filename.Form("%s/run%05d/catch_cft_lowmip_load_%05d.pdf", pdf_path.Data(), runnumber, runnumber);
  c1->Print(filename + "[", "pdf");
  TH1D *h1[NumOfPlaneCFT][NumOfSegCFT[7]];
  TF1 *f1[NumOfPlaneCFT][NumOfSegCFT[7]];

  Int_t layer, seg;
  std::cout << "#D Start drawing and fitting hists" << std::endl;
  std::string line;
  Double_t hdprm_cft[NumOfPlaneCFT][NumOfSegCFT_PHI4];
  while(ifs.good() && std::getline(ifs, line)){
    if(line[0] == '3' && line[1] == '1'){
      std::istringstream input_line( line );
      Int_t cid, plid, segid, aort, uord;
      Double_t p0, p1;
      if(input_line >> cid >> plid >> segid >> aort >> uord >> p0 >> p1){
	if(aort == AorT && uord == UorD){
	  hdprm_cft[plid][segid] = p1;
	}
      }
    }
  }

  for(int layer = 0; layer<NumOfPlaneCFT; layer++){
    Int_t count = 0;
    if(layer%2 == 0){
      ofs << "# UV " << layer/2 + 1 << std::endl;
      output << "# UV " << layer/2 + 1 << std::endl;
    }else{
      ofs << "# PHI " << (layer-1)/2 + 1 << std::endl;
      output << "# PHI " << (layer-1)/2 + 1 << std::endl;
    }
    for(int seg = 0; seg<NumOfSegCFT[layer]; seg++){
      std::cout << "" << std::endl;
      std::cout << "" << std::endl;
      std::cout << "layer : " << layer << ", seg : " << seg << std::endl;
      std::cout << "" << std::endl;
      c2->cd();

      hist.Form("h%d%03d", layer+1, seg);
      h1[layer][seg] = (TH1D *)fin->Get(hist);
      h1[layer][seg]->Rebin(10);

      h1[layer][seg]->Draw();

      // Fit : 1st time
      hist.Form("f%d%03d", layer+1, seg);
      f1[layer][seg] = new TF1(hist, "gaus");

      h1[layer][seg]->Fit(hist, "", "", hdprm_cft[layer][seg]-60, hdprm_cft[layer][seg]+60);

      Double_t N = 0;
      Double_t mean = 0;
      Double_t sig  = 0;
      if(h1[layer][seg]->GetFunction(hist)){
	N    = f1[layer][seg]->GetParameter(0);
	mean = f1[layer][seg]->GetParameter(1);
	sig  = f1[layer][seg]->GetParameter(2);
      }

      h1[layer][seg]->Fit(hist, "", "", mean-2*sig, mean+2*sig);
      if(h1[layer][seg]->GetFunction(hist)){
	N    = f1[layer][seg]->GetParameter(0);
	mean = f1[layer][seg]->GetParameter(1);
	sig  = f1[layer][seg]->GetParameter(2);
      }


      ofs << CId << "\t" << layer << "\t" << seg << "\t" << AorT << "\t" << UorD << "\t"
	  << 0 << "\t"
	  << std::fixed << std::setprecision(3) << mean << std::endl;

      if(!IsPhysicalFit(h1[layer][seg], mean, sig)){
	output << layer << "\t" << seg << std::endl;
	mean = 0; sig = 0;
      }

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
  }

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


bool IsPhysicalFit(TH1D *h, Double_t mean, Double_t sig)
{
  if(sig<=0 || sig>100) return false;
  if(mean>800) return false;
  return mean > h->GetXaxis()->GetXmin() && mean < h->GetXaxis()->GetXmax();
}
