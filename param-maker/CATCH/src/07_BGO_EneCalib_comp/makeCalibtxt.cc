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

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t MaxDepth = 16;

void GetTree(TChain *chain, int runnumber, int run_start, int run_end);
void SetBranchAddress(TChain *chain);

struct Event{
  std::vector<Double_t>* Dir_x_pi;
  std::vector<Double_t>* Dir_y_pi;
  std::vector<Double_t>* Dir_z_pi;
  std::vector<Double_t>* Dir_x_p;
  std::vector<Double_t>* Dir_y_p;
  std::vector<Double_t>* Dir_z_p;
  Double_t decayLambda_vtx_x;
  Double_t decayLambda_vtx_y;
  Double_t decayLambda_vtx_z;
  Double_t decayLambda_mom_pi;
  Double_t decayLambda_mom_p;
};
static Event event;

int main(int argc, char **argv)
{
  if (argc != 5) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <run_start> <run_end> <count>"
	      << std::endl;
    return 0;
  }

  int runnumber = std::atoi(argv[1]);
  int run_start = std::atoi(argv[2]);
  int run_end   = std::atoi(argv[3]);
  int count     = std::atoi(argv[4]);

  std::cout << "#D To make txt file" << std::endl;
  TChain *chain = new TChain("tree");
  if(count == 0)
    GetTree(chain, runnumber, run_start, run_end);
  else if(count == 1)
    chain->Add(Form("%s/run%05d/GammaCATCHCFT_%05d_%05d.root", root_path.Data(), runnumber, run_start, run_end));
  else
    return -1;
  SetBranchAddress(chain);

  TString filename;
  filename.Form("%s/run%05d/catch_makeCalibtxt.txt.%05d.%d", txt_path.Data(), runnumber, runnumber, count);
  std::ofstream ofs(filename);
  std::cout << "   txt file : " << filename << std::endl;

  std::cout << "#D Analysis" << std::endl;
  Int_t entries = chain->GetEntries();
  for(Int_t i=0; i<entries; i++){
    chain->GetEntry(i);
    if(i%100000==0){
      std::cout << entries << ":" << i << " " << std::endl;
    }
    if(event.decayLambda_mom_p>0){
      Double_t D_p  = sqrt(pow(event.Dir_x_p->at(0), 2) + pow(event.Dir_y_p->at(0), 2) + pow(event.Dir_z_p->at(0), 2));
      Double_t D_pi = sqrt(pow(event.Dir_x_pi->at(0), 2) + pow(event.Dir_y_pi->at(0), 2) + pow(event.Dir_z_pi->at(0), 2));
      ofs << event.decayLambda_vtx_x << "\t"
	  << event.decayLambda_vtx_y << "\t"
	  << event.decayLambda_vtx_z << "\t"
	  << event.Dir_x_p->at(0)*event.decayLambda_mom_p/D_p << "\t"
	  << event.Dir_y_p->at(0)*event.decayLambda_mom_p/D_p << "\t"
	  << event.Dir_z_p->at(0)*event.decayLambda_mom_p/D_p << "\t"
	  << event.Dir_x_pi->at(0)*event.decayLambda_mom_pi/D_pi << "\t"
	  << event.Dir_y_pi->at(0)*event.decayLambda_mom_pi/D_pi << "\t"
	  << event.Dir_z_pi->at(0)*event.decayLambda_mom_pi/D_pi
	  << std::endl;
    }else{
      ofs << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999 << "\t"
	  << -999
	  << std::endl;
    }
  }
}

//---------------------------------------------------------------
void GetTree(TChain *chain, int runnumber, int run_start, int run_end){

  // TString dirname = Form("%s/run%05d/", root_path.Data(), runnumber);
  TString dirname = Form("/group/had/sks/Users/miyu/hyps-analyzer/root_Kid/");

  int count = 0;
  for(Int_t i=run_start; i<=run_end; i++){
    TString filename = dirname + Form("run%05d_GammaCATCHCFT.root", i);

    std::filesystem::path filepath = filename.Data();

    if(std::filesystem::exists(filepath)){
      chain->Add(filename);

      if(count%100 == 0)
	std::cout << "#D the number of files : " << count << std::endl;

      count++;
    }
  }
  std::cout << "#D the total number of files : " << count << std::endl;
}

//---------------------------------------------------------------
void SetBranchAddress(TChain *chain)
{
  chain->SetBranchAddress("Dir_x_pi", &event.Dir_x_pi);
  chain->SetBranchAddress("Dir_y_pi", &event.Dir_y_pi);
  chain->SetBranchAddress("Dir_z_pi", &event.Dir_z_pi);
  chain->SetBranchAddress("Dir_x_p",  &event.Dir_x_p);
  chain->SetBranchAddress("Dir_y_p",  &event.Dir_y_p);
  chain->SetBranchAddress("Dir_z_p",  &event.Dir_z_p);
  chain->SetBranchAddress("decayLambda_vtx_x",                               &event.decayLambda_vtx_x);
  chain->SetBranchAddress("decayLambda_vtx_y",                               &event.decayLambda_vtx_y);
  chain->SetBranchAddress("decayLambda_vtx_z",                               &event.decayLambda_vtx_z);
  chain->SetBranchAddress("decayLambda_mom_pi",                              &event.decayLambda_mom_pi);
  chain->SetBranchAddress("decayLambda_mom_p",                               &event.decayLambda_mom_p);

}
