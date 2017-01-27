
#include <stdio.h>
#include <stdlib.h>
#include "./aom_dsp_rtcd.h"
#include <assert.h>
#include <string.h>

void init_block(tran_low_t *const block, int block_size, int value) {
  int x, y;
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      block[y * block_size + x] = value;
    }
  }
}

int main(int _argc,char **_argv) {

  int x,y;
  int dc;
  const int block_size = atoi(_argv[1]);
  const int block_square = block_size * block_size;

  // DCT function
  void (*dct)(const int16_t*, tran_low_t*, int);
  void (*fdct1)(const int16_t*, tran_low_t*, int);
  void (*idct)(const int16_t*, uint8_t*, int);

  tran_low_t *block = (tran_low_t*) calloc(block_square, sizeof(tran_low_t));

  tran_low_t *dct_block = (tran_low_t*) calloc(block_square, sizeof(tran_low_t));

  uint8_t *idct_block = (uint8_t*) calloc(block_square, sizeof(uint8_t));

  switch (block_size)
  {
    case 4:
      dct = &aom_fdct4x4_c;
      fdct1 = &aom_fdct4x4_1_c;
      idct = &aom_idct4x4_16_add_c;
      break;
    case 8:
      dct = &aom_fdct8x8_c;
      fdct1 = &aom_fdct8x8_1_c;
      idct = &aom_idct8x8_64_add_c;
      break;
    case 16:
      dct = &aom_fdct16x16_c;
      fdct1 = &aom_fdct16x16_1_c;
      idct = &aom_idct16x16_256_add_c;
      break;
    case 32:
      dct = &aom_fdct32x32_c;
      fdct1 = &aom_fdct32x32_1_c;
      idct = &aom_idct32x32_1024_add_c;
      break;
    default:
      fprintf(stderr, "invalid block size\n");
      fprintf(stderr, "values are: 4, 8, 16, 32\n");
      return -1;
  }

  FILE *fid = fopen("results.csv", "w");
  FILE *fid_error = fopen("error.csv", "w");
  fprintf(fid, "Pixel, DCT DC, DCT_1 DC, FAST DC, FAST DC IDCT\n");
  fprintf(fid_error, "DCT_DC vs DCT_1_DC Error\n");
  int err_fdct = 0;
  int err_proposed = 0;
  for (int v = 0; v < 256; v += 1) {

    init_block(block, block_size, v);
    dct(block, dct_block, block_size);
    const int dct_dc = dct_block[0];

    memset(idct_block, 0, sizeof(uint8_t)*block_square);
    idct(dct_block, idct_block, block_size);
    for (x = 0; x < block_square; x++)
    {
      assert(idct_block[x] == v);
    }

    memset(dct_block, 0, sizeof(tran_low_t)*block_square);
    memset(idct_block, 0, sizeof(uint8_t)*block_square);
    fdct1(block, dct_block, block_size);
    const int fdct_dc = dct_block[0];
    idct(dct_block, idct_block, block_size);
    for (x = 0; x < block_square; x++)
    {
      assert(idct_block[x] == v);
    }

    memset(dct_block, 0, sizeof(tran_low_t)*block_square);
    memset(idct_block, 0, sizeof(uint8_t)*block_square);
    if (block_size < 32) {
      dct_block[0] = block[0] * block_size << 3;
    } else {
      dct_block[0] = block[0] * 128;
    }
    const int proposed = dct_block[0];
    idct(dct_block, idct_block, block_size);
    for (x = 0; x < block_square; x++)
    {
      assert(idct_block[x] == v);
    }
    const int proposed_inv = idct_block[0];

    fprintf(fid, "%d, %d, %d, %d, %d\n", v, dct_dc, fdct_dc, proposed,
        proposed_inv);
    fprintf(fid_error, "%d\n", abs(dct_dc - fdct_dc));
    err_fdct += abs(dct_dc - fdct_dc);
    err_proposed += abs(proposed - fdct_dc);
  }
  fclose(fid);
  fclose(fid_error);

  printf("Sum of the absolute error between DCT_DC and DCT_1_DC: %d\n",
      err_fdct);

  printf("Sum of the absolute error between DCT_1_DC and FAST DC: %d\n",
      err_proposed);

  free(block);
  free(dct_block);
}
