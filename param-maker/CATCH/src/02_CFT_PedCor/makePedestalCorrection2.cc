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

#include "TApplication.h"
#include "TRint.h"

#include "DetectorID.hh"
#include "PathID.hh"


int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usge : " << argv[0] << " <runnumber> <layer(0-7)>" << std::endl;
    return 0;
  }

  int runnumber = atoi(argv[1]);
  int layer     = atoi(argv[2]);

  TString filename = Form("%s/run%05d/run%05d_CFT.root", root_path.Data(), runnumber, runnumber);

  TFile *fin = new TFile(filename);
  fin->cd();

  double thr[NumOfPlaneCFT][NumOfSegCFT_PHI4];
  double Sigma0[NumOfPlaneCFT][NumOfSegCFT_PHI4];
  for (int layer=0; layer<NumOfPlaneCFT; layer++) {
    TH2F *hadc = (TH2F *)fin->Get(Form("h%d", 1000*(layer+1)+100+4));

    for (int segment=1; segment <= NumOfSegCFT[layer]; segment++) {
      std::cout << "layer " << layer << ", segment " << segment << std::endl;
      TH1D *hproj = (TH1D *)hadc->ProjectionY("", segment, segment) ;
      hproj->Draw();
      double mean = hproj->GetMean();
      double rms = hproj->GetRMS();
      hproj->Fit("gaus","","", mean-3*rms, mean+3*rms);

      double sigma = hproj->GetFunction("gaus")->GetParameter(2);
      mean = hproj->GetFunction("gaus")->GetParameter(1);

      Sigma0[layer][segment-1] = sigma;

      thr[layer][segment-1] = mean + 3*sigma;
      if (3*sigma>250.)
	thr[layer][segment-1] = mean + 250.;

      delete hproj;
    }
  }


  TChain *chain = new TChain("tree");
  chain->Add(filename);

  filename = Form("%s/run%05d/run%05d_CATCH_CFT_makePedestalCorrection_%05d_%d.txt", txt_path.Data(), runnumber, runnumber, runnumber, layer);
  std::ifstream ifs(filename);

  if (!ifs.is_open()) {
    std::cerr << "#E " << " file open fail : " << filename << std::endl;
    return -1;
  }

  filename.Form("%s/run%05d/CFTPedParam_hyps_common_run%05d_layer%d", txt_path.Data(), runnumber, runnumber, layer);
  std::ofstream outputfile(filename);

  struct Struct {
    int layer;
    int segment;
    double sigma;
    int cor_layer;
    int cor_segment;
    double thr;
    double p0_HG;
    double p1_HG;
    double p0_LG;
    double p1_LG;

    bool operator<(const Struct& another) const {
      return sigma < another.sigma;
    }
  };

  std::vector <Struct> vec_result[NumOfPlaneCFT][NumOfSegCFT_PHI4];

  char buf1[100], buf2[100];
  std::string line;
  while( ifs.good() && std::getline( ifs, line ) ){
    std::istringstream input_line( line );
    int layer = -1, segment = -1;
    double sigma = 999.;
    int cor_layer = -1, cor_segment = -1;
    double p0, p1;

    int entry = 5000;

    if (input_line >> layer >> segment >> sigma >> cor_layer >> cor_segment >> p0 >> p1) {

      TH2F *htmp = new TH2F("htmp","", 500, 500, 1000, 500, 500, 1000);
      sprintf(buf1,"cftadc_h[%d][%d]:cftadc_h[%d][%d]", layer, segment,
		cor_layer, cor_segment);

      chain->Project("htmp", buf1, "", "", entry);
      htmp->Fit("pol1","Q");

      // gPad->Update();
      // getchar();

      p0 = htmp->GetFunction("pol1")->GetParameter(0);
      p1 = htmp->GetFunction("pol1")->GetParameter(1);

      TH1F *htmp2 = new TH1F("htmp2","", 500, -500, 1000);

      sprintf(buf1,"cftadc_h[%d][%d]-(%lf + %lf * cftadc_h[%d][%d])", layer, segment, p0, p1,
	      cor_layer, cor_segment);

      chain->Project("htmp2", buf1, "", "", entry);

      double mean = htmp2->GetMean();
      double rms =  htmp2->GetRMS();

      htmp2->Fit("gaus","Q","", mean-3*rms, mean+3*rms);

      // gPad->Update();
      // getchar();

      mean = htmp2->GetFunction("gaus")->GetParameter(1);
      double sigma = htmp2->GetFunction("gaus")->GetParameter(2);

      double sigma0 = Sigma0[layer][segment];

      std::cout << "layer " << layer << ", seg " << segment
		<< ", sigma " << sigma << "( " << sigma/sigma0 << " )"
		<< ", cor_layer " << cor_layer
		<< ", cor_segment " << cor_segment << std::endl;

      Struct res;
      res.layer = layer;
      res.segment = segment;
      res.sigma = sigma;
      res.cor_layer = cor_layer;
      res.cor_segment = cor_segment;
      res.thr = thr[cor_layer][cor_segment];
      res.p0_HG = p0 + mean;
      res.p1_HG = p1;

      if (sigma>1 && sigma/sigma0 < 0.8) {
	TH2F *htmp_low = new TH2F("htmp_low","", 500, 500, 1000, 250, 650, 900);
	sprintf(buf1,"cftadc_l[%d][%d]:cftadc_h[%d][%d]", layer, segment,
		cor_layer, cor_segment);

	chain->Project("htmp_low", buf1, "", "", entry);
	htmp_low->Fit("pol1","Q");

	// gPad->Update();
	// getchar();
	double p0_low= htmp_low->GetFunction("pol1")->GetParameter(0);
	double p1_low = htmp_low->GetFunction("pol1")->GetParameter(1);


	TH1F *htmp2_low = new TH1F("htmp2_low","", 200, -100, 100);

	sprintf(buf1,"cftadc_l[%d][%d]-(%lf + %lf * cftadc_h[%d][%d])", layer, segment, p0_low, p1_low,
		cor_layer, cor_segment);

	chain->Project("htmp2_low", buf1, "", "", entry);

	mean = htmp2_low->GetMean();
	rms =  htmp2_low->GetRMS();

	htmp2_low->Fit("gaus","Q","", mean-3*rms, mean+3*rms);

	// gPad->Update();
	// getchar();

	mean = htmp2_low->GetFunction("gaus")->GetParameter(1);

	res.p0_LG = p0_low + mean;
	res.p1_LG = p1_low;


	vec_result[layer][segment].push_back(res);

	delete htmp_low;
	delete htmp2_low;

      }
      delete htmp;
      delete htmp2;

    }
  }


  for (int j=0; j<NumOfSegCFT[layer]; j++) {
    std::cout << "layer " << layer << ", seg " << j << std::endl;
    sort(vec_result[layer][j].begin(), vec_result[layer][j].end());

    for (int ii=0; ii<vec_result[layer][j].size(); ii++) {
      outputfile << layer << " " << j
		 << " " << vec_result[layer][j][ii].sigma
		 << " " << vec_result[layer][j][ii].cor_layer
		 << " " << vec_result[layer][j][ii].cor_segment
		 << " " << vec_result[layer][j][ii].thr
		 << " " << vec_result[layer][j][ii].p0_HG
		 << " " << vec_result[layer][j][ii].p1_HG
		 << " " << vec_result[layer][j][ii].p0_LG
		 << " " << vec_result[layer][j][ii].p1_LG
		 << std::endl;

    }

  }

  outputfile.close();
  return 0;
}
