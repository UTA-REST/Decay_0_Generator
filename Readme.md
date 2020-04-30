# Decay_0_Generator

Decay0 is a generator for double beta decay and neutrinoless doube beta decay events. It was originally written in fortran, this version was translated into C++ by Paul Lebrun for the NEXT collaboration. 
further documentation can be found here 
http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.261.4646&rep=rep1&type=pdf

## Installation

Decay0 needs GSL at the moment. Version 2.6 is currently working (http://gnu.mirror.constant.com/gsl/). and can be installed by downloading, unzipping moving into the folder

```bash
sudo make clean
sudo chown -R $USER .
./configure && make
make install
```

## Usage

The code can be compiled by 
```bash
g++ -std=c++11 *.cpp -o Run_Decay0 -lgsl
```
This outputs the 2 betas energy, decay time, a momentum vector.


