#include "../sdc_common.hh"
/**
 * @file    initial_XT_SdcOut.C
 * @brief   Analysis of initial XT calibration
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */

void initial_XT_SDCOut(TString FileName2 = {"07162_Out"} , int run=7162){

  // TString FileName2 = {"07162_Out"};//write xx of file name "runxx.root"
  FileName2 = sdc::stem(FileName2);
  sdc::outputDirectory(run);
  TFile *file2;
  TString OutFileName =  Form("DCDriftParam_hyps_test_out_%05d", run);
  TString PDFName = Form("../../../output/run%05d/Initial_SDCOut_%05d.pdf", run, run);
  
  TString filename2 = Form("../../../root/run%05d/run%s.root", run, FileName2.Data());
  file2 = sdc::open(filename2);
  TString outfilename = Form("../../../output/run%05d/%s", run, OutFileName.Data());
  std::ostringstream outfile;
  outfile << std::setprecision(17);

  const int nLayer = kNumOfLayersSDCOut;
  double MaxL[nLayer]={10,10,10,10,10,10,10,10,10,10};
  double Mindt[nLayer]={-5, -5, -5, -5, -5, -5, -5, -5, -5, -5};
  double Maxdt[nLayer]={270, 270, 270, 270, 270, 270, 270, 270, 270, 270};
  
  TGraph *g;
  TCanvas *c1 = new TCanvas("c1", "c1", 1000, 750);
  c1->Divide(3, 4);
  TF1 *f;
  
  for(int i=0; i<nLayer; i++){
    int Id = sdc::out[i].id;
    int WireId = 0;
    int Type = 6;
    int NParam = 6;

    // TString hist = Form("h%d", (i+1)*100+12);
    TString hist = Form("h%d", (i+1)*100+30);
    TH1 *h1 = sdc::get<TH1>(file2, hist);
    if(!h1){
      std::cerr << "Warning: Histogram " << hist << "layer=" << i+1 << " not found. Skipping." << std::endl;
      continue;
    }
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
    f->SetParameter(1, 1.0267e-1);
    f->SetParameter(2,-1.87541e-3);
    f->SetParameter(3, 2.0871e-5);
    f->SetParameter(4, -1.41421e-7);
    f->SetParameter(5, 4.25782e-10);

    if(int(g->Fit(f, "", "same", Mindt[i]+5, Maxdt[i]-5))!=0) throw std::runtime_error("Initial XT fit failed");
    f->SetRange(Mindt[i], Maxdt[i]);
    f->SetLineColor(kBlue);
    f->Draw("same");

    double p1 = f->GetParameter(1);
    double p2 = f->GetParameter(2);
    double p3 = f->GetParameter(3);
    double p4 = f->GetParameter(4);
    double p5 = f->GetParameter(5);
    
    for(int k=1;k<=5;++k) if(!std::isfinite(f->GetParameter(k)))
      throw std::runtime_error("Nonfinite initial XT coefficient");
    std::cout << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
              << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    if(i==0) outfile << "#SDC2" << std::endl;
    if(i==kNumOfLayersSDC2) outfile << "#SDC3" << std::endl;
    outfile << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
	    << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
  }
  sdc::writeParameters(outfilename,outfile.str());

  TCanvas *c0 = new TCanvas("titleCanvas", "Title Page", 800, 600);
  c0->cd();
  c0->Print(PDFName + "[", "pdf");
  c1->Print(PDFName, "pdf");
  c0->Print(PDFName + "]", "pdf");
  
  file2->Close();
  delete file2;
}
