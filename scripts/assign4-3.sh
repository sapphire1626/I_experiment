#!/bin/bash

./build/sin 50000 3528 1000 > ./data/sin.raw

./build/read_data2 ./data/sin.raw > ./data/sin.data