/**
 * @file    show_hist_hyps.C
 * @brief   Draw hist of HypsTracking result
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */
#include <ctime>

void show_hist_hyps(TString FileName={"run03962_Hyps"}, int run=7163){
  gROOT->SetBatch(kTRUE); //not open canvas
  TLatex latex;
  latex.SetTextSize(0.08);
  latex.SetTextAlign(22);  
  
  const int NumOfFile = 1;
  const int NumOfHist = 14;
  int NameOfHist[NumOfHist] = {1, 2, 3, 5, 6,  11, 12, 13, 15, 15, 16, 17, 19, 22};
  
  TFile *file[NumOfFile];
  TCanvas *c0;
  TCanvas *h1[NumOfFile][NumOfHist];
  TCanvas *plane[10][NumOfFile];
  TCanvas *plane_track[10][NumOfFile];
  TH1D* hist;  
  int NumOfPlane =20;
  
  for(int i=0; i<NumOfFile; i++){
    TString PDFName = Form("../../../output/run%05d/%s.pdf", run, FileName[i].Data());
    c0 = new TCanvas("titleCanvas", "Title Page", 800, 600);
    c0->cd();
    c0->Print(PDFName + "[", "pdf");
    TString filename = Form("../../../root/run%05d/%s.root", run, FileName[i].Data());
    file[i] = new TFile(filename);
    int Flag = 0;
    for(int k=0; k<NumOfHist; k++){
      int hnum = NameOfHist[k];
      TString hname = Form("hist_%s_%d_%d", FileName[i].Data(),hnum, k);
      h1[i][k] = new TCanvas(hname, hname, 1000, 750);
      // h1[i][k]->Divide(4,3); //for 10 plane
      h1[i][k]->Divide(5,4); //for 20 plane
      for(int j=0; j<NumOfPlane; j++){
	h1[i][k]->cd(j+1);
	int l;
	if(j<10) l = j;
	else l = j + 20;
	TString histname = Form("h%d",100*(l+1)+hnum);
	hist=(TH1D*) gROOT->FindObject(histname);
	hist->Draw();
	if(hnum==15 && Flag==1){
	  double mean, sigma;
	  hist->Fit("gaus", "","", -0.5, 0.5);
	  mean = hist->GetFunction("gaus")->GetParameter(1);
	  sigma = hist->GetFunction("gaus")->GetParameter(2);
	  TString text1 = Form("mean = %f", mean);
	  TString text2 = Form("sigma = %f", sigma);
	  int max = hist->GetMaximum();
	  latex.DrawLatex(1.0, max*0.8, text1.Data());
	  latex.DrawLatex(1.0, max*0.7, text2.Data());
	}
      }
      if(hnum==15) Flag += 1;
      h1[i][k]->Print(PDFName, "pdf");
    }
    c0->Print(PDFName + "]", "pdf");    
  }
}
