#!/bin/bash
./build/i1i2i3_phone 10.100.60.52 $1 $2 server | play -t raw -b 16 -c 1 -e s -r 44100 -