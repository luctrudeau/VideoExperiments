#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dct.h"
#include "vidinput.h"
#include "intra.h"

#include "utils/luma2png.h"

int main(int _argc,char **_argv) {
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
  const int block_square = block_size * block_size;
  const int image_square = height * width;

  od_coeff block[block_square];
  od_coeff *dct_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);
  od_coeff *tf_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);
  od_coeff *idct_image = (od_coeff*) malloc(sizeof(od_coeff)* image_square);

  uint8_t *out = (uint8_t*) malloc(sizeof(uint8_t) * image_square);

  for (int y = 0; y < height; y += block_size) {
    for (int x = 0; x < width; x +=  block_size) {
      for (int by = 0; by < block_size; by++) {
	int fy = by + y;
	// Copy pixels 8 bit pixels into a block of 32 bit coeff.
        for (int bx = 0; bx < block_size; bx++) {
	  int fx = bx + x;
          block[by * block_size + bx] = f[0].data[fy * width + fx];
        }
      }
      // 8x8 Transform
      od_bin_fdct8x8(&dct_image[y * width + x], width, block, block_size);
    }
  }

  for (int y = 0; y < height; y += 16) {
    int row = y * width;
    for (int x = 0; x < width; x += 16) {
      od_tf_up_hv_lp(&tf_image[row + x], width, &dct_image[row + x], width,
		      block_size, block_size, block_size);
      od_bin_idct16x16(&idct_image[row + x], width, &tf_image[row + x], width);
    }
  }

  for (int y = 0; y < height; y++) {
    int row = y * width;
    for (int x = 0; x < width; x++) {
      int val = idct_image[row + x];
      if (val < 0) val = 0;
      if (val > 255) val = 255;
      out[row + x] = val;
    }
  }

  luma2png("daala_tf.png", out, width, height);

  free(dct_image);
  free(tf_image);
  free(idct_image);
}
