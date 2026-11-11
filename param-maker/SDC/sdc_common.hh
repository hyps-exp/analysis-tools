#ifndef SDC_COMMON_HH_
#define SDC_COMMON_HH_
#include "../../include/hyps_constant.hh"
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TF1.h>
#include <TLatex.h>
#include <TROOT.h>
#include <TSystem.h>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <memory>
#include <iomanip>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
namespace sdc {
template <size_t N, size_t M>
constexpr auto join(const std::array<geom_info::PlaneInfo,N>& a,
                    const std::array<geom_info::PlaneInfo,M>& b) {
  std::array<geom_info::PlaneInfo,N+M> result{};
  for(size_t i=0;i<N;++i) result[i]=a[i];
  for(size_t i=0;i<M;++i) result[N+i]=b[i];
  return result;
}
constexpr auto in=join(geom_info::kSDC0,geom_info::kSDC1);
constexpr auto out=join(geom_info::kSDC2,geom_info::kSDC3);
constexpr auto planes=join(in,out);
inline int index(int id) {
  for(size_t i=0;i<planes.size();++i) if(planes[i].id==id) return i;
  return -1;
}
inline bool isIn(int id) {
  for(auto plane:in) if(plane.id==id) return true;
  return false;
}
// Tracking In/Out histograms use local, one-based layer indices, not DCGEO IDs.
inline int histogramLayer(int id) {
  for(size_t i=0;i<in.size();++i) if(in[i].id==id) return i+1;
  for(size_t i=0;i<out.size();++i) if(out[i].id==id) return i+1;
  throw std::runtime_error("Unknown SDC layer");
}
inline TString stem(TString name) {
  if(name.EndsWith(".root")) name.Remove(name.Length()-5);
  if(name.BeginsWith("run")) name.Remove(0,3);
  return name;
}
inline TFile* open(const TString& name) {
  auto f=TFile::Open(name,"READ");
  if(!f || f->IsZombie()) throw std::runtime_error("Cannot open ROOT file: "+std::string(name.Data()));
  return f;
}
template<class H> H* get(TFile* file,const TString& name) {
  auto h=dynamic_cast<H*>(file->Get(name));
  if(!h) throw std::runtime_error("Missing/wrong histogram: "+std::string(name.Data()));
  return h;
}
// Publish only a fully computed parameter file. Failed fits/parsing leave the
// previous output intact. A sibling temporary file makes rename atomic.
inline void writeParameters(const TString& path, const std::string& text) {
  std::string pattern=std::string(path.Data())+".tmp.XXXXXX";
  std::vector<char> name(pattern.begin(),pattern.end()); name.push_back('\0');
  int fd=::mkstemp(name.data());
  if(fd<0) throw std::runtime_error("Cannot create temporary parameter file");
  struct stat previous;
  const auto mode=::stat(path.Data(),&previous)==0 ? previous.st_mode & 0777 : 0644;
  if(::fchmod(fd,mode)!=0) { ::close(fd); std::remove(name.data());
    throw std::runtime_error("Cannot set parameter file permissions"); }
  ::close(fd);
  try {
    std::ofstream output(name.data(),std::ios::binary);
    output<<text;
    output.close();
    if(!output) throw std::runtime_error("Parameter write failed");
    if(std::rename(name.data(),path.Data())!=0)
      throw std::runtime_error("Cannot publish parameter file");
  } catch(...) { std::remove(name.data()); throw; }
}
inline void outputDirectory(int run) {
  auto dir=Form("../../../output/run%05d",run);
  if(gSystem->AccessPathName(dir) && gSystem->mkdir(dir,true)!=0)
    throw std::runtime_error("Cannot create output directory");
}
}
#endif
