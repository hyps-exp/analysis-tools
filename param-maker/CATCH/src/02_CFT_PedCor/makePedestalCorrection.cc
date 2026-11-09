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
  int LayerId   = atoi(argv[2]);

  TString filename = Form("%s/run%05d/run%05d_CFT.root", root_path.Data(), runnumber, runnumber);
  TChain *chain = new TChain("tree");
  chain->Add(filename);

  filename.Form("src/02_CFT_PedCor/pedestal_channel.txt");
  std::ifstream ifs(filename);

  if (!ifs.is_open()) {
    std::cerr << "#E " << " file open fail : " << filename << std::endl;
    return 0;

  }

  filename = Form("%s/run%05d/catch_makePedestalCorrection_%05d_%d.txt", txt_path.Data(), runnumber, runnumber, LayerId);
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
