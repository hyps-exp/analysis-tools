const Int_t NumOfSegTagPL = 10;
const Int_t TagPLHid = 30000;

void check_tag_pl(Int_t runnumber)
{

  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;
  std::cout << "" << std::endl;
  std::cout << "" << std::endl;

  TFile *file = new TFile(Form("/group/had/sks/Users/miyu/hyps-analyzer/root_param/run%05d_Hodoscope.root", runnumber));

  TString filename;
  filename.Form("/home/had/miyu/work/HYPS/yoshida/hyps-param/run/%05d/tag_pl_hdprm_hist_%05d.txt", runnumber, runnumber);

  std::cout << "txtfile : " << filename << std::endl;

  std::ofstream outputfile(filename);

  filename.Form("/home/had/miyu/work/HYPS/yoshida/hyps-param/run/%05d/tag_pl_hdprm_hist_%05d.pdf", runnumber, runnumber);
  std::cout << "pdffile : " << filename << std::endl;
  std::cout << "" << std::endl;

  TCanvas *c1 = new TCanvas("c1", "c1");
  c1->Divide(4, 3);
  c1->Print(filename + "[", "pdf");

  // h1 : TAGPL
  TH1D *h1[NumOfSegTagPL];
  TF1 *f1[NumOfSegTagPL];
  TString title, hist, cond;
  Double_t max_bin, max;
  Double_t mean[NumOfSegTagPL];
  Int_t num;

  std::cout << "" << std::endl;
  std::cout << "#D START : TAG SFF" << std::endl;
  for(Int_t i=0; i<NumOfSegTagPL; i++){ // TAGPL
    num = i%12 + 1;
    c1->cd(num);

    std::cout << "" << std::endl;
    std::cout << "TAG PL : " << i << std::endl;
    std::cout << "" << std::endl;

    // hist 1st time
    hist.Form("h%d", TagPLHid + 100*(i+1) + 3);
    h1[i] = (TH1D*)gROOT->FindObject(hist);

    max_bin = h1[i]->GetMaximumBin();
    max     = h1[i]->GetBinCenter(max_bin);

    // hist 2nd time
    h1[i]->GetXaxis()->SetRangeUser(max-100, max+100);

    cond.Form("tagplt[%d]", i);
    max_bin = h1[i]->GetMaximumBin();
    max     = h1[i]->GetBinCenter(max_bin);

    // Fit
    h1[i]->Draw();
    h1[i]->Fit("gaus", "", "", max-10, max+10);
    f1[i]    = h1[i]->GetFunction("gaus");
    mean[i]  = f1[i]->GetParameter(1);

    if(num == 12 || i == NumOfSegTagPL-1)
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

  outputfile << "# Tag-PL Tdc" << std::endl;
  for(Int_t i=0; i<NumOfSegTagPL; i++){
    outputfile << 3 << "\t" << 0 << "\t" << i
	       << "\t" << 1 << "\t" << 0 << "\t"
	       << mean[i] << "\t" << p1 << std::endl;
  }

}
