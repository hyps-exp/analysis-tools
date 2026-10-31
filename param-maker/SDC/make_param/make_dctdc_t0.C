/**
 * @file    make_dctdc_t0.C
 * @brief   Analysis of t0 calibration
 * @author  Akari Haratani
 * @date    2026-10-31
 *
 * Change history:
 *   2026-10-31  Akari Haratani First version
 */

// root -l -b -q 'make_dctdc_t0.C("07162_In","07162_Out",7162,-1.5,"DCTdcCalib_hyps_run07162_test3")'

namespace DCTdcT0Settings {
  constexpr const char* kRootDirectory = "../../../root/run%05d/";
  constexpr const char* kDctdcDirectory = "../../../output/run%05d/";
  constexpr const char* kPdfDirectory = "../../../output/run%05d/";  // ユーザー設定を保持

  constexpr const char* kDctdcNameFormat = "DCTdcCalib_hyps_run%05d_t0";
  constexpr const char* kPdfNameFormat = "make_dctdc_t0_run%05d.pdf";
}

#include <TFile.h>
#include <TH1.h>
#include <TF1.h>
#include <TFitResultPtr.h>
#include <TFitResult.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TLatex.h>
#include <TLine.h>
#include <TLegend.h>
#include <TSystem.h>
#include <TROOT.h>
#include <TStyle.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>

// ============================================================================
// 2. 内部データとフィット判定
// ============================================================================
namespace DCTdcT0Early {
  using Key=std::pair<int,int>;
  struct Row { int layer,wire,line; double gain,offset; size_t start,end; };
  struct Window { double edge=NAN,left=25,right=15; };
  struct Fit {
    bool ok=false; std::string reason="NO_FIT";
    int status=-1,cov=-1;
    double n=0,edge=NAN,error=NAN,width=NAN,bg=NAN,amp=NAN,deviance=NAN,ndf=0,lo=NAN,hi=NAN;
    std::vector<double> par;
    double slope=NAN,local=NAN,curv=NAN;
    bool sharp=false;
  };
  struct Result {
    Row row;
    Fit fit,narrow,wide;
    double oldEdge,proposed,stored,stability=NAN,seed=NAN,total=NAN,adopted=NAN;
    std::string reason,diagnostic,method="FERMI";
    std::shared_ptr<TH1> hist;
    int order=-1;
    bool broad=false;
  };
// Local shape tolerance: kLocalBase + kLocalSigma/sqrt(f(t0))  (relative to f(t0)).
// Window stability tolerance: max(minTol, kStabilitySigma * nominal t0 error) [ch].
// minTol default 1 ch (the spread is also added to the quoted error); tune it from the
// window-shift distribution printed at the end of every run.
const double kLocalBase=0.05,kLocalSigma=5.,kStabilityMin=1.0,kStabilitySigma=3.,kModelDeviance=9.;
// kModelDeviance: a plateau term (slope / curvature) is kept only if it lowers the Poisson
// deviance by at least this much per dropped parameter (~3 sigma); otherwise the simpler model is used.
inline bool accepted(const std::string& s){return s.find("ACCEPTED")==0;}
inline int histogramLayer(int l){return l<=10?l:l-30;}
inline std::string chamber(int l){return l<=4?"SDC0":l<=10?"SDC1":l<=35?"SDC2":"SDC3";}
inline std::string windowDecision(const Fit& nominal,const Fit& narrow,const Fit& wide,double& spread,double minTol=kStabilityMin,double* adopted=nullptr){
 if(adopted)*adopted=NAN;
 bool nominalShapeOnly=!nominal.ok&&nominal.reason=="POOR_FERMI_SHAPE";
 if(!nominal.ok&&!nominalShapeOnly)return nominal.reason;
 double tol=std::max(minTol,kStabilitySigma*nominal.error);
 spread=0;bool anyGood=false,agree=false,poorBoth=true,poorAgree=true;
 for(auto f:{narrow,wide}){
  if(f.ok)anyGood=true;
  if(f.reason!="POOR_FERMI_SHAPE")poorBoth=false;
  if(f.ok||f.reason=="POOR_FERMI_SHAPE"){
    double d=std::abs(f.edge-nominal.edge);
    spread=std::max(spread,d);
    if(f.ok&&d<=tol)agree=true;
    if(d>tol)poorAgree=false;
  }
 }
 if(!anyGood){
  // No fully good alternate. If BOTH alternates are shape-only failures (the edge shape is
  // not Fermi-like in every window, e.g. a dip next to the edge) but all three t0 agree
  // within tol, the t0 is still robust: adopt it as a note. A single shape-only alternate
  // (or one that failed otherwise) is never enough.
  if(poorBoth){
    if(!poorAgree) return "WINDOW_UNSTABLE";
    if(adopted)*adopted=nominal.edge;
    return "ACCEPTED_WINDOW_NOTE";
  }
  return "WINDOW_CHECK_FAILED";
 }
 if(agree){
   if(adopted)*adopted=nominal.edge;
   return nominal.ok&&narrow.ok&&wide.ok&&spread<=tol?"ACCEPTED":"ACCEPTED_WINDOW_NOTE";
 }
 if(narrow.ok&&wide.ok&&std::abs(narrow.edge-wide.edge)<=std::max(minTol,kStabilitySigma*std::max(narrow.error,wide.error))){
   if(adopted)*adopted=.5*(narrow.edge+wide.edge);
   return "ACCEPTED_ALT_WINDOWS";
 }
 return "WINDOW_UNSTABLE";
}
  inline bool target(int l){
    return (l>=1&&l<=10)||(l>=31&&l<=40);
  }
  // 入力DCTDC・任意のfit窓設定を読み込む。
  inline std::string read(const char* path){
    std::ifstream f(path);
    if(!f)throw std::runtime_error(std::string("Cannot read ")+path);
    return std::string(std::istreambuf_iterator<char>(f),{});
  }
  inline std::vector<std::string> fields(std::string s){
    s=s.substr(0,s.find('#'));
    std::istringstream f(s);
    std::vector<std::string> v;
    std::string x;
    while(f>>x)v.push_back(x);
    return v;
  }
  inline std::string number(double x){
    if(!std::isfinite(x))return "";
    std::ostringstream s;
    s<<std::setprecision(17)<<x;
    return s.str();
  }
  inline double fermi(double *x,double *p){
    double z=(x[0]-p[2])/p[3];
    double d=x[0]-p[2];
    double a=p[1]+p[4]*d+p[5]*d*d;
    if(a<0) a=0;
    return p[0]+a/(1+std::exp(std::max(-700.,std::min(700.,z))));
  }
  // Largest |data-fit|/f(t0) within |x-t0|<4w. Infinity if the fit is unusable.
  inline double localResidual(const TH1& h,TF1& f){   // TF1::Integral is non-const
    double t0=f.GetParameter(2),w=f.GetParameter(3),ref=f.Eval(t0);
    if(!(ref>0)||!(w>0)||!std::isfinite(ref+w+t0))return INFINITY;
    double worst=0;
    auto ax=h.GetXaxis();
    double reach=std::max(4*w,1.5*ax->GetBinWidth(1));   // edges sharper than a bin still see the neighbouring bins
    for(int i=1;i<=h.GetNbinsX();i++){
      double x=ax->GetBinCenter(i);
      if(std::abs(x-t0)>reach)continue;
      double lo=ax->GetBinLowEdge(i),hi=ax->GetBinUpEdge(i),mu=f.Integral(lo,hi)/(hi-lo);   // same bin-integrated model as the fit
      worst=std::max(worst,std::abs(h.GetBinContent(i)-mu)/ref);
    }
    return worst;
  }
  inline std::vector<Row> parseTdc(const std::string& text,std::vector<std::string>& lines){
    std::istringstream in(text);
    std::string line;
    std::vector<Row> rows;
    std::set<Key> seen;
    std::regex token("\\S+");
    while(std::getline(in,line)){
      int index=lines.size();
      lines.push_back(line);
      auto v=fields(line);
      if(v.empty())continue;
      int l=std::stoi(v[0]);
      if(!target(l))continue;
      if(v.size()!=4)throw std::runtime_error("DCTDC target row must be: layer wire gain offset");
      int w=std::stoi(v[1]);
      double a=std::stod(v[2]),b=std::stod(v[3]);
      if(w<0||w>=1000||a<=0||!std::isfinite(a+b)||!seen.insert({l,w}).second)throw std::runtime_error("Bad/duplicate DCTDC row");
      std::vector<std::pair<size_t,size_t>> spans;std::string body=line.substr(0,line.find('#'));
      for(auto it=std::sregex_iterator(body.begin(),body.end(),token);it!=std::sregex_iterator();++it)spans.push_back({it->position(),it->position()+it->length()});
      rows.push_back({l,w,index,a,b,spans[3].first,spans[3].second});
    }
    if(rows.empty())throw std::runtime_error("No SDC rows in DCTDC");
    return rows;
  }
  inline std::map<Key,Window> windows(const char* path){
    std::map<Key,Window> out;
    if(!path||!path[0])return out;
    std::ifstream f(path);
    if(!f)throw std::runtime_error("Cannot open windows CSV");
    std::string line;
    if(!std::getline(f,line)||line!="layer,wire,edge_ch,left_ch,right_ch")throw std::runtime_error("Window CSV header must be layer,wire,edge_ch,left_ch,right_ch");
    while(std::getline(f,line)){
      if(line.empty()||line[0]=='#') continue;
      std::replace(line.begin(),line.end(),',',' ');
      std::istringstream s(line);
      int l,w;
      Window x;
      std::string extra;
      if(!(s>>l>>w>>x.edge>>x.left>>x.right)||(s>>extra)||!target(l)||w< -1||x.left<=0||x.right<=0||!std::isfinite(x.edge+x.left+x.right)||!out.emplace(Key(l,w),x).second)throw std::runtime_error("Invalid/duplicate window row");
    }return out;
  }
  inline bool countHistogram(const TH1& h){
    if(h.GetDimension()!=1)return false;
    for(int i=0;i<=h.GetNbinsX()+1;i++){
      double n=h.GetBinContent(i);
      if(!std::isfinite(n)||n<0||std::abs(n-std::round(n))>1e-6)return false;
      if(h.GetSumw2N()&&std::abs(h.GetBinError(i)*h.GetBinError(i)-n)>1e-5*std::max(1.,n))return false;
    }return true;
  }
  inline double findEdge(const TH1& h,double guess,double search){
    auto ax=h.GetXaxis();
    int a=std::max(4,ax->FindFixBin(guess-search)),b=std::min(h.GetNbinsX()-3,ax->FindFixBin(guess+search));
    double best=0,edge=NAN;
    for(int i=a;i<=b;i++){
      double left=0,right=0;
      for(int k=1;k<=3;k++){
	left+=h.GetBinContent(i-k);
	right+=h.GetBinContent(i+k);
      }
      double drop=left-right;
      if(drop>best){
	best=drop;
	edge=ax->GetBinCenter(i);
      }
    }return edge;
  }
  inline Fit fit(TH1& h,double seed,double left,double right,double minCounts,double maxError,int order=1){
    Fit r;
    auto ax=h.GetXaxis();
    double dx=ax->GetBinWidth(1);
    if(!std::isfinite(seed)){
      r.reason="NO_FALLING_EDGE";
      return r;
    }
    if(seed-left<ax->GetXmin()||seed+right>ax->GetXmax()){
      r.reason="RANGE_OUTSIDE_HISTOGRAM";
      return r;
    }
    for(int i=2;i<=h.GetNbinsX();i++)
      if(std::abs(ax->GetBinWidth(i)-dx)>1e-8*dx){
	r.reason="NONUNIFORM_BINS";
	return r;
      }
    int b1=ax->FindFixBin(seed-left),b2=ax->FindFixBin(seed+right);
    r.lo=ax->GetBinLowEdge(b1);
    r.hi=ax->GetBinUpEdge(b2);
    r.n=h.Integral(b1,b2);
    r.ndf=b2-b1+1-(order>1?6:order==1?5:4);
    if(r.n<minCounts||r.ndf<8){
      r.reason="LOW_STAT";
      return r;
    }
    double peak=0,tail=0,plateau=0;
    int nb=std::max(2,(b2-b1+1)/6);
    for(int i=b1;i<=b2;i++) peak=std::max(peak,h.GetBinContent(i));
    for(int i=0;i<nb;i++){
      plateau+=h.GetBinContent(b1+i)/nb;
      tail+=h.GetBinContent(b2-i)/nb;
    }
    // Plateau level just left of the edge (7-14 ch before the seed) to seed the slope.
    double xfar=ax->GetBinCenter(b1)+.5*(nb-1)*dx,near=plateau,xnear=xfar,slope0=0;
    {int bs=ax->FindFixBin(seed),c1=bs-int(std::ceil(14./dx)),c2=bs-int(std::ceil(7./dx));
      if(c1>=b1+nb&&c2>=c1){
	double s=0;
	for(int i=c1;i<=c2;i++)s+=h.GetBinContent(i);near=s/(c2-c1+1);xnear=.5*(ax->GetBinCenter(c1)+ax->GetBinCenter(c2));
	if(xnear>xfar) slope0=(near-plateau)/(xnear-xfar);
      }
    }
    if(order<=0){slope0=0;near=plateau;xnear=xfar;}   // flat plateau model
    if(std::max(plateau,near)<=tail||peak<=0){
      r.reason="NO_FALLING_CONTRAST";
      return r;
    }
    double wmin=.15*dx,wmax=std::min(15.,(r.hi-r.lo)/4.);
    double smax=3*peak/std::max(dx,seed-r.lo);
    slope0=std::max(-smax,std::min(smax,slope0));
    TF1 f("dctdc_fermi_fit",fermi,r.lo,r.hi,6);
    f.SetParNames("background","amplitude","t0_TDC_ch","width_ch","plateau_slope","plateau_curvature");
    f.SetParameters(std::max(.01,tail),std::max(1.,near-tail+slope0*(seed-xnear)),seed,std::max(wmin*2,std::min(2.*dx,wmax/2)),slope0,0.);
    f.SetParLimits(0,0,10*peak+1);
    f.SetParLimits(1,0,10*peak+1);
    f.SetParLimits(2,r.lo+dx,r.hi-dx);
    f.SetParLimits(3,wmin,wmax);
    if(order>0)f.SetParLimits(4,-smax,smax);
    else f.FixParameter(4,0);
    {
      double span=std::max(dx,seed-r.lo),cmax=2*peak/(span*span);
      if(order>1)f.SetParLimits(5,-cmax,cmax);
      else f.FixParameter(5,0);
    }
    auto result=h.Fit(&f,"LIRSQN");
    r.status=int(result);
    r.cov=result.Get()?result->CovMatrixStatus():-1;
    r.edge=f.GetParameter(2);
    r.error=f.GetParError(2);
    r.width=f.GetParameter(3);
    r.bg=f.GetParameter(0);
    r.amp=f.GetParameter(1);
    r.slope=f.GetParameter(4);
    r.curv=f.GetParameter(5);
    r.par={r.bg,r.amp,r.edge,r.width,r.slope,r.curv};
    r.deviance=0;
    for(int i=b1;i<=b2;i++){
      double n=h.GetBinContent(i),mu=f.Integral(ax->GetBinLowEdge(i),ax->GetBinUpEdge(i))/dx;
      if(!(mu>0)||!std::isfinite(mu)){
	r.deviance=NAN;
	break;
      }
      r.deviance+=2*(mu-n+(n>0?n*std::log(n/mu):0));
    }
    if(r.status!=0||r.cov!=3){
      r.reason="FIT_OR_COVARIANCE_FAILED";
      return r;
    }
    if(!std::isfinite(r.edge+r.error+r.width+r.deviance)||r.error<=0||r.error>maxError){
      r.reason="T0_ERROR";
      return r;
    }
    r.local=localResidual(h,f);
    if(r.width<=wmin*1.02){
      // Width at its lower limit = an edge sharper than the TDC bin. Legitimate when there is a
      // clear step across t0 (>=5 sigma between the 3 bins before and after); otherwise it is a
      // degenerate fit (noise spike) and is rejected as before.
      int bt=ax->FindFixBin(r.edge);
      double lsum=0,rsum=0;
      for(int k=1;k<=3;k++){
	lsum+=h.GetBinContent(bt-k);
	rsum+=h.GetBinContent(bt+k);
      }
      if(!((lsum-rsum)/std::sqrt(lsum+rsum+1)>=5)){
	r.reason="WIDTH_AT_LOWER_LIMIT";
	return r;
      }
      r.sharp=true;
    }
    if(r.width>=wmax*.98){r.reason="WIDTH_AT_UPPER_LIMIT";return r;}
    if(r.edge-r.lo<3*r.width){r.reason="INSUFFICIENT_LEFT_RANGE";return r;}
    if(r.hi-r.edge<3*r.width){r.reason="INSUFFICIENT_RIGHT_RANGE";return r;}
    if(f.GetParError(1)<=0||r.amp/f.GetParError(1)<5){r.reason="WEAK_EDGE";return r;}
    // Shape: edge-local residual only (global deviance/ndf always grows with statistics).
    if(!(r.local<=kLocalBase+kLocalSigma/std::sqrt(std::max(1.,f.Eval(r.edge))))){r.reason="POOR_FERMI_SHAPE";return r;}
    r.ok=true;r.reason="ACCEPTED";return r;
  }
  // Seed for a smeared edge (hump whose falling flank is much broader than the usual edge):
  // peak of a 9-bin box average in [expected-80, expected+40], then the steepest descent to
  // the right of it, measured over 6 bins on each side. peakPos returns the peak position.
  inline double broadSeed(const TH1& h,double expected,double& peakPos){
    auto ax=h.GetXaxis();int n=h.GetNbinsX();
    int a=std::max(8,ax->FindFixBin(expected-80)),b=std::min(n-7,ax->FindFixBin(expected+40));
    if(b<=a)return NAN;
    int pk=-1;double best=0;
    for(int i=a;i<=b;i++){
      double sm=0;
      for(int k=-4;k<=4;k++)sm+=h.GetBinContent(i+k);
      if(sm>best){best=sm;pk=i;}}
    if(pk<0)return NAN;
    peakPos=ax->GetBinCenter(pk);
    double bestDrop=0,edge=NAN;
    for(int i=pk;i<=b;i++){
      double l=0,r=0;
      for(int k=1;k<=6;k++){
	l+=h.GetBinContent(i-k);
	r+=h.GetBinContent(i+k);
      }
      if(l-r>bestDrop){
	bestDrop=l-r;
	edge=ax->GetBinCenter(i);
      }
    }
    return edge;
  }
  // Fit, and if the window was merely too small (range / width-limit failures), widen it
  // and refit. left/right return the window actually used (alternates scale from it).
  // Left growth is capped at 1.6x the requested value to avoid swallowing the hump.
  inline Fit fitAdaptive(TH1& h,double seed,double& left,double& right,double minCounts,double maxError,int order){
    Fit f=fit(h,seed,left,right,minCounts,maxError,order);double left0=left,xmax=h.GetXaxis()->GetXmax();
    for(int k=0;k<3;k++){
      double nl=left,nr=right;
      if(f.reason=="INSUFFICIENT_RIGHT_RANGE")nr=std::max(1.2*right,(f.edge-seed)+4*f.width);
      else if(f.reason=="INSUFFICIENT_LEFT_RANGE")nl=std::min(1.6*left0,std::max(1.2*left,4*f.width-(f.edge-seed)));
      else if(f.reason=="WIDTH_AT_UPPER_LIMIT"){nl=std::min(1.6*left0,1.3*left);nr=1.3*right;}
      else break;
      nr=std::min(nr,xmax-seed-1e-6);
      if(nl==left&&nr==right)break;
      left=nl;right=nr;f=fit(h,seed,left,right,minCounts,maxError,order);
    }
    return f;
  }
  // Plateau-model choice. nominal=true: widen the window if needed, and prefer the simpler
  // plateau unless the extra term lowers the deviance significantly (kModelDeviance).
  // Poorly constrained fits (WEAK_EDGE / T0_ERROR / FIT_OR_COVARIANCE_FAILED) are retried
  // with a simpler plateau. nominal=false (alternate windows): fixed window, no selection,
  // only the failure fallback. 'used' returns the plateau order that was kept.
  inline Fit fitLadder(TH1& h,double seed,double& left,double& right,double minCounts,double maxError,int order,int& used,bool nominal=true){
    used=order;
    double l0=left,r0=right;
    auto go=[&](double& l,double& r,int o){ return nominal?fitAdaptive(h,seed,l,r,minCounts,maxError,o):fit(h,seed,l,r,minCounts,maxError,o); };    
 Fit f=go(left,right,order);
 for(int o=order-1;o>=0&&(f.reason=="WEAK_EDGE"||f.reason=="T0_ERROR"||f.reason=="FIT_OR_COVARIANCE_FAILED");o--){
   double l=l0,r=r0;Fit g=go(l,r,o);
   if(g.ok||g.reason=="POOR_FERMI_SHAPE"){f=g;left=l;right=r;used=o;break;}
 }
 if(nominal)
   while(used>0&&(f.ok||f.reason=="POOR_FERMI_SHAPE")){
     Fit g=fit(h,seed,left,right,minCounts,maxError,used-1);          // same window: deviances comparable
     if(!(g.ok||g.reason=="POOR_FERMI_SHAPE")||(f.ok&&!g.ok)||!std::isfinite(f.deviance+g.deviance)||f.deviance-g.deviance>=kModelDeviance)break;
     f=g;used--;
   }
 return f;
  }
  // Final adoption policy requested 2026-10-21: statistics gates adoption;
  // numerical/shape/window checks remain diagnostics and model-selection aids.
  // Empirical fallback is explicitly NOT reported as a Fermi fit or a measured fit error.
  inline double histogramEstimate(const TH1& h,double expected,double reach,std::string& method){
    auto ax=h.GetXaxis();
    int a=std::max(1,ax->FindFixBin(expected-reach)),b=std::min(h.GetNbinsX(),ax->FindFixBin(expected+reach));
    auto smooth=[&](int i){double y=0;int n=0;
      for(int j=std::max(a,i-2);j<=std::min(b,i+2);j++){
	y+=h.GetBinContent(j);
	n++;
      }return n?y/n:0.;
    };
    int pk=a;
    for(int i=a+1;i<=b;i++)if(smooth(i)>smooth(pk))pk=i;
    double tail=0;int n=0;for(int i=std::max(a,b-4);i<=b;i++){tail+=h.GetBinContent(i);n++;}tail/=std::max(1,n);
    double level=.5*(smooth(pk)+tail);
    if(smooth(pk)>tail)for(int i=pk;i<b;i++){
	double y=smooth(i),z=smooth(i+1);
	if(y>=level&&z<level){
	  method="HISTOGRAM_HALF_HEIGHT";
	  return ax->GetBinCenter(i)+(ax->GetBinCenter(i+1)-ax->GetBinCenter(i))*(y-level)/(y-z);
	}
      }
    // No descending half-height exists (e.g. flat input). Supply a reproducible
    // histogram quantile as an INITIAL fallback, with an explicit diagnostic.
    double total=h.Integral(a,b),sum=0;for(int i=a;i<=b;i++){
      sum+=h.GetBinContent(i);
      if(sum>=.95*total&&total>0){
	method="HISTOGRAM_Q95_FALLBACK";
	return ax->GetBinCenter(i);
      }
    }
    return NAN;
  }
  inline void adoptInitial(Result& r,double expected,double reach,double minCounts){
    r.diagnostic=r.reason;
    if(!r.hist)return;
    auto ax=r.hist->GetXaxis();
    int a=std::max(1,ax->FindFixBin(expected-reach)),b=std::min(r.hist->GetNbinsX(),ax->FindFixBin(expected+reach));
    double n=std::isfinite(r.fit.n)&&r.fit.n>0?r.fit.n:r.hist->Integral(a,b);
    if(n<minCounts){r.reason="LOW_STAT";r.adopted=NAN;r.total=NAN;return;}
    if(accepted(r.reason)&&std::isfinite(r.adopted))return; // preserve the user's existing successful choice
    if(std::isfinite(r.fit.edge)&&!r.fit.par.empty()&&r.fit.width>0&&std::all_of(r.fit.par.begin(),r.fit.par.end(),[](double x){return std::isfinite(x);})){
      r.adopted=r.fit.edge;r.reason="ACCEPTED_WITH_NOTE";r.method="FERMI";
      double spread=0;bool have=false;
      for(auto f:{r.narrow,r.wide})if(std::isfinite(f.edge)){spread=std::max(spread,std::abs(f.edge-r.adopted));have=true;}
      if(have)r.stability=spread;
    }else{
      r.adopted=histogramEstimate(*r.hist,expected,reach,r.method);
      r.reason=std::isfinite(r.adopted)?"ACCEPTED_HISTOGRAM":"NO_DATA_ESTIMATE";
      r.total=NAN; // histogram fallback has no fitted statistical uncertainty
    }
  }
  // 全ワイヤーをfitし、最後にPDFとDCTDCへ直接書き込む。
  inline std::vector<Result> Run(const char* inRoot,const char* outRoot,const char* tdcPath,const char* output,const char* pdfOutput,const char* windowCsv="",double minCounts=500,double search=40,double maxError=2,double stabilityTol=kStabilityMin,int plateauOrder=1,double earlyNs=0){
    if(!std::isfinite(minCounts)||!std::isfinite(search)||!std::isfinite(maxError)||!std::isfinite(stabilityTol)||minCounts<=0||search<0||maxError<=0||stabilityTol<=0||(plateauOrder<0||plateauOrder>2))throw std::runtime_error("Invalid fit settings");
    auto text=read(tdcPath);
    std::vector<std::string> lines;
    auto rows=parseTdc(text,lines);
    if(!std::isfinite(earlyNs))throw std::runtime_error("earlyNs must be finite (positive, zero or negative)");
    auto settings=windows(windowCsv);
    auto open=[](const char* p)->std::unique_ptr<TFile>{
      if(!p||!p[0])return {};
      std::unique_ptr<TFile> f(TFile::Open(p,"READ"));
      if(!f||f->IsZombie())throw std::runtime_error(std::string("Bad input ROOT: ")+p);
      return f;
    };
    auto fin=open(inRoot),fout=open(outRoot);
    if(!fin&&!fout)throw std::runtime_error("Supply an In or Out ROOT file");
    std::vector<Result> all;
    int acceptedCount=0;
    for(auto row:rows){
      Result r;
      r.row=row;
      r.oldEdge=-row.offset;
      r.proposed=NAN;
      r.stored=row.offset;
      auto file=row.layer<=10?fin.get():fout.get();
      std::string name="h"+std::to_string(10000*histogramLayer(row.layer)+row.wire);
      auto obj=file?file->Get(name.c_str()):nullptr;
      auto h=dynamic_cast<TH1*>(obj);
      if(!file)r.reason="INPUT_NOT_SUPPLIED";
      else if(!h)r.reason="MISSING_HISTOGRAM";
      else if(std::string(h->GetTitle()).find(chamber(row.layer))==std::string::npos)r.reason="HISTOGRAM_CHAMBER_MISMATCH";
      else if(!countHistogram(*h))r.reason="NOT_UNWEIGHTED_1D_COUNTS";
      else{
	r.hist.reset(dynamic_cast<TH1*>(h->Clone(("tdc_L"+std::to_string(row.layer)+"_W"+std::to_string(row.wire)).c_str())));
	r.hist->SetDirectory(nullptr);
	Window w;
	if(settings.count({row.layer,-1}))w=settings.at({row.layer,-1});
	if(settings.count({row.layer,row.wire}))w=settings.at({row.layer,row.wire});
	double expected=std::isfinite(w.edge)?w.edge:r.oldEdge;
	// Nominal + narrow/wide analysis for a given seed and window (reused by the broad-edge rescue).
	auto analyse=[&](Result& o,double seed,double L,double R){
	  o.seed=seed;
	  int used=plateauOrder;
	  o.fit=fitLadder(*r.hist,seed,L,R,minCounts,maxError,plateauOrder,used);
	  o.order=used;
	  o.reason=o.fit.reason;
	  o.narrow=Fit();
	  o.wide=Fit();
	  o.stability=NAN;
	  o.adopted=NAN;
	  // Alternates also run when the nominal fit failed only on shape: stability decides.
	  if(o.fit.ok||o.fit.reason=="POOR_FERMI_SHAPE"){
	    {
	      double nl=.8*L,nr=.8*R,wl=1.2*L,wr=1.2*R;
	      int u1,u2;
	      o.narrow=fitLadder(*r.hist,seed,nl,nr,.5*minCounts,2*maxError,used,u1,false);
	      o.wide=fitLadder(*r.hist,seed,wl,wr,.5*minCounts,2*maxError,used,u2,false);
	    }
	    o.reason=windowDecision(o.fit,o.narrow,o.wide,o.stability,stabilityTol,&o.adopted);
	  }
	};
	analyse(r,search>0?findEdge(*r.hist,expected,search):expected,w.left,w.right);
	// Rescue for smeared edges: seed from the peak of the hump and its steepest descent, with a
	// window that starts at the peak and reaches the end of the tail. Used only if it is accepted.
	static const std::set<std::string> rescuable={"T0_ERROR","WEAK_EDGE","WIDTH_AT_UPPER_LIMIT","WIDTH_AT_LOWER_LIMIT","FIT_OR_COVARIANCE_FAILED","INSUFFICIENT_LEFT_RANGE","INSUFFICIENT_RIGHT_RANGE","WINDOW_UNSTABLE","WINDOW_CHECK_FAILED"};
	if(search>0&&rescuable.count(r.reason)){
	  double peakPos=NAN,bs=broadSeed(*r.hist,expected,peakPos);
	  if(std::isfinite(bs)&&bs>peakPos){
	    Result t=r;
	    analyse(t,bs,std::max(w.left,bs-peakPos+4),std::max(w.right,1.5*(bs-peakPos)));
	    if(accepted(t.reason)){
	      r.seed=t.seed;
	      r.fit=t.fit;
	      r.narrow=t.narrow;
	      r.wide=t.wide;
	      r.stability=t.stability;
	      r.adopted=t.adopted;
	      r.order=t.order;
	      r.reason=t.reason;
	      r.broad=true;
	    }
	  }
	}
	adoptInitial(r,expected,std::max(80.,search),minCounts);
	if(std::isfinite(r.fit.edge))r.proposed=-r.fit.edge;
	if(accepted(r.reason)){
	  if(!std::isfinite(r.adopted))r.adopted=r.fit.edge;
	  r.proposed=-r.adopted;
	  r.stored=r.proposed+earlyNs/row.gain;
          if(!std::isfinite(r.stored))throw std::runtime_error("Nonfinite output offset");
	  acceptedCount++;
	  lines[row.line].replace(row.start,row.end-row.start,number(r.stored));
	  {
	    double e=r.reason=="ACCEPTED_ALT_WINDOWS"?std::max(r.narrow.error,r.wide.error):r.fit.error,sys=std::isfinite(r.stability)?r.stability/2:0.,fl=r.fit.sharp?r.hist->GetXaxis()->GetBinWidth(1)/std::sqrt(12.):0.;   // sharp edge: add bin quantisation
	    r.total=r.method=="FERMI"&&std::isfinite(e)&&e>0?std::sqrt(e*e+sys*sys+fl*fl):NAN;
	  }
	}
      }
      all.push_back(r);
    }
    // Distribution of the window-dependence of t0, to choose stabilityTol from data.
    std::vector<double> shifts;
    for(auto& r:all)if(std::isfinite(r.stability))shifts.push_back(r.stability);
    std::sort(shifts.begin(),shifts.end());
    std::ostringstream shiftInfo;
    shiftInfo<<"window_shift_ch n="<<shifts.size();
    if(!shifts.empty()){
      auto q=[&](double p){
	return shifts[std::min(shifts.size()-1,size_t(p*shifts.size()))];
      };
      shiftInfo<<" median="<<q(.5)<<" p90="<<q(.9)<<" p95="<<q(.95)<<" max="<<shifts.back();
    }
    std::cout<<shiftInfo.str()<<std::endl;
    gStyle->SetOptStat(0);
    TCanvas canvas("dctdc_t0_review","Per-wire initial t0",1200,900);
    std::string pdf=pdfOutput;
    canvas.Print((pdf+"[").c_str());
    canvas.Clear();
    TLatex info;
    info.SetNDC();
    info.SetTextSize(.035);
    info.DrawLatex(.08,.85,"Per-wire Fermi edge (sloped plateau): INITIAL calibration candidate");
    info.DrawLatex(.08,.75,Form("Accepted %d / %zu; other rows KEEP template values",acceptedCount,all.size()));
    info.DrawLatex(.08,.65,"DCTDC offset = - fitted TDC edge + shift [ns] / gain");
    info.DrawLatex(.08,.55,"Red: fitted edge; dotted blue: written zero-time TDC");
    info.DrawLatex(.08,.45,"In: hist layer = param layer; Out: hist layer = param layer - 30");
    info.DrawLatex(.08,.25,"Statistics-only adoption; shape / width / window flags are diagnostics");
    info.DrawLatex(.08,.35,"Input files and gain are unchanged");
    canvas.cd();
    canvas.Modified();
    canvas.Update();
    canvas.Print(pdf.c_str());
    // First show accepted wire edges versus unchanged template fallbacks, by layer.
    std::vector<int> layers;
    for(auto r:rows)if(std::find(layers.begin(),layers.end(),r.layer)==layers.end())layers.push_back(r.layer);
    for(size_t page=0;page<layers.size();page+=6){
      canvas.cd();
      canvas.Clear();
      canvas.Divide(3,2);
      for(size_t k=0;k<6&&page+k<layers.size();k++){
	int layer=layers[page+k];
	canvas.cd(k+1);
	gPad->SetTopMargin(.25);
	TGraphErrors good,kept;
	double xmin=1e9,xmax=-1e9,ymin=1e9,ymax=-1e9;
	for(auto& r:all)if(r.row.layer==layer){
	    bool ok=accepted(r.reason);
	    auto& g=ok?good:kept;
	    int n=g.GetN();
	    double edge=ok?r.adopted:r.oldEdge;
	    g.SetPoint(n,r.row.wire,edge);
	    g.SetPointError(n,0,ok&&std::isfinite(r.total)?r.total:0);
	    xmin=std::min(xmin,double(r.row.wire));
	    xmax=std::max(xmax,double(r.row.wire));
	    ymin=std::min(ymin,edge);
	    ymax=std::max(ymax,edge);
	  }
	double pad=std::max(3.,(ymax-ymin)*.15);
	gPad->DrawFrame(xmin-1,ymin-pad,xmax+1,ymax+pad,Form("Layer %d: initial t0;Wire ID (0-based);TDC edge [ch]",layer));
	good.SetName(Form("t0_accepted_L%d",layer));
	good.SetMarkerStyle(20);
	good.SetMarkerSize(.5);
	good.SetMarkerColor(kBlue+1);
	good.SetLineColor(kBlue+1);
	if(good.GetN()) good.DrawClone("P SAME");
	kept.SetName(Form("t0_kept_template_L%d",layer));
	kept.SetMarkerStyle(24);
	kept.SetMarkerSize(.5);
	kept.SetMarkerColor(kGray+2);
	if(kept.GetN()) kept.DrawClone("P SAME");
	TLegend legend(.12,.80,.92,.91);
	legend.SetTextSize(.035);
	legend.AddEntry(&good,"Fitted t0 before requested shift","p");
	legend.AddEntry(&kept,"Template kept (NOT a measured t0)","p");
	legend.DrawClone();
      }
      canvas.cd();
      canvas.Modified();
      canvas.Update();
      canvas.Print(pdf.c_str());
    }
    for(size_t page=0;page<all.size();page+=6){
      canvas.cd();
      canvas.Clear();
      canvas.Divide(3,2);
      for(size_t k=0;k<6&&page+k<all.size();k++){
	auto& r=all[page+k];
	canvas.cd(k+1);
	gPad->SetTopMargin(.25);
	TLatex label;
	label.SetNDC();
	label.SetTextSize(.055);
	if(r.hist){
	  auto h=dynamic_cast<TH1*>(r.hist->Clone());
	  h->SetTitle(Form("Layer %d Wire %d;TDC [ch];Counts / bin",r.row.layer,r.row.wire));
	  double x=std::isfinite(r.seed)?r.seed:r.oldEdge;
	  double xlo=x-60,xhi=x+40;
	  if(!r.fit.par.empty()){
	    xlo=std::min(xlo,r.fit.lo-15);
	    xhi=std::max(xhi,r.fit.hi+15);
	  }
	  if(std::abs(r.oldEdge-x)<100){
	    xlo=std::min(xlo,r.oldEdge-15);
	    xhi=std::max(xhi,r.oldEdge+15);
	  }
	  h->GetXaxis()->SetRangeUser(xlo,xhi);
	  h->DrawCopy("HIST");
	  delete h;
	  gPad->Modified();
	  gPad->Update();
	  double ymax=gPad->GetUymax();
	  if(!r.fit.par.empty()){
	    TF1 f("draw_fermi",fermi,r.fit.lo,r.fit.hi,6);
	    f.SetParameters(r.fit.par.data());
	    f.SetLineColor(kRed);
	    f.DrawCopy("SAME");
	    double xe=accepted(r.reason)&&std::isfinite(r.adopted)?r.adopted:r.fit.edge;
	    TLine line(xe,0,xe,ymax);
	    line.SetLineColor(kRed);
	    line.DrawClone();
	  }
	}
	if(!r.hist) label.DrawLatex(.12,.93,Form("Layer %d Wire %d",r.row.layer,r.row.wire));
	{
	  std::string shown=accepted(r.reason)?"ACCEPTED":r.reason;
	  label.SetTextSize(shown.size()>20?.04:.055);
	  label.SetTextColor(accepted(r.reason)?kGreen+2:kRed+1);
	  label.DrawLatex(.12,.88,shown.c_str());
	  label.SetTextColor(kBlack);
	}
	if(accepted(r.reason)){
          double written=r.adopted-earlyNs/r.row.gain;
          label.SetTextSize(.026);
          label.DrawLatex(.12,.64,Form("Written t0 %.3f ch; #Delta drift time %+.3f ns",written,earlyNs));
          TLine shifted(written,0,written,gPad->GetUymax());
          shifted.SetLineColor(kBlue+1);shifted.SetLineStyle(3);shifted.DrawClone();
        }
        double value=accepted(r.reason)?r.adopted:r.fit.edge;
	if(std::isfinite(value)){
	  if(std::isfinite(r.total))
	    label.DrawLatex(.12,.80,Form("t0 %.3f #pm %.3f ch",value,r.total));
	  else
	    label.DrawLatex(.12,.80,Form("t0 %.3f ch (error unavailable)",value));
	}
	if(r.method!="FERMI"){label.SetTextSize(.026);label.DrawLatex(.12,.72,r.method.c_str());
	  if(std::isfinite(r.adopted)){
	    TLine line(r.adopted,0,r.adopted,gPad->GetUymax());
	    line.SetLineColor(kRed);
	    line.DrawClone();
	  }
	}
      }
      canvas.cd();
      canvas.Modified();
      canvas.Update();
      canvas.Print(pdf.c_str());
    }
    canvas.Print((pdf+"]").c_str());
    std::ofstream candidate(output,std::ios::out|std::ios::trunc);
    if(!candidate)throw std::runtime_error("Cannot open output DCTDC");
    for(size_t i=0;i<lines.size();i++)candidate<<lines[i]<<((i+1<lines.size()||(!text.empty()&&text.back()=='\n'))?"\n":"");
    candidate.close();
    if(!candidate)throw std::runtime_error("DCTDC write failed");
    std::ifstream checkPdf(pdf,std::ios::binary);char header[5]={};checkPdf.read(header,5);
    if(checkPdf.gcount()!=5||std::string(header,5)!="%PDF-")throw std::runtime_error("PDF write failed");
    std::cout<<"DCTDC T0 MACRO COMPLETE accepted="<<acceptedCount<<" kept="<<all.size()-acceptedCount<<" output="<<output<<std::endl;
    return all;
  }
}

// ============================================================================
// 3. パスの確認と2ファイルの出力
// ============================================================================
namespace DCTdcT0Early {
  inline std::string expandOutput(const char* raw) {
    TString path(raw ? raw : "");
    gSystem->ExpandPathName(path);
    return path.Data();
  }

  inline std::vector<Result> WriteTwoFiles(
      const char* inRoot, const char* outRoot,
      const char* tdcTemplate, const char* dctdcOutput,
      double earlyNs = 1.5,
      const char* pdfName = "",
      const char* pdfDirectory = DCTdcT0Settings::kPdfDirectory,
      const char* windowsCsv = "",
      double minFitCounts = 500, double searchHalfwidth = 40,
      double maxT0Error = 2, double stabilityTol = 1.0, int plateauOrder = 1) {
    const auto dc = expandOutput(dctdcOutput);
    const auto tdc = expandOutput(tdcTemplate);
    const auto in = expandOutput(inRoot);
    const auto out = expandOutput(outRoot);
    const auto win = expandOutput(windowsCsv);
    if (dc.empty() || tdc.empty())
      throw std::runtime_error("DCTDC input and output filenames are required");

    // PDF名が空なら、DCTDCのファイル名に.pdfを付ける。
    const std::string name = pdfName && pdfName[0]
        ? pdfName : std::string(gSystem->BaseName(dc.c_str())) + ".pdf";
    if (name.find('/') != std::string::npos || name.size() < 5 ||
        name.substr(name.size() - 4) != ".pdf")
      throw std::runtime_error("pdfName must be a filename ending in .pdf (no directory)");
    const std::string pdf = expandOutput(pdfDirectory) + "/" + name;

    // 上書き可能か、入力と同じファイルでないかを確認する。
    for (const auto& dest : {dc, pdf}) {
      struct stat outputStat;
      if (::stat(dest.c_str(), &outputStat) == 0) {
        if (!S_ISREG(outputStat.st_mode))
          throw std::runtime_error("Output is not a regular file: " + dest);
        if (gSystem->AccessPathName(dest.c_str(), kWritePermission))
          throw std::runtime_error("Output is not writable: " + dest);
        for (const auto& source : {in, out, tdc, win}) {
          if (source.empty()) continue;
          struct stat inputStat;
          if (::stat(source.c_str(), &inputStat) == 0 &&
              inputStat.st_dev == outputStat.st_dev &&
              inputStat.st_ino == outputStat.st_ino)
            throw std::runtime_error("Output must not overwrite an input file");
        }
      }
      if (gSystem->AccessPathName(gSystem->DirName(dest.c_str()), kWritePermission))
        throw std::runtime_error("Output parent directory is missing or not writable: " + dest);
    }

    struct stat dcStat, pdfStat;
    if (dc == pdf ||
        (::stat(dc.c_str(), &dcStat) == 0 && ::stat(pdf.c_str(), &pdfStat) == 0 &&
         dcStat.st_dev == pdfStat.st_dev && dcStat.st_ino == pdfStat.st_ino))
      throw std::runtime_error("DCTDC and PDF outputs must be different files");

    auto results = Run(in.c_str(), out.c_str(), tdc.c_str(), dc.c_str(), pdf.c_str(),
                       win.c_str(), minFitCounts, searchHalfwidth, maxT0Error,
                       stabilityTol, plateauOrder, earlyNs);
    std::cout << "DCTDC saved: " << dc << "\nPDF saved: " << pdf
              << "\nSigned drift-time shift: " << earlyNs << " ns\n";
    return results;
  }
}

// ============================================================================
// 4. 引数から入力パスと出力名を組み立てる
// ============================================================================
namespace DCTdcT0Early {
  // ディレクトリにもファイル名にも使える。%sは省略可能。
  inline std::string runPattern(const char* pattern, int run,
                                const std::string& tag = "") {
    std::string value = pattern ? pattern : "";
    std::ostringstream number;
    number << std::setfill('0') << std::setw(5) << run;
    auto replace = [&](const std::string& token, const std::string& text) {
      size_t pos = 0;
      while ((pos = value.find(token, pos)) != std::string::npos) {
        value.replace(pos, token.size(), text);
        pos += text.size();
      }
    };
    replace("%05d", number.str());
    replace("%s", tag);
    if (value.empty() || value.find('%') != std::string::npos)
      throw std::runtime_error("Empty path or unsupported format (use %05d and optional %s)");
    return expandOutput(value.c_str());
  }

  inline std::string inputRootPath(const char* fileName, int run) {
    std::string name = expandOutput(fileName);
    if (name.empty() || name.find('/') != std::string::npos) return name;
    if (name.size() < 5 || name.substr(name.size() - 5) != ".root") {
      if (name.rfind("run", 0) != 0) name = "run" + name;
      name += ".root";
    }
    return runPattern(DCTdcT0Settings::kRootDirectory, run) + "/" + name;
  }

  inline std::string dctdcInputPath(const char* fileName, int run) {
    std::string name = expandOutput(fileName);
    if (name.empty()) throw std::runtime_error("Previous DCTDC filename is required");
    if (name.find('/') != std::string::npos) return name;
    return runPattern(DCTdcT0Settings::kDctdcDirectory, run) + "/" + name;
  }

  inline std::string shiftTag(double shiftNs) {
    if (!std::isfinite(shiftNs)) throw std::runtime_error("Time shift must be finite");
    if (shiftNs == 0) return "0ns";
    std::ostringstream value;
    value << std::setprecision(12) << std::abs(shiftNs);
    std::string tag = value.str();
    for (char& c : tag) {
      if (c == '.') c = 'p';
      else if (c == '-') c = 'm';
      else if (c == '+') c = 'p';
    }
    return std::string(shiftNs < 0 ? "m" : "p") + tag + "ns";
  }

  inline std::string outputName(const char* pattern, int run, const std::string& tag) {
    const auto name = runPattern(pattern, run, tag);
    if (name.find('/') != std::string::npos)
      throw std::runtime_error("Output filename must not contain a directory");
    return name;
  }
}

// ============================================================================
// 5. ROOTから呼ぶ関数
//    In ROOT、Out ROOT、run番号、時間シフト[ns]、前回のDCTDC
// ============================================================================
void make_dctdc_t0(
    const char* inRoot = "07162_In",
    const char* outRoot = "07162_Out",
    int run = 7162,
    double timeShiftNs = 1.5,
    const char* previousDctdc = "DCTDC.old") {
  try {
    using namespace DCTdcT0Early;
    if (run <= 0) throw std::runtime_error("run must be positive");
    const auto tag = shiftTag(timeShiftNs);
    const auto dcName = outputName(DCTdcT0Settings::kDctdcNameFormat, run, tag);
    const auto pdfName = outputName(DCTdcT0Settings::kPdfNameFormat, run, tag);
    const auto dcOutput = runPattern(DCTdcT0Settings::kDctdcDirectory, run) + "/" + dcName;
    const auto pdfDirectory = runPattern(DCTdcT0Settings::kPdfDirectory, run);
    const auto in = inputRootPath(inRoot, run);
    const auto out = inputRootPath(outRoot, run);
    const auto previous = dctdcInputPath(previousDctdc, run);
    WriteTwoFiles(in.c_str(), out.c_str(), previous.c_str(), dcOutput.c_str(),
                  timeShiftNs, pdfName.c_str(), pdfDirectory.c_str());
  } catch (const std::exception& e) {
    Error("make_dctdc_t0", "%s", e.what());
    gSystem->Exit(1);
  }
}
