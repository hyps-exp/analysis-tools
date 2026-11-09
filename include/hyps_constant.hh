#ifndef HYPS_CONSTANT_HH_
#define HYPS_CONSTANT_HH_

#include <array>
#include <ostream>
#include <string_view>
#include <type_traits>

//------------------------------------------------------------------------------
// det_id: Detector IDs for HDPRM (same value with DIGIT)
//------------------------------------------------------------------------------
namespace det_id {

  constexpr int kRF     =  1;
  constexpr int kTAGSF  =  2;
  constexpr int kTAGPL  =  3;
  constexpr int kUpVeto = 10;
  constexpr int kT0     = 11;
  constexpr int kSAC    = 12;
  constexpr int kEVeto  = 13;
  constexpr int kTOF    = 14;
  constexpr int kSDC0   = 21;
  constexpr int kSDC1   = 22;
  constexpr int kSDC2   = 23;
  constexpr int kSDC3   = 24;
  constexpr int kCFT    = 31;
  constexpr int kBGO    = 32;
  constexpr int kPiID   = 33;

} // namespace det_id

//------------------------------------------------------------------------------
// geom_info: Geometry Information for DCGEO
//   This is the single source for layer IDs, layer names and channel counts.
//   Do not duplicate these numbers elsewhere; refer to geom_info::kXXX[i].
//------------------------------------------------------------------------------
namespace geom_info {

  struct PlaneInfo {
    int id;
    std::string_view detector_name;
    std::string_view plane_name;
    int n_channels = 1;
  };

  constexpr auto kSDC0 = std::array{
    PlaneInfo{1, "SDC0", "X",  112},
    PlaneInfo{2, "SDC0", "XP", 112},
    PlaneInfo{3, "SDC0", "U",  112},
    PlaneInfo{4, "SDC0", "UP", 112},
  };

  constexpr auto kSDC1 = std::array{
    PlaneInfo{ 5, "SDC1", "XPP", 48},
    PlaneInfo{ 6, "SDC1", "V",   48},
    PlaneInfo{ 7, "SDC1", "UP",  48},
    PlaneInfo{ 8, "SDC1", "U",   48},
    PlaneInfo{ 9, "SDC1", "XP",  48},
    PlaneInfo{10, "SDC1", "X",   48},
  };

  constexpr auto kSDC2 = std::array{
    PlaneInfo{31, "SDC2", "V",   79},
    PlaneInfo{32, "SDC2", "UP",  78},
    PlaneInfo{33, "SDC2", "U",   78},
    PlaneInfo{34, "SDC2", "XP", 104},
    PlaneInfo{35, "SDC2", "X",  104},
  };

  constexpr auto kSDC3 = std::array{
    PlaneInfo{36, "SDC3", "V",   79},
    PlaneInfo{37, "SDC3", "UP",  78},
    PlaneInfo{38, "SDC3", "U",   78},
    PlaneInfo{39, "SDC3", "XP", 104},
    PlaneInfo{40, "SDC3", "X",  104},
  };

  constexpr auto kCFT = std::array{
    PlaneInfo{301, "CFT", "UV1",  426},
    PlaneInfo{302, "CFT", "PHI1", 584},
    PlaneInfo{303, "CFT", "UV2",  472},
    PlaneInfo{304, "CFT", "PHI2", 692},
    PlaneInfo{305, "CFT", "UV3",  510},
    PlaneInfo{306, "CFT", "PHI3", 800},
    PlaneInfo{307, "CFT", "UV4",  538},
    PlaneInfo{308, "CFT", "PHI4", 910},
  };

  constexpr auto kVP = std::array{
    PlaneInfo{16, "VP", "1"},            // n_channels = 1 (default)
  };

} // namespace geom_info

//------------------------------------------------------------------------------
// hist_id: Histogram IDs
//------------------------------------------------------------------------------
namespace hist_id {

  constexpr int kRF     = 10000;
  constexpr int kTAGSF  = 20000;
  constexpr int kTAGPL  = 30000;
  constexpr int kUpVeto = 40000;
  constexpr int kT0     = 50000;
  constexpr int kSAC    = 60000;
  constexpr int kEVeto  = 70000;
  constexpr int kTOF    = 80000;

} // namespace hist_id

//------------------------------------------------------------------------------
// readout: Readout channel identifiers (side / signal type)
//------------------------------------------------------------------------------
namespace readout {

  enum class UpOrDown    : int {kUp, kDown, kCount};
  enum class LeftOrRight : int {kLeft, kRight, kCount};
  enum class AdcOrTdc    : int {kAdcHigh, kTdc, kAdcLow, kTdc2, kCount}; // same as HodoParamMan::eAorT
  enum class HighOrLow   : int {kHigh, kLow, kCount};

  // enum classes that can be streamed as their underlying integer.
  // Add a line here when a new enum class needs operator<<.
  template <typename T>
  constexpr bool is_printable_enum()
  {
    return std::is_same_v<T, UpOrDown>    ||
           std::is_same_v<T, LeftOrRight> ||
           std::is_same_v<T, AdcOrTdc>    ||
           std::is_same_v<T, HighOrLow>;
  }

  template <typename T, std::enable_if_t<is_printable_enum<T>(), int> = 0>
  std::ostream& operator<<(std::ostream& os, T v)
  {
    return os << static_cast<std::underlying_type_t<T>>(v);
  }

} // namespace readout

//------------------------------------------------------------------------------
// module_lsb: Time per count of readout modules [ns]
//   Values are positive. Sign in parameter files:
//     HDPRM TDC : p1 = -LSB (Common Stop)
//     DCTDC     : p1 = +LSB (minus is applied later: dt = p[0] - ctime)
//------------------------------------------------------------------------------
namespace module_lsb {

  constexpr double kHulLrTdc    = 0.8333;      // true value 5/6 ns; 0.8333 adopted
  constexpr double kHulHrTdc    = 1.0 / 1024;  // 0.0009765625 ns
  constexpr double kVeasirocTdc = 1.0;

} // namespace module_lsb

//------------------------------------------------------------------------------
// Global Constants
//------------------------------------------------------------------------------
constexpr int kNumOfSegRF      =  1;
constexpr int kNumOfSegTAGSF   = 55;
constexpr int kNumOfSegTAGPL   = 10;
constexpr int kNumOfSegUpVeto  =  1;
constexpr int kNumOfSegT0      =  1;
constexpr int kNumOfSegSAC     =  5;   // 4 seg + SUM
constexpr int kNumOfSegEVeto   =  1;
constexpr int kNumOfSegTOF     = 48;
constexpr int kNumOfSegBGO     = 24;
constexpr int kNumOfSegPiID    = 32;

// Layer counts are always derived from geom_info (never hand-written).
constexpr int kNumOfLayersSDC0   = static_cast<int>(geom_info::kSDC0.size());
constexpr int kNumOfLayersSDC1   = static_cast<int>(geom_info::kSDC1.size());
constexpr int kNumOfLayersSDC2   = static_cast<int>(geom_info::kSDC2.size());
constexpr int kNumOfLayersSDC3   = static_cast<int>(geom_info::kSDC3.size());
constexpr int kNumOfLayersCFT    = static_cast<int>(geom_info::kCFT.size());
constexpr int kNumOfLayersSDCIn  = kNumOfLayersSDC0 + kNumOfLayersSDC1;
constexpr int kNumOfLayersSDCOut = kNumOfLayersSDC2 + kNumOfLayersSDC3;

#endif // HYPS_CONSTANT_HH_
