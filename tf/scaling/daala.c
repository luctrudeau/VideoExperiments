#include <stdio.h>
#include <stdlib.h>
#include "dct.h"

void init_block(od_coeff *const block, int block_size, int value) {
  int x, y;
  for (y = 0; y < block_size; y++) {
    for (x = 0; x < block_size; x++) {
      block[y * block_size + x] = value;
    }
  }
}

void print_block(const od_coeff *const block, int block_size) {
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

  od_coeff *block = (od_coeff*) calloc(block_square, sizeof(od_coeff));
  init_block(block, block_size, 127);

  printf("Pixel Values\n");
  print_block(block, block_size);
  printf("\n");

  od_coeff *dct_block = (od_coeff*) calloc(big_block_square, sizeof(od_coeff));

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

  dct(dct_block, big_block_size, block, block_size);
  dct(&dct_block[block_size], big_block_size, block, block_size);
  dct(&dct_block[bottom_left], big_block_size, block, block_size);
  dct(&dct_block[bottom_right], big_block_size, block, block_size);

  printf("Transformed Coefficients\n");
  print_block(dct_block, big_block_size);
  printf("\n");

  od_coeff *tf_block = (od_coeff*) calloc(block_square, sizeof(od_coeff));
  // TF merge 4 blocks (a big_block) into 1 block
  od_tf_up_hv_lp(tf_block, block_size, dct_block, big_block_size,
		      block_size, block_size, block_size);
  printf("TF\n");
  print_block(tf_block, block_size);
  printf("\n");

  od_coeff *out = (od_coeff*) calloc(block_square, sizeof(od_coeff));
  idct(out, block_size, tf_block, block_size);
  printf("Reconstructed Pixels\n");
  print_block(out, block_size);
  printf("\n");
  
  free(block);
  free(dct_block);
  free(tf_block);
  free(out);
}
