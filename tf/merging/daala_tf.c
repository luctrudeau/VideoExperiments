/*
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>

#include "dct.h"
#include "vidinput.h"
#include "intra.h"

#include "utils/luma2png.h"

#define LUMA_PLANE (0)

int main(int _argc,char **_argv) {
  int x, y;
  int bx, by; // Position inside the block
  int fx, fy; // Position inside the frame

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
  od_coeff *block = (od_coeff*) calloc(big_block_square, sizeof(od_coeff));
  // Holds 4 transformed blocks
  od_coeff *dct_block = (od_coeff*) calloc(big_block_square, sizeof(od_coeff));
  // TF block
  od_coeff *tf_block = (od_coeff*) calloc(block_square, sizeof(od_coeff));
  // Inverse transformed block
  od_coeff *idct_block = (od_coeff*) calloc(block_square, sizeof(od_coeff));
  // Final pixel block
  uint8_t *out = (uint8_t*) calloc(out_image_square, sizeof(uint8_t));

  // DCT function
  void (*dct)(od_coeff*, int, const od_coeff*, int);
  void (*idct)(od_coeff*, int, const od_coeff*, int);

  switch (block_size)
  {
    case 4:
      dct = &od_bin_fdct4x4;
      idct = &od_bin_idct4x4;
      break;
    case 8:
      dct = &od_bin_fdct8x8;
      idct = &od_bin_idct8x8;
      break;
    case 16:
      dct = &od_bin_fdct16x16;
      idct = &od_bin_idct16x16;
      break;
    case 32:
      dct = &od_bin_fdct32x32;
      idct = &od_bin_idct32x32;
      break;
    default:
      fprintf(stderr, "Invalid block size\n");
      fprintf(stderr, "Values are: 4, 8, 16, 32\n");
      return -1;
  }

  for (y = 0; y < height; y += big_block_size) {
    for (x = 0; x < width; x +=  big_block_size) {

      // Copy 8 bit pixels into a block of 32 bit coeffs
      for (by = 0; by < big_block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < big_block_size; bx++) {
	  fx = bx + x;
          block[by * big_block_size + bx] = f[LUMA_PLANE].data[fy * width + fx];
        }
      }

      // DCT Transform the 32 bit block coeffs
      // Top left
      dct(dct_block, big_block_size, block, big_block_size);
      // Top right
      dct(&dct_block[block_size], big_block_size, &block[block_size], big_block_size);
      // Bottom left
      dct(&dct_block[bottom_left], big_block_size, &block[bottom_left], big_block_size);
      // Bottom right
      dct(&dct_block[bottom_right], big_block_size, &block[bottom_right], big_block_size);

      // TF merge 4 blocks (a big_block) into 1 block
      od_tf_up_hv_lp(tf_block, block_size, dct_block, big_block_size,
		      block_size, block_size, block_size);

      // Daala requires scaling the TF by half because its transforms are unit
      // scale.
      for (by = 0; by < block_size; by++) {
        for (bx = 0; bx < block_size; bx++) {
	  tf_block[by * block_size + bx] >>= 1;
	}
      }

      // Inverse transform the TF block
      idct(idct_block, block_size, tf_block, block_size);

      for (by = 0; by < block_size; by++) {
	fy = by + (y >> 1);
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + (x >> 1);
          // Clamp the 32 bit block coeffs to 8 bit
          int val = idct_block[by * block_size + bx];
          if (val < 0) val = 0;
          if (val > 255) val = 255;

          // Copy the 32 bit block coeffs to 8 bit pixels
          out[fy * out_width + fx] = val;
        }
      }
    }
  }

  char* out_filename;
  asprintf(&out_filename, "daala_tf_%d.png", block_size);
  luma2png(out_filename, out, out_width, out_height);
  free(out_filename);

  free(block);
  free(dct_block);
  free(tf_block);
  free(idct_block);
  free(out);
}
