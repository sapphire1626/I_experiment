#!/bin/bash

sox ./data/doremi.wav -t raw -c 1 - | ./build/bandpass 8192 500 10000 |\
 play -t raw -b 16 -c 1 -e s -r 44100 -