#!/bin/bash

custom="--custom:12:64:5"           # for tournament of perceptron and gshare
# custom="--tournament:13:12:11"        # beats all but fp_1
perceptron="--perceptron:164:5"
tournament="--tournament:9:10:10"
gshare="--gshare:13"

fEq() {
    if (( $(echo "$1 == $2" |bc -l) )); then
        echo ✅
    else
        echo ❌
    fi
}

make
printf "%s\t\t%s\t\t%s\t\t%s\t\t%s\n" "File name" "Custom" "Percep" "Tour" "GShare"
for f in ../traces/*.bz2; do
    c=$(bunzip2 -kc $f | ./predictor $custom | grep Mis | cut -d":" -f 2)
    p=$(bunzip2 -kc $f | ./predictor $perceptron | grep Mis | cut -d":" -f 2)
    t=$(bunzip2 -kc $f | ./predictor $tournament | grep Mis | cut -d":" -f 2)
    g=$(bunzip2 -kc $f | ./predictor $gshare | grep Mis | cut -d":" -f 2)
    sm=$c
    if (( $(echo "$p < $sm" |bc -l) )); then sm=$p; fi
    if (( $(echo "$t < $sm" |bc -l) )); then sm=$t; fi
    if (( $(echo "$g < $sm" |bc -l) )); then sm=$g; fi
    printf "%s\t%06.3f %s\t%06.3f %s\t%06.3f %s\t%06.3f %s\n" $f $c $(fEq $c $sm) $p $(fEq $p $sm) $t $(fEq $t $sm) $g $(fEq $g $sm)
done
