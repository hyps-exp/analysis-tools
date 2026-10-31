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
#include <TH1.h>
#include <TH2.h>
#include <TChain.h>
#include <TChainElement.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TSpectrum.h>

#include "DetectorID.hh"
#include "PathID.hh"

// ref : cft_lowmipnew

const Int_t MaxDepth = 16;

const Int_t CId  = 31;
const Int_t AorT = 0;
const Int_t UorD = 1;
const Double_t p1 = -1;

// Peak search tuning
const Int_t    SnapWindow        = 5;   // search radius (bins) used to snap a reported peak position
                                         // to the true local maximum of the raw histogram nearby
const Int_t    SpikeNeighborWindow = 10; // how far to look when judging whether the lowest-ADC bin is a spike
const Double_t SpikeRelThreshold = 3.0; // clamp the lowest-ADC bin if it is this many times its neighborhood
const Int_t    TSpectrumMaxPeaks = 5;   // headroom for TSpectrum::Search before we sort/dedupe and cap at 2
const Double_t TSpectrumSigma    = 2.0; // expected peak width (bins) passed to TSpectrum::Search
const Double_t TSpectrumThreshold = 0.10; // ignore peaks below this fraction of the spectrum's own max

struct LocalMax{ Int_t bin; Double_t value; };

// Finds up to the first two local maxima of the histogram, using
// TSpectrum::Search (deconvolution-based, with its background estimation
// left on) to locate candidate positions -- this resolves real-but-shallow
// peaks sitting close to a much taller pedestal that a simple
// derivative/prominence scan can be too conservative to accept (an earlier
// version of this function rejected such peaks with its own valley/height
// check; TSpectrum's background-subtracted search already separates them
// correctly, so that extra check was dropped instead of overriding it).
// Each candidate is snapped to the true local maximum of the raw (rebinned)
// histogram content nearby, since TSpectrum's internal smoothing shifts and
// distorts peak heights; duplicate candidates that snap to the same raw bin
// are then merged.
// The lowest-ADC bin is temporarily clamped before the search if it stands
// out sharply from its neighborhood -- this is the common zero-suppressed
// "no hit" spike at the left edge of these ADC spectra, which would
// otherwise get picked up as a fake peak -- then restored immediately
// after, so the histogram itself (used later for the fit) is untouched.
// Returns the number of peaks found (0, 1, or 2) in peak_bin[0..n-1].
Int_t FindTwoPeaks(TH1D *h, Int_t peak_bin[2])
{
  const Int_t nbins = h->GetNbinsX();
  std::vector<Double_t> content(nbins + 1);
  for(Int_t bin = 1; bin <= nbins; bin++) content[bin] = h->GetBinContent(bin);

  const Double_t origBin1 = content[1];
  bool clamped = false;
  if(nbins >= 2){
    Double_t neighborMax = 0;
    for(Int_t bin = 2; bin <= std::min(nbins, 1 + SpikeNeighborWindow); bin++)
      neighborMax = std::max(neighborMax, content[bin]);
    if(content[1] > SpikeRelThreshold * neighborMax){
      h->SetBinContent(1, neighborMax);
      clamped = true;
    }
  }

  TSpectrum spectrum(TSpectrumMaxPeaks);
  const Int_t nfound = spectrum.Search(h, TSpectrumSigma, "nodraw", TSpectrumThreshold);
  if(clamped) h->SetBinContent(1, origBin1);

  std::vector<LocalMax> candidates;
  const Double_t *xpos = spectrum.GetPositionX();
  for(Int_t i = 0; i < nfound; i++){
    const Int_t approxBin = h->GetXaxis()->FindBin(xpos[i]);
    const Int_t lo = std::max(1, approxBin - SnapWindow);
    const Int_t hi = std::min(nbins, approxBin + SnapWindow);
    Int_t best = lo;
    for(Int_t bin = lo; bin <= hi; bin++) if(content[bin] > content[best]) best = bin;
    candidates.push_back({best, content[best]});
  }
  std::sort(candidates.begin(), candidates.end(),
      [](const LocalMax &a, const LocalMax &b){ return a.bin < b.bin; });
  candidates.erase(std::unique(candidates.begin(), candidates.end(),
      [](const LocalMax &a, const LocalMax &b){ return a.bin == b.bin; }), candidates.end());

  const Int_t nPeaks = std::min<Int_t>(candidates.size(), 2);
  for(Int_t i = 0; i < nPeaks; i++) peak_bin[i] = candidates[i].bin;

  return nPeaks;
}

// Estimates a Gaussian sigma for the peak at peak_bin from the half-maximum
// width of the (rebinned) histogram around it, so the fit range no longer
// depends on a hand-picked constant or the other peak's position.
// The half-max walk also stops at the nearest valley (where content starts
// rising again), not just at the half-max crossing: when a second, shorter
// peak sits close to a much taller one, the valley between them often never
// dips below half of the *shorter* peak's own height, so a half-max-only
// walk would cross straight through the valley into the neighboring peak
// and return a wildly inflated sigma.
Double_t EstimateSigma(TH1D *h, Int_t peak_bin)
{
  const Int_t nbins = h->GetNbinsX();
  const Double_t peakContent = h->GetBinContent(peak_bin);
  if(peakContent <= 0) return 0;

  const Double_t halfMax = peakContent / 2.0;
  Int_t lo = peak_bin;
  while(lo > 1 && h->GetBinContent(lo) > halfMax && h->GetBinContent(lo - 1) <= h->GetBinContent(lo)) lo--;
  Int_t hi = peak_bin;
  while(hi < nbins && h->GetBinContent(hi) > halfMax && h->GetBinContent(hi + 1) <= h->GetBinContent(hi)) hi++;

  const Double_t fwhm = h->GetBinCenter(hi) - h->GetBinCenter(lo);
  return fwhm / 2.3548; // FWHM = 2*sqrt(2*ln2)*sigma for a Gaussian
}

// Rejects a Gaussian fit whose mean/sigma couldn't possibly describe the
// histogram (e.g. mean outside the histogram's x-range, which happens when
// the fit range drifts onto a decaying tail with no real peak in it).
bool IsPhysicalFit(TH1D *h, Double_t mean, Double_t sig)
{
  if(sig <= 0) return false;
  return mean > h->GetXaxis()->GetXmin() && mean < h->GetXaxis()->GetXmax();
}

struct Event{
  Int_t ntCFT;
  Int_t MaxSegment[NumOfPlaneCFT][MaxDepth];
  Double_t MaxMipLow[NumOfPlaneCFT][MaxDepth];
  Double_t MaxAdcLow[NumOfPlaneCFT][MaxDepth];
  Double_t theta_cft[MaxDepth];
  Int_t segBGOt[NumOfSegBGO+1];
  Double_t energyBGO[NumOfSegBGO+1];
  Int_t segPiIDt[MaxDepth];
};
static Event event;


int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cout << "Usage : " << argv[0] << " <runnumber>"
	      << std::endl;
    return 0;
  }

  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << " " << std::endl;
  std::cout << "Start cft_lowmipnew" << std::endl;
  std::cout << "..." << std::endl;
  std::cout << ".." << std::endl;
  std::cout << "." << std::endl;
  time_t start_time, end_time;
  start_time = time(NULL);

  int runnumber = std::atoi(argv[1]);

  // data setup
  TString filename;
  filename.Form("%s/run%05d/cft_proton_mip_%05d.root", root_path.Data(), runnumber, runnumber);
  TFile *fin = TFile::Open(filename, "READ");
  if( !fin || fin->IsZombie() ){
    std::cerr << "#E : Cannot open " << filename << std::endl;
    return 1;
  }
  fin->cd();

  filename.Form("%s/run%05d/catch_cft_lowmip_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream ofs(filename);
  ofs << "### CFT LowGain MIP" << std::endl;

  filename.Form("%s/run%05d/catch_error_cft_lowmip_%05d.txt", txt_path.Data(), runnumber, runnumber);
  std::ofstream output(filename);
  output << "### CFT LowGain MIP" << std::endl;

  // analysis (search MIP)

  TString hist, title, cond, cut;
  TCanvas *c1 = new TCanvas("c1", "c1", 800, 1200);
  c1->Divide(4, 8);
  filename.Form("%s/run%05d/catch_cft_lowgain_mip_%05d.pdf", pdf_path.Data(), runnumber, runnumber);
  c1->Print(filename + "[", "pdf");
  TH1D *h1[NumOfPlaneCFT][NumOfSegCFT[7]] = {nullptr};
  TF1 *f1[NumOfPlaneCFT][NumOfSegCFT[7]];

  std::cout << "#D Start drawing and fitting hists" << std::endl;
  for(Int_t layer=0; layer<NumOfPlaneCFT; layer++){
    if(layer%2 == 0){
      ofs << "# UV " << layer/2 + 1 << std::endl;
      output << "# UV " << layer/2 + 1 << std::endl;
    }else{
      ofs << "# PHI " << (layer-1)/2 + 1 << std::endl;
      output << "# PHI " << (layer-1)/2 + 1 << std::endl;
    }
    for(Int_t seg=0; seg<NumOfSegCFT[layer]; seg++){
      std::cout << "" << std::endl;
      std::cout << "" << std::endl;
      std::cout << "layer : " << layer << ", seg : " << seg << std::endl;
      std::cout << "" << std::endl;
      Int_t num = seg%32 + 1;
      c1->cd(num);

      hist.Form("h%d%03d", layer+1, seg);
      h1[layer][seg] = (TH1D *)fin->Get(hist);

      Double_t N    = 0;
      Double_t mean = 0;
      Double_t sig  = 0;

      if(!h1[layer][seg] || h1[layer][seg]->GetEntries()==0){
	output << "#E : histogram " << hist << " not found (layer " << layer
	       << ", seg " << seg << ")" << std::endl;
	output << layer << "\t" << seg << std::endl;
      }else{
	h1[layer][seg]->Rebin(20);
	h1[layer][seg]->Draw();

	int peak_bin[2] = {0, 0};
	Int_t nPeaks = FindTwoPeaks(h1[layer][seg], peak_bin);

	// peak_bin[0] is the lower-ADC peak (pedestal-side), peak_bin[1] the
	// higher-ADC one (MIP); the MIP peak is what we fit.
	Int_t mipBin = (nPeaks >= 2) ? peak_bin[1] : peak_bin[0];
	double peak1 = 0;
	double sigma1 = 0;
	if(nPeaks == 0){
	  output << "#E : could not find a peak (layer " << layer
		 << ", seg " << seg << ")" << std::endl;
	  output << layer << "\t" << seg << std::endl;
	}else{
	  peak1 = h1[layer][seg]->GetBinCenter(mipBin);
	  sigma1 = EstimateSigma(h1[layer][seg], mipBin);
	  if(sigma1<=0){
	    output << "#E : could not estimate peak width (layer " << layer
		   << ", seg " << seg << ")" << std::endl;
	    output << layer << "\t" << seg << std::endl;
	  }
	}

	// Fit : 1st time
	hist.Form("f%d%03d", layer, seg);
	f1[layer][seg] = new TF1(hist, "gaus");

	if(sigma1>0){
	  std::cout << "fit range : " << peak1-sigma1 << " - " <<  peak1+4*sigma1 << std::endl;
	  h1[layer][seg]->Fit(hist, "", "", peak1-sigma1, peak1+4*sigma1);
	  if(h1[layer][seg]->GetFunction(hist)){
	    mean = f1[layer][seg]->GetParameter(1);
	    sig  = f1[layer][seg]->GetParameter(2);
	  }

	  if(!IsPhysicalFit(h1[layer][seg], mean, sig)){
	    output << "#E : 1st-pass fit diverged (mean=" << mean << ", sig=" << sig
		   << ") (layer " << layer << ", seg " << seg << ")" << std::endl;
	    output << layer << "\t" << seg << std::endl;
	    mean = 0; sig = 0;
	  }else{
	    // Fit : 2nd time
	    h1[layer][seg]->Fit(hist, "", "", mean-2*sig, mean+2*sig);
	    if(h1[layer][seg]->GetFunction(hist)){
	      N    = f1[layer][seg]->GetParameter(0);
	      mean = f1[layer][seg]->GetParameter(1);
	      sig  = f1[layer][seg]->GetParameter(2);
	    }

	    if(!IsPhysicalFit(h1[layer][seg], mean, sig)){
	      output << "#E : 2nd-pass fit diverged (mean=" << mean << ", sig=" << sig
		     << ") (layer " << layer << ", seg " << seg << ")" << std::endl;
	      output << layer << "\t" << seg << std::endl;
	      N = 0; mean = 0; sig = 0;
	    }
	  }
	}

	TString write = Form("mean : %03f", mean);
	TLatex tex;
	tex.SetTextSize(0.1);
	tex.SetTextColor(kBlack);
	tex.DrawLatex(500, N*1.5, write);
      }


      ofs << CId << "\t" << layer << "\t" << seg << "\t" << AorT << "\t" << UorD << "\t"
	  << 0 << "\t"
	  << std::fixed << std::setprecision(3) << mean << std::endl;

      if(num == 32 || seg == NumOfSegCFT[layer] - 1){
	c1->Print(filename, "pdf");
      }
    }
  }
  c1->Print(filename + "]", "pdf");

  end_time = time(NULL);

  std::cout << "#D End this program" << std::endl;
  std::cout << "   Start time   : " << start_time << std::endl;
  std::cout << "   End time     : " << end_time << std::endl;
  std::cout << "   Process time : " << end_time - start_time << std::endl;

  return 0;

}
