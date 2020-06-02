[![Build Status](https://travis-ci.com/C0D3-M4513R/CD74HCT299.svg?branch=master)](https://travis-ci.com/C0D3-M4513R/CD74HCT299)

This is a c++ cli implementation for the CD74HCT299
(for the Raspberry Pi). 

Reading from IO Pins is currently WIP, but should work.
This Repo is currently restricted to Raspberry Pi only, because i'm currently using wiringPi.

## Compilation
If you want to compile this project, use those commands in order:
```
#cd
#cd Downloads
git clone https://github.com/C0D3-M4513R/CD74HCT299.git
cd CD74HCT299
cmake .
make
```
Afterwards you should find a file named untitled in the Directory CD74HCT299.
#Pins
The Following pins are used:
* 19 &larr; Q7
* 21: &rarr; DS0
* 23: &rarr; CP
* 24: &rarr; S0
* 26: &rarr; S1
