# <detector name> calibration
11;rgb:0000/0000/0000This is an example of README.md for each detector.
Please copy this, and edit as you want.

## Overview
Please describe the rule of the directory briefly.

## Calibration Procedure
If you have multiple steps, please describe them here.
You can use Numbered list as follows:
1. Run `UserHodoscope`
2. Execute `Foo.C`
3. Execute `Bar.C`

...

## How to Use
Please explain how to use each macro here.

### 1. Generate a rootfile
nAt first, you need to generate the rootfile for the input.
Go to the `hyps-analyzer` and run the `UserHodoscope`.
Note that you should specify the correct conf file.

### 2. Make HDPRM
To make HDPRM, execute the `Foo.C`.
```bash
root -l -b -q Foo.C
```

If there is any problem, the error message will be printed.

### 3. Check the result
The fit result is printed as a PDF file.
The path for the file is,
`analysis-tools/output/pdf/run<run number>/Foo.pdf`

[Figure 1](#fig1) shows the fit result of pedestal and MIP.

<p align="center">
  <img src="../images/test.png" alt="Fit result of pedestal and MIP" width="480">
  <br>
  <em>Figure 1. Fit result of pedestal and MIP.</em>
</p>

** If you want to use an image, please put it inside `analysis-tools/images/`.
Only `PNG` files are acceptable.**
