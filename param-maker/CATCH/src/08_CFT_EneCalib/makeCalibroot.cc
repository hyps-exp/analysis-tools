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
  std::vector<Int_t>* track_proton;
  std::vector<Int_t>* track_pion;
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
  std::vector<Double_t>* theta_cft;
  std::vector<Int_t>* pid;
  std::vector<Double_t>* dE_cft;
  std::vector<Int_t>* segBGOt;
  std::vector<Double_t>* energyBGO;
  std::vector<Double_t>* pulseheightBGO;
  std::vector<Int_t>* segPiIDt;
  std::vector<Double_t>* Dir_x_p;
  std::vector<Double_t>* Dir_y_p;
  std::vector<Double_t>* Dir_z_p;
  std::vector<Double_t>* Dir_x_pi;
  std::vector<Double_t>* Dir_y_pi;
  std::vector<Double_t>* Dir_z_pi;
  std::vector<Int_t>* segBGOt_p;
  std::vector<Double_t>* energyBGO_p;
  std::vector<Double_t>* pulseheightBGO_p;
  std::vector<Double_t>* Total_dE_p;
  std::vector<Int_t>* segBGOt_pi;
  std::vector<Double_t>* energyBGO_pi;
  std::vector<Double_t>* pulseheightBGO_pi;
  std::vector<Double_t>* Total_dE_pi;
  Double_t decayLambda_vtx_x;
  Double_t decayLambda_vtx_y;
  Double_t decayLambda_vtx_z;
  Double_t decayLambda_theta_pion;
  Double_t decayLambda_theta_proton;
  Double_t decayLambda_mom_pi;
  Double_t decayLambda_mom_p;
  Double_t decayLambda_energyloss_pion;
  Double_t decayLambda_energyloss_proton;
  std::array<Double_t, NumOfPlaneCFT> decayLambda_energylossCFT_pion{};
  std::array<Double_t, NumOfPlaneCFT> decayLambda_energylossCFT_proton{};
};

struct Event_sim{
  Int_t ntCFT;
  Int_t nP_CFT;
  Int_t nPi_CFT;
  Double_t CFT_MaxFiberEdep0[MaxHits2];
  Double_t CFT_MaxFiberEdep1[MaxHits2];
  Double_t CFT_MaxFiberEdep2[MaxHits2];
  Double_t CFT_MaxFiberEdep3[MaxHits2];
  Double_t CFT_MaxFiberEdep4[MaxHits2];
  Double_t CFT_MaxFiberEdep5[MaxHits2];
  Double_t CFT_MaxFiberEdep6[MaxHits2];
  Double_t CFT_MaxFiberEdep7[MaxHits2];
  Double_t CFT_MaxFiberEdep0_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep1_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep2_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep3_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep4_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep5_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep6_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep7_proton[MaxHits2];
  Double_t CFT_MaxFiberEdep0_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep1_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep2_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep3_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep4_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep5_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep6_pion[MaxHits2];
  Double_t CFT_MaxFiberEdep7_pion[MaxHits2];
  Double_t CFT_TotalEdep[MaxHits2];
  Double_t CFT_NormTotalEdep[MaxHits2];
  Double_t BGO_Edep[MaxHits2];
  Double_t BGO_Edep_proton[MaxHits2];
  Double_t BGO_Edep_pion[MaxHits2];
  Double_t TotalEdep[MaxHits2];
};

struct Src{
  Int_t ntCFT_raw;
  Int_t nP_CFT_raw;
  Int_t nPi_CFT_raw;
  std::vector<Int_t>* track_proton;
  std::vector<Int_t>* track_pion;
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
  std::vector<Double_t>* theta_cft;
  std::vector<Int_t>* pid;
  std::vector<Double_t>* dE_cft;
  std::vector<Int_t>* segBGOt;
  std::vector<Double_t>* energyBGO;
  std::vector<Double_t>* pulseheightBGO;
  std::vector<Int_t>* segPiIDt;
  std::vector<Double_t>* Dir_x_p;
  std::vector<Double_t>* Dir_y_p;
  std::vector<Double_t>* Dir_z_p;
  std::vector<Double_t>* Dir_x_pi;
  std::vector<Double_t>* Dir_y_pi;
  std::vector<Double_t>* Dir_z_pi;
  std::vector<Int_t>* segBGOt_p;
  std::vector<Double_t>* energyBGO_p;
  std::vector<Double_t>* pulseheightBGO_p;
  std::vector<Double_t>* Total_dE_p;
  std::vector<Int_t>* segBGOt_pi;
  std::vector<Double_t>* energyBGO_pi;
  std::vector<Double_t>* pulseheightBGO_pi;
  std::vector<Double_t>* Total_dE_pi;
  Double_t decayLambda_vtx_x;
  Double_t decayLambda_vtx_y;
  Double_t decayLambda_vtx_z;
  Double_t decayLambda_theta_pion;
  Double_t decayLambda_theta_proton;
  Double_t decayLambda_mom_pi;
  Double_t decayLambda_mom_p;
  Double_t decayLambda_energyloss_pion;
  Double_t decayLambda_energyloss_proton;
  std::array<Double_t, NumOfPlaneCFT> decayLambda_energylossCFT_pion{};
  std::array<Double_t, NumOfPlaneCFT> decayLambda_energylossCFT_proton{};

  Int_t ntCFT_sim;
  Int_t nP_CFT_sim;
  Int_t nPi_CFT_sim;
  std::vector<Double_t>* CFT_MaxFiberEdep0;
  std::vector<Double_t>* CFT_MaxFiberEdep1;
  std::vector<Double_t>* CFT_MaxFiberEdep2;
  std::vector<Double_t>* CFT_MaxFiberEdep3;
  std::vector<Double_t>* CFT_MaxFiberEdep4;
  std::vector<Double_t>* CFT_MaxFiberEdep5;
  std::vector<Double_t>* CFT_MaxFiberEdep6;
  std::vector<Double_t>* CFT_MaxFiberEdep7;
  std::vector<Double_t>* CFT_MaxFiberEdep0_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep1_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep2_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep3_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep4_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep5_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep6_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep7_proton;
  std::vector<Double_t>* CFT_MaxFiberEdep0_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep1_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep2_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep3_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep4_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep5_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep6_pion;
  std::vector<Double_t>* CFT_MaxFiberEdep7_pion;
  std::vector<Double_t>* CFT_TotalEdep;
  std::vector<Double_t>* CFT_NormTotalEdep;
  std::vector<Double_t>* BGO_Edep;
  std::vector<Double_t>* BGO_Edep_proton;
  std::vector<Double_t>* BGO_Edep_pion;
  std::vector<Double_t>* TotalEdep;
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
  filename.Form("%s/run%05d/GammaCATCHCFT_%05d_%05d.root", root_path.Data(), runnumber, run_start, run_end);

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
  chain->SetBranchAddress("track_proton",      &event_raw.track_proton);
  chain->SetBranchAddress("track_pion",        &event_raw.track_pion);
  chain->SetBranchAddress("MaxSegment_0",      &event_raw.MaxSegment_0);
  chain->SetBranchAddress("MaxSegment_1",      &event_raw.MaxSegment_1);
  chain->SetBranchAddress("MaxSegment_2",      &event_raw.MaxSegment_2);
  chain->SetBranchAddress("MaxSegment_3",      &event_raw.MaxSegment_3);
  chain->SetBranchAddress("MaxSegment_4",      &event_raw.MaxSegment_4);
  chain->SetBranchAddress("MaxSegment_5",      &event_raw.MaxSegment_5);
  chain->SetBranchAddress("MaxSegment_6",      &event_raw.MaxSegment_6);
  chain->SetBranchAddress("MaxSegment_7",      &event_raw.MaxSegment_7);
  chain->SetBranchAddress("MaxMipLow_0",       &event_raw.MaxMipLow_0);
  chain->SetBranchAddress("MaxMipLow_1",       &event_raw.MaxMipLow_1);
  chain->SetBranchAddress("MaxMipLow_2",       &event_raw.MaxMipLow_2);
  chain->SetBranchAddress("MaxMipLow_3",       &event_raw.MaxMipLow_3);
  chain->SetBranchAddress("MaxMipLow_4",       &event_raw.MaxMipLow_4);
  chain->SetBranchAddress("MaxMipLow_5",       &event_raw.MaxMipLow_5);
  chain->SetBranchAddress("MaxMipLow_6",       &event_raw.MaxMipLow_6);
  chain->SetBranchAddress("MaxMipLow_7",       &event_raw.MaxMipLow_7);
  chain->SetBranchAddress("theta_cft",         &event_raw.theta_cft);
  chain->SetBranchAddress("pid",               &event_raw.pid);
  chain->SetBranchAddress("dE_cft",            &event_raw.dE_cft);
  chain->SetBranchAddress("segBGOt",           &event_raw.segBGOt);
  chain->SetBranchAddress("energyBGO",         &event_raw.energyBGO);
  chain->SetBranchAddress("pulseheightBGO",    &event_raw.pulseheightBGO);
  chain->SetBranchAddress("segPiIDt",          &event_raw.segPiIDt);
  chain->SetBranchAddress("Dir_x_pi",          &event_raw.Dir_x_pi);
  chain->SetBranchAddress("Dir_y_pi",          &event_raw.Dir_y_pi);
  chain->SetBranchAddress("Dir_z_pi",          &event_raw.Dir_z_pi);
  chain->SetBranchAddress("Dir_x_p",           &event_raw.Dir_x_p);
  chain->SetBranchAddress("Dir_y_p",           &event_raw.Dir_y_p);
  chain->SetBranchAddress("Dir_z_p",           &event_raw.Dir_z_p);
  chain->SetBranchAddress("segBGOt_p",         &event_raw.segBGOt_p);
  chain->SetBranchAddress("energyBGO_p",       &event_raw.energyBGO_p);
  chain->SetBranchAddress("pulseheightBGO_p",  &event_raw.pulseheightBGO_p);
  chain->SetBranchAddress("Total_dE_p",        &event_raw.Total_dE_p);
  chain->SetBranchAddress("segBGOt_pi",        &event_raw.segBGOt_pi);
  chain->SetBranchAddress("energyBGO_pi",      &event_raw.energyBGO_pi);
  chain->SetBranchAddress("pulseheightBGO_pi", &event_raw.pulseheightBGO_pi);
  chain->SetBranchAddress("Total_dE_pi",       &event_raw.Total_dE_pi);
  chain->SetBranchAddress("decayLambda_vtx_x",                &event_raw.decayLambda_vtx_x);
  chain->SetBranchAddress("decayLambda_vtx_y",                &event_raw.decayLambda_vtx_y);
  chain->SetBranchAddress("decayLambda_vtx_z",                &event_raw.decayLambda_vtx_z);
  chain->SetBranchAddress("decayLambda_theta_pion",           &event_raw.decayLambda_theta_pion);
  chain->SetBranchAddress("decayLambda_theta_proton",         &event_raw.decayLambda_theta_proton);
  chain->SetBranchAddress("decayLambda_mom_pi",               &event_raw.decayLambda_mom_pi);
  chain->SetBranchAddress("decayLambda_mom_p",                &event_raw.decayLambda_mom_p);
  chain->SetBranchAddress("decayLambda_energyloss_pion",      &event_raw.decayLambda_energyloss_pion);
  chain->SetBranchAddress("decayLambda_energyloss_proton",    &event_raw.decayLambda_energyloss_proton);
  chain->SetBranchAddress("decayLambda_energylossCFT_pion",   &event_raw.decayLambda_energylossCFT_pion);
  chain->SetBranchAddress("decayLambda_energylossCFT_proton", &event_raw.decayLambda_energylossCFT_proton);
}

void SetBranchAddress_sim(TChain *chain)
{
  chain->SetBranchAddress("ntCFT",                    &event_sim.ntCFT);
  chain->SetBranchAddress("nP_CFT",                   &event_sim.nP_CFT);
  chain->SetBranchAddress("nPi_CFT",                  &event_sim.nPi_CFT);
  chain->SetBranchAddress("CFT_MaxFiberEdep0",        event_sim.CFT_MaxFiberEdep0);
  chain->SetBranchAddress("CFT_MaxFiberEdep1",        event_sim.CFT_MaxFiberEdep1);
  chain->SetBranchAddress("CFT_MaxFiberEdep2",        event_sim.CFT_MaxFiberEdep2);
  chain->SetBranchAddress("CFT_MaxFiberEdep3",        event_sim.CFT_MaxFiberEdep3);
  chain->SetBranchAddress("CFT_MaxFiberEdep4",        event_sim.CFT_MaxFiberEdep4);
  chain->SetBranchAddress("CFT_MaxFiberEdep5",        event_sim.CFT_MaxFiberEdep5);
  chain->SetBranchAddress("CFT_MaxFiberEdep6",        event_sim.CFT_MaxFiberEdep6);
  chain->SetBranchAddress("CFT_MaxFiberEdep7",        event_sim.CFT_MaxFiberEdep7);
  chain->SetBranchAddress("CFT_MaxFiberEdep0_proton", event_sim.CFT_MaxFiberEdep0_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep1_proton", event_sim.CFT_MaxFiberEdep1_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep2_proton", event_sim.CFT_MaxFiberEdep2_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep3_proton", event_sim.CFT_MaxFiberEdep3_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep4_proton", event_sim.CFT_MaxFiberEdep4_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep5_proton", event_sim.CFT_MaxFiberEdep5_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep6_proton", event_sim.CFT_MaxFiberEdep6_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep7_proton", event_sim.CFT_MaxFiberEdep7_proton);
  chain->SetBranchAddress("CFT_MaxFiberEdep0_pion",   event_sim.CFT_MaxFiberEdep0_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep1_pion",   event_sim.CFT_MaxFiberEdep1_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep2_pion",   event_sim.CFT_MaxFiberEdep2_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep3_pion",   event_sim.CFT_MaxFiberEdep3_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep4_pion",   event_sim.CFT_MaxFiberEdep4_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep5_pion",   event_sim.CFT_MaxFiberEdep5_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep6_pion",   event_sim.CFT_MaxFiberEdep6_pion);
  chain->SetBranchAddress("CFT_MaxFiberEdep7_pion",   event_sim.CFT_MaxFiberEdep7_pion);
  chain->SetBranchAddress("CFT_TotalEdep",            event_sim.CFT_TotalEdep);
  chain->SetBranchAddress("CFT_NormTotalEdep",        event_sim.CFT_NormTotalEdep);
  chain->SetBranchAddress("BGO_Edep",                 event_sim.BGO_Edep);
  chain->SetBranchAddress("BGO_Edep_proton",          event_sim.BGO_Edep_proton);
  chain->SetBranchAddress("BGO_Edep_pion",            event_sim.BGO_Edep_pion);
  chain->SetBranchAddress("TotalEdep",                event_sim.TotalEdep);
}

void MakeTree(TTree *tree)
{
  tree->Branch("ntCFT_raw",         &src.ntCFT_raw,        "ntCFT_raw/I");
  tree->Branch("nP_CFT_raw",        &src.nP_CFT_raw,       "nP_CFT_raw/I");
  tree->Branch("nPi_CFT_raw",       &src.nPi_CFT_raw,      "nPi_CFT_raw/I");
  tree->Branch("track_proton",      &src.track_proton);
  tree->Branch("track_pion",        &src.track_pion);
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
  tree->Branch("theta_cft",         &src.theta_cft);
  tree->Branch("pid",               &src.pid);
  tree->Branch("dE_cft",            &src.dE_cft);
  tree->Branch("segBGOt",           &src.segBGOt);
  tree->Branch("energyBGO",         &src.energyBGO);
  tree->Branch("pulseheightBGO",    &src.pulseheightBGO);
  tree->Branch("segPiIDt",          &src.segPiIDt);
  tree->Branch("Dir_x_pi",          &src.Dir_x_pi);
  tree->Branch("Dir_y_pi",          &src.Dir_y_pi);
  tree->Branch("Dir_z_pi",          &src.Dir_z_pi);
  tree->Branch("Dir_x_p",           &src.Dir_x_p);
  tree->Branch("Dir_y_p",           &src.Dir_y_p);
  tree->Branch("Dir_z_p",           &src.Dir_z_p);
  tree->Branch("segBGOt_p",         &src.segBGOt_p);
  tree->Branch("energyBGO_p",       &src.energyBGO_p);
  tree->Branch("pulseheightBGO_p",  &src.pulseheightBGO_p);
  tree->Branch("Total_dE_p",        &src.Total_dE_p);
  tree->Branch("segBGOt_pi",        &src.segBGOt_pi);
  tree->Branch("energyBGO_pi",      &src.energyBGO_pi);
  tree->Branch("pulseheightBGO_pi", &src.pulseheightBGO_pi);
  tree->Branch("Total_dE_pi",       &src.Total_dE_pi);
  tree->Branch("decayLambda_vtx_x", &src.decayLambda_vtx_x, "decayLambda_vtx_x/D");
  tree->Branch("decayLambda_vtx_y", &src.decayLambda_vtx_y, "decayLambda_vtx_y/D");
  tree->Branch("decayLambda_vtx_z", &src.decayLambda_vtx_z, "decayLambda_vtx_z/D");
  tree->Branch("decayLambda_theta_pion",    &src.decayLambda_theta_pion,   "decayLambda_theta_pion/D");
  tree->Branch("decayLambda_theta_proton",  &src.decayLambda_theta_proton, "decayLambda_theta_proton/D");
  tree->Branch("decayLambda_mom_pi", &src.decayLambda_mom_pi, "decayLambda_mom_pi/D");
  tree->Branch("decayLambda_mom_p",  &src.decayLambda_mom_p,  "decayLambda_mom_p/D");
  tree->Branch("decayLambda_energyloss_pion",   &src.decayLambda_energyloss_pion,   "decayLambda_energyloss_pion/D");
  tree->Branch("decayLambda_energyloss_proton", &src.decayLambda_energyloss_proton, "decayLambda_energyloss_proton/D");
  tree->Branch("decayLambda_energylossCFT_pion",   &src.decayLambda_energylossCFT_pion,   Form("decayLambda_energylossCFT_pion[%d]/D",   NumOfPlaneCFT));
  tree->Branch("decayLambda_energylossCFT_proton", &src.decayLambda_energylossCFT_proton, Form("decayLambda_energylossCFT_proton[%d]/D", NumOfPlaneCFT));

  tree->Branch("ntCFT_sim",                 &src.ntCFT_sim,                "ntCFT_sim/I");
  tree->Branch("nP_CFT_sim",                &src.nP_CFT_sim,               "nP_CFT_sim/I");
  tree->Branch("nPi_CFT_sim",               &src.nPi_CFT_sim,              "nPi_CFT_sim/I");
  tree->Branch("CFT_MaxFiberEdep0",         &src.CFT_MaxFiberEdep0);
  tree->Branch("CFT_MaxFiberEdep1",         &src.CFT_MaxFiberEdep1);
  tree->Branch("CFT_MaxFiberEdep2",         &src.CFT_MaxFiberEdep2);
  tree->Branch("CFT_MaxFiberEdep3",         &src.CFT_MaxFiberEdep3);
  tree->Branch("CFT_MaxFiberEdep4",         &src.CFT_MaxFiberEdep4);
  tree->Branch("CFT_MaxFiberEdep5",         &src.CFT_MaxFiberEdep5);
  tree->Branch("CFT_MaxFiberEdep6",         &src.CFT_MaxFiberEdep6);
  tree->Branch("CFT_MaxFiberEdep7",         &src.CFT_MaxFiberEdep7);
  tree->Branch("CFT_MaxFiberEdep0_proton",  &src.CFT_MaxFiberEdep0_proton);
  tree->Branch("CFT_MaxFiberEdep1_proton",  &src.CFT_MaxFiberEdep1_proton);
  tree->Branch("CFT_MaxFiberEdep2_proton",  &src.CFT_MaxFiberEdep2_proton);
  tree->Branch("CFT_MaxFiberEdep3_proton",  &src.CFT_MaxFiberEdep3_proton);
  tree->Branch("CFT_MaxFiberEdep4_proton",  &src.CFT_MaxFiberEdep4_proton);
  tree->Branch("CFT_MaxFiberEdep5_proton",  &src.CFT_MaxFiberEdep5_proton);
  tree->Branch("CFT_MaxFiberEdep6_proton",  &src.CFT_MaxFiberEdep6_proton);
  tree->Branch("CFT_MaxFiberEdep7_proton",  &src.CFT_MaxFiberEdep7_proton);
  tree->Branch("CFT_MaxFiberEdep0_pion",    &src.CFT_MaxFiberEdep0_pion);
  tree->Branch("CFT_MaxFiberEdep1_pion",    &src.CFT_MaxFiberEdep1_pion);
  tree->Branch("CFT_MaxFiberEdep2_pion",    &src.CFT_MaxFiberEdep2_pion);
  tree->Branch("CFT_MaxFiberEdep3_pion",    &src.CFT_MaxFiberEdep3_pion);
  tree->Branch("CFT_MaxFiberEdep4_pion",    &src.CFT_MaxFiberEdep4_pion);
  tree->Branch("CFT_MaxFiberEdep5_pion",    &src.CFT_MaxFiberEdep5_pion);
  tree->Branch("CFT_MaxFiberEdep6_pion",    &src.CFT_MaxFiberEdep6_pion);
  tree->Branch("CFT_MaxFiberEdep7_pion",    &src.CFT_MaxFiberEdep7_pion);
  tree->Branch("CFT_TotalEdep",             &src.CFT_TotalEdep);
  tree->Branch("CFT_NormTotalEdep",         &src.CFT_NormTotalEdep);
  tree->Branch("BGO_Edep",                  &src.BGO_Edep);
  tree->Branch("BGO_Edep_proton",           &src.BGO_Edep_proton);
  tree->Branch("BGO_Edep_pion",             &src.BGO_Edep_pion);
  tree->Branch("TotalEdep",                 &src.TotalEdep);
}

void Clear_event()
{
  event_raw.ntCFT        = qnan;
  event_raw.nP_CFT       = qnan;
  event_raw.nPi_CFT      = qnan;
  event_sim.ntCFT        = qnan;
  event_sim.nP_CFT       = qnan;
  event_sim.nPi_CFT      = qnan;
  for(Int_t i=0; i<NumOfPlaneCFT; i++){
    event_raw.decayLambda_energylossCFT_pion[i]   = qnan;
    event_raw.decayLambda_energylossCFT_proton[i] = qnan;
  }
  for(Int_t i=0; i<MaxHits2; i++){
    event_sim.CFT_MaxFiberEdep0[i]        = qnan;
    event_sim.CFT_MaxFiberEdep1[i]        = qnan;
    event_sim.CFT_MaxFiberEdep2[i]        = qnan;
    event_sim.CFT_MaxFiberEdep3[i]        = qnan;
    event_sim.CFT_MaxFiberEdep4[i]        = qnan;
    event_sim.CFT_MaxFiberEdep5[i]        = qnan;
    event_sim.CFT_MaxFiberEdep6[i]        = qnan;
    event_sim.CFT_MaxFiberEdep7[i]        = qnan;
    event_sim.CFT_MaxFiberEdep0_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep1_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep2_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep3_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep4_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep5_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep6_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep7_proton[i] = qnan;
    event_sim.CFT_MaxFiberEdep0_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep1_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep2_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep3_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep4_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep5_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep6_pion[i]   = qnan;
    event_sim.CFT_MaxFiberEdep7_pion[i]   = qnan;
    event_sim.CFT_TotalEdep[i]            = qnan;
    event_sim.CFT_NormTotalEdep[i]        = qnan;
    event_sim.BGO_Edep[i]                 = qnan;
    event_sim.TotalEdep[i]                = qnan;
  }
  event_raw.decayLambda_vtx_x             = qnan;
  event_raw.decayLambda_vtx_y             = qnan;
  event_raw.decayLambda_vtx_z             = qnan;
  event_raw.decayLambda_theta_pion        = qnan;
  event_raw.decayLambda_theta_proton      = qnan;
  event_raw.decayLambda_mom_pi            = qnan;
  event_raw.decayLambda_mom_p             = qnan;
  event_raw.decayLambda_energyloss_pion   = qnan;
  event_raw.decayLambda_energyloss_proton = qnan;
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

  for(Int_t i=0; i<NumOfPlaneCFT; i++){
    src.decayLambda_energylossCFT_pion[i]   = qnan;
    src.decayLambda_energylossCFT_proton[i] = qnan;
  }

  src.track_proton = new std::vector<Int_t>();
  src.track_pion   = new std::vector<Int_t>();
  src.theta_cft    = new std::vector<Double_t>();
  src.pid          = new std::vector<Int_t>();
  src.dE_cft   = new std::vector<Double_t>();
  src.segPiIDt = new std::vector<Int_t>();
  src.Dir_x_pi = new std::vector<Double_t>();
  src.Dir_y_pi = new std::vector<Double_t>();
  src.Dir_z_pi = new std::vector<Double_t>();
  src.Dir_x_p = new std::vector<Double_t>();
  src.Dir_y_p = new std::vector<Double_t>();
  src.Dir_z_p = new std::vector<Double_t>();
  src.Total_dE_p = new std::vector<Double_t>();
  src.Total_dE_pi = new std::vector<Double_t>();

  src.CFT_MaxFiberEdep0 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep1 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep2 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep3 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep4 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep5 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep6 = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep7 = new std::vector<Double_t>();

  src.CFT_MaxFiberEdep0_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep1_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep2_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep3_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep4_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep5_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep6_proton = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep7_proton = new std::vector<Double_t>();

  src.CFT_MaxFiberEdep0_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep1_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep2_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep3_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep4_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep5_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep6_pion = new std::vector<Double_t>();
  src.CFT_MaxFiberEdep7_pion = new std::vector<Double_t>();

  src.CFT_TotalEdep     = new std::vector<Double_t>();
  src.CFT_NormTotalEdep = new std::vector<Double_t>();
  src.BGO_Edep          = new std::vector<Double_t>();
  src.BGO_Edep_proton   = new std::vector<Double_t>();
  src.BGO_Edep_pion     = new std::vector<Double_t>();
  src.TotalEdep         = new std::vector<Double_t>();

  src.segBGOt        = new std::vector<Int_t>();
  src.energyBGO      = new std::vector<Double_t>();
  src.pulseheightBGO = new std::vector<Double_t>();
  src.segBGOt_p      = new std::vector<Int_t>();
  src.energyBGO_p    = new std::vector<Double_t>();
  src.pulseheightBGO_p = new std::vector<Double_t>();
  src.segBGOt_pi       = new std::vector<Int_t>();
  src.energyBGO_pi      = new std::vector<Double_t>();
  src.pulseheightBGO_pi = new std::vector<Double_t>();

  src.decayLambda_vtx_x             = qnan;
  src.decayLambda_vtx_y             = qnan;
  src.decayLambda_vtx_z             = qnan;
  src.decayLambda_theta_pion        = qnan;
  src.decayLambda_theta_proton      = qnan;
  src.decayLambda_mom_pi            = qnan;
  src.decayLambda_mom_p             = qnan;
  src.decayLambda_energyloss_pion   = qnan;
  src.decayLambda_energyloss_proton = qnan;
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

  for(Int_t i=0; i<NumOfPlaneCFT; i++){
    src.decayLambda_energylossCFT_pion[i]   = qnan;
    src.decayLambda_energylossCFT_proton[i] = qnan;
  }

  src.track_proton->clear();
  src.track_pion->clear();
  src.theta_cft->clear();
  src.pid->clear();
  src.dE_cft->clear();
  src.segPiIDt->clear();
  src.Dir_x_pi->clear();
  src.Dir_y_pi->clear();
  src.Dir_z_pi->clear();
  src.Dir_x_p->clear();
  src.Dir_y_p->clear();
  src.Dir_z_p->clear();
  src.Total_dE_p->clear();
  src.Total_dE_pi->clear();

  src.CFT_MaxFiberEdep0->clear();
  src.CFT_MaxFiberEdep1->clear();
  src.CFT_MaxFiberEdep2->clear();
  src.CFT_MaxFiberEdep3->clear();
  src.CFT_MaxFiberEdep4->clear();
  src.CFT_MaxFiberEdep5->clear();
  src.CFT_MaxFiberEdep6->clear();
  src.CFT_MaxFiberEdep7->clear();
  src.CFT_MaxFiberEdep0_proton->clear();
  src.CFT_MaxFiberEdep1_proton->clear();
  src.CFT_MaxFiberEdep2_proton->clear();
  src.CFT_MaxFiberEdep3_proton->clear();
  src.CFT_MaxFiberEdep4_proton->clear();
  src.CFT_MaxFiberEdep5_proton->clear();
  src.CFT_MaxFiberEdep6_proton->clear();
  src.CFT_MaxFiberEdep7_proton->clear();
  src.CFT_MaxFiberEdep0_pion->clear();
  src.CFT_MaxFiberEdep1_pion->clear();
  src.CFT_MaxFiberEdep2_pion->clear();
  src.CFT_MaxFiberEdep3_pion->clear();
  src.CFT_MaxFiberEdep4_pion->clear();
  src.CFT_MaxFiberEdep5_pion->clear();
  src.CFT_MaxFiberEdep6_pion->clear();
  src.CFT_MaxFiberEdep7_pion->clear();
  src.CFT_TotalEdep->clear();
  src.CFT_NormTotalEdep->clear();
  src.BGO_Edep->clear();
  src.BGO_Edep_proton->clear();
  src.BGO_Edep_pion->clear();
  src.TotalEdep->clear();

  src.segBGOt->clear();
  src.energyBGO->clear();
  src.pulseheightBGO->clear();
  src.segBGOt_p->clear();
  src.energyBGO_p->clear();
  src.pulseheightBGO_p->clear();
  src.segBGOt_pi->clear();
  src.energyBGO_pi->clear();
  src.pulseheightBGO_pi->clear();

  src.decayLambda_vtx_x             = qnan;
  src.decayLambda_vtx_y             = qnan;
  src.decayLambda_vtx_z             = qnan;
  src.decayLambda_theta_pion        = qnan;
  src.decayLambda_theta_proton      = qnan;
  src.decayLambda_mom_pi            = qnan;
  src.decayLambda_mom_p             = qnan;
  src.decayLambda_energyloss_pion   = qnan;
  src.decayLambda_energyloss_proton = qnan;
}

void assignment()
{
  src.ntCFT_raw   = event_raw.ntCFT;
  src.nP_CFT_raw  = event_raw.nP_CFT;
  src.nPi_CFT_raw = event_raw.nPi_CFT;
  src.ntCFT_sim   = event_sim.ntCFT;
  src.nP_CFT_sim  = event_sim.nP_CFT;
  src.nPi_CFT_sim = event_sim.nPi_CFT;

  src.MaxSegment_0 = event_raw.MaxSegment_0;
  src.MaxSegment_1 = event_raw.MaxSegment_1;
  src.MaxSegment_2 = event_raw.MaxSegment_2;
  src.MaxSegment_3 = event_raw.MaxSegment_3;
  src.MaxSegment_4 = event_raw.MaxSegment_4;
  src.MaxSegment_5 = event_raw.MaxSegment_5;
  src.MaxSegment_6 = event_raw.MaxSegment_6;
  src.MaxSegment_7 = event_raw.MaxSegment_7;
  src.MaxMipLow_0  = event_raw.MaxMipLow_0;
  src.MaxMipLow_1  = event_raw.MaxMipLow_1;
  src.MaxMipLow_2  = event_raw.MaxMipLow_2;
  src.MaxMipLow_3  = event_raw.MaxMipLow_3;
  src.MaxMipLow_4  = event_raw.MaxMipLow_4;
  src.MaxMipLow_5  = event_raw.MaxMipLow_5;
  src.MaxMipLow_6  = event_raw.MaxMipLow_6;
  src.MaxMipLow_7  = event_raw.MaxMipLow_7;
  for(Int_t i=0; i<NumOfPlaneCFT; i++){
    src.decayLambda_energylossCFT_pion[i]   = event_raw.decayLambda_energylossCFT_pion[i];
    src.decayLambda_energylossCFT_proton[i] = event_raw.decayLambda_energylossCFT_proton[i];
  }

  src.theta_cft      = event_raw.theta_cft;
  src.pid            = event_raw.pid;
  src.dE_cft         = event_raw.dE_cft;
  src.segBGOt        = event_raw.segBGOt;
  src.segPiIDt       = event_raw.segPiIDt;
  src.energyBGO      = event_raw.energyBGO;
  src.pulseheightBGO = event_raw.pulseheightBGO;
  src.Total_dE_p     = event_raw.Total_dE_p;

  src.track_pion        = event_raw.track_pion;
  src.Dir_x_pi          = event_raw.Dir_x_pi;
  src.Dir_y_pi          = event_raw.Dir_y_pi;
  src.Dir_z_pi          = event_raw.Dir_z_pi;
  src.segBGOt_pi        = event_raw.segBGOt_pi;
  src.energyBGO_pi      = event_raw.energyBGO_pi;
  src.pulseheightBGO_pi = event_raw.pulseheightBGO_pi;
  src.Total_dE_pi       = event_raw.Total_dE_pi;

  src.track_proton     = event_raw.track_proton;
  src.Dir_x_p          = event_raw.Dir_x_p;
  src.Dir_y_p          = event_raw.Dir_y_p;
  src.Dir_z_p          = event_raw.Dir_z_p;
  src.segBGOt_p        = event_raw.segBGOt_p;
  src.energyBGO_p      = event_raw.energyBGO_p;
  src.pulseheightBGO_p = event_raw.pulseheightBGO_p;


  for(Int_t i=0; i<event_sim.ntCFT; i++){
    src.CFT_TotalEdep->push_back(event_sim.CFT_TotalEdep[i]);
    src.CFT_NormTotalEdep->push_back(event_sim.CFT_NormTotalEdep[i]);
    src.BGO_Edep->push_back(event_sim.BGO_Edep[i]);
    src.TotalEdep->push_back(event_sim.TotalEdep[i]);
    src.CFT_MaxFiberEdep0->push_back(event_sim.CFT_MaxFiberEdep0[i]);
    src.CFT_MaxFiberEdep1->push_back(event_sim.CFT_MaxFiberEdep1[i]);
    src.CFT_MaxFiberEdep2->push_back(event_sim.CFT_MaxFiberEdep2[i]);
    src.CFT_MaxFiberEdep3->push_back(event_sim.CFT_MaxFiberEdep3[i]);
    src.CFT_MaxFiberEdep4->push_back(event_sim.CFT_MaxFiberEdep4[i]);
    src.CFT_MaxFiberEdep5->push_back(event_sim.CFT_MaxFiberEdep5[i]);
    src.CFT_MaxFiberEdep6->push_back(event_sim.CFT_MaxFiberEdep6[i]);
    src.CFT_MaxFiberEdep7->push_back(event_sim.CFT_MaxFiberEdep7[i]);
  }

  for(Int_t i=0; i<event_sim.nP_CFT; i++){
    src.BGO_Edep_proton->push_back(event_sim.BGO_Edep_proton[i]);
    src.CFT_MaxFiberEdep0_proton->push_back(event_sim.CFT_MaxFiberEdep0_proton[i]);
    src.CFT_MaxFiberEdep1_proton->push_back(event_sim.CFT_MaxFiberEdep1_proton[i]);
    src.CFT_MaxFiberEdep2_proton->push_back(event_sim.CFT_MaxFiberEdep2_proton[i]);
    src.CFT_MaxFiberEdep3_proton->push_back(event_sim.CFT_MaxFiberEdep3_proton[i]);
    src.CFT_MaxFiberEdep4_proton->push_back(event_sim.CFT_MaxFiberEdep4_proton[i]);
    src.CFT_MaxFiberEdep5_proton->push_back(event_sim.CFT_MaxFiberEdep5_proton[i]);
    src.CFT_MaxFiberEdep6_proton->push_back(event_sim.CFT_MaxFiberEdep6_proton[i]);
    src.CFT_MaxFiberEdep7_proton->push_back(event_sim.CFT_MaxFiberEdep7_proton[i]);
  }

  for(Int_t i=0; i<event_sim.nPi_CFT; i++){
    src.BGO_Edep_pion->push_back(event_sim.BGO_Edep_pion[i]);
    src.CFT_MaxFiberEdep0_pion->push_back(event_sim.CFT_MaxFiberEdep0_pion[i]);
    src.CFT_MaxFiberEdep1_pion->push_back(event_sim.CFT_MaxFiberEdep1_pion[i]);
    src.CFT_MaxFiberEdep2_pion->push_back(event_sim.CFT_MaxFiberEdep2_pion[i]);
    src.CFT_MaxFiberEdep3_pion->push_back(event_sim.CFT_MaxFiberEdep3_pion[i]);
    src.CFT_MaxFiberEdep4_pion->push_back(event_sim.CFT_MaxFiberEdep4_pion[i]);
    src.CFT_MaxFiberEdep5_pion->push_back(event_sim.CFT_MaxFiberEdep5_pion[i]);
    src.CFT_MaxFiberEdep6_pion->push_back(event_sim.CFT_MaxFiberEdep6_pion[i]);
    src.CFT_MaxFiberEdep7_pion->push_back(event_sim.CFT_MaxFiberEdep7_pion[i]);
  }

  src.decayLambda_vtx_x             = event_raw.decayLambda_vtx_x;
  src.decayLambda_vtx_y             = event_raw.decayLambda_vtx_y;
  src.decayLambda_vtx_z             = event_raw.decayLambda_vtx_z;
  src.decayLambda_theta_pion        = event_raw.decayLambda_theta_pion;
  src.decayLambda_theta_proton      = event_raw.decayLambda_theta_proton;
  src.decayLambda_mom_pi            = event_raw.decayLambda_mom_pi;
  src.decayLambda_mom_p             = event_raw.decayLambda_mom_p;
  src.decayLambda_energyloss_pion   = event_raw.decayLambda_energyloss_pion;
  src.decayLambda_energyloss_proton = event_raw.decayLambda_energyloss_proton;
}
