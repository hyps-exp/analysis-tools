> [!NOTE]
> This is an example of README.md for each detector.
> Please copy this, and edit as you want.

# (Detector Name) Calibration
Please describe the rule of the directory briefly.

## Generated Patameters
- HDPRM
- HDPHC

## Required Inputs
- Rootfile created by `UserHodoscope`
  - For HDPRM: 1 run
  - For HDPHC: More than 30M events (~ 3-5 runs)

## Procedure
If you have multiple steps, please describe them here.
You can use Numbered list as follows:
0. Run `UserHodoscope` ([details](#0-generate-rootfiles))
1. Execute `Foo.C` ([details](#1-make-hdprm))
2. Execute `Bar.C`[details](#2-check-the-results)
3. Re-run `UserHodoscope` [details](#3-re-run-the-analyer)

...

## How to Use
Please explain how to use each macro here.

### 0. Generate rootfiles
At first, you need to generate the rootfile for the input.
Go to the `hyps-analyzer` and run the `UserHodoscope`.
Note that you should specify the correct conf file.

### 1. Make HDPRM
To make HDPRM, execute the `Foo.C`.
```bash
root -l -b -q Foo.C
```

If there is any problem, the error message will be printed.

### 2. Check the fit result
The fit result is printed as a PDF file.
The path for the file is,
`analysis-tools/output/pdf/run<run number>/Foo.pdf`

### 3. Re-run the analyzer
After checkig the fit result, you should re-run the analyzer
using new parameters you created.

---
## *Please delete the below section before you publish*

## Adding and Link Images
You can add an image and link it as follows:


[Figure 1](#fig1) shows the fit result of pedestal and MIP.

<p align="center" id="fig1">
  <img src="../images/test.png" alt="Fit result of pedestal and MIP" width="480">
  <br>
  <em>Figure 1. Fit result of pedestal and MIP.</em>
</p>

I you want to use an image, please put it inside `analysis-tools/images/`.
> [!NOTE]
> **Only `PNG` files are acceptable for images.**
