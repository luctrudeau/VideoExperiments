#! /bin/bash
set -e

gcc -g scaling.c aom/aom_dsp/fwd_txfm.c aom/aom_dsp/inv_txfm.c -Iaom/ -Iaom/build/ -o scaling

./scaling
