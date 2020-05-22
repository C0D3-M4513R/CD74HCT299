This is a c++ cli implementation for the CD74HCT299
(for the Raspberry Pi). 

Reading from IO Pins is currently a WIP, and the only thing, 
restricting this repo to Raspberry pi-only, because reading uses the wiringPi library.

This cli allows setting the state of the parallel Pins, 
but getting the state of the parallel Pins doesn't work. 