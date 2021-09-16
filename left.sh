#!/bin/sh

avr-size $1 | tail -1 | cut -c1-20 | (read i j&& echo $((1024 * 7 - (i + j))) "bytes left")