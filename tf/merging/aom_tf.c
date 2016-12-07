/*
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vidinput.h"
#include "./aom_dsp_rtcd.h"
#include "av1/common/cfl.h"


#include "utils/luma2png.h"

#define LUMA_PLANE (0)

int main(int _argc,char **_argv) {
  int x,y;
  int bx,by; // Position inside the block
  int fx,fy; // Position inside frame

  if (_argc != 3) {
    fprintf(stderr, "Invalid number of arguments!\n");
    fprintf(stderr, "usage: image.y4m blocksize\n");
    return -1;
  }

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

  const int block_size = atoi(_argv[2]);
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
  // TF block (TF requires more bits for bigger transforms)
  tran_high_t *tf_block = (tran_high_t*) calloc(block_square,
		  sizeof(tran_high_t));
  // TF block scaled
  tran_low_t *tf_block_scaled = (tran_low_t*) calloc(block_square,
		  sizeof(tran_low_t));
  // Inverse transformed block
  uint8_t *idct_block = (uint8_t*) calloc(block_square,
		  sizeof(tran_low_t));
  // Final pixel block
  uint8_t *out = (uint8_t*) calloc(out_image_square,
		  sizeof(uint8_t));

  void (*dct)(const int16_t*, tran_low_t*, int);
  void (*idct)(const tran_low_t*, uint8_t *, int);

  switch (block_size)
  {
    case 4:
      dct = &aom_fdct4x4_c;
      idct = &aom_idct4x4_16_add_c;
      break;
    case 8:
      dct = &aom_fdct8x8_c;
      idct = &aom_idct8x8_64_add_c;
      break;
    case 16:
      dct = &aom_fdct16x16_c;
      idct = &aom_idct16x16_256_add_c;
      break;
    case 32:
      dct = &aom_fdct32x32_c;
      idct = &aom_idct32x32_1024_add_c;
      break;
    default:
      fprintf(stderr, "Invalid block size\n");
      fprintf(stderr, "Values are: 4, 8, 16, 32\n");
      return -1;
  }
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
      dct(block, dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }

      // Top right
      dct(&block[block_size], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[by * big_block_size + bx + block_size]
		  = dct_block[by * block_size + bx];
	}
      }
      // Bottom left
      dct(&block[bottom_left], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[bottom_left + by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }
      // Bottom right
      dct(&block[bottom_right], dct_block, big_block_size);
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
          dct_big_block[bottom_right + by * big_block_size + bx]
		  = dct_block[by * block_size + bx];
	}
      }
      // TF merge 4 blocks (a big_block) into 1 block
      od_tf_up_hv_lp(tf_block, block_size, dct_big_block, big_block_size,
		      block_size, block_size, block_size);

      // Subsampling requires scaling
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
	  tf_block_scaled[by * block_size + bx] = tf_block[by * block_size + bx] >> 1;
	}
      }

      memset(idct_block, 0, sizeof(uint8_t) * block_square);
      idct(tf_block_scaled, idct_block, block_size);

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

  char* out_filename;
  asprintf(&out_filename, "aom_tf_%d.png", block_size);
  luma2png(out_filename, out, out_width, out_height);
  free(out_filename);

  free(block);
  free(dct_block);
  free(dct_big_block);
  free(idct_block);
  free(out);
}
