## param tools

## Directory Structure

```
analysis_tools/
├── README.md
├── .gitignore
├── fit_range/                 # Fit range settings
│   └── TOF/
│       └── tof_mip_fit_range.txt
├── param-maker/               # Parameter generation tools
│   ├── CATCH/
│   │   ├── Makefile.org
│   │   └── src/
│   │       ├── 01_CATCH_HDPRM_TDC/
│   │       │   ├── bgo_hdprm_tdc.cc
│   │       │   ├── cft_hdprm_tdc.cc
│   │       │   └── piid_hdprm_tdc.cc
│   │       └── 02_CFT_PedCor/
│   │           ├── makePedestalCorrection.cc
│   │           ├── makePedestalCorrection2.cc
│   │           ├── pedestal_channel.txt -> pedestal_channel_01215.txt
│   │           ├── pedestal_channel_01215.txt
│   │           └── writePedCh.cc
│   ├── SDC/
│   │   └── make_param/
│   │       ├── initial_XT_SDCIn.C
│   │       ├── initial_XT_SDCOut.C
│   │       ├── update_XT_SdcIn.C
│   │       └── update_XT_SdcOut.C
│   ├── T0/
│   │   └── update_T0_param.C
│   ├── TAG/
│   └── TOF/
│       ├── make_TOF_ADC_param.C
│       ├── make_tof_ttof-cstof_param.C
│       ├── make_tof_ttof-stof_param.C
│       └── make_tof_ud_param.C
├── script/                    # Helper scripts
├── output     -> ../hyps-analyzer/dev/root/demooutput      (symlink)
│  └── run07013/
│      ├── T0_param_run07013.txt
│      ├── TOF_ADC_param_run07013.txt
│      ├── TOF_TDC_param_run07013.txt
│      ├── TOF_ttof-cstof_param_run07013.txt
│      └── TOF_ttof-stof_param_run07013.txt
├── root       -> ../hyps-analyzer/dev/root/demoparammake   (symlink)
│ 　└── run07013/
│       ├── run07013_Hodoscope.root
│       ├── run07013_Hodoscope_pedrun.root
│       └── run07013_HypsTracking.root
└── runmanager -> ../hyps-analyzer/dev/runmanager           (symlink)
```