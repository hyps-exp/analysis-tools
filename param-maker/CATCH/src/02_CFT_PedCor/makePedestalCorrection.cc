// Author : Miyu Yoshida
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

// CFT
const int NumOfPlaneCFT   =   8;
enum CFT_PLANE{CFT_U1, CFT_PHI1, CFT_V2, CFT_PHI2, CFT_U3, CFT_PHI3, CFT_V4, CFT_PHI4};
enum CFT_PLANE_{CFT_UV1, CFT_PHI1_, CFT_UV2, CFT_PHI2_, CFT_UV3, CFT_PHI3_, CFT_UV4, CFT_PHI4_};
const int NumOfSegCFT_UV1   = 426;
const int NumOfSegCFT_PHI1  = 584;
const int NumOfSegCFT_UV2   = 472;
const int NumOfSegCFT_PHI2  = 692;
const int NumOfSegCFT_UV3   = 510;
const int NumOfSegCFT_PHI3  = 800;
const int NumOfSegCFT_UV4   = 538;
const int NumOfSegCFT_PHI4  = 910;
const int NumOfSegCFT[NumOfPlaneCFT]  = {426,584,472,692,510,800,538,910};

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usge : " << argv[0] << "[runnumber] [layer# (0-7)]" << std::endl;
    return 0;
  }

  int runnumber = atoi(argv[1]);
  int LayerId   = atoi(argv[2]);

  TString filename = Form("../../root/run%05d_CFT.root", runnumber);
  TChain *chain = new TChain("tree");
  chain->Add(filename);

  filename.Form("src/02_CFT_PedCor/pedestal_channel.txt");
  std::ifstream ifs(filename);

  if (!ifs.is_open()) {
    std::cerr << "#E " << " file open fail : " << filename << std::endl;
    return 0;

  }

  filename = Form("../../output/run%05d/makePedestalCorrection_%05d_%d.txt", runnumber, runnumber, LayerId);
  std::ofstream outputfile(filename);

  std::vector <std::pair<int, int>> pairCont;

  std::string line;
  while( ifs.good() && std::getline( ifs, line ) ){
    std::istringstream input_line( line );
    int layer = -1, channel = -1;
    if (input_line >> layer >> channel) {
      std::pair<int, int> p = std::make_pair(layer, channel);
      pairCont.push_back(p);
    }
  }

  struct Struct {
    double sigma;
    int layer;
    int segment;
    double p0;
    double p1;

    bool operator<(const Struct& another) const {
      return sigma < another.sigma;
    }
  };

  char buf1[100], buf2[100];
  for (int j=0; j<NumOfSegCFT[LayerId]; j++) {
    std::cout << "layer " << LayerId << ", segment " << j << std::endl;
    std::vector <Struct> vec_result;

    for (int k=0; k<pairCont.size(); k++) {
      TH2F *htmp = new TH2F("htmp","", 500, 500, 1000, 500, 500, 1000);
      sprintf(buf1,"cftadc_h[%d][%d]:cftadc_h[%d][%d]", LayerId, j,
	      pairCont[k].first, pairCont[k].second);


      chain->Project("htmp", buf1, "", "", 100 );

      htmp->Fit("pol1","Q");

      //gPad->Update();
      double p0 = htmp->GetFunction("pol1")->GetParameter(0);
      double p1 = htmp->GetFunction("pol1")->GetParameter(1);

      TH1F *htmp2 = new TH1F("htmp2","", 500, -500, 1000);

      sprintf(buf1,"cftadc_h[%d][%d]-(%lf + %lf * cftadc_h[%d][%d])", LayerId, j, p0, p1,
	      pairCont[k].first, pairCont[k].second);

      chain->Project("htmp2", buf1, "", "", 100 );

      double mean = htmp2->GetMean();
      double rms =  htmp2->GetRMS();

      htmp2->Fit("gaus","Q","", mean-3*rms, mean+3*rms);

      //gPad->Update();
      //getchar();

      mean = htmp2->GetFunction("gaus")->GetParameter(1);
      double sigma = htmp2->GetFunction("gaus")->GetParameter(2);

      Struct res;
      res.sigma = sigma;
      res.layer = pairCont[k].first;
      res.segment = pairCont[k].second;
      res.p0 = p0 + mean;
      res.p1 = p1;

      vec_result.push_back(res);

      //std::cout << "layer " << pairCont[k].first
      //<< ", segment " << pairCont[k].second
      //<< ", sigma " << sigma << std::endl;
      delete htmp;
      delete htmp2;
    }

    sort(vec_result.begin(), vec_result.end());

    for (int ii=0; ii<6; ii++) {

      std::cout << "sigma = " << vec_result[ii].sigma
		<< ", layer = " << vec_result[ii].layer
		<< ", segment = " << vec_result[ii].segment
		<< std::endl;

      outputfile << LayerId << " " << j
		 << " " << vec_result[ii].sigma
		 << " " << vec_result[ii].layer
		 << " " << vec_result[ii].segment
		 << " " << vec_result[ii].p0
		 << " " << vec_result[ii].p1
		 << std::endl;
    }

  }

  outputfile.close();
  return 0;
}
