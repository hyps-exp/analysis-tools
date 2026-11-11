#include "../sdc_common.hh"
/**
 * @file    initial_XT_SdcIn.C
 * @brief   Analysis of initial XT calibration
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */

void initial_XT_SDCIn(TString FileName1 = {"07162_In"} , int run=7162){

  // TString FileName1 = {"07162_In"};//write xx of file name "runxx.root"
  FileName1 = sdc::stem(FileName1);
  sdc::outputDirectory(run);
  TFile *file1;
  TString OutFileName = Form("DCDriftParam_hyps_test_in_%05d", run);
  TString PDFName = Form("../../../output/run%05d/Initial_XT_SDCIn_%05d.pdf", run, run);
  
  TString filename1 = Form("../../../root/run%05d/run%s.root", run, FileName1.Data());
  file1 = sdc::open(filename1);
  TString outfilename = Form("../../../output/run%05d/%s", run, OutFileName.Data());
  std::ostringstream outfile;
  outfile << std::setprecision(17);

  const int nLayer = kNumOfLayersSDCIn;
  double MaxL[nLayer]={2.5,2.5,2.5,2.5,6.0,6.0,6.0,6.0,6.0,6.0};
  double Mindt[nLayer]={-5, -5, -5, -5, -5, -5, -5, -5, -5, -5};
  double Maxdt[nLayer]={50, 50, 50, 50, 125, 125, 125, 125, 125, 125};
  
  TGraph *g;
  TCanvas *c1 = new TCanvas("c1", "c1", 1000, 750);
  c1->Divide(3, 4);
  TF1 *f;

  for(int i=0; i<nLayer; i++){
    int Id = sdc::in[i].id;
    int WireId = 0;
    int Type = 6;
    int NParam = 6;
      
    // TString hist = Form("h%d", (i+1)*100+30); //For initial
    TString hist = Form("h%d", (i+1)*100+12); //100*layer+12:DriftTime[Track]
    TH1 *h1 = sdc::get<TH1>(file1, hist);
    if(h1->GetDimension()!=1) throw std::runtime_error("Initial XT requires a 1D histogram");
    int MinBin = h1->FindBin(Mindt[i]);
    int MaxBin = h1->FindBin(Maxdt[i]);
      
    double tEntry = h1->Integral(MinBin, MaxBin);
    if(!std::isfinite(tEntry) || tEntry<=0) throw std::runtime_error("Empty initial XT histogram");
    double ml = MaxL[i];
    int nBin = MaxBin - MinBin + 1;
    double x[nBin], y[nBin];
      
    for(int j=MinBin; j<=MaxBin; j++){
      int idx = j - MinBin;
      double Integral = h1->Integral(MinBin, j);
      x[idx] = h1->GetBinCenter(j);
      y[idx] = ml*Integral/tEntry;
    }
    c1->cd(i+1);
      
    g = new TGraph(nBin, x, y);
    g->SetName("g");
    g->SetMarkerColor(kRed);
    g->SetMarkerStyle(2);
    g->SetMarkerSize(0.9);
    g->Draw("ap");
      
    TString func = Form("func_%d", i);
    f = new TF1(func, "pol5(0)", -15., 200.);
    f->FixParameter(0, 0);
    f->SetParameter(1, 0);
    f->SetParameter(2, 0);
    f->SetParameter(3, 0);
    f->SetParameter(4, 0);
    f->SetParameter(5, 0);
    if(int(g->Fit(f, "", "same", Mindt[i]+5, Maxdt[i]))!=0) throw std::runtime_error("Initial XT fit failed");
    f->SetRange(Mindt[i], Maxdt[i]);
    f->SetLineColor(kBlue);
    f->Draw("same");
    c1->Update();
    
    double p1 = f->GetParameter(1);
    double p2 = f->GetParameter(2);
    double p3 = f->GetParameter(3);
    double p4 = f->GetParameter(4);
    double p5 = f->GetParameter(5);

    for(int k=1;k<=5;++k) if(!std::isfinite(f->GetParameter(k)))
      throw std::runtime_error("Nonfinite initial XT coefficient");
    std::cout << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
	      << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    if(i==0) outfile << "#SDC0" << std::endl;
    if(i==kNumOfLayersSDC0) outfile << "#SDC1" << std::endl;
    outfile << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
	    << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
  }
  sdc::writeParameters(outfilename,outfile.str());

  TCanvas *c0 = new TCanvas("titleCanvas", "Title Page", 800, 600);
  c0->cd();
  c0->Print(PDFName + "[", "pdf");
  c1->Print(PDFName, "pdf");
  c0->Print(PDFName + "]", "pdf");
  
  file1->Close();
  delete file1;
}
