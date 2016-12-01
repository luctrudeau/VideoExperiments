/*
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vidinput.h"
#include "aom_dsp/fwd_txfm.h"
#include "aom_dsp/inv_txfm.h"

#include "utils/luma2png.h"

#define LUMA_PLANE (0)

int main(int _argc,char **_argv) {
  int x,y;
  int bx,by; // Position inside the block
  int fx,fy; // Position inside frame

  // Open Y4M
  FILE *fin = fopen(_argv[1], "rb");
  video_input vid;
  video_input_info info;
  video_input_open(&vid,fin);
  video_input_get_info(&vid,&info);
  video_input_ycbcr f;
  int ret1=video_input_fetch_frame(&vid,f,NULL);

  const int height = info.pic_h;
  const int width = info.pic_w;
  const int image_square = height * width;
  // Out image is subsampled by 2
  const int out_width = width >> 1;
  const int out_height = height >> 1;
  const int out_image_square = out_height * out_width;

  const int block_size = 8;
  const int block_square = block_size * block_size;
  const int big_block_size = block_size << 1;
  const int big_block_square = big_block_size * big_block_size;
  const int bottom_left = block_size * big_block_size;
  const int bottom_right = bottom_left + block_size;

  // Original block copied from Y4M
  tran_low_t *block = (tran_low_t*) calloc(big_block_square,
		  sizeof(tran_low_t));
  // Holds 1 transformed blocks
  tran_low_t *dct_block = (tran_low_t*) calloc(block_square,
		  sizeof(tran_low_t));
  // Holds 4 transformed blocks
  tran_low_t *dct_big_block = (tran_low_t*) calloc(big_block_square,
		  sizeof(tran_low_t));
  // TF block
  tran_low_t *tf_block = (tran_low_t*) calloc(block_square,
		  sizeof(tran_low_t));
  // Inverse transformed block
  uint8_t *idct_block = (uint8_t*) calloc(block_square,
		  sizeof(tran_low_t));
  // Final pixel block
  uint8_t *out = (uint8_t*) calloc(out_image_square,
		  sizeof(uint8_t));

  for (y = 0; y < height; y += big_block_size) {
    for (x = 0; x < width; x +=  big_block_size) {

      // Copy 8 bit pixels into 16 bit block coeff
      for (by = 0; by < big_block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < big_block_size; bx++) {
	  fx = bx + x;
          block[by * big_block_size + bx] = f[LUMA_PLANE].data[fy * width + fx];
        }
      }

      // DCT Transform the 16 bit block coeffs
      // Top left
      aom_fdct8x8_c(block, dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }

      // Top right
      aom_fdct8x8_c(&block[block_size], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[by * big_block_size + bx + block_size]
		  = dct_block[by * block_size + bx];
	}
      }
      // Bottom left
      aom_fdct8x8_c(&block[bottom_left], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[bottom_left + by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }
      // Bottom right
      aom_fdct8x8_c(&block[bottom_right], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[bottom_right + by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }
      // TF merge 4 8x8 into a 16x16
      od_tf_up_hv_lp(tf_block, block_size, dct_big_block, big_block_size,
		      block_size, block_size, block_size);

      memset(idct_block, 0, sizeof(uint8_t) * block_square);
      aom_idct8x8_64_add_c(tf_block, idct_block, block_size);

      for (by = 0; by < block_size; by++) {
	fy = by + (y >> 1);
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + (x >> 1);

          // Copy the 32 bit block coeffs to 8 bit pixels
          out[fy * out_width + fx] = idct_block[by * block_size + bx];
        }
      }
    }
  }

  luma2png("aom_tf.png", out, out_width, out_height);

  free(block);
  free(dct_block);
  free(dct_big_block);
  free(idct_block);
  free(out);
}
