#! /bin/bash
set -e

gcc -g adst2dct.c aom/aom_dsp/fwd_txfm.c aom/aom_dsp/inv_txfm.c -Iaom/ -Iaom/build/ -o adst2dct

./adst2dct
