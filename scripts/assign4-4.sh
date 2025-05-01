#!/bin/bash

sox $1 -t raw -c 1 - | ./build/fft 8192 | play -t raw -b 16 -c 1 -e s -r 44100 -