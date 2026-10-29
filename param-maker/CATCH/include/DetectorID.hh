// -*- C++ -*-

#ifndef DETECTOR_ID_HH
#define DETECTOR_ID_HH

#include <iostream>
#include <map>
#include <vector>
#include <array>
#include <TString.h>

// Counters ___________________________________________________________
const Int_t NumOfSegTagSF  = 55;//HYPS
const Int_t NumOfSegTagPL  = 10;//HYPS
const Int_t NumOfSegRF     =  1;//HYPS
const Int_t NumOfSegU_Veto =  1;//HYPS
const Int_t NumOfSegT0     =  1;//HYPS
const Int_t NumOfSegSAC    =  5;
const Int_t NumOfSegE_Veto =  1;//HYPS
const Int_t NumOfSegTOF    = 48; // HYPS

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
const std::vector<int> NumOfSegCFT = {426,584,472,692,510,800,538,910};
const double offsetCATCH = 155;


// BGO
const Double_t BGO_X = 30.;
const Double_t BGO_Y = 25.;
const Double_t BGO_Z = 400.;
const Int_t    NumOfBGOUnit = 8;
const Int_t    NumOfBGOInOneUnit = 2;//pair unit
const Double_t RadiusOfBGOSurface = 100.;
const Int_t    NumOfBGOInOneUnit2 = 1;//single unit
const Double_t RadiusOfBGOSurface2 = 120.;
const Int_t NumOfSegBGO = NumOfBGOUnit*(NumOfBGOInOneUnit+NumOfBGOInOneUnit2);//24

// PiID counter
const Int_t NumOfSegPiID =  32;

const Int_t NumOfPiIDUnit = 8;
const Int_t NumOfPiIDInOneUnit = 3;
const Double_t PiID_X = 30.;
const Double_t PiID_Y = 10.;
const Double_t PiID_Z = 400.;
const Double_t RadiusOfPiIDSurface = 164.;

const Int_t    NumOfPiIDInOneUnit2 = 1;//single unit
const Double_t PiID2_X = 40.;
const Double_t PiID2_Y = 10.;
const Double_t PiID2_Z = 400.;
const Double_t RadiusOfPiID2Surface = 180.;

#endif
