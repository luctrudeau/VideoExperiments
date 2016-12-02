#! /bin/bash
set -e

DAALA=~/Workspace/daala
AOM=~/Workspace/aom

gcc -g aom_tf.c $AOM/av1/common/cfl.c $AOM/aom_dsp/fwd_txfm.c $AOM/aom_dsp/inv_txfm.c ../../utils/luma2png.c $DAALA/tools/vidinput.c $DAALA/tools/y4m_input.c -I$DAALA/tools/ -I$AOM/build -I$AOM/ -I../../ -lpng -o aom_tf


gcc -g daala_tf.c $DAALA/src/dct.c $DAALA/src/intra.c $DAALA/src/tf.c $DAALA/tools/vidinput.c $DAALA/tools/y4m_input.c ../../utils/luma2png.c -I$DAALA/tools/ -I/$DAALA/src -I$DAALA/ -I../../ -lpng -o daala_tf

./aom_tf ~/Videos/owl.y4m 4
convert -comment 'aom 4x4'  aom_tf_4.png  aom_tf_4.png
./aom_tf ~/Videos/owl.y4m 8
convert -comment 'aom 8x8'  aom_tf_8.png  aom_tf_8.png
./aom_tf ~/Videos/owl.y4m 16
convert -comment 'aom 16x16'  aom_tf_16.png  aom_tf_16.png
./aom_tf ~/Videos/owl.y4m 32
convert -comment 'aom 32x32'  aom_tf_32.png  aom_tf_32.png

./daala_tf ~/Videos/owl.y4m 4
convert -comment 'daala 4x4'  daala_tf_4.png  daala_tf_4.png
./daala_tf ~/Videos/owl.y4m 8
convert -comment 'daala 8x8'  daala_tf_8.png  daala_tf_8.png
./daala_tf ~/Videos/owl.y4m 16
convert -comment 'daala 16x16'  daala_tf_16.png  daala_tf_16.png
./daala_tf ~/Videos/owl.y4m 32
convert -comment 'daala 32x32'  daala_tf_32.png  daala_tf_32.png

# Measure PNSR
#echo ================ Converting to PNGs to Y4Ms
#$DAALA/tools/png2y4m aom_tf.png -o aom_tf.y4m
#$DAALA/tools/png2y4m daala_tf.png -o daala_tf.y4m
#echo ================ PSNR:
#$DAALA/tools/dump_psnr aom_tf.y4m daala_tf.y4m

montage -font "DejaVu-Sans-Bold" -pointsize 36 -geometry 640x480 -tile 2x4 -title "AOM TF vs Daala TF" -label '%c' aom_tf_4.png daala_tf_4.png aom_tf_8.png daala_tf_8.png aom_tf_16.png daala_tf_16.png aom_tf_32.png daala_tf_32.png sidebyside.png

gpicview sidebyside.png
