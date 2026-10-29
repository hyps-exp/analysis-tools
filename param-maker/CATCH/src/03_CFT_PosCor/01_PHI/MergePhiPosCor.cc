#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include <iomanip>

#include "PathID.hh"

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "Usage : " << argv[0] << " <runnumber> <count>"
	      << std::endl;
    return 0;
  }

  int runnumber = std::atoi(argv[1]);
  int count     = std::atoi(argv[2]);

  const int NumOfPhiLayer = 4;
  const int NumOfAngle = 36;
  const int NumOfPhiPar = 6;
  double PhiPosPar[NumOfPhiLayer][NumOfAngle][NumOfPhiPar];

  std::ostringstream oss;

  oss << txt_path
      << "/run"
      << std::setw(5) << std::setfill('0') << runnumber
      << "/CATCH_calPhiResidual_16layer_phi_pos_corr_phicor_"
      << std::setw(5) << std::setfill('0') << runnumber
      << ".txt";

  if(count == 1){
    oss.str("");
    oss.clear();
    oss << txt_path
	<< "/run"
	<< std::setw(5) << std::setfill('0') << runnumber
	<< "/CATCH_calPhiResidual_16layer_phi_pos_corr_phiuvphicor_"
	<< std::setw(5) << std::setfill('0') << runnumber
	<< ".txt";
  }
  std::string filename = oss.str();
  oss.str("");
  oss.clear();

  std::ifstream ifile_offset(filename);
  if(!ifile_offset) {
    std::cout<<"Offset File: "<< filename <<" is not exist"<<std::endl;
    return 0;
  }

  int type_id;
  int plid, angle, seg, theta_id;
  double par[6];


  std::string line;
  while( ifile_offset.good() && std::getline(ifile_offset, line) )
    {
      if(line[0] =='#' || line.empty())
	continue;

      std::istringstream input_line( line );
      input_line >> type_id ;

      if(type_id==0){
	if( input_line >> plid >> angle
	    >> par[0] >> par[1] >> par[2] >> par[3] >> par[4] >> par[5]) {
	  if (type_id==0) {
	    for (int k=0; k<NumOfPhiPar; k++) {
	      PhiPosPar[plid][angle][k] = par[k];
	      //printf("%e ", par[k]);
	    }
	    //printf("\n");
	  }
	}
      }
    }

  oss << "../../param/CFTPOS/CftPosParam_nocor";

  if(count == 1){
    oss.str("");
    oss.clear();
    oss << "../../param/CFTPOS/CftPosParam_hyps_common"
	<< std::setw(5) << std::setfill('0') << runnumber
	<< "_phiuvcor";
  }
  filename = oss.str();
  oss.str("");
  oss.clear();

  std::ifstream ifile_param(filename);
  if(!ifile_param) {
    std::cout<<"CFTPosParam File: "<< filename <<" is not exist"<<std::endl;
    return 0;
  }

  oss << "../../param/CFTPOS/CftPosParam_hyps_common_"
      << std::setw(5) << std::setfill('0') << runnumber
      << "_phicor";

  if(count == 1){
    oss.str("");
    oss.clear();
    oss << "../../param/CFTPOS/CftPosParam_hyps_common_"
	<< std::setw(5) << std::setfill('0') << runnumber
	<< "_phiuvphicor";
  }
  filename = oss.str();
  oss.str("");
  oss.clear();

  std::ofstream ofs(filename);
  while( ifile_param.good() && std::getline(ifile_param, line) )
    {
      if(count == 0){
	if(line == "## phi layer 0 ##"){
	  ofs << line << std::endl;
	  for(int angle=0; angle<36; angle++){
	    ofs << 0 << " " << 0 << " " << angle << " "
		<< PhiPosPar[0][angle][0] << " " << PhiPosPar[0][angle][1] << " "
		<< PhiPosPar[0][angle][2] << " " << PhiPosPar[0][angle][3] << " "
		<< PhiPosPar[0][angle][4] << " " << PhiPosPar[0][angle][5]
		<< std::endl;
	  }
	}else if(line == "## phi layer 1 ##"){
	  ofs << line << std::endl;
	  for(int angle=0; angle<36; angle++){
	    ofs << 0 << " " << 1 << " " << angle << " "
		<< PhiPosPar[1][angle][0] << " " << PhiPosPar[1][angle][1] << " "
		<< PhiPosPar[1][angle][2] << " " << PhiPosPar[1][angle][3] << " "
		<< PhiPosPar[1][angle][4] << " " << PhiPosPar[1][angle][5]
		<< std::endl;
	  }
	}else if(line == "## phi layer 2 ##"){
	  ofs << line << std::endl;
	  for(int angle=0; angle<36; angle++){
	    ofs << 0 << " " << 2 << " " << angle << " "
		<< PhiPosPar[2][angle][0] << " " << PhiPosPar[2][angle][1] << " "
		<< PhiPosPar[2][angle][2] << " " << PhiPosPar[2][angle][3] << " "
		<< PhiPosPar[2][angle][4] << " " << PhiPosPar[2][angle][5]
		<< std::endl;
	  }
	}else if(line == "## phi layer 3 ##"){
	  ofs << line << std::endl;
	  for(int angle=0; angle<36; angle++){
	    ofs << 0 << " " << 3 << " " << angle << " "
		<< PhiPosPar[3][angle][0] << " " << PhiPosPar[3][angle][1] << " "
		<< PhiPosPar[3][angle][2] << " " << PhiPosPar[3][angle][3] << " "
		<< PhiPosPar[3][angle][4] << " " << PhiPosPar[3][angle][5]
		<< std::endl;
	  }
	}else{
	  ofs << line << std::endl;
	}
      }else if(count == 1){
	if(line[0] =='#' || line.empty()) {
	  ofs << line << std::endl;
	  continue;
	}

	std::istringstream input_line( line );
	input_line >> type_id ;

	if(type_id==0){
	  if( input_line >> plid >> angle
	      >> par[0] >> par[1] >> par[2] >> par[3] >> par[4] >> par[5]) {
	    for (int k=0; k<NumOfPhiPar; k++) {
	      par[k] += PhiPosPar[plid][angle][k];
	    }

	    ofs << type_id << " " << plid << " " << angle << " "
		<< par[0] << " " << par[1] << " " << par[2] << " "
		<< par[3] << " " << par[4] << " " << par[5]
		<< std::endl;

	  }
	} else {
	  ofs << line << std::endl;
	}
      }
    }
  ofs.close();
  return 0;
}
