#! /bin/bash
set -e

DAALA=~/Workspace/daala

gcc -g aom.c aom/aom_dsp/fwd_txfm.c aom/aom_dsp/inv_txfm.c -Iaom/ -Iaom/build -o aom_dct

#gcc -g daala.c $DAALA/src/dct.c $DAALA/src/intra.c $DAALA/src/tf.c -I$DAALA/tools/ -I/$DAALA/src -I$DAALA/ -I../../ -o daala

#./daala 4
#./daala 8
#./daala 16
#./daala 32
#./aom 4
#./aom 8
#./aom 16
./aom_dct 4
