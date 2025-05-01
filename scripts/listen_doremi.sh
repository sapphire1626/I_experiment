#!/bin/bash

nc -l 16260 | play -t raw -b 16 -c 1 -e s -r 44100 -