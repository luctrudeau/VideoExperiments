#! /bin/bash
set -e

gcc -g fastdc.c aom/aom_dsp/fwd_txfm.c aom/aom_dsp/inv_txfm.c -Iaom/ -Iaom/build/ -o fastdc

./fastdc 4
gnuplot hist.dem
convert -comment '4x4 Flat Blocks' hist.png  hist4.png

./fastdc 8
gnuplot hist.dem
convert -comment '8x8 Flat Blocks' hist.png  hist8.png

./fastdc 16
gnuplot hist.dem
convert -comment '16x16 Flat Blocks' hist.png  hist16.png

./fastdc 32
gnuplot hist.dem
convert -comment '32x32 Flat Blocks' hist.png  hist32.png

montage -font "DejaVu-Sans-Bold" -pointsize 36 -geometry 600x300 -tile 1x4 -title "DC Error" -label '%c' hist4.png hist8.png hist16.png hist32.png sidebyside.png

eog sidebyside.png
