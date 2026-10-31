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
  
  // TString FileName = {"07162_In"};//write xx of file name "runxx.root"
  TString OutFileName = Form("DCDriftParam_hyps_test_in_update_%05d", run);
  TString OutPDFName = Form("Update_DCDriftParam_in_%05d", run);
  TString PDFName = Form("../../../output/run%05d/%s.pdf", run, OutPDFName.Data());
  
  TFile *file;
  const int NumOfLayers = 10;

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
  
  TString filename = Form("../../../root/run%05%/run%s.root", run, FileName.Data());
  file = new TFile(filename);
  TString outfilename = Form("../../../output/run%05d/%s", run, OutFileName.Data());
  std::ofstream outfile(outfilename);

  for(int i=0; i<NumOfLayers; i++){
  // for(int i=8; i<9; i++){
    int Id = i + 1;
    int WireId = 0;
    int Type = 6;
    int NParam = 6;
    
    TString hist = Form("h%d", (i+1)*100+22);
    TString c_name = Form("canvas_h%d", (i+1)*100+22);
    c1[i] = new TCanvas(c_name, c_name, 1000, 750);
    TH2F *h1 = (TH2F*) gROOT->FindObject(hist);
    c1[i]->cd();
    h1->Draw("col");
    c1[i]->Update();
    bool Flag = false;
    //The if bellow is used to determine the array size of h2. The size is equal to "NumOfArray".
    if(false){
      int nBin = h1->GetNbinsX();
      TH1D *h2_0;
      Flag = true;
      for(int nbin=1; nbin<nBin+1; nbin++){
	h2_0 = (TH1D*) h1->ProjectionY("h2_0", nbin, nbin);
	int ebin = h2_0->GetEntries();
	std::cout << nbin << "=" << ebin << endl;
      }
    }
    if(Flag) continue;
    int NumOfArray = NumOfArrays[i];
    double mean[NumOfArray], sigma[NumOfArray];
    TH1D *h2[NumOfArray];
    TString c2_name = Form("canvas2_h01_%d", i);
    c2[i] = new TCanvas(c2_name, c2_name, 1300, 1000);
    if(i<4) c2[i]->Divide(10,15);
    if(4<=i) c2[i]->Divide(15,18);
    g = new TGraphErrors();
    int count = 0;
    for(int j=0; j<NumOfArray; j++){
      double bin = j + NumOfEmpArrays[i] + 1;
      TString h2_name = Form("h2[%d]", bin);
      h2[j] = (TH1D*) h1->ProjectionY(h2_name, bin, bin);
      int nmax = h2[j]->GetMaximumBin();
      c2[i]->cd(j+1);
      h2[j]->Draw();
      double xmax = h2[j]->GetBinCenter(nmax);
      if(j<NumOfStart[i]) h2[j]->Fit("gaus", "", "", xmax-0.5, xmax+0.5);
      if(NumOfStart[i]<=j) h2[j]->Fit("gaus");
      TF1 *fitfunc = h2[j]->GetFunction("gaus");
      if(fitfunc){
	double dx = h1->GetXaxis()->GetBinCenter(bin);
	mean[j] = h2[j]->GetFunction("gaus")->GetParameter(1);
	if(mean[j] < minvalue) continue;
	sigma[j] = h2[j]->GetFunction("gaus")->GetParameter(2);
	g->SetPoint(j, dx, mean[j]);
	g->SetPointError(j, 0, sigma[j]);
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
    if(i<4) fn = new TF1(fn_name, "pol5", -5, 70);
    if(4<=i) fn = new TF1(fn_name, "pol5", -5, fnmax[i]);
    fn->SetParameter(0, 0);
    fn->SetParameter(1, 4.0e-2);
    fn->SetParameter(2, 8.0e-4);
    fn->SetParameter(3, -9.0e-6);
    fn->SetParameter(4, -1.41421e-7);
    fn->SetParameter(5, 1.55782e-9);

    g->Fit(fn, "R");
    fn->Draw("same");
    c3[i]->Update();

    p1 = fn->GetParameter(1);
    p2 = fn->GetParameter(2);
    p3 = fn->GetParameter(3);
    p4 = fn->GetParameter(4);
    p5 = fn->GetParameter(5);

    std::cout << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
            << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    if(i==0) outfile << "#SDC0" << std::endl;
    if(i==4) outfile << "#SDC1" << std::endl;
    outfile << Id << " " << WireId << " " << Type << " " << NParam << " " << 0.00 << " "
            << p1 << " " << p2 << " " << p3 << " " << p4 << " " << p5 << std::endl;
    }
  outfile.close();

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
