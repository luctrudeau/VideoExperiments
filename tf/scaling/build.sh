#! /bin/bash
set -e

DAALA=~/Workspace/daala
AOM=~/Workspace/aom

gcc -g aom.c $AOM/av1/common/cfl.c $AOM/aom_dsp/fwd_txfm.c $AOM/aom_dsp/inv_txfm.c -I$AOM/build -I$AOM/ -I../../ -o aom


gcc -g daala.c $DAALA/src/dct.c $DAALA/src/intra.c $DAALA/src/tf.c -I$DAALA/tools/ -I/$DAALA/src -I$DAALA/ -I../../ -o daala

./daala 4
#./daala 8
#./daala 16
./aom 4
#./aom 8
#./aom 16
