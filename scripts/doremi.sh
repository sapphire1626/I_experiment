#!/bin/bash

sox ./data/doremi.wav -t raw -b 16 -c 1 -e s -r 44100 - \
| ./build/downsample "$1" | play -t raw -b 16 -c 1 -e s -r $((44100 / $1)) -