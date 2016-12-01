#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <assert.h>
#include "dct.h"
#include "vidinput.h"
#include "intra.h"


void luma2png(char *filename, od_coeff *luma, int width, int height) {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_GRAY,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int val = luma[y * width + x];
      if (val < 0) val = 0;
      if (val > 255) val = 255;
      row_pointers[y][x] = val;
    }
  }

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);
}


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

  luma2png("daala_tf.png", idct_image, width, height);

  free(dct_image);
  free(tf_image);
  free(idct_image);
}
