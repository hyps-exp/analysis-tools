#include "../sdc_common.hh"
/**
 * @file    update_XT_SdcIn.C
 * @brief   Update XT calibration
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */

void update_XT_SdcIn(TString FileName = {"07162_In"}, int run=7162){
  
  FileName = sdc::stem(FileName);
  sdc::outputDirectory(run);
  // TString FileName = {"07162_In"};//write xx of file name "runxx.root"
  TString OutFileName = Form("DCDriftParam_hyps_test_in_update_%05d", run);
  TString OutPDFName = Form("Update_DCDriftParam_in_%05d", run);
  TString PDFName = Form("../../../output/run%05d/%s.pdf", run, OutPDFName.Data());
  
  TFile *file;
  const int NumOfLayers = kNumOfLayersSDCIn;

  int NumOfStart[NumOfLayers] = {80, 75, 80, 80, 185, 185, 200, 199, 185, 185}; //write the value of j which should change the range of fit to all range. 0 origin.
  int NumOfArrays[NumOfLayers] = {150, 150, 150, 150, 270, 270, 270, 270, 270, 270};
  int NumOfEmpArrays[NumOfLayers] = {30, 30, 30, 30, 54, 54, 54, 54, 54, 54};
  int fnmax[NumOfLayers] = {120, 120, 120, 120, 220, 220, 220, 220, 220, 220};
  // int fnmax[NumOfLayers] = {120, 120, 120, 120, 140, 140, 140, 140, 140, 140}; //tmp
  TCanvas *c0;
  TCanvas *c1[NumOfLayers];
  TCanvas *c2[NumOfLayers];
  TCanvas *c3[NumOfLayers];
  // c3[NumOfLayersDC0]->SetGrid();
  TGraphErrors *g;
  TF1 *fn;
  double minvalue = -1.5;

  double p1, p2, p3, p4, p5;
  
  TString filename = Form("../../../root/run%05d/run%s.root", run, FileName.Data());
  file = sdc::open(filename);
  TString outfilename = Form("../../../output/run%05d/%s", run, OutFileName.Data());
  std::ostringstream outfile;
  outfile << std::setprecision(17);

  for(int i=0; i<NumOfLayers; i++){
  // for(int i=8; i<9; i++){
    int Id = sdc::in[i].id;
    int WireId = 0;
    int Type = 6;
    int NParam = 6;
    
    TString hist = Form("h%d", (i+1)*100+22);
    TString c_name = Form("canvas_h%d", (i+1)*100+22);
    c1[i] = new TCanvas(c_name, c_name, 1000, 750);
    TH2 *h1 = sdc::get<TH2>(file, hist);
    c1[i]->cd();
    h1->Draw("col");
    c1[i]->Update();
    int NumOfArray = NumOfArrays[i];
    if(NumOfEmpArrays[i]+NumOfArray > h1->GetNbinsX())
      throw std::runtime_error("XT projection bins exceed histogram range");
    double mean[NumOfArray], sigma[NumOfArray];
    TH1D *h2[NumOfArray];
    TString c2_name = Form("canvas2_h01_%d", i);
    c2[i] = new TCanvas(c2_name, c2_name, 1300, 1000);
    if(i<kNumOfLayersSDC0) c2[i]->Divide(10,15);
    if(kNumOfLayersSDC0<=i) c2[i]->Divide(15,18);
    g = new TGraphErrors();
    int count = 0;
    for(int j=0; j<NumOfArray; j++){
      int bin = j + NumOfEmpArrays[i] + 1;
      TString h2_name = Form("h2_layer%d_bin%d", Id, bin);
      h2[j] = (TH1D*) h1->ProjectionY(h2_name, bin, bin);
      int nmax = h2[j]->GetMaximumBin();
      c2[i]->cd(j+1);
      h2[j]->Draw();
      double xmax = h2[j]->GetBinCenter(nmax);
      int fitStatus;
      if(j<NumOfStart[i]) fitStatus=h2[j]->Fit("gaus", "Q", "", xmax-0.5, xmax+0.5);
      else fitStatus=h2[j]->Fit("gaus", "Q");
      TF1 *fitfunc = h2[j]->GetFunction("gaus");
      if(fitStatus==0 && fitfunc){
	double dx = h1->GetXaxis()->GetBinCenter(bin);
	mean[j] = h2[j]->GetFunction("gaus")->GetParameter(1);
	if(!std::isfinite(mean[j]) || mean[j] < minvalue) continue;
	sigma[j] = h2[j]->GetFunction("gaus")->GetParameter(2);
	if(!std::isfinite(sigma[j]) || sigma[j]<=0) continue;
	g->SetPoint(count, dx, mean[j]);
	g->SetPointError(count, 0, sigma[j]);
	count ++;
      }
      h2[j]->Draw("same");
    }
    c2[i]->Update();

    TString c3_name = Form("canvas_%d",i);
    c3[i] = new TCanvas(c3_name, c3_name, 1000, 750);
    c3[i]->cd();
    g->Draw("AP");

    TString fn_name = Form("fn_%d",i);
    if(i<kNumOfLayersSDC0) fn = new TF1(fn_name, "pol5", -5, 70);
    if(kNumOfLayersSDC0<=i) fn = new TF1(fn_name, "pol5", -5, fnmax[i]);
    fn->FixParameter(0, 0);
    fn->SetParameter(1, 4.0e-2);
    fn->SetParameter(2, 8.0e-4);
    fn->SetParameter(3, -9.0e-6);
    fn->SetParameter(4, -1.41421e-7);
    fn->SetParameter(5, 1.55782e-9);

    if(g->GetN()<5) throw std::runtime_error("Too few valid XT points");
    if(int(g->Fit(fn, "R"))!=0) throw std::runtime_error("XT fit failed");
    fn->Draw("same");
    c3[i]->Update();

    p1 = fn->GetParameter(1);
    p2 = fn->GetParameter(2);
    p3 = fn->GetParameter(3);
    p4 = fn->GetParameter(4);
    p5 = fn->GetParameter(5);

    for(int k=1;k<=5;++k) if(!std::isfinite(fn->GetParameter(k)))
      throw std::runtime_error("Nonfinite XT coefficient");
    std::cout << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
            << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    if(i==0) outfile << "#SDC0" << std::endl;
    if(i==kNumOfLayersSDC0) outfile << "#SDC1" << std::endl;
    outfile << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
            << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    }
  sdc::writeParameters(outfilename,outfile.str());

  c0 = new TCanvas("titleCanvas", "Title Page", 800, 600);
  c0->cd();
  c0->Print(PDFName + "[", "pdf");
  for(int i=0; i<NumOfLayers; i++){
    c1[i]->Print(PDFName, "pdf");
    c2[i]->Print(PDFName, "pdf");
    c3[i]->Print(PDFName, "pdf");
  }
  c0->Print(PDFName + "]", "pdf");
  
}
