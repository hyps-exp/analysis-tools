#include "../sdc_common.hh"
/**
 * @file    read_write_T0.C
 * @brief   Update t0 calibration
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */

// root -l -b -q read_write_T0.C
#include <TString.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

void read_write_T0(  TString InFileName = {"DCTdcCalib_hyps_run03207_test"}, TString OutFileName = {"DCTdcCalib_hyps_run3962"}, int run=1000){

  TString DiffFileName = {"../../../config/SDC/diff_t0.txt"};
  double diff[sdc::planes.size()];
  {
    std::ifstream diffFile(DiffFileName.Data());
    if(!diffFile){ throw std::runtime_error("Cannot open diff_t0 input"); }
    int group=-1, count[4]={0,0,0,0};
    const int size[4]={kNumOfLayersSDC0,kNumOfLayersSDC1,kNumOfLayersSDC2,kNumOfLayersSDC3};
    const int start[4]={0,kNumOfLayersSDC0,kNumOfLayersSDCIn,kNumOfLayersSDCIn+kNumOfLayersSDC2};
    bool seen[4]={false,false,false,false};
    std::string line;
    while(std::getline(diffFile,line)){
      auto first=line.find_first_not_of(" \t\r");
      if(first==std::string::npos) continue;
      line.erase(0,first);
      if(line[0]=='#'){
        if(line.compare(0,4,"#SDC")==0){
          std::istringstream heading(line.substr(4));
          std::string extra;
          if(!(heading>>group) || (heading>>extra) || group<0 || group>3 || seen[group]){
            throw std::runtime_error("Invalid/duplicate SDC heading");
          }
          seen[group]=true;
        }
        continue;
      }
      line=line.substr(0,line.find('#'));
      std::replace(line.begin(),line.end(),',',' ');
      std::istringstream values(line);
      std::string token;
      while(values>>token){
        size_t used=0; double value;
        try{ value=std::stod(token,&used); }
        catch(...){ throw std::runtime_error("Invalid diff value"); }
        if(group<0 || count[group]>=size[group] || used!=token.size() || !std::isfinite(value)){
          throw std::runtime_error("Invalid value or too many diff values");
        }
        diff[start[group]+count[group]++]=value;
      }
    }
    for(int i=0;i<4;++i) if(!seen[i] || count[i]!=size[i]){
      throw std::runtime_error("Missing diff values for SDC"+std::to_string(i));
    }
  }
 
  TString infilename = Form("../../../root/run%05d/%s", run, InFileName.Data());
  std::ifstream infile(infilename);
  TString outfilename = Form("../../../output/run%05d/%s", run, OutFileName.Data());
  if(!infile) throw std::runtime_error("Cannot open input DCTDC");
  if(infilename==outfilename) throw std::runtime_error("Input and output must differ");
  sdc::outputDirectory(run);
  std::ostringstream outfile;
  outfile << std::setprecision(17);
  
  std::string line;

  while(std::getline(infile, line)){
    int Id;
    int WireId;
    double p1;
    double p0;

    auto body=line.substr(0,line.find('#'));
    if(body.find_first_not_of(" \t\r")==std::string::npos){
      outfile << line << std::endl;
      continue;
    }
    std::istringstream iss(body);
    std::string extra;
    if(!(iss >> Id >> WireId >> p1 >> p0) || (iss>>extra) || !std::isfinite(p1+p0))
      throw std::runtime_error("Invalid DCTDC row");
    const int index=sdc::index(Id);
    if(index<0) { outfile<<line<<std::endl; continue; }
    if(WireId<0 || WireId>=sdc::planes[index].n_channels)
      throw std::runtime_error("Invalid SDC wire");
    p0 -= diff[index]; // diff_t0.txt is in raw TDC channels.
    p1 = module_lsb::kHulLrTdc;
    outfile << Id << " " << WireId << " " << std::setprecision(5) << p1 << " " << std::setprecision(17) << p0 << std::endl;
  }
  if(infile.bad()) throw std::runtime_error("DCTDC input read failed");
  infile.close();
  sdc::writeParameters(outfilename,outfile.str());
}
