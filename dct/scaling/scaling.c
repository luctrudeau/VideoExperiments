
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "aom/aom_integer.h"
#include "aom_dsp/aom_dsp_common.h"
#include "aom/aom_dsp/txfm_common.h"

// AV1 Constants
#define DCT_CONST_BITS 14
#define ROW_SIZE (4)
#define ROW_SQUARE (16)

/* Shift down with rounding for use when n >= 0, value >= 0 */
#define ROUND_POWER_OF_TWO(value, n) (((value) + (((1 << (n)) >> 1))) >> (n))

#define WRAPLOW(x) ((((int32_t)check_range(x)) << 16) >> 16)

static INLINE tran_high_t check_range(tran_high_t input) {
#if CONFIG_COEFFICIENT_RANGE_CHECKING
  // For valid AV1 input streams, intermediate stage coefficients should always
  // stay within the range of a signed 16 bit integer. Coefficients can go out
  // of this range for invalid/corrupt AV1 streams. However, strictly checking
  // this range for every intermediate coefficient can burdensome for a decoder,
  // therefore the following assertion is only enabled when configured with
  // --enable-coefficient-range-checking.
  assert(INT16_MIN <= input);
  assert(input <= INT16_MAX);
#endif  // CONFIG_COEFFICIENT_RANGE_CHECKING
  return input;
}

// AV1 Utility functions
static INLINE tran_high_t fdct_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}

static INLINE tran_high_t dct_const_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}

static INLINE uint8_t clip_pixel_add(uint8_t dest, tran_high_t trans) {
  trans = WRAPLOW(trans);
  return clip_pixel(dest + (int)trans);
}

/*This is the strength reduced version of ((_a)/(1 << (_b))).
  This will not work for _b == 0, however currently this is only used for
  b == 1 anyway.*/
# define OD_UNBIASED_RSHIFT32(_a, _b) \
  (((int32_t)(((uint32_t)(_a) >> (32 - (_b))) + (_a))) >> (_b))

# define OD_DCT_RSHIFT(_a, _b) OD_UNBIASED_RSHIFT32(_a, _b)

void print_row(const tran_low_t *const row, const char const* title) {
  int i;
  printf("%s:", title);
  for (i = 0; i < ROW_SIZE; i++) {
    printf(" %d", row[i]);
  }
  printf("\n");

}

void print_block(const tran_low_t *const row, const char const* title) {
  int i, j;
  printf("%s:\n", title);
  for (j = 0; j < ROW_SIZE; j++) {
    for (i = 0; i < ROW_SIZE; i++) {
      printf(" %d", row[j * ROW_SIZE + i]);
    }
    printf("\n");
  }
  printf("\n");
}

void print_block_uint8(const uint8_t *const row, const char const* title) {
  int i, j;
  printf("%s:\n", title);
  for (j = 0; j < ROW_SIZE; j++) {
    for (i = 0; i < ROW_SIZE; i++) {
      printf(" %d", row[j * ROW_SIZE + i]);
    }
    printf("\n");
  }
  printf("\n");
}

// AV1 Forward DCT
static void aom_fdct4(const tran_low_t *input, tran_low_t *output) {

  tran_high_t temp;
  tran_low_t step[4];

  // stage 0
  //range_check(input, 4, 14);

  // stage 1
  output[0] = input[0] + input[3];
  output[1] = input[1] + input[2];
  output[2] = input[1] - input[2];
  output[3] = input[0] - input[3];

  //range_check(output, 4, 15);

  // stage 2
  temp = output[0] * cospi_16_64 + output[1] * cospi_16_64;
  step[0] = (tran_low_t)fdct_round_shift(temp);
  temp = output[1] * -cospi_16_64 + output[0] * cospi_16_64;
  step[1] = (tran_low_t)fdct_round_shift(temp);
  temp = output[2] * cospi_24_64 + output[3] * cospi_8_64;
  step[2] = (tran_low_t)fdct_round_shift(temp);
  temp = output[3] * cospi_24_64 + output[2] * -cospi_8_64;
  step[3] = (tran_low_t)fdct_round_shift(temp);

  //range_check(step, 4, 16);

  // stage 3
  output[0] = step[0];
  output[1] = step[2];
  output[2] = step[1];
  output[3] = step[3];

  //range_check(output, 4, 16);
}

static void daala_fdct4(const tran_low_t *input, tran_low_t *output) {

  /*9 adds, 2 shifts, 3 "muls".*/
  int t0;
  int t1;
  int t2;
  int t2h;
  int t3;
  /*Initial permutation:*/
  t0 = input[0];
  t2 = input[1];
  t1 = input[2];
  t3 = input[3];
  /*+1/-1 butterflies:*/
  t3 = t0 - t3;
  t2 += t1;
  t2h = OD_DCT_RSHIFT(t2, 1);
  t1 = t2h - t1;
  t0 -= OD_DCT_RSHIFT(t3, 1);
  /*+ Embedded 2-point type-II DCT.*/
  t0 += t2h;
  t2 = t0 - t2;
  /*+ Embedded 2-point type-IV DST.*/
  /*23013/32768 ~= 4*sin(\frac{\pi}{8}) - 2*tan(\frac{\pi}{8}) ~=
    0.70230660471416898931046248770220*/
  //OD_DCT_OVERFLOW_CHECK(t1, 23013, 16384, 0);
  t3 -= (t1*23013 + 16384) >> 15;
  /*21407/32768~=\sqrt{1/2}*cos(\frac{\pi}{8}))
    ~=0.65328148243818826392832158671359*/
  //OD_DCT_OVERFLOW_CHECK(t3, 21407, 16384, 1);
  t1 += (t3*21407 + 16384) >> 15;
  /*18293/16384 ~= 4*sin(\frac{\pi}{8}) - tan(\frac{\pi}{8}) ~=
    1.1165201670872640381121512119119*/
  //OD_DCT_OVERFLOW_CHECK(t1, 18293, 8192, 2);
  t3 -= (t1*18293 + 8192) >> 14;

  // Checking for overflow since Daala code uses 32bit ints.
  assert(t0 < 32767 && t0 > -32767);
  assert(t1 < 32767 && t1 > -32767);
  assert(t2 < 32767 && t2 > -32767);
  assert(t3 < 32767 && t3 > -32767);

  output[0] = (tran_low_t)t0;
  output[1] = (tran_low_t)t1;
  output[2] = (tran_low_t)t2;
  output[3] = (tran_low_t)t3;
}

void fdct4x4(const tran_low_t *input,
    tran_low_t *aom_output, tran_low_t *daala_output) {

  int i, j;
  tran_low_t aom_temp_in[ROW_SIZE], aom_temp_out[ROW_SIZE];
  tran_low_t daala_temp_in[ROW_SIZE], daala_temp_out[ROW_SIZE];
  tran_low_t aom_out[ROW_SIZE * ROW_SIZE];
  tran_low_t daala_out[ROW_SIZE * ROW_SIZE];

  // Columns
  for (i = 0; i < ROW_SIZE; ++i) {
    for (j = 0; j < ROW_SIZE; ++j) {
      aom_temp_in[j] = input[j * ROW_SIZE + i] * 16;
      daala_temp_in[j] = input[j * ROW_SIZE + i] * 8;
    }

    if (i == 0 && aom_temp_in[0]) aom_temp_in[0] += 1;

    aom_fdct4(aom_temp_in, aom_temp_out);
    daala_fdct4(daala_temp_in, daala_temp_out);
    for (j = 0; j < ROW_SIZE; ++j) {
      aom_out[j * ROW_SIZE + i] = aom_temp_out[j];
      daala_out[j * ROW_SIZE + i] = daala_temp_out[j];
    }
  }

  print_block(aom_out, "AOM DCT COLUMNS");
  print_block(daala_out, "DAALA DCT COLUMNS");

  // Rows
  for (i = 0; i < ROW_SIZE; ++i) {
    for (j = 0; j < ROW_SIZE; ++j) {
      aom_temp_in[j] = aom_out[j + i * ROW_SIZE];
      daala_temp_in[j] = daala_out[j + i * ROW_SIZE];
    }
    aom_fdct4(aom_temp_in, aom_temp_out);
    daala_fdct4(daala_temp_in, daala_temp_out);
    for (j = 0; j < ROW_SIZE; ++j) {
      aom_output[j + i * 4] = (aom_temp_out[j] + 1) >> 2;
      daala_output[j + i * 4] = daala_temp_out[j];
     }
  }
 }
void aom_idct4(const tran_low_t *input, tran_low_t *output) {
  tran_low_t step[4];
  tran_high_t temp1, temp2;
  // stage 1
  temp1 = (input[0] + input[2]) * cospi_16_64;
  temp2 = (input[0] - input[2]) * cospi_16_64;
  step[0] = WRAPLOW(dct_const_round_shift(temp1));
  step[1] = WRAPLOW(dct_const_round_shift(temp2));
  temp1 = input[1] * cospi_24_64 - input[3] * cospi_8_64;
  temp2 = input[1] * cospi_8_64 + input[3] * cospi_24_64;
  step[2] = WRAPLOW(dct_const_round_shift(temp1));
  step[3] = WRAPLOW(dct_const_round_shift(temp2));

  // stage 2
  output[0] = WRAPLOW(step[0] + step[3]);
  output[1] = WRAPLOW(step[1] + step[2]);
  output[2] = WRAPLOW(step[1] - step[2]);
  output[3] = WRAPLOW(step[0] - step[3]);
}

void daala_idct4(const tran_low_t *input, tran_low_t *output) {
  int t0;
  int t1;
  int t2;
  int t2h;
  int t3;
  t0 = input[0];
  t1 = input[1];
  t2 = input[2];
  t3 = input[3];
  t3 += (t1*18293 + 8192) >> 14;
  t1 -= (t3*21407 + 16384) >> 15;
  t3 += (t1*23013 + 16384) >> 15;
  t2 = t0 - t2;
  t2h = OD_DCT_RSHIFT(t2, 1);
  t0 -= t2h - OD_DCT_RSHIFT(t3, 1);
  t1 = t2h - t1;
  output[0] = (tran_low_t)t0;
  output[1] = (tran_low_t)(t2 - t1);
  output[2] = (tran_low_t)t1;
  output[3] = (tran_low_t)(t0 - t3);
}

void idct4x4(const tran_low_t *aom_input, tran_low_t *daala_input,
    uint8_t *aom_output, uint8_t *daala_output) {

  tran_low_t aom_out[4 * 4];
  tran_low_t *aom_outptr = aom_out;

  tran_low_t daala_out[4 * 4];
  tran_low_t *daala_outptr = daala_out;

  int i, j;
  tran_low_t aom_temp_in[4], aom_temp_out[4];
  tran_low_t daala_temp_in[4], daala_temp_out[4];

  // Rows
  for (i = 0; i < 4; ++i) {
    aom_idct4(aom_input, aom_outptr);
    daala_idct4(daala_input, daala_outptr);
    aom_input += 4;
    daala_input += 4;
    aom_outptr += 4;
    daala_outptr += 4;
  }

  // Columns
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      aom_temp_in[j] = aom_out[j * 4 + i];
      daala_temp_in[j] = daala_out[j * 4 + i];
    }
    aom_idct4(aom_temp_in, aom_temp_out);
    daala_idct4(daala_temp_in, daala_temp_out);
    for (j = 0; j < 4; ++j) {
      aom_output[j * 4 + i] = clip_pixel_add(aom_output[j * 4 + i],
          ROUND_POWER_OF_TWO(aom_temp_out[j], 4));
      daala_output[j * 4 + i] = clip_pixel_add(daala_output[j * 4 + i],
          ROUND_POWER_OF_TWO(daala_temp_out[j], 3));
    }
  }
}

// Utility functions for the experiment
void rand_row(tran_low_t *const row) {
  int i;
  for (i = 0; i < ROW_SIZE; i++) {
      row[i] = rand() % 256;
  }
}


int main(int _argc,char **_argv) {

  tran_low_t row[ROW_SIZE] = {0};
  tran_low_t aom_dct_row[ROW_SIZE] = {0};
  tran_low_t daala_dct_row[ROW_SIZE] = {0};
  tran_low_t aom_idct_row[ROW_SIZE] = {0};
  tran_low_t daala_idct_row[ROW_SIZE] = {0};

  rand_row(row);
  print_row(row, "Values");

  aom_fdct4(row, aom_dct_row);
  aom_idct4(aom_dct_row, aom_idct_row);
  print_row(aom_dct_row, "aom_fdct4");
  print_row(aom_idct_row, "aom_idct4");

  daala_fdct4(row, daala_dct_row);
  daala_idct4(daala_dct_row, daala_idct_row);
  print_row(daala_dct_row, "daala_fdct4");
  print_row(daala_idct_row, "daala_idct4");

  tran_low_t aom_output[ROW_SIZE * ROW_SIZE];
  tran_low_t daala_output[ROW_SIZE * ROW_SIZE];

  tran_low_t flat[ROW_SIZE * ROW_SIZE] = {
    127, 127, 127, 127,
    127, 127, 127, 127,
    127, 127, 127, 127,
    127, 127, 127, 127};
  print_block(flat, "Flat Block");

  fdct4x4(flat, aom_output, daala_output);
  print_block(aom_output, "AOM 2D DCT");
  print_block(daala_output, "Daala 2D DCT");

  tran_low_t input[ROW_SIZE * ROW_SIZE] = {0};

  tran_low_t *in = input;
  rand_row(in);
  rand_row(in + 4);
  rand_row(in + 8);
  rand_row(in + 12);

  print_block(input, "Input Pixels");

  fdct4x4(input, aom_output, daala_output);

  print_block(aom_output, "AOM 2D DCT");
  print_block(daala_output, "Daala 2D DCT");


  uint8_t aom_idct_block[ROW_SQUARE] = {0};
  uint8_t daala_idct_block[ROW_SQUARE] = {0};
  idct4x4(aom_output, daala_output, aom_idct_block, daala_idct_block);
  print_block_uint8(aom_idct_block, "AOM reconstruction");
  print_block_uint8(daala_idct_block, "DAALA reconstruction");
}
