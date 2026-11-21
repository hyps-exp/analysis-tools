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
#include <TGraphErrors.h>

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t MaxDepth = 16;
const Int_t MaxHits2 = 60;
const Int_t NumOfSegBGO2 = NumOfSegBGO + 1;
const auto qnan = TMath::QuietNaN();

void SetBranchAddress_raw(TChain *chain);
void SetBranchAddress_sim(TChain *chain);
void MakeTree(TTree *tree);
void Clear_event();
void Clear_src_ini();
void Clear_src();
void assignment();

struct Event_raw{
  Int_t ntCFT;
  Int_t nP_CFT;
  Int_t nPi_CFT;
  Int_t track_proton[MaxHits2];
  Int_t MaxSegment[NumOfPlaneCFT][MaxHits2];
  Double_t MaxMipLow[NumOfPlaneCFT][MaxHits2];
};

struct Event_sim{
  Int_t ntCFT;
  Int_t nP_CFT;
  Int_t nPi_CFT;
  Double_t CFT_MaxFiberEdep0_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep1_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep2_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep3_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep4_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep5_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep6_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep7_proton[MaxHits2];
};

struct Src{
  Int_t ntCFT_raw;
  Int_t nP_CFT_raw;
  Int_t nPi_CFT_raw;
  std::vector<Int_t>* track_proton;
  std::vector<Int_t>* MaxSegment_0;
  std::vector<Int_t>* MaxSegment_1;
  std::vector<Int_t>* MaxSegment_2;
  std::vector<Int_t>* MaxSegment_3;
  std::vector<Int_t>* MaxSegment_4;
  std::vector<Int_t>* MaxSegment_5;
  std::vector<Int_t>* MaxSegment_6;
  std::vector<Int_t>* MaxSegment_7;
  std::vector<Double_t>* MaxMipLow_0;
  std::vector<Double_t>* MaxMipLow_1;
  std::vector<Double_t>* MaxMipLow_2;
  std::vector<Double_t>* MaxMipLow_3;
  std::vector<Double_t>* MaxMipLow_4;
  std::vector<Double_t>* MaxMipLow_5;
  std::vector<Double_t>* MaxMipLow_6;
  std::vector<Double_t>* MaxMipLow_7;

  Int_t ntCFT_sim;
  Int_t nP_CFT_sim;
  Int_t nPi_CFT_sim;
  std::vector<Double_t>* CFT_MaxFiberEdep0_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep1_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep2_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep3_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep4_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep5_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep6_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep7_proton;
};

static Event_raw event_raw;
static Event_sim event_sim;
static Src src;

int main(int argc, char **argv)
{
  if (argc != 5) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <run_sim> <run_start> <run_end>"
	      << std::endl;
    return 0;
  }

  int runnumber = std::atoi(argv[1]);
  int run_sim   = std::atoi(argv[2]);
  int run_start = std::atoi(argv[3]);
  int run_end   = std::atoi(argv[4]);

  TString filename;
  filename.Form("%s/run%05d/GammaCATCHHypsTracking_%05d_%05d.root", root_path.Data(), runnumber, run_start, run_end);

  std::cout << "#D To make root file" << std::endl;
  std::cout << "   root file : " << filename << std::endl;

  Clear_event();

  TChain *chain1 = new TChain("tree");
  chain1->Add(filename);
  SetBranchAddress_raw(chain1);

  filename.Form("/group/had/sks/Users/miyu/hyps-geant4/rootfile/run%d_ana.root", run_sim); // pion and proton
  TChain *chain2 = new TChain("tree");
  chain2->Add(filename);
  SetBranchAddress_sim(chain2);

  filename.Form("%s/run%05d/cft_ana_geant4_%05d_%05d_%d.root", root_path.Data(), runnumber, run_start, run_end, run_sim);
  TFile *fout = new TFile(filename, "recreate");
  TTree *tree = new TTree("tree", "tree");
  MakeTree(tree);

  std::cout << "#D Analysis" << std::endl;
  Int_t entries = chain1->GetEntries();
  Clear_src_ini();
  for(Int_t i=0; i<entries; i++){
    if(i!=0)
      Clear_src();
    chain1->GetEntry(i);
    chain2->GetEntry(i);
    if(i%100000==0){
      std::cout << entries << ":" << i << " " << std::endl;
    }
    assignment();
    // std::cout << __LINE__ << std::endl;
    tree->Fill();
  }
  tree->Write();
  fout->Write();
  fout->Close();
}

void SetBranchAddress_raw(TChain *chain)
{
  chain->SetBranchAddress("ntCFT",             &event_raw.ntCFT);
  chain->SetBranchAddress("nP_CFT",            &event_raw.nP_CFT);
  chain->SetBranchAddress("nPi_CFT",           &event_raw.nPi_CFT);
  chain->SetBranchAddress("track_proton",      event_raw.track_proton);
  chain->SetBranchAddress("MaxSegment",        event_raw.MaxSegment);
  chain->SetBranchAddress("MaxMipLow",         event_raw.MaxMipLow);
}

void SetBranchAddress_sim(TChain *chain)
{
  chain->SetBranchAddress("ntCFT",                    &event_sim.ntCFT);
  chain->SetBranchAddress("nP_CFT",                   &event_sim.nP_CFT);
  chain->SetBranchAddress("nPi_CFT",                  &event_sim.nPi_CFT);
  chain->SetBranchAddress("CFT_MaxFiberEdep0_proton", event_sim.CFT_MaxFiberEdep0_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep1_proton", event_sim.CFT_MaxFiberEdep1_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep2_proton", event_sim.CFT_MaxFiberEdep2_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep3_proton", event_sim.CFT_MaxFiberEdep3_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep4_proton", event_sim.CFT_MaxFiberEdep4_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep5_proton", event_sim.CFT_MaxFiberEdep5_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep6_proton", event_sim.CFT_MaxFiberEdep6_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep7_proton", event_sim.CFT_MaxFiberEdep7_proton);
}

void MakeTree(TTree *tree)
{
  tree->Branch("ntCFT_raw",         &src.ntCFT_raw,        "ntCFT_raw/I");
  tree->Branch("nP_CFT_raw",        &src.nP_CFT_raw,       "nP_CFT_raw/I");
  tree->Branch("nPi_CFT_raw",       &src.nPi_CFT_raw,      "nPi_CFT_raw/I");
  tree->Branch("track_proton",      &src.track_proton);
  tree->Branch("MaxSegment_0",      &src.MaxSegment_0);
  tree->Branch("MaxSegment_1",      &src.MaxSegment_1);
  tree->Branch("MaxSegment_2",      &src.MaxSegment_2);
  tree->Branch("MaxSegment_3",      &src.MaxSegment_3);
  tree->Branch("MaxSegment_4",      &src.MaxSegment_4);
  tree->Branch("MaxSegment_5",      &src.MaxSegment_5);
  tree->Branch("MaxSegment_6",      &src.MaxSegment_6);
  tree->Branch("MaxSegment_7",      &src.MaxSegment_7);
  tree->Branch("MaxMipLow_0",       &src.MaxMipLow_0);
  tree->Branch("MaxMipLow_1",       &src.MaxMipLow_1);
  tree->Branch("MaxMipLow_2",       &src.MaxMipLow_2);
  tree->Branch("MaxMipLow_3",       &src.MaxMipLow_3);
  tree->Branch("MaxMipLow_4",       &src.MaxMipLow_4);
  tree->Branch("MaxMipLow_5",       &src.MaxMipLow_5);
  tree->Branch("MaxMipLow_6",       &src.MaxMipLow_6);
  tree->Branch("MaxMipLow_7",       &src.MaxMipLow_7);

  tree->Branch("ntCFT_sim",                 &src.ntCFT_sim,                "ntCFT_sim/I");
  tree->Branch("nP_CFT_sim",                &src.nP_CFT_sim,               "nP_CFT_sim/I");
  tree->Branch("nPi_CFT_sim",               &src.nPi_CFT_sim,              "nPi_CFT_sim/I");
  tree->Branch("CFT_MaxFiberEdep0_proton",  &src.CFT_MaxFiberEdep0_proton);
  tree->Branch("CFT_MaxFiberEdep1_proton",  &src.CFT_MaxFiberEdep1_proton);
  tree->Branch("CFT_MaxFiberEdep2_proton",  &src.CFT_MaxFiberEdep2_proton);
  tree->Branch("CFT_MaxFiberEdep3_proton",  &src.CFT_MaxFiberEdep3_proton);
  tree->Branch("CFT_MaxFiberEdep4_proton",  &src.CFT_MaxFiberEdep4_proton);
  tree->Branch("CFT_MaxFiberEdep5_proton",  &src.CFT_MaxFiberEdep5_proton);
  tree->Branch("CFT_MaxFiberEdep6_proton",  &src.CFT_MaxFiberEdep6_proton);
  tree->Branch("CFT_MaxFiberEdep7_proton",  &src.CFT_MaxFiberEdep7_proton);
}

void Clear_event()
{
  event_raw.ntCFT        = qnan;
  event_raw.nP_CFT       = qnan;
  event_raw.nPi_CFT      = qnan;
  event_sim.ntCFT        = qnan;
  event_sim.nP_CFT       = qnan;
  event_sim.nPi_CFT      = qnan;
  for(Int_t i=0; i<MaxHits2; i++){
    event_raw.track_proton[i] = qnan;
    for(Int_t layer=0; layer<NumOfPlaneCFT; layer++){
      event_raw.MaxSegment[layer][i] = qnan;
      event_raw.MaxMipLow[layer][i]  = qnan;
    }

    event_sim.CFT_MaxFiberEdep0_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep1_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep2_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep3_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep4_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep5_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep6_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep7_proton[i] = qnan;
  }
}

void Clear_src_ini()
{
  src.ntCFT_raw   = qnan;
  src.nP_CFT_raw  = qnan;
  src.nPi_CFT_raw = qnan;
  src.ntCFT_sim   = qnan;
  src.nP_CFT_sim  = qnan;
  src.nPi_CFT_sim = qnan;

  src.MaxSegment_0 = new std::vector<Int_t>();
  src.MaxSegment_1 = new std::vector<Int_t>();
  src.MaxSegment_2 = new std::vector<Int_t>();
  src.MaxSegment_3 = new std::vector<Int_t>();
  src.MaxSegment_4 = new std::vector<Int_t>();
  src.MaxSegment_5 = new std::vector<Int_t>();
  src.MaxSegment_6 = new std::vector<Int_t>();
  src.MaxSegment_7 = new std::vector<Int_t>();

  src.MaxMipLow_0 = new std::vector<Double_t>();
  src.MaxMipLow_1 = new std::vector<Double_t>();
  src.MaxMipLow_2 = new std::vector<Double_t>();
  src.MaxMipLow_3 = new std::vector<Double_t>();
  src.MaxMipLow_4 = new std::vector<Double_t>();
  src.MaxMipLow_5 = new std::vector<Double_t>();
  src.MaxMipLow_6 = new std::vector<Double_t>();
  src.MaxMipLow_7 = new std::vector<Double_t>();

  src.track_proton = new std::vector<Int_t>();

  src.CFT_MaxFiberEdep0_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep1_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep2_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep3_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep4_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep5_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep6_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep7_proton = new std::vector<Double_t>();

}

void Clear_src()
{
  src.ntCFT_raw   = qnan;
  src.nP_CFT_raw  = qnan;
  src.nPi_CFT_raw = qnan;
  src.ntCFT_sim   = qnan;
  src.nP_CFT_sim  = qnan;
  src.nPi_CFT_sim = qnan;

  src.MaxSegment_0->clear();
  src.MaxSegment_1->clear();
  src.MaxSegment_2->clear();
  src.MaxSegment_3->clear();
  src.MaxSegment_4->clear();
  src.MaxSegment_5->clear();
  src.MaxSegment_6->clear();
  src.MaxSegment_7->clear();
  src.MaxMipLow_0->clear();
  src.MaxMipLow_1->clear();
  src.MaxMipLow_2->clear();
  src.MaxMipLow_3->clear();
  src.MaxMipLow_4->clear();
  src.MaxMipLow_5->clear();
  src.MaxMipLow_6->clear();
  src.MaxMipLow_7->clear();

  src.track_proton->clear();

  src.CFT_MaxFiberEdep0_proton->clear();
  src.CFT_MaxFiberEdep1_proton->clear();
  src.CFT_MaxFiberEdep2_proton->clear();
  src.CFT_MaxFiberEdep3_proton->clear();
  src.CFT_MaxFiberEdep4_proton->clear();
  src.CFT_MaxFiberEdep5_proton->clear();
  src.CFT_MaxFiberEdep6_proton->clear();
  src.CFT_MaxFiberEdep7_proton->clear();
}

void assignment()
{
  src.ntCFT_raw   = event_raw.ntCFT;
  src.nP_CFT_raw  = event_raw.nP_CFT;
  src.nPi_CFT_raw = event_raw.nPi_CFT;
  src.ntCFT_sim   = event_sim.ntCFT;
  src.nP_CFT_sim  = event_sim.nP_CFT;
  src.nPi_CFT_sim = event_sim.nPi_CFT;

  for(Int_t it=0; it<MaxHits2; it++){
    src.MaxSegment_0->push_back(event_raw.MaxSegment[0][it]);
    src.MaxSegment_1->push_back(event_raw.MaxSegment[1][it]);
    src.MaxSegment_2->push_back(event_raw.MaxSegment[2][it]);
    src.MaxSegment_3->push_back(event_raw.MaxSegment[3][it]);
    src.MaxSegment_4->push_back(event_raw.MaxSegment[4][it]);
    src.MaxSegment_5->push_back(event_raw.MaxSegment[5][it]);
    src.MaxSegment_6->push_back(event_raw.MaxSegment[6][it]);
    src.MaxSegment_7->push_back(event_raw.MaxSegment[7][it]);
    src.MaxMipLow_0->push_back(event_raw.MaxMipLow[0][it]);
    src.MaxMipLow_1->push_back(event_raw.MaxMipLow[1][it]);
    src.MaxMipLow_2->push_back(event_raw.MaxMipLow[2][it]);
    src.MaxMipLow_3->push_back(event_raw.MaxMipLow[3][it]);
    src.MaxMipLow_4->push_back(event_raw.MaxMipLow[4][it]);
    src.MaxMipLow_5->push_back(event_raw.MaxMipLow[5][it]);
    src.MaxMipLow_6->push_back(event_raw.MaxMipLow[6][it]);
    src.MaxMipLow_7->push_back(event_raw.MaxMipLow[7][it]);

    src.track_proton->push_back(event_raw.track_proton[it]);
  }


  for(Int_t i=0; i<event_sim.nP_CFT; i++){
    src.CFT_MaxFiberEdep0_proton->push_back(event_sim.CFT_MaxFiberEdep0_proton[i]);
    src.CFT_MaxFiberEdep1_proton->push_back(event_sim.CFT_MaxFiberEdep1_proton[i]);
    src.CFT_MaxFiberEdep2_proton->push_back(event_sim.CFT_MaxFiberEdep2_proton[i]);
    src.CFT_MaxFiberEdep3_proton->push_back(event_sim.CFT_MaxFiberEdep3_proton[i]);
    src.CFT_MaxFiberEdep4_proton->push_back(event_sim.CFT_MaxFiberEdep4_proton[i]);
    src.CFT_MaxFiberEdep5_proton->push_back(event_sim.CFT_MaxFiberEdep5_proton[i]);
    src.CFT_MaxFiberEdep6_proton->push_back(event_sim.CFT_MaxFiberEdep6_proton[i]);
    src.CFT_MaxFiberEdep7_proton->push_back(event_sim.CFT_MaxFiberEdep7_proton[i]);
  }

}
