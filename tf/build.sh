#! /bin/bash
set -e

DAALA=~/Workspace/daala
AOM=~/Workspace/aom

gcc -g aom_tf.c $AOM/av1/common/cfl.c $AOM/aom_dsp/fwd_txfm.c $AOM/aom_dsp/inv_txfm.c ../utils/luma2png.c $DAALA/tools/vidinput.c $DAALA/tools/y4m_input.c -I$DAALA/tools/ -I$AOM/build -I$AOM/ -I../ -lpng -o aom_tf

./aom_tf ~/Videos/owl.y4m

gcc -g daala_tf.c $DAALA/src/dct.c $DAALA/src/intra.c $DAALA/src/tf.c $DAALA/tools/vidinput.c $DAALA/tools/y4m_input.c ../utils/luma2png.c -I$DAALA/tools/ -I/$DAALA/src -I$DAALA/ -I../ -lpng -o daala_tf

./daala_tf ~/Videos/owl.y4m

# Measure PNSR
echo ================ Converting to PNGs to Y4Ms
$DAALA/tools/png2y4m aom_tf.png -o aom_tf.y4m
$DAALA/tools/png2y4m daala_tf.png -o daala_tf.y4m
echo ================ PSNR:
$DAALA/tools/dump_psnr aom_tf.y4m daala_tf.y4m

montage -font "DejaVu-Sans-Bold" -geometry 640x480 -title "AOM TF vs Daala TF" aom_tf.png daala_tf.png sidebyside.png

gpicview sidebyside.png
