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
#include <TDatabasePDG.h>

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t MaxHits2 = 60;
const Int_t NumOfCut = 14;
const Double_t Bin1 = 10;
const Double_t Bin2 = 20;
// const Double_t Bin = 200/NumOfCut;
const Int_t Num = 200000;
const auto qnan = TMath::QuietNaN();

void GetTree(TChain *chain, int runnumber, int run_start, int run_end);
void SetBranchAddress(TChain *chain_raw, TChain *chain_sim);
void Clear();

struct Event_raw{
  Int_t ntCFT;
  Int_t nP_CFT;
  Int_t nPi_CFT;

  std::vector<Int_t>* track_proton;
  std::vector<Int_t>* track_pion;
  std::vector<Int_t>* segBGOt;
  std::vector<Double_t>* pulseheightBGO;
  std::vector<Int_t>* segPiIDt;
  std::vector<Int_t>* segBGOt_p;
  std::vector<Double_t>* pulseheightBGO_p;
  std::vector<Int_t>* segBGOt_pi;
  std::vector<Double_t>* pulseheightBGO_pi;
  Double_t decayLambda_mom_pi;
  Double_t decayLambda_mom_p;
};

struct Event_sim{
  Int_t ntCFT;
  Int_t nP_CFT;
  Int_t nPi_CFT;

  double BGO_Edep[MaxHits2];
  double BGO_Edep_proton[MaxHits2];
  double BGO_Edep_pion[MaxHits2];
};

static Event_raw event_raw;
static Event_sim event_sim;

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

  TString file, pdffile, txtfile;
  file.Form("/group/had/sks/Users/miyu/hyps-geant4/rootfile/run%d_ana.root", run_sim);

  //chain
  TChain *chain_raw = new TChain("tree");
  GetTree(chain_raw, runnumber, run_start, run_end);

  TChain *chain_sim = new TChain("tree");
  chain_sim->Add(file);

  SetBranchAddress(chain_raw, chain_sim);

  pdffile.Form("%s/run%05d/catch_bgo_calib_sim_%05d_%05d.pdf", pdf_path.Data(), runnumber, run_start, run_end);
  // change tag
  txtfile.Form("%s/run%05d/BGOEnergyCalib_hyps_common_run%05d", txt_path.Data(), runnumber, runnumber);
  std::ofstream ofs(txtfile);

  TCanvas *c1 = new TCanvas("c1", "c1");
  c1->Divide(3, 5);
  TCanvas *c2 = new TCanvas("c2", "c2", 1500, 1000);
  c2->Divide(4, 6);
  TCanvas *c3 = new TCanvas("c3", "c3");

  c1->cd();
  c1->Print(pdffile + "[", "pdf");

  Double_t mass = TDatabasePDG::Instance()->GetParticle(2212)->Mass(); // proton
  TString hist, title, cond, cut, func;
  TH2D *h1[NumOfSegBGO];
  TH1D *h2[NumOfSegBGO][NumOfCut];
  TF1 *f1, *f2[NumOfSegBGO];
  TGraphErrors *gr1[NumOfSegBGO];
  Double_t x_func[Num], x_graph[NumOfSegBGO][NumOfCut], x_err[NumOfSegBGO][NumOfCut], y_err[NumOfSegBGO][NumOfCut], adc;

  for(Int_t i=0; i<Num; i++){
    x_func[i] = i*0.02;
  }

  for(Int_t i=0; i<NumOfSegBGO; i++){
    hist.Form("h1%02d", i);
    title.Form("BGO pulse height vs E_kin seg : %d", i);
    h1[i] = new TH2D(hist, title, 250, 0, 250, 100, 0, 20000);
    h1[i]->SetLabelSize(0.08, "x");
    h1[i]->SetLabelSize(0.08, "y");
    for(Int_t j=0; j<NumOfCut; j++){
      hist.Form("h2%02d%02d", i, j);
      h2[i][j] = new TH1D(hist, hist, 100, 0, 20000);
    }
  }

  std::cout << "#D Analysis" << std::endl;
  Int_t entries = chain_raw->GetEntries();
  for(Int_t i=0; i<entries; i++){
    chain_raw->GetEntry(i);
    chain_sim->GetEntry(i);
    if(i%100000==0){
      std::cout << entries << ":" << i << " " << std::endl;
    }
    if(event_raw.decayLambda_mom_p>0 && event_raw.nP_CFT==1){
      if(event_raw.pulseheightBGO_p->size()==1 && event_sim.BGO_Edep_proton[0]>0){
	if(event_raw.segBGOt_p->at(0)>=0 && event_raw.segBGOt_p->at(0)<24){
	  Double_t BGO_Edep = 0;
	  h1[event_raw.segBGOt_p->at(0)]->Fill(event_sim.BGO_Edep_proton[0], event_raw.pulseheightBGO_p->at(0));

	  if(event_sim.BGO_Edep_proton[0]>=0 && event_sim.BGO_Edep_proton[0]<Bin2*1)
	    h2[event_raw.segBGOt_p->at(0)][0]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin2*1 && event_sim.BGO_Edep_proton[0]<Bin2*2)
	    h2[event_raw.segBGOt_p->at(0)][1]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*2+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*3+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][2]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*3+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*4+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][3]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*4+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*5+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][4]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*5+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*6+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][5]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*6+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*7+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][6]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*7+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*8+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][7]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*8+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*9+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][8]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin1*9+Bin2 && event_sim.BGO_Edep_proton[0]<Bin1*10+Bin2)
	    h2[event_raw.segBGOt_p->at(0)][9]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin2*6 && event_sim.BGO_Edep_proton[0]<Bin2*7)
	    h2[event_raw.segBGOt_p->at(0)][10]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin2*7 && event_sim.BGO_Edep_proton[0]<Bin2*8)
	    h2[event_raw.segBGOt_p->at(0)][11]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin2*8 && event_sim.BGO_Edep_proton[0]<Bin2*9)
	    h2[event_raw.segBGOt_p->at(0)][12]->Fill(event_raw.pulseheightBGO_p->at(0));
	  else if(event_sim.BGO_Edep_proton[0]>=Bin2*9 && event_sim.BGO_Edep_proton[0]<Bin2*10)
	    h2[event_raw.segBGOt_p->at(0)][13]->Fill(event_raw.pulseheightBGO_p->at(0));

	}
      }
    }
  }


  // make param
  ofs << "#seg \t n \t energy \t adc" << std::endl;
  for(Int_t i=0; i<NumOfSegBGO; i++){ // make hist(ph vs adc (seg))
    std:: cout << "#D start for loop : " << i << std::endl;
    ofs << "#seg: " << i << std::endl;
    c2->cd(i+1);
    Double_t mean[NumOfCut];
    Int_t count = 0;

    h1[i]->Draw();
    for(Int_t j=0; j<NumOfCut; j++){ // make hist(cut each energy)
      TF1 *f3;
      c1->cd(j+1);

      h2[i][j]->Draw();

      Int_t bin = h2[i][j]->GetMaximumBin();

      Double_t gmean = h2[i][j]->GetBinCenter(bin);
      Double_t grms = h2[i][j]->GetRMS();

      h2[i][j]->Fit("gaus", "", "", gmean-grms, gmean+grms);
      f3 = h2[i][j]->GetFunction("gaus");
      if(f3){
	Double_t error = f3->GetParameter(2);
	Int_t entry = h2[i][j]->GetEntries();
	gmean = f3->GetParameter(1);
	if(error<3000 && entry>=10){
	  if(count>0){
	    if((gmean - mean[count-1])<1000) continue;
	  }
	  if(j<2){
	    x_graph[i][count] = (j*Bin2 + (j+1)*Bin2)/2;
	    x_err[i][count] = Bin2/2;
	  }else if(j>=2 && j<10){
	    x_graph[i][count] = (j*Bin1 + Bin2 + (j+1)*Bin1 + Bin2)/2;
	    x_err[i][count] = Bin1/2;
	  }else{
	    x_graph[i][count] = ((j-4)*Bin2 + (j-3)*Bin2)/2;
	    x_err[i][count] = Bin2/2;
	  }
	  mean[count] = f3->GetParameter(1);
	  y_err[i][count] = error;
	  count++;
	}
      }
    }
    c1->Print(pdffile, "pdf");

    // plot point and fit func
    c2->cd(i+1);
    gr1[i] = new TGraphErrors(count, x_graph[i], mean, x_err[i], y_err[i]);
    gr1[i]->SetMarkerSize(0.5);
    gr1[i]->SetMarkerStyle(20);
    gr1[i]->SetMarkerColor(kRed);
    gr1[i]->Draw("P");

    func.Form("[0]*(x - [1]*TMath::Log((x + [1])/[1]))");
    f1 = new TF1("f1", func);
    f1->SetParameters(100, 1);
    gr1[i]->Fit("f1", "", "", x_graph[i][0], x_graph[i][count-1]);
    f2[i] = gr1[i]->GetFunction("f1");
    for(Int_t j=0; j<Num; j++){
      adc = f2[i]->Eval(x_func[j]);
      ofs << i << "\t" << j << "\t" << x_func[j] << "\t" << adc << std::endl;
    }
    f2[i]->SetRange(0, 250);
    f2[i]->SetLineColor(kRed);
    f2[i]->SetLineWidth(2);
    f2[i]->Draw("same");

  }
  c2->Print(pdffile, "pdf");
  c2->Print(pdffile + "]", "pdf");
}

//-------------------------------------------------------------------
void Clear()
{
  event_raw.ntCFT   = qnan;
  event_raw.nP_CFT  = qnan;
  event_raw.nPi_CFT = qnan;

  event_sim.ntCFT   = qnan;
  event_sim.nP_CFT  = qnan;
  event_sim.nPi_CFT = qnan;

  event_raw.decayLambda_mom_pi            = qnan;
  event_raw.decayLambda_mom_p             = qnan;
}

//---------------------------------------------------------------
void GetTree(TChain *chain, int runnumber, int run_start, int run_end){

  TString dirname = Form("%s/run%05d/", root_path.Data(), runnumber);
  // TString dirname = Form("/group/had/sks/Users/miyu/hyps-analyzer/root_Kid/");

  int count = 0;
  for(int i=run_start; i<=run_end; i++){
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

//-------------------------------------------------------------------
void SetBranchAddress(TChain *chain_raw, TChain *chain_sim)
{
  chain_raw->SetBranchAddress("ntCFT",         &event_raw.ntCFT);
  chain_raw->SetBranchAddress("nP_CFT",        &event_raw.nP_CFT);
  chain_raw->SetBranchAddress("nPi_CFT",       &event_raw.nPi_CFT);

  chain_raw->SetBranchAddress("track_proton",      &event_raw.track_proton);
  chain_raw->SetBranchAddress("track_pion",        &event_raw.track_pion);

  chain_raw->SetBranchAddress("segBGOt",           &event_raw.segBGOt);
  chain_raw->SetBranchAddress("pulseheightBGO",    &event_raw.pulseheightBGO);
  chain_raw->SetBranchAddress("segPiIDt",          &event_raw.segPiIDt);

  chain_raw->SetBranchAddress("segBGOt_p",         &event_raw.segBGOt_p);
  chain_raw->SetBranchAddress("pulseheightBGO_p",  &event_raw.pulseheightBGO_p);

  chain_raw->SetBranchAddress("segBGOt_pi",        &event_raw.segBGOt_pi);
  chain_raw->SetBranchAddress("pulseheightBGO_pi", &event_raw.pulseheightBGO_pi);

  chain_raw->SetBranchAddress("decayLambda_mom_pi", &event_raw.decayLambda_mom_pi);
  chain_raw->SetBranchAddress("decayLambda_mom_p",  &event_raw.decayLambda_mom_p);

  chain_sim->SetBranchAddress("ntCFT",   &event_sim.ntCFT);
  chain_sim->SetBranchAddress("nP_CFT",  &event_sim.nP_CFT);
  chain_sim->SetBranchAddress("nPi_CFT", &event_sim.nPi_CFT);
  chain_sim->SetBranchAddress("BGO_Edep",    event_sim.BGO_Edep);
  chain_sim->SetBranchAddress("BGO_Edep_proton", event_sim.BGO_Edep_proton);
  chain_sim->SetBranchAddress("BGO_Edep_pion",   event_sim.BGO_Edep_pion);
}
