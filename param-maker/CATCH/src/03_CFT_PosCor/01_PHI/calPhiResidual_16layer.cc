#include <cstdio>
#include <cstdlib>
#include <math.h>
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

#include "TApplication.h"
#include "TRint.h"

#include "DetectorID.hh"
#include "PathID.hh"

const int MaxHits2=60;

struct Event {
  int ntCFT_16layer;
  int nhXY_16layer[MaxHits2];
  int nhZ_16layer[MaxHits2];
  double chisqrXY_16layer[MaxHits2];
  double chisqrZ_16layer[MaxHits2];
  int    nhPhi_16layer[4];
  double hitMeanSegPhi_16layer[4][MaxHits2];
  double phi_16layer[4][MaxHits2];
  double resPhi_16layer[4][MaxHits2];
  double calZPhi_16layer[4][MaxHits2];
};
static Event event;

void SetBranchAddress(TChain *chain);

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <count>"
	      << std::endl;
    return -1;
  }

  int runnumber = std::atoi(argv[1]);
  int count     = std::atoi(argv[2]);

  TString filename = Form("%s/run%05d/run%05d_CFTTrackingCosmic.root", root_path.Data(), runnumber, runnumber);

  TChain *chain = new TChain("tree");
  chain->Add(filename);

  TCanvas *c1 = new TCanvas("c1","c1");
  c1->Divide(2,2);

  char buf1[300], buf2[300], buf_cut[300];

  TH2F *h[4][36];

  double par[4][6];

  TString name;
  if(count == 0)
    name.Form("%s/run%05d/CATCH_calPhiResidual_16layer_phicor_%05d.pdf", pdf_path.Data(), runnumber, runnumber);
  else if(count == 1)
    name.Form("%s/run%05d/CATCH_calPhiResidual_16layer_phicor_%05d_phiuvphicor.pdf", pdf_path.Data(), runnumber, runnumber);

  c1->Print(name+"[","pdf");

  for (int i=0; i<36; i++) {
    for (int j=0; j<4; j++) {
      c1->cd(j+1);
      sprintf(buf1, "h%d_%d", j+1, i);
      sprintf(buf2, "layer%d, angle (%d - %d)", j+1, i*10, (i+1)*10);
      // h[j][i] = new TH2F(buf1, buf2, 200, -150, 250, 200, -5, 5);
      h[j][i] = new TH2F(buf1, buf2, 200, 0, 400, 200, -5, 5);

      sprintf(buf2, "resPhi%d_16layer:calZPhi%d_16layer+%f", j+1, j+1, offsetCATCH);
      //sprintf(buf_cut, "phi%d >= %d && phi%d <= %d", j+1, i*10, j+1, (i+1)*10);
      sprintf(buf_cut, "phi%d_16layer >= %d && phi%d_16layer <= %d", j+1, i*10, j+1, (i+1)*10);

      chain->Project(buf1, buf2, buf_cut);

      h[j][i]->Draw();
      h[j][i]->Fit("pol5");

      TF1 *func = h[j][i]->GetFunction("pol5");
      for (int k=0; k<6; k++) {
	if(!func){
	  std::cout << "#E cannot GetFunction : " << i << " : " << j << std::endl;
	  par[j][k] = 0;
	  continue;
	}
	par[j][k] = func->GetParameter(k);
      }

    }
    c1->Update();
    c1->Print(name, "pdf");
  }
  c1->Print(name+"]", "pdf");

  SetBranchAddress(chain);

  TH2F *h2[4][36];
  TF1 *func2[4][36];

  for (int i=0; i<36; i++) {
    for (int j=0; j<4; j++) {
      sprintf(buf1, "h2_%d_%d", j+1, i);
      sprintf(buf2, "layer%d, angle (%d - %d)", j+1, i*10, (i+1)*10);
      // h2[j][i] = new TH2F(buf1, buf2, 200, -150, 250, 200, -5, 5);
      h2[j][i] = new TH2F(buf1, buf2, 200, 0, 400, 200, -5, 5);

      sprintf(buf1, "func2_%d_%d", j+1, i);
      func2[j][i] = new TF1(buf1, "pol5", -150, 250);
      for (int k=0; k<6; k++)
	func2[j][i]->SetParameter(k, par[j][k]);
    }
  }

  int n_entries = chain->GetEntries();
  std::cout << n_entries << std::endl;

  for (Int_t i=0;i<n_entries;i++) {
    if(i%10000==0)
      std::cout<<i<< " : " << n_entries << std::endl;

    chain->GetEntry(i);

    for (int j=0; j<4; j++) {
      int nhPhi = event.nhPhi_16layer[j];
      //std::cout << "layer " << j << " : nhPhi = " << nhPhi << std::endl;
      for (int nh=0; nh<event.nhPhi_16layer[j]; nh++) {
	double phi = event.phi_16layer[j][nh];
	double res = event.resPhi_16layer[j][nh];
	double calz = event.calZPhi_16layer[j][nh] + offsetCATCH;

	int index = ((int)phi)/10;
	double cal_res = func2[j][index]->Eval(calz);

	if (res-cal_res<3)
	  h2[j][index]->Fill(calz, res);
      }
    }
  }

  double par_pol5[4][36][6];
  for (int i=0; i<36; i++) {
    for (int j=0; j<4; j++) {
      c1->cd(j+1);
      h2[j][i]->Draw();
      h2[j][i]->Fit("pol5");

      TF1 *func = h2[j][i]->GetFunction("pol5");
      for (int k=0; k<6; k++){
	if(!func){
	  std::cout << "#E cannot GetFunction : " << i << " : " << j << std::endl;
	  par_pol5[j][i][k] = 0;
	  continue;
	}
	par_pol5[j][i][k] = func->GetParameter(k);
      }
    }
    c1->Update();
    // getchar();
  }


  if(count == 0)
    name.Form("%s/run%05d/CATCH_calPhiResidual_16layer_phi_pos_corr_phicor_%05d.txt", txt_path.Data(), runnumber, runnumber);
  else if(count == 1)
    name.Form("%s/run%05d/CATCH_calPhiResidual_16layer_phi_pos_corr_phiuvphicor_%05d.txt", txt_path.Data(), runnumber, runnumber);

  FILE *fp = fopen(name,"w");
  if (!fp) {
    fprintf(stderr, "cannnot open output file\n");
    return 0;
  }
  fprintf(fp, "## phi layer\n");
  fprintf(fp, "# PHIorU layer angle par0 ....\n");
  for (int j=0; j<4; j++) {
    fprintf(fp, "## phi layer %d ##\n", j);
    for (int i=0; i<36; i++) {
      fprintf(fp, "0 %d %d  ", j, i);
      for (int k=0; k<2; k++) {
	fprintf(fp, "%lf ", par_pol5[j][i][k]);
      }
      for (int k=2; k<6; k++) {
	fprintf(fp, "%e ", par_pol5[j][i][k]);
      }
      fprintf(fp, "\n");
    }
  }
  fclose(fp);

}

void SetBranchAddress(TChain *chain)
{
  chain->SetBranchAddress("ntCFT_16layer", &event.ntCFT_16layer);
  chain->SetBranchAddress("nhXY_16layer", event.nhXY_16layer);
  chain->SetBranchAddress("nhZ_16layer", event.nhZ_16layer);

  chain->SetBranchAddress("chisqrXY_16layer", event.chisqrXY_16layer);
  chain->SetBranchAddress("chisqrZ_16layer", event.chisqrZ_16layer);

  chain->SetBranchAddress("nhPhi1_16layer", &event.nhPhi_16layer[0]);
  chain->SetBranchAddress("nhPhi2_16layer", &event.nhPhi_16layer[1]);
  chain->SetBranchAddress("nhPhi3_16layer", &event.nhPhi_16layer[2]);
  chain->SetBranchAddress("nhPhi4_16layer", &event.nhPhi_16layer[3]);

  chain->SetBranchAddress("hitMeanSegPhi1_16layer", &event.hitMeanSegPhi_16layer[0]);
  chain->SetBranchAddress("hitMeanSegPhi2_16layer", &event.hitMeanSegPhi_16layer[1]);
  chain->SetBranchAddress("hitMeanSegPhi3_16layer", &event.hitMeanSegPhi_16layer[2]);
  chain->SetBranchAddress("hitMeanSegPhi4_16layer", &event.hitMeanSegPhi_16layer[3]);

  chain->SetBranchAddress("phi1_16layer", &event.phi_16layer[0]);
  chain->SetBranchAddress("phi2_16layer", &event.phi_16layer[1]);
  chain->SetBranchAddress("phi3_16layer", &event.phi_16layer[2]);
  chain->SetBranchAddress("phi4_16layer", &event.phi_16layer[3]);

  chain->SetBranchAddress("resPhi1_16layer", &event.resPhi_16layer[0]);
  chain->SetBranchAddress("resPhi2_16layer", &event.resPhi_16layer[1]);
  chain->SetBranchAddress("resPhi3_16layer", &event.resPhi_16layer[2]);
  chain->SetBranchAddress("resPhi4_16layer", &event.resPhi_16layer[3]);

  chain->SetBranchAddress("calZPhi1_16layer", &event.calZPhi_16layer[0]);
  chain->SetBranchAddress("calZPhi2_16layer", &event.calZPhi_16layer[1]);
  chain->SetBranchAddress("calZPhi3_16layer", &event.calZPhi_16layer[2]);
  chain->SetBranchAddress("calZPhi4_16layer", &event.calZPhi_16layer[3]);

}
