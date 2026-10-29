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
  if (argc != 2) {
    std::cout << "Usge : " << argv[0] << " <runnumber>" << std::endl;
    return 0;
  }

  int runnumber = atoi(argv[1]);

  TString filename = Form("../../root/run%05d_CFT.root", runnumber);

  TChain *chain = new TChain("tree");
  chain->Add(filename);

  filename.Form("src/02_CFT_PedCor/pedestal_channel_%05d.txt", runnumber);
  std::ofstream outputfile(filename);

  std::cout << "#D start" << std::endl;

  char buf1[100], buf2[100];
  for (int i=0; i<NumOfPlaneCFT; i++) {
    for (int j=0; j<NumOfSegCFT[i]; j++) {
      std::cout << "#D check channel" << std::endl;
      std::cout << i << " " << j << std::endl;

      TH1F *htmp = new TH1F("htmp","", 4000, 0, 4000);
      sprintf(buf1,"cftadc_h[%d][%d]", i, j);
      sprintf(buf2,"cftadc_h[%d][%d]>=100 && cftadc_h[%d][%d]<700", i, j, i, j);

      double n = chain->Project("htmp", buf1, buf2);
      if (n>0) {
	std::cout << i << " " << j << std::endl;
	outputfile << i << " " << j << std::endl;
      }
      delete htmp;
    }

  }
  outputfile.close();

}
