
#include <stdio.h>
#include <stdlib.h>
#include "./aom_dsp_rtcd.h"

void init_block(tran_low_t *const block, int block_size, int value) {
  int x, y;
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      block[y * block_size + x] = value;
    }
  }
}

void print_block(const tran_low_t *const block, int block_size) {
  int x, y;
  for (x = 0; x < block_size; x++) {
    printf("|   ");
  }
  printf("|\n");
  for (x = 0; x < block_size; x++) {
    printf("| --- ");
  }
  printf("|\n");

  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      printf("| %d ", block[y * block_size + x]);
    }
    printf("|\n");
  }
}

int main(int _argc,char **_argv) {

  int x,y;
  const int block_size = atoi(_argv[1]);
  const int block_square = block_size * block_size;
  const int big_block_size = block_size << 1;
  const int big_block_square = big_block_size * big_block_size;

  const int bottom_left = block_size * big_block_size;
  const int bottom_right = bottom_left + block_size;

  tran_low_t *block = (tran_low_t*) calloc(block_square, sizeof(tran_low_t));
  init_block(block, block_size, 127);

  printf("Pixel Values\n");
  print_block(block, block_size);
  printf("\n");

  tran_low_t *dct_block = (tran_low_t*) calloc(big_block_square, sizeof(tran_low_t));

  // Holds 4 transformed blocks
  tran_low_t *dct_big_block = (tran_low_t*) calloc(big_block_square,
		  sizeof(tran_low_t));

  uint8_t *idct_block = (uint8_t*) calloc(block_square, sizeof(uint8_t));

  // DCT function
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
      fprintf(stderr, "invalid block size\n");
      fprintf(stderr, "values are: 4, 8, 16, 32\n");
      return -1;
  }

  // dct transform the 16 bit block coeffs
  // top left
  dct(block, dct_block, block_size);
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      dct_big_block[y * big_block_size + x]
	= dct_block[y * block_size + x];
    }
  }
  // top right
  dct(block, dct_block, block_size);
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      dct_big_block[y * big_block_size + x + block_size]
        = dct_block[y * block_size + x];
    }
  }
  // bottom left
  dct(block, dct_block, block_size);
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      dct_big_block[bottom_left + y * big_block_size + x]
        = dct_block[y * block_size + x];
    }
  }
  // Bottom right
  dct(block, dct_block, block_size);
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      dct_big_block[bottom_right + y * big_block_size + x]
	= dct_block[y * block_size + x];
    }
  }

  printf("Transformed Coefficients\n");
  print_block(dct_big_block, big_block_size);
  printf("\n");

  // TF block (TF requires more bits for bigger transforms)
  tran_high_t *tf_block = (tran_high_t*) calloc(block_square,
		  sizeof(tran_high_t));
  // TF block scaled
  tran_low_t *tf_block_scaled = (tran_low_t*) calloc(block_square,
		  sizeof(tran_low_t));
  
  
  od_tf_up_hv_lp(tf_block, block_size, dct_big_block, big_block_size,
		      block_size, block_size, block_size);


  // Subsampling requires scaling
  for (by = 0; by < block_size; by++) {
    for (bx = 0; bx < block_size; bx++) {
      tf_block_scaled[by * block_size + bx] = tf_block[by * block_size + bx] >> 1;
    }
  }

  printf("TF\n");
  print_block(tf_block, block_size);
  printf("\n");

  idct(dct_block, idct_block, block_size);

  free(block);
  free(dct_block);
}
