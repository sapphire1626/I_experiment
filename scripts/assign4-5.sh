#!/bin/bash

sox ./data/obama.wav -t raw -c 1 - | ./build/bandpass 8192 10 20000 |\
 play -t raw -b 16 -c 1 -e s -r 44100 -