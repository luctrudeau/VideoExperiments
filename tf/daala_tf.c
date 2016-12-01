#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dct.h"
#include "vidinput.h"
#include "intra.h"

#include "utils/luma2png.h"

#define LUMA_PLANE (0)

int main(int _argc,char **_argv) {
  int x, y;
  int bx, by; // Position inside the block
  int fx, fy; // Position inside the frame
  int row; // Current row

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
  const int block_size = 8;
  const int big_block_size = block_size << 1;
  const int block_square = block_size * block_size;
  const int image_square = height * width;

  od_coeff block[block_square];
  od_coeff *dct_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);
  od_coeff *tf_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);
  od_coeff *idct_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);

  uint8_t *out = (uint8_t*) malloc(sizeof(uint8_t) * image_square);

  for (y = 0; y < height; y += block_size) {
    for (x = 0; x < width; x +=  block_size) {

      // Copy 8 bit pixels into a block of 32 bit coeffs
      for (by = 0; by < block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + x;
          block[by * block_size + bx] = f[LUMA_PLANE].data[fy * width + fx];
        }
      }

      // DCT Transform the 32 bit block coeffs
      od_bin_fdct8x8(&dct_image[y * width + x], width, block, block_size);
    }
  }

  for (y = 0; y < height; y += big_block_size) {
    int row = y * width;
    for (x = 0; x < width; x += big_block_size) {
      // Perform TF on 4 transformed blocks
      od_tf_up_hv_lp(&tf_image[row + x], width, &dct_image[row + x], width,
		      block_size, block_size, block_size);

      // Inverse transform the TF block
      od_bin_idct16x16(&idct_image[row + x], width, &tf_image[row + x], width);
    }
  }

  for (y = 0; y < height; y++) {
    int row = y * width;
    for (x = 0; x < width; x++) {

      // Clamp the 32 bit block coeffs to 8 bit
      int val = idct_image[row + x];
      if (val < 0) val = 0;
      if (val > 255) val = 255;

      // Copy the 32 bit block coeffs to 8 bit pixels
      out[row + x] = val;
    }
  }

  luma2png("daala_tf.png", out, width, height);

  free(dct_image);
  free(tf_image);
  free(idct_image);
}
