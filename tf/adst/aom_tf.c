/*
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vidinput.h"

#include "av1/common/av1_fwd_txfm1d.h"

#include "utils/luma2png.h"

#define LUMA_PLANE (0)

/* Prototypes of the AV1 transform functions */
void av1_fwd_txfm2d_4x4_c(const int16_t *input, int32_t *output, int stride,
    int tx_type, int bd);

void av1_fwd_txfm2d_8x8_c(const int16_t *input, int32_t *output, int stride,
                          int tx_type, int bd);

void av1_fwd_txfm2d_16x16_c(const int16_t *input, int32_t *output, int stride,
    int tx_type, int bd);

void av1_inv_txfm2d_add_8x8_c(const int32_t *input, uint16_t *output,
    int stride, int tx_type, int bd);

void av1_inv_txfm2d_add_16x16_c(const int32_t *input, uint16_t *output,
                              int stride, int tx_type, int bd);

void av1_inv_txfm2d_add_32x32_c(const int32_t *input, uint16_t *output,
    int stride, int tx_type, int bd);

/*This is an in-place, reversible, orthonormal Haar transform in 7 adds,
  1 shift (2 operations per sample).
  It is its own inverse (but requires swapping lh and hl on one side for
  bit-exact reversibility).
  It is defined in a macro here so it can be reused in various places.*/
#define OD_HAAR_KERNEL(ll, lh, hl, hh) \
  do { \
    tran_high_t llmhh_2__; \
    (ll) += (hl); \
    (hh) -= (lh); \
    llmhh_2__ = ((ll) - (hh)) >> 1; \
    (lh) = llmhh_2__ - (lh); \
    (hl) = llmhh_2__ - (hl); \
    (ll) -= (lh); \
    (hh) += (hl); \
  } \
while(0)


/*Increase horizontal and vertical frequency resolution of a 2x2 group of
  nxn blocks, combining them into a single 2nx2n block.*/
void od_tf_up_hv(tran_high_t *dst, int dstride, const tran_high_t *const  src,
    int sstride, int n) {
  int x;
  int y;
  for (y = 0; y < n; y++) {
    int vswap;
    vswap = y & 1;
    for (x = 0; x < n; x++) {
      tran_high_t ll;
      tran_high_t lh;
      tran_high_t hl;
      tran_high_t hh;
      int hswap;
      ll = src[y*sstride + x];
      lh = src[y*sstride + x + n];
      hl = src[(y + n)*sstride + x];
      hh = src[(y + n)*sstride + x + n];
      /*We have to swap lh and hl for exact reversibility with od_tf_up_down.*/
      OD_HAAR_KERNEL(ll, hl, lh, hh);
      hswap = x & 1;
      dst[(2*y + vswap)*dstride + 2*x + hswap] = ll;
      dst[(2*y + vswap)*dstride + 2*x + 1 - hswap] = lh;
      dst[(2*y + 1 - vswap)*dstride + 2*x + hswap] = hl;
      dst[(2*y + 1 - vswap)*dstride + 2*x + 1 - hswap] = hh;
    }
  }
}

int main(int _argc,char **_argv) {
  int x,y, i, j;
  int bx,by; // position inside the block
  int fx,fy; // Position inside frame
  int bbx, bby;

  if (_argc != 5) {
    fprintf(stderr, "Invalid number of arguments!\n");
    fprintf(stderr, "usage: image.y4m blocksize\n");
    return -1;
  }

  // Open Y4M
  FILE *fin = fopen(_argv[1], "rb");
  printf("Opening %s Block size %dx%d\n", _argv[1], atoi(_argv[2]), atoi(_argv[2]));
  video_input vid;
  video_input_info info;
  video_input_open(&vid,fin);
  video_input_get_info(&vid,&info);
  video_input_ycbcr f;
  int ret1=video_input_fetch_frame(&vid,f,NULL);

  const int height = info.pic_h;
  const int width = info.pic_w;
  const int image_square = height * width;
  const int out_width = width;
  const int out_height = height;
  const int out_image_square = out_height * out_width;

  const int block_size = atoi(_argv[2]);
  const int fwd_tran_type = atoi(_argv[3]);
  const int inv_tran_type = atoi(_argv[4]);

  const int block_square = block_size * block_size;
  const int big_block_size = block_size << 1;
  const int big_block_square = big_block_size * big_block_size;
  const int bottom_left = block_size * big_block_size;
  const int bottom_right = bottom_left + block_size;

  // Original block copied from Y4M
  tran_low_t *block = (tran_low_t*) calloc(big_block_square,
		  sizeof(tran_low_t));
  // Holds 1 transformed blocks
  tran_high_t *dct_block = (tran_high_t*) calloc(block_square,
		  sizeof(tran_high_t));
  // Holds 4 transformed blocks
  tran_high_t *big_dct_block = (tran_high_t*) calloc(big_block_square,
      sizeof(tran_high_t));
  // Holds the TF Merge
  tran_high_t *tf_block = (tran_high_t*) calloc(big_block_square,
      sizeof(tran_high_t));
  // Inverse transformed block
  tran_low_t *idct_block = (tran_low_t*) calloc(big_block_square,
		  sizeof(tran_low_t));
  // Final pixel block
  uint8_t *out = (uint8_t*) calloc(out_image_square,
		  sizeof(uint8_t));

  void (*dct)(const int16_t *input, int32_t *output, int stride, int tx_type,
      int bd);
  void (*idct)(const int32_t *input, uint16_t *output, int stride, int tx_type,
      int bd);


  switch (block_size)
  {
    case 4:
      dct = &av1_fwd_txfm2d_4x4_c;
      idct = &av1_inv_txfm2d_add_8x8_c;
      break;
    case 8:
      dct = &av1_fwd_txfm2d_8x8_c;
      idct = &av1_inv_txfm2d_add_16x16_c;
      break;
    case 16:
      dct = &av1_fwd_txfm2d_16x16_c;
      idct = &av1_inv_txfm2d_add_32x32_c;
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

      for (bby = 0; bby < big_block_size; bby += block_size) {
        for (bbx = 0; bbx < big_block_size; bbx+= block_size) {
          dct(&block[big_block_size * bby + bbx], dct_block, big_block_size, fwd_tran_type, 0);

          for (by = 0; by < block_size; by++) {
            for (bx = 0; bx < block_size; bx++) {
              big_dct_block[big_block_size * (bby+by) + (bbx + bx)] =
                dct_block[block_size * by + bx];
              idct_block[big_block_size * (bby+by) + (bbx + bx)] = 0;
            }
          }
        }
      }

      od_tf_up_hv(tf_block, big_block_size, big_dct_block, big_block_size,
                  block_size);

      idct(tf_block, idct_block, big_block_size, inv_tran_type, 14);

      for (by = 0; by < big_block_size; by++) {
        fy = y + by;
        for (bx = 0; bx < big_block_size; bx++) {
          fx = x + bx;

          // Copy the 32 bit block coeffs to 8 bit pixels
          out[out_width * fy + fx] = idct_block[big_block_size * by + bx];
        }
      }
    }
  }

  video_input_close(&vid);
  char* out_filename;
  asprintf(&out_filename, "aom_tf_%d.png", block_size);
  luma2png(out_filename, out, out_width, out_height);
  free(out_filename);

  free(block);
  free(dct_block);
  free(big_dct_block);
  free(tf_block);
  free(idct_block);
  free(out);
}
