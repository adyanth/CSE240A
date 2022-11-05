#!/bin/bash

make
find ../traces -name *.bz2 -exec sh -c "echo {}; bunzip2 -kc {} | ./predictor $@ && echo Done" \;
