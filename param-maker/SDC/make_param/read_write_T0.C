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

  TString DiffFileName = {"../../diff_t0/diff_t0.txt"};
  double diff[20];
  {
    std::ifstream diffFile(DiffFileName.Data());
    if(!diffFile){ std::cerr << "Cannot open " << DiffFileName << std::endl; return; }
    int group=-1, count[4]={0,0,0,0};
    const int size[4]={4,6,5,5}, start[4]={0,4,10,15};
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
            std::cerr << "Invalid/duplicate SDC heading: " << line << std::endl; return;
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
        catch(...){ std::cerr << "Invalid diff: " << token << std::endl; return; }
        if(group<0 || count[group]>=size[group] || used!=token.size() || !std::isfinite(value)){
          std::cerr << "Invalid value or too many values: " << token << std::endl; return;
        }
        diff[start[group]+count[group]++]=value;
      }
    }
    for(int i=0;i<4;++i) if(!seen[i] || count[i]!=size[i]){
      std::cerr << "SDC" << i << " needs " << size[i] << " diff values" << std::endl; return;
    }
  }
 
  TString infilename = Form("../../root/run%05d/%s", run, InFileName.Data());
  std::ifstream infile(infilename);
  TString outfilename = Form("../../output/run%05d/%s", run, OutFileName.Data());
  std::ofstream outfile(outfilename);
  
  std::string line;

  while(std::getline(infile, line)){
    int Id;
    int WireId;
    double p1;
    double p0;

    if(!line.empty() && line[0] == '#'){
      outfile << line << std::endl;
      continue;
    }    
    std::istringstream iss(line);
    iss >> Id >> WireId >> p1 >> p0;
    for(int i=0; i<10; i++){
      if(Id == i+1){
	p0 = p0 - diff[i];
      }
    }
    for(int i=30; i<40; i++){
      if(Id == i+1){
	p0 = p0 - diff[i-20];
      }
    }
    outfile << Id << " " << WireId << " " << p1 << " " << p0 << std::endl;
  }
  infile.close();
  outfile.close();
}
