#!/bin/bash -e

for i in *.json; do
    [ -f "$i" ] || break
    ./json2h.py $i
done
