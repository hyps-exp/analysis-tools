/**
 * @file    show_hist_sdcout.C
 * @brief   Draw hist of SdcOutTracking result
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */
#include <ctime>

void show_hist_sdcout(TString FileName={"run03962_Out"}, int run=3962){
  gROOT->SetBatch(kTRUE); //not open canvas
  TLatex latex;
  latex.SetTextSize(0.08);
  latex.SetTextAlign(22);

  const int NumOfFile = 1;
  // TString FileName[NumOfFile] = {"01530_Out", "01543_Out"};//write xx of file name "runxx.root"
  int residual_dcout =1; //if you want to draw redidual of DC2 and DC3, set to 1
  int residual_tof =1; //if you want to draw redidual of TOF, set to 1
  const int NumOfHist = 8; //Number of hist for each layer
  int NameOfHist[NumOfHist] = {3, 4, 12, 16, 19, 20, 22, 30};//Name of hist for each layer
  // int NameOfHist[NumOfHist] = {1, 2, 5, 6, 7, 11, 40};//Name of hist for each layer
  const int NumOfHistAll = 2; //Number of hist which is common for all layer
  int NameOfHistAll[NumOfHistAll] = {53, 56};//Name of hist which is common for all layer
  
  TFile *file[NumOfFile];
  TCanvas *c0;
  TCanvas *c1[NumOfFile];
  TCanvas *c2[NumOfFile];
  TCanvas *h1[NumOfFile][NumOfHist];
  TCanvas *h2[NumOfFile][NumOfHistAll];
  TH1D* hist;  
  TH1D* hist2;  

  int NumOfPlane = 12;
  int NumOfSDCOut = 10;
  int NumOfTOF = 2;
  
  for(int i=0; i<NumOfFile; i++){
    TString PDFName = Form("../../../output/run%05d/%s.pdf", run, FileName[i].Data());
    c0 = new TCanvas("titleCanvas", "Title Page", 800, 600);
    c0->cd();
    c0->Print(PDFName + "[", "pdf");
    TString filename = Form("../../../root/run%05d/run%s.root", run, FileName[i].Data());
    file[i] = new TFile(filename);
    if(residual_dcout==1){
      TString c1name = Form("c1_%s", FileName[i].Data());
      c1[i] = new TCanvas(c1name, c1name, 1000, 750);
      c1[i]->Divide(4,3);
      c1[i]->cd(1);
      hist=(TH1D*) gROOT->FindObject("h10");
      hist->Draw();
      c1[i]->cd(2);
      hist=(TH1D*) gROOT->FindObject("h12");
      hist->Draw();
      for(int j=0; j<NumOfSDCOut; j++){
	c1[i]->cd(j+3);
	TString histname = Form("h%d",100*(j+1)+15);
	hist=(TH1D*) gROOT->FindObject(histname);
	hist->Draw();
      }
      c1[i]->Print(PDFName, "pdf");
    }
    if(residual_tof==1){
      int TOFHist[6] = {15, 21, 71, 72, 73, 74};
      TString c2name = Form("c2_%s", FileName[i].Data());
      c2[i] = new TCanvas(c2name, c2name, 1000, 750);
      c2[i]->Divide(4,3);
      for(int j=0; j<NumOfTOF; j++){
	for(int jj=0; jj<6; jj++){
	  c2[i]->cd(6*j+jj+1);
	  int h = TOFHist[jj];
	  TString histname = Form("h%d",100*(j+11)+h);
	  hist=(TH1D*) gROOT->FindObject(histname);
	  //	  if(!hist) continue;
	  hist->Draw();
	}
      }
      c2[i]->Print(PDFName, "pdf");
    }
    for(int k=0; k<NumOfHist; k++){
      int hnum = NameOfHist[k];
      TString hname = Form("hist_%s_%d", FileName[i].Data(),hnum);
      h1[i][k] = new TCanvas(hname, hname, 1000, 750);
      h1[i][k]->Divide(4,3);
      for(int j=0; j<NumOfPlane; j++){
	h1[i][k]->cd(j+1);
	TString histname = Form("h%d",100*(j+1)+hnum);
	hist=(TH1D*) gROOT->FindObject(histname);
	if(!hist) continue;
	hist->Draw();
        if(hnum==15){
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
      h1[i][k]->Print(PDFName, "pdf");
    }
    for(int l=0; l<NumOfHistAll; l++){
      int hnumall = NameOfHistAll[l];
      TString hnameall = Form("hist_%s_%d_all", FileName[i].Data(),hnumall);
      h2[i][l] = new TCanvas(hnameall, hnameall, 1000, 750);
      h2[i][l]->cd();
      TString histname = Form("h%d",hnumall);
      hist2=(TH1D*) gROOT->FindObject(histname);
      hist2->Draw();
      h2[i][l]->Print(PDFName, "pdf");
    }
    c0->Print(PDFName + "]", "pdf");
  }
}
