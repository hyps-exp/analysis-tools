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

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t bin = 40;

int SetFitParam(double fit_par_gaus[bin][2], double limit_sigma[2], double fit_par_exp[2][2]);

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <count>"
	      << std::endl;
    return -1;
  }

  int runnumber = std::atoi(argv[1]);
  int count     = std::atoi(argv[2]);

  TChain *chain = new TChain("tree");
  chain->Add(Form("%s/run%05d/run%05d_CATCH.root", root_path.Data(), runnumber, runnumber));

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Divide(2, 4);
  TCanvas *c2 = new TCanvas("c2", "c2");
  c2->cd();

  // bin = 100;
  TH2D *h;
  TH1D *hh[bin];
  TF1 *f[bin], *f1, *f2;
  TGraphErrors *gr1 = new TGraphErrors(bin); // high
  TGraphErrors *gr2 = new TGraphErrors(bin); // low
  double mean_error[bin];
  double fit_par_gaus[bin][2];
  double limit_sigma[2];
  double fit_par_exp[2][2];
  int flag = SetFitParam(fit_par_gaus, limit_sigma, fit_par_exp);
  if(flag != 0){
    std::cerr << "#E parameter get fail" << std::endl;
    return -1;
  }

  TString hist, title, cond, cut, pdfname, txtfile;
  txtfile.Form("%s/run%05d/CATCHPid_hyps_common_run%05d_%d", txt_path.Data(), runnumber, runnumber, count);
  pdfname.Form("%s/run%05d/CATCHPid_hyps_common_run%05d_%d.pdf", pdf_path.Data(), runnumber, runnumber, count);

  c1->Print(pdfname + "[", "pdf");
  cond.Form("dE_cft");
  for(Int_t i=0; i<bin; i++){
    Int_t num = i%8 + 1;
    c1->cd(num);
    gPad->SetLogy();
    hist.Form("h%2d", i);
    title.Form("dE_cft ( %d < enrgyBGO < %d)", i*5, (i+1)*5);
    cut.Form("segBGOt<24 && segBGOt>=0 && energyBGO[segBGOt]>%d && energyBGO[segBGOt]<%d && segPiIDt==-1", i*5, (i+1)*5);
    std::cout << i*5 <<std::endl;

    Double_t mean, sigma;

    hh[i] = new TH1D(hist, title, 140, 0, 7);
    chain->Project(hist, cond, cut);
    hh[i]->Draw();

    hh[i]->Fit("gaus", "", "", fit_par_gaus[i][0], fit_par_gaus[i][1]);

    f[i] = hh[i]->GetFunction("gaus");

    mean          = f[i]->GetParameter(1);
    sigma         = f[i]->GetParameter(2);
    mean_error[i] = f[i]->GetParError(1);

    gr1->SetPoint(i, i*5+2.5, mean+limit_sigma[0]*sigma);
    gr2->SetPoint(i, i*5+2.5, mean+limit_sigma[1]*sigma);
    gr1->SetPointError(i, 2.5, mean_error[i]*100);
    gr2->SetPointError(i, 2.5, mean_error[i]*50);


    if(num == 8 || num == bin-1)
      c1->Print(pdfname, "pdf");
  }

  c2->cd();
  gPad->SetLogy(0);
  gPad->SetLogz();

  hist.Form("h");
  title.Form("#Delta E - E");
  cond.Form("dE_cft:energyBGO[segBGOt]");
  cut.Form("segBGOt<24 && segBGOt>=0 && segPiIDt<0");

  h = new TH2D(hist, title, 200, 0, 200, 140, 0, 7);
  chain->Project(hist, cond, cut, "colz");
  h->Draw();

  Double_t par1[4], par2[4];

  f1 = new TF1("f1", "[0]*exp(x*[1]) + [2]*exp(x*[3])");
  f2 = new TF1("f2", "[0]*exp(x*[1]) + [2]*exp(x*[3])");

  f1->SetLineColor(kBlack);
  f2->SetLineColor(kBlack);

  f1->SetLineWidth(3);
  f2->SetLineWidth(3);

  gr1->SetMarkerStyle(20);
  gr1->SetMarkerColor(kRed);
  gr2->SetMarkerStyle(20);
  gr2->SetMarkerColor(kBlue);
  gr1->Draw("P");
  gr2->Draw("P");

  gr1->Fit("f1", "", "", fit_par_exp[0][0], fit_par_exp[0][1]);
  gr2->Fit("f2", "", "", fit_par_exp[1][0], fit_par_exp[1][1]);

  c2->Print(pdfname, "pdf");

  for(Int_t i=0; i<4; i++){
    par1[i] = f1->GetParameter(i);
    par2[i] = f2->GetParameter(i);
  }

  std::cout << "Low line" << std::endl;
  std::cout << par2[0] << "\t" << par2[1] << "\t" << par2[2] << "\t" << par2[3] << std::endl;

  std::cout << "High line" << std::endl;
  std::cout << par1[0] << "\t" << par1[1] << "\t" << par1[2] << "\t" << par1[3] << std::endl;

  std::ofstream output(txtfile);
  output << "# Low line" << std::endl;
  output << 0 << "\t" << par2[0] << "\t" << par2[1] << "\t" << par2[2] << "\t" << par2[3] << std::endl;

  output << "# High line" << std::endl;
  output << 1 << "\t" << par1[0] << "\t" << par1[1] << "\t" << par1[2] << "\t" << par1[3] << std::endl;

  h->Draw();

  f1->SetParameters(par1[0], par1[1], par1[2], par1[3]);
  f2->SetParameters(par2[0], par2[1], par2[2], par2[3]);
  f1->SetRange(0, 200);
  f2->SetRange(0, 200);
  f1->Draw("same");
  f2->Draw("same");
  c2->Print(pdfname, "pdf");

  c2->Print(pdfname + "]", "pdf");

}

//-------------------------------------------------------------------------
int SetFitParam(double fit_par_gaus[bin][2], double limit_sigma[2], double fit_par_exp[2][2]){

  TString filename = Form("%s/makePid_dE_fit_range.txt", param_path.Data());
  std::ifstream ifs(filename);

  if(!ifs.is_open()){
    std::cerr << "#E file open fail : " << filename << std::endl;
    return -1;
  }

  std::string line;
  while(ifs.good() && std::getline(ifs, line)){
    if(line[0] != '#'){
      std::istringstream input_line(line);
      int tag, i;
      double p0, p1;
      if(input_line >> tag){
	if(tag == 0){
	  if(input_line >> i >> p0 >> p1){
	    fit_par_gaus[i][0] = p0;
	    fit_par_gaus[i][1] = p1;
	  }
	}else if(tag == 1){
	  if(input_line >> p0 >> p1){
	    limit_sigma[0] = p0;
	    limit_sigma[1] = p1;
	  }
	}else if(tag == 2){
	  if(input_line >> i >> p0 >> p1){
	    fit_par_exp[i][0] = p0;
	    fit_par_exp[i][1] = p1;
	  }
	}
      }
    }
  }

  return 0;
}
