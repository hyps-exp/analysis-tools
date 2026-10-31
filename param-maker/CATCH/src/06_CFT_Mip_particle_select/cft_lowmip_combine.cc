#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

#include <TROOT.h>
#include <TFile.h>
#include <TLatex.h>

#include "DetectorID.hh"
#include "PathID.hh"

const Int_t CId  = 31;
const Int_t AorT = 0;
const Int_t UorD = 1;
const Double_t p1 = -1;

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return 0;
  }
  int runnumber = std::atoi(argv[1]);

  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << "./cft_lowmip_combine " << runnumber << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;

  TString txtfile;

  // get HDPRM

  // CFT
  Double_t hdprm_cft[NumOfPlaneCFT][NumOfSegCFT_PHI4];
  std::vector<Int_t> ch;
  txtfile.Form("%s/run%05d/catch_cft_lowmip_hand_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ifstream cftfile(txtfile);
  std::string line;
  if(!cftfile.is_open()){
    std::cerr << "#E file open fail : " << txtfile << std::endl;
  }else{
    while(cftfile.good() && std::getline(cftfile, line)){
      if(line[0] != '#'){
	std::istringstream input_line( line );
	Int_t  cid, plid, segid, aort, uord;
	Double_t p0, p1;
	if(input_line >> cid >> plid >> segid >> aort >> uord >> p0 >> p1){
	  hdprm_cft[plid][segid] = p1;
	  ch.push_back(plid*1000 + segid);
	}
      }
    }
  }

  std::sort(ch.begin(), ch.end());

  // prepare write a file
  txtfile.Form("%s/run%05d/catch_cft_lowmip_combine_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream writefile(txtfile);

  txtfile.Form("%s/run%05d/catch_cft_lowmip_load_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ifstream readfile(txtfile);
  if(!readfile.is_open()){
    std::cerr << "#E file open fail : " << txtfile << std::endl;
  }

  writefile << "####################################################################################" << std::endl;
  writefile << "## text file : " << txtfile << std::endl;
  writefile << "####################################################################################" << std::endl;

  while(readfile.good() && std::getline(readfile, line)){
    if(line[0] == '#'){
      writefile << std::fixed << std::setprecision(11) << line << std::endl;
    }else{
      std::istringstream input_line( line );
      Int_t PlId, SegId;
      Double_t p0;
      if(input_line >> PlId >> SegId >> p0){
	Bool_t flag = false;
	for(Int_t i : ch){
	  if(i==PlId*1000 + SegId){
	    if(hdprm_cft[PlId][SegId]>0 && hdprm_cft[PlId][SegId]<1000){
	      writefile << CId << "\t" << PlId << "\t" << SegId << "\t"
			<< AorT << "\t" << UorD << "\t"
			<< 0 << "\t"
			<< std::fixed << std::setprecision(3) << hdprm_cft[PlId][SegId]
			<< std::endl;
	      flag = true;
	    }
	    break;
	  }
	}
	if(!flag)
	  writefile << std::fixed << std::setprecision(3) << line << std::endl;
      }else{
	writefile << std::fixed << std::setprecision(3) << line << std::endl;
      }
    }
  }
}
