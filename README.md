## param tools

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
├── script/                    # Helper scripts
├── output     -> ../hyps-analyzer/dev/root/demooutput      (symlink)
│   └── run07013/
│       ├── T0_param_run07013.txt
│       ├── TOF_ADC_param_run07013.txt
│       ├── TOF_TDC_param_run07013.txt
│       ├── TOF_ttof-cstof_param_run07013.txt
│       └── TOF_ttof-stof_param_run07013.txt
├── root       -> ../hyps-analyzer/dev/root/demoparammake   (symlink)
│   └── run07013/
│       ├── run07013_Hodoscope.root
│       ├── run07013_Hodoscope_pedrun.root
│       └── run07013_HypsTracking.root
└── runmanager -> ../hyps-analyzer/dev/runmanager           (symlink)
```