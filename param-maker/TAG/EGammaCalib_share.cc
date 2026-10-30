void EGammaCalib_share(int runnumber){
  string filename = Form("../root/run%05d/run%05d_GammaPKplusLambda.root", runnumner, runnumber);
  TChain *hyps = new TChain("hyps");
  hyps->Add(filename.c_str());

  const int numofslice=30;
  double sfcenter[numofslice];
  double sferr[numofslice];
  double ecenter[numofslice];
  double eerr[numofslice];
  double param[2][3];

  TH2D* hist;
  TH1D* histproj;
  string temphistname;
  string tempdraw;
  string cutcondition;

  TCanvas *c1=new TCanvas("c1","c1",1200,600);
  c1->Divide(2,1);
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(0);

  c1->cd(1);
  tempdraw="EGammacalc_npi:tagsffclhitpat>>hsff(30,0,30,300,1.5,3.0)";
  cutcondition="";
  temphistname="hsff";

  hyps->Draw(tempdraw.c_str(),cutcondition.c_str());
  hist=(TH2D*) gROOT->FindObject(temphistname.c_str());


  for(int j=0;j<numofslice;j++){
    histproj=hist->ProjectionY("",j+1,j+1);
    sfcenter[j]=0.5+j;
    sferr[j]=0.5;
    double max=histproj->GetBinCenter(histproj->GetMaximumBin());
    histproj->Fit("gaus","Q","",max-0.1,max+0.1);
    ecenter[j]=histproj->GetFunction("gaus")->GetParameter(1);
    eerr[j]=histproj->GetFunction("gaus")->GetParameter(2);
  }
  hist->Draw();
  TGraphErrors *graph=new TGraphErrors(numofslice,sfcenter,ecenter,sferr,eerr);
  graph->SetMarkerColor(kRed);
  graph->Draw("P");

  TF1 *pol2 = new TF1("pol2", "pol2");

  pol2->SetParameter(0,1.50);
  pol2->SetParLimits(0,1.40,1.70);
  pol2->SetParLimits(1,-0.05,0.05);
  pol2->SetParLimits(2,-0.0005,0.0005);
  graph->Fit("pol2","","",0,28);
  pol2->GetParameters(param[0]);

  c1->cd(2);

  tempdraw="EGammacalc_npi:tagsfbclhitpat>>hsfb(30,0,30,300,1.5,3.0)";
  cutcondition="";
  temphistname="hsfb";

  hyps->Draw(tempdraw.c_str(),cutcondition.c_str());
  hist=(TH2D*) gROOT->FindObject(temphistname.c_str());


  for(int j=0;j<numofslice;j++){
    histproj=hist->ProjectionY("",j+1,j+1);
    sfcenter[j]=0.5+j;
    sferr[j]=0.5;
    double max=histproj->GetBinCenter(histproj->GetMaximumBin());
    histproj->Fit("gaus","Q","",max-0.1,max+0.1);
    ecenter[j]=histproj->GetFunction("gaus")->GetParameter(1);
    eerr[j]=histproj->GetFunction("gaus")->GetParameter(2);
  }
  hist->Draw();
  TGraphErrors *graph2=new TGraphErrors(numofslice,sfcenter,ecenter,sferr,eerr);
  graph2->SetMarkerColor(kRed);
  graph2->Draw("P");

  pol2->SetParameter(0,1.50);
  pol2->SetParLimits(0,1.40,1.70);
  pol2->SetParLimits(1,-0.05,0.05);
  pol2->SetParLimits(2,-0.0005,0.0005);
  graph2->Fit("pol2","","",0,28);
  pol2->GetParameters(param[1]);

  std::cout<<"parf: "<<param[0][0]<<"\t"<<param[0][1]<<"\t"<<param[0][2]<<std::endl;
  std::cout<<"parb: "<<param[1][0]<<"\t"<<param[1][1]<<"\t"<<param[1][2]<<std::endl;


}
