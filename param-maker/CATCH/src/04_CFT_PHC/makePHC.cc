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

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return -1;
  }

  int runnumber = std::atoi(argv[1]);

  TString filename = Form("%s/run%05d/run%05d_CFTTracking.root", root_path.Data(), runnumber, runnumber);

  TChain *chain = new TChain("tree");
  chain->Add(filename);


  TFile *fin = new TFile(filename);
  fin->cd();

  std::ofstream outputfile(Form("%s/run%05d/run%05d_CATCH_CFT_HDPHC.txt", txt_path.Data(), runnumber, runnumber));
  TString pdfname;
  pdfname.Form("%s/run%05d/run%05d_CATCH_HDPHC.pdf", pdf_path.Data(), runnumber, runnumber);

  int NumOfAna = 50000;

  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Print(pdfname + "[", "pdf");
  c1->Divide(2, 8);

  double a_ref = 1000;
  TH2D *h[NumOfPlaneCFT][NumOfSegCFT[7]];
  TF1 *func1[NumOfPlaneCFT][NumOfSegCFT[7]];
  TGraph *gr[16];
  for (int layer=0; layer<NumOfPlaneCFT; layer++) {
    for (int seg=0; seg<NumOfSegCFT[layer]; seg++) {
      Int_t num = seg%16;
      c1->cd(num+1);


      TString hist = Form("h%d", ((layer+1)*1000+seg)*10+6);
      h[layer][seg] = (TH2D*)fin->Get(hist);

      int n_entries = h[layer][seg]->GetEntries();

      gr[num] = new TGraph();
      int count = 0;
      double a = 0;
      double y_bef = 0;
      for(int bin=0; bin<100; bin=bin+2){
	TH1D *h1 = (TH1D*)h[layer][seg]->ProjectionY("", bin, bin+1);

	if(h1->GetEntries()<n_entries/50)
	  continue;

	double max_bin = h1->GetMaximumBin();
	double max = h1->GetBinCenter(max_bin);

	h1->Fit("gaus", "", "", max-5, max+5);

	if(!h1->GetFunction("gaus"))
	   continue;

	double mean = h1->GetFunction("gaus")->GetParameter(1);
	double sigma = h1->GetFunction("gaus")->GetParameter(2);

	if(sigma<10){
	  if(count==0){
	    y_bef = mean;
	    double x = (bin+0.5)*0.0015;
	    gr[num]->SetPoint(count, x, mean);
	    count++;
	  }else{
	    std::cout << "mean : " << mean << std::endl;
	    std::cout << "y_bef : " << y_bef << std::endl;
	    std::cout << "now : " << (mean - y_bef)/0.0015 << std::endl;
	    std::cout << "a : " << a << std::endl;
	    if(a!=0){
	      if(abs(a - (mean - y_bef)/0.0015)<a_ref){
		a = (mean - y_bef)/0.0015;
		double x = (bin+0.5)*0.0015;
		gr[num]->SetPoint(count, x, mean);
		count++;
		y_bef = mean;
	      }
	    }else{
	      a = (mean - y_bef)/0.0015;
	      double x = (bin+0.5)*0.0015;
	      gr[num]->SetPoint(count, x, mean);
	      count++;
	      y_bef = mean;
	    }
	  }
	}

      }

      h[layer][seg]->Draw();
      gr[num]->Draw("P");

      hist = Form("h%d", ((layer+1)*1000+seg)*10+1);
      func1[layer][seg] = new TF1(hist, "pol1");
      gr[num]->Fit(hist);

      double p0 = func1[layer][seg]->GetParameter(0);
      double p1 = func1[layer][seg]->GetParameter(1);

      if(p1>0){
	int UorD = 0, type = 1, nParam = 3;
	outputfile << "31   " << layer << "     " << seg << "     "
		   << UorD << "     " << type << "     " << nParam
		   << "     " << p1 << "     0.0     " << -p0 << std::endl;
      }else{
	int UorD = 0, type = 0, nParam = 3;
	outputfile << "31   " << layer << "     " << seg << "     "
		   << UorD << "     " << type << "     " << nParam
		   << "     " << p1 << "     0.0     " << -p0 << std::endl;
      }

      if(num==15 || seg==NumOfSegCFT[layer]-1){
	c1->Print(pdfname, "pdf");
      }
    }
  }
  c1->Print(pdfname + "]", "pdf");
}
