#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>

#include "vidinput.h"
#include "aom_dsp/fwd_txfm.h"

void luma2png(char *filename, uint8_t *luma, int width, int height) {
  int x,y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
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

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);
  png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      row_pointers[y][x] = luma[y * width + x];
    }
  }

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(y = 0; y < height; y++) {
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

  tran_low_t block[64] = {0}; // original block copied in from Y4M
  tran_low_t dct_block[64] = {0}; // DCT transformed block
  tran_low_t tf_block[256] = {0}; // TF block
  uint8_t idct_block[256] = {0}; // inverse DCT transform block

  // DCT transformed image
  tran_low_t *dct_image = (tran_low_t*) malloc(sizeof(tran_low_t) * image_square);
  // inverse DCT transformed image
  uint8_t *idct_image = (uint8_t*) malloc(sizeof(uint8_t) * image_square);

  int x,y;
  int bx,by; // index inside block
  int fx,fy; // index inside frame

  for (y = 0; y < height; y += block_size) {
    for (x = 0; x < width; x +=  block_size) {

      // Copy 8 bit pixels into 16 bit block coeff
      for (by = 0; by < block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + x;
          block[by * block_size + bx] = f[0].data[fy * width + fx];
        }
      }

      // 8x8 DCT Transform
      aom_fdct8x8_c(block, dct_block, block_size);

      // Copy DCT block into image
      for (by = 0; by < block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + x;
          dct_image[fy * width + fx] = dct_block[by * block_size + bx];
        }
      }
    }
  }

  for (y = 0; y < height; y += 16) {
    int row = y * width;
    for (x = 0; x < width; x += 16) {

      // TF merge 4 8x8 into a 16x16
      od_tf_up_hv_lp(tf_block, 16, &dct_image[row + x], width,
		      block_size, block_size, block_size);

      for (by = 0; by < 16; by++) {
        for (bx = 0; bx < 16; bx++) {
	  tf_block[by * 16 + bx] *= 2;
	}
      }

      memset(idct_block, 0, sizeof(uint8_t)*16*16);
      aom_idct16x16_256_add_c(tf_block, idct_block, 16);

      for (by = 0; by < 16; by++) {
	fy = by + y;
        for (bx = 0; bx < 16; bx++) {
	  fx = bx + x;
          idct_image[fy * width + fx] = idct_block[by * 16 + bx];
        }
      }
    }
  }

  /*for (y = 0; y < height; y += block_size) {
    for (x = 0; x < width; x += block_size) {

      // Copy DCT Coeff into block
      for (by = 0; by < block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + x;
          dct_block[by * block_size + bx] = dct_image[fy * width + fx];
        }
      }

      // Set idct_block to zero because inverse transform also adds.
      memset(idct_block, 0, sizeof(uint8_t) * block_square);

      //aom_idct16x16_10_add_c(dct_block, idct_block, block_size);
      aom_idct8x8_64_add_c(dct_block, idct_block, block_size);

      for (by = 0; by < block_size; by++) {
	fy = by + y;
        for (bx = 0; bx < block_size; bx++) {
	  fx = bx + x;
          idct_image[fy * width + fx] = idct_block[by * block_size + bx];
        }
      }
    }
  }*/
  luma2png("aom_tf.png", idct_image, width, height);

  free(dct_image);
  free(idct_image);
}
