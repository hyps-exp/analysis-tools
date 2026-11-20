## param tools

## Requirements

The scripts in `script/` run in the conda environment that is used for
the Analyzer. `script/calib_run_map.py` needs the `tomli` package in
that environment (Python 3.10 or older):

```
conda activate <your environment>
conda install tomli
```

## Calibration run map

`calib-run-map/<detector>.toml` tells, for each parameter of one
detector, which runs are analyzed to make it and for which runs it is
used. See [calib-run-map/README.md](calib-run-map/README.md) for how to
write and check a map.

Read the maps through `script/calib_run_map.py`, not by parsing the
files, so that the file format can change without changing the scripts.
The first argument is the detector name, that is, the file name without
`.toml`:

```python
from calib_run_map import CalibRunMapError, find_entry

try:
    entry = find_entry('TOF', 'HDPHC', 7005)   # the entry used for run 7005
except CalibRunMapError as e:           # the map is wrong, or no entry covers 7005
    sys.exit(f'error: {e}')

entry.valid_from, entry.valid_to        # runs that use the parameter (valid_to is None if written as -1)
entry.is_open_ended                     # True if valid_to is None
entry.filename_run                      # run number for the name of the output file
entry.input_first, entry.input_last     # runs to analyze
entry.input_runs                        # the same, as a list of runs
entry.pedestal_run                      # pedestal run (None if the entry has no pedestal_run)
```

`load_map('TOF')` returns all the entries of a map as
`{parameter name: [entry, ...]}`.

## Directory Structure

```
analysis_tools/
├── README.md
├── .gitignore
├── fit_range/                 # Fit range settings
│   ├── RF/
│   │   ├── RF_PHC_param.txt
│   │   └── rf_fit_conf.txt
│   └── TOF/
│       └── tof_mip_fit_range.txt
├── param-maker/               # Parameter generation tools
│   ├── CATCH/
│   │   ├── Makefile.org
│   │   ├── include/
│   │   │   ├── DetectorID.hh
│   │   │   └── PathID.hh
│   │   └── src/
│   │       ├── 01_CATCH_HDPRM_TDC/
│   │       │   ├── bgo_hdprm_tdc.cc
│   │       │   ├── cft_hdprm_tdc.cc
│   │       │   └── piid_hdprm_tdc.cc
│   │       ├── 02_CFT_PedCor/
│   │       │   ├── makePedestalCorrection.cc
│   │       │   ├── makePedestalCorrection2.cc
│   │       │   ├── pedestal_channel.txt -> pedestal_channel_01215.txt
│   │       │   ├── pedestal_channel_01215.txt
│   │       │   └── writePedCh.cc
│   │       └── 03_CFT_PosCor/
│   │           ├── 01_PHI/
│   │           │   ├── MergePhiPosCor.cc
│   │           │   └── calPhiResidual_16layer.cc
│   │           └── 02_UV/
│   │               ├── calPosUV_lin.cc
│   │               ├── calPosUV_point.cc
│   │               ├── calPosUV_res.cc
│   │               └── calPosUV_res2.cc
│   ├── RF/
│   │   ├── make_rf_param.C
│   │   └── make_rf_phc_param.C
│   ├── SDC/
│   │   └── make_param/
│   │       ├── initial_XT_SDCIn.C
│   │       ├── initial_XT_SDCOut.C
│   │       ├── update_XT_SdcIn.C
│   │       └── update_XT_SdcOut.C
│   ├── T0/
│   │   └── update_T0_param.C
│   ├── TAG/
│   │   ├── EGammaCalib_share.cc
│   │   ├── check_tag_pl.C
│   │   └── check_tag_sf.C
│   └── TOF/
│       ├── make_TOF_ADC_param.C
│       ├── make_tof_ttof-cstof_param.C
│       ├── make_tof_ttof-stof_param.C
│       ├── make_tof_ud_param.C
│       └── update_PHC_TOF.C
├── script/                    # Analysis scripts (run with a run number)
│   ├── Initialize.sh           # build the CATCH executables (run first)
│   ├── sample.py                # template for new scripts
│   ├── catch_01_hdprm_tdc.py    # CATCH HDPRM TDC calibration
│   ├── catch_02_cft_pedcor.py   # CFT pedestal correction
│   ├── t0_01_update_param.py    # T0 ADC/TDC calibration
│   ├── tof_01_tdc_param.py      # TOF Up/Down TDC calibration
│   ├── tof_02_adc_param.py      # TOF ADC calibration
│   ├── tof_03_ttof_stof_param.py # TOF abs. timing (tTofCalc-stof); needs tof_01's output
│   ├── tof_04_phc_param.py      # TOF time-walk (PHC) calibration
│   ├── tof_05_ttof_cstof_param.py # TOF abs. timing (cstof-tTofCalc); needs tof_04's output
│   ├── rf_01_rf_param.py        # RF timing calibration
│   └── rf_02_rf_phc_param.py    # RF time-walk (PHC) calibration
├── output     -> ../hyps-analyzer/dev/root/demooutput      (symlink)
│   └── run07013/
│       ├── run07013_T0_HDPRM_ADC_TDC.txt
│       ├── run07013_TOF_HDPRM_ADC.txt
│       ├── run07013_TOF_HDPRM_TDC.txt
│       ├── run07013_TOF_HDPRM_ttof-stof.txt
│       ├── run07013_TOF_HDPHC.txt
│       ├── run07013_TOF_HDPHC_ttof-cstof.txt
│       ├── run07013_RF_HDPRM_TDC.txt
│       ├── run07013_RF_USER_Bunch.txt
│       └── run07013_RF_HDPHC.txt
├── root       -> ../hyps-analyzer/dev/root/demoparammake   (symlink)
│   └── run07013/
│       ├── run07013_Hodoscope.root
│       ├── run07013_Hodoscope_pedrun.root
│       └── run07013_HypsTracking.root
└── runmanager -> ../hyps-analyzer/dev/runmanager           (symlink)
```