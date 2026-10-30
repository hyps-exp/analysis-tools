const Int_t NumOfSegTagSF = 55;
const Int_t TagSFHid = 20000;

void check_tag_sf(Int_t runnumber)
{

  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;

  TFile *file = new TFile(Form("/group/had/sks/Users/miyu/hyps-analyzer/root_param/run%05d_Hodoscope.root", runnumber));

  TString filename;
  filename.Form("/home/had/miyu/work/HYPS/yoshida/hyps-param/run/%05d/tag_sf_hdprm_hist_%05d.txt", runnumber, runnumber);

  std::cout << "txtfile : " << filename << std::endl;

  std::ofstream outputfile(filename);

  filename.Form("/home/had/miyu/work/HYPS/yoshida/hyps-param/run/%05d/tag_sf_hdprm_hist_%05d.pdf", runnumber, runnumber);
  std::cout << "pdffile : " << filename << std::endl;
  std::cout << "" << std::endl;

  TCanvas *c1 = new TCanvas("c1", "c1");
  c1->Divide(4, 3);
  c1->Print(filename + "[", "pdf");

  // h1 : TAGSFF, h2 : TAGSFB
  TH1D *h1[NumOfSegTagSF], *h2[NumOfSegTagSF];
  TF1 *f1[NumOfSegTagSF], *f2[NumOfSegTagSF];
  TString title, hist, cond;
  Double_t max_bin, max;
  Double_t mean[NumOfSegTagSF][2];
  Int_t num;

  std::cout << "" << std::endl;
  std::cout << "#D START : TAG SFF" << std::endl;
  for(Int_t i=0; i<NumOfSegTagSF; i++){ // TAGSFF
    num = i%12 + 1;
    c1->cd(num);

    std::cout << "" << std::endl;
    std::cout << "TAG SFF : " << i << std::endl;
    std::cout << "" << std::endl;

    // hist 1st time
    hist.Form("h%d", TagSFHid + 100*(i+1) + 3);
    h1[i] = (TH1D*)gROOT->FindObject(hist);

    max_bin = h1[i]->GetMaximumBin();
    max     = h1[i]->GetBinCenter(max_bin);

    // hist 2nd time
    h1[i]->GetXaxis()->SetRangeUser(max-100, max+100);

    max_bin = h1[i]->GetMaximumBin();
    max     = h1[i]->GetBinCenter(max_bin);

    // Fit
    h1[i]->Draw();
    h1[i]->Fit("gaus", "", "", max-10, max+10);
    f1[i]    = h1[i]->GetFunction("gaus");
    mean[i][0] = f1[i]->GetParameter(1);

    if(num == 12 || i == NumOfSegTagSF-1)
      c1->Print(filename, "pdf");
  }

  for(Int_t i=0; i<NumOfSegTagSF; i++){ // TAGSFB
    num = i%12 + 1;
    c1->cd(num);

    std::cout << "" << std::endl;
    std::cout << "TAG SFB : " << i << std::endl;
    std::cout << "" << std::endl;

    // hist 1st time
    hist.Form("h%d", TagSFHid + 100*(i+1) + 4);
    h2[i] = (TH1D*)gROOT->FindObject(hist);

    max_bin = h2[i]->GetMaximumBin();
    max     = h2[i]->GetBinCenter(max_bin);

    // hist 2nd time
    h2[i]->GetXaxis()->SetRangeUser(max-100, max+100);

    max_bin = h2[i]->GetMaximumBin();
    max     = h2[i]->GetBinCenter(max_bin);

    // Fit
    h2[i]->Draw();
    h2[i]->Fit("gaus", "", "", max-20, max+20);
    f2[i]    = h2[i]->GetFunction("gaus");
    mean[i][1] = f2[i]->GetParameter(1);

    if(num == 12 || i == NumOfSegTagSF-1)
      c1->Print(filename, "pdf");
  }

  c1->Print(filename + "]", "pdf");

  std::cout << "" << std::endl;
  std::cout << "#D Write HDPRM" << std::endl;
  std::cout << "" << std::endl;

  Double_t p1 = -0.8333;
  outputfile << "# HDPRM" << std::endl;
  outputfile << "# Cld" << "\t" << "PlId" << "\t" << "SegId"
	     << "\t" << "AorT" << "\t" << "UorD" << "\t"
	     << "p0" << "\t" << "p1" << std::endl;

  outputfile << "# Tag-SF Tdc" << std::endl;
  for(Int_t i=0; i<2; i++){
    for(Int_t j=0; j<NumOfSegTagSF; j++){
      outputfile << 2 << "\t" << i << "\t" << j
		 << "\t" << 1 << "\t" << 0 << "\t"
		 << mean[j][i] << "\t" << p1 << std::endl;
    }
  }

}
