# An Efficient Gustavson-based Sparse Matrix-matrix Multiplication Accelerator on Embedded FPGAs

Shield: [![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

This work is licensed under a
[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License][cc-by-nc-sa].

[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg

We implement and evaluate the design on the Xilinx ZCU106 platform. We synthesis and generate the bitstream using the Xilinx Vitis toolchain v2022.1.

+ The ***Preprocessing*** includes the python code of converting .mtx file to binaries.
+ The ***HLS*** folder includes the source code of SpMM design. 
+ The ***SDK*** folder includes the driver code for the FPGA design in the Vitis IDE.  

Currently, two cases are not supported: 1) empty rows exist; 2) the number of partial results exceed 1000. We will support these cases in the future.