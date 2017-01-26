#! /bin/bash
set -e

UTILS=../../utils

gcc -lasan -fsanitize=address -g aom_tf.c av1/common/av1_fwd_txfm2d.c av1/common/av1_fwd_txfm1d.c av1/common/av1_inv_txfm2d.c av1/common/av1_inv_txfm1d.c $UTILS/luma2png.c $UTILS/vidinput.c $UTILS/y4m_input.c -I$UTILS/ -I../../ -I. -lpng -o aom_tf

./aom_tf ~/Videos/owl.y4m 4 0 0
convert -comment 'DCT -> TF -> iDCT\n 4x4 -> 8x8'  aom_tf_4.png  dct_4.png
./aom_tf ~/Videos/owl.y4m 8 0 0
convert -comment 'DCT -> TF -> iDCT\n 8x8 -> 16x16'  aom_tf_8.png  dct_8.png
./aom_tf ~/Videos/owl.y4m 16 0 0
convert -comment 'DCT -> TF -> iDCT\n 16x16 -> 32x32'  aom_tf_16.png  dct_16.png

./aom_tf ~/Videos/owl.y4m 4 3 3
convert -comment 'ADST -> TF -> ADST\n 4x4 -> 8x8'  aom_tf_4.png  adst_4.png
./aom_tf ~/Videos/owl.y4m 8 3 3
convert -comment 'ADST -> TF -> ADST\n 8x8 -> 16x16'  aom_tf_8.png  adst_8.png
./aom_tf ~/Videos/owl.y4m 16 3 3
convert -comment 'ADST -> TF -> ADST\n 16x16 -> 32x32'  aom_tf_16.png  adst_16.png

./aom_tf ~/Videos/owl.y4m 4 3 0
convert -comment 'ADST -> TF -> iDCT\n 4x4 -> 8x8'  aom_tf_4.png  adst_idct_4.png
./aom_tf ~/Videos/owl.y4m 8 3 0
convert -comment 'ADST -> TF -> iDCT\n 8x8 -> 16x16'  aom_tf_8.png  adst_idct_8.png
./aom_tf ~/Videos/owl.y4m 16 3 0
convert -comment 'ADST -> TF -> iDCT\n 16x16 -> 32x32'  aom_tf_16.png  adst_idct_16.png

montage -font "DejaVu-Sans-Bold" -pointsize 36 -geometry 640x480 -tile 3x3 -title "TF and ADST" -label '%c' dct_4.png adst_4.png adst_idct_4.png dct_8.png adst_8.png adst_idct_8.png dct_16.png adst_16.png adst_idct_16.png sidebyside.png

eog sidebyside.png
