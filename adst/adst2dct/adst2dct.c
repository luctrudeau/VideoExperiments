
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
#define WRAPLOW(x) ((((int32_t) x) << 16) >> 16)

/* Shift down with rounding for use when n >= 0, value >= 0 */
#define ROUND_POWER_OF_TWO(value, n) (((value) + (((1 << (n)) >> 1))) >> (n))

// AV1 Utility functions
static INLINE tran_high_t fdct_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}

static INLINE tran_high_t dct_const_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}

// AV1 Forward ADST
static void fadst4(const tran_low_t *input, tran_low_t *output) {
  tran_high_t x0, x1, x2, x3;
  tran_high_t s0, s1, s2, s3, s4, s5, s6, s7;

  x0 = input[0];
  x1 = input[1];
  x2 = input[2];
  x3 = input[3];

  if (!(x0 | x1 | x2 | x3)) {
    output[0] = output[1] = output[2] = output[3] = 0;
    return;
  }

  s0 = sinpi_1_9 * x0;
  s1 = sinpi_4_9 * x0;
  s2 = sinpi_2_9 * x1;
  s3 = sinpi_1_9 * x1;
  s4 = sinpi_3_9 * x2;
  s5 = sinpi_4_9 * x3;
  s6 = sinpi_2_9 * x3;
  s7 = x0 + x1 - x3;

  x0 = s0 + s2 + s5;
  x1 = sinpi_3_9 * s7;
  x2 = s1 - s3 + s6;
  x3 = s4;

  s0 = x0 + x3;
  s1 = x1;
  s2 = x2 - x3;
  s3 = x2 - x0 + x3;

  // 1-D transform scaling factor is sqrt(2).
  output[0] = (tran_low_t)fdct_round_shift(s0);
  output[1] = (tran_low_t)fdct_round_shift(s1);
  output[2] = (tran_low_t)fdct_round_shift(s2);
  output[3] = (tran_low_t)fdct_round_shift(s3);
}

// AV1 Inverse DCT
void iadst4_c(const tran_low_t *input, tran_low_t *output) {
  tran_high_t s0, s1, s2, s3, s4, s5, s6, s7;

  tran_low_t x0 = input[0];
  tran_low_t x1 = input[1];
  tran_low_t x2 = input[2];
  tran_low_t x3 = input[3];

  if (!(x0 | x1 | x2 | x3)) {
    output[0] = output[1] = output[2] = output[3] = 0;
    return;
  }

  s0 = sinpi_1_9 * x0;
  s1 = sinpi_2_9 * x0;
  s2 = sinpi_3_9 * x1;
  s3 = sinpi_4_9 * x2;
  s4 = sinpi_1_9 * x2;
  s5 = sinpi_2_9 * x3;
  s6 = sinpi_4_9 * x3;
  s7 = WRAPLOW(x0 - x2 + x3);

  s0 = s0 + s3 + s5;
  s1 = s1 - s4 - s6;
  s3 = s2;
  s2 = sinpi_3_9 * s7;

  // 1-D transform scaling factor is sqrt(2).
  // The overall dynamic range is 14b (input) + 14b (multiplication scaling)
  // + 1b (addition) = 29b.
  // Hence the output bit depth is 15b.
  output[0] = WRAPLOW(dct_const_round_shift(s0 + s3)) >> 1;
  output[1] = WRAPLOW(dct_const_round_shift(s1 + s3)) >> 1;
  output[2] = WRAPLOW(dct_const_round_shift(s2)) >> 1;
  output[3] = WRAPLOW(dct_const_round_shift(s0 + s1 - s3)) >> 1;
}

// AV1 Forward DCT
static void fdct4(const tran_low_t *input, tran_low_t *output) {
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

// This is the experiment!
// Converts an ADST into a DCT
// This is done by combining the operations of the iADST with the fDCT.
void adst2dct(const tran_low_t *input, tran_low_t *output) {

  int x0 = input[0];
  int x1 = input[1];
  int x2 = input[2];
  int x3 = input[3];

  int s2 = x1 * sinpi_3_9;

  int t0 = (x0 - x2 + x3) * sinpi_3_9;
  int t1 = -(x2 * sinpi_1_9) + (x0 * sinpi_2_9) - (x3 * sinpi_4_9);
  int t2 = (x0 * sinpi_1_9) + (x3 * sinpi_2_9) + (x2 * sinpi_4_9);
  int t3 = s2 + t0;

  int t4 = t1 + s2 - t0; //  s1 = a1 - a2
  int t5 = -t1 + 2 * s2; //  s2 = a0 - a3


  //========================================
  //  int o0 = (a0 + a1 + a2 + a3) * cospi_16_64;
  int o0 = (2 * (t1 + t2) + t3) * cospi_16_64;

  //========================================
  //  int o1 = (a1 - a2) * cospi_24_64 + (a0 - a3) * cospi_8_64;
  int o1 = t4 * cospi_24_64 + t5 * cospi_8_64;

  //========================================
  int o2 = (2 * t2 - t3) * cospi_24_64;

  //========================================
  //  int o3 = (a0 - a3) * cospi_24_64 - (a1 - a2) * cospi_8_64;
  int o3 = t5 * cospi_24_64 - t4 * cospi_8_64;

  output[0] = fdct_round_shift(o0 >> 6);
  output[1] = fdct_round_shift(o1 >> 9);
  output[2] = fdct_round_shift(o2 >> 8);
  output[3] = fdct_round_shift(o3 >> 10);
}

// Utility functions for the experiment
void rand_row(tran_low_t *const row) {
  int i;
  for (i = 0; i < ROW_SIZE; i++) {
      row[i] = rand() % 256;
  }
}

void print_row(const tran_low_t *const row, const char const* title) {
  int i;
  printf("%s:", title);
  for (i = 0; i < ROW_SIZE; i++) {
      printf(" %d", row[i]);
  }
  printf("\n");
}

int main(int _argc,char **_argv) {

  tran_low_t row[ROW_SIZE] = {0};
  tran_low_t dct_row[ROW_SIZE] = {0};
  tran_low_t adst_row[ROW_SIZE] = {0};
  //tran_low_t iadst_row[ROW_SIZE] = {0};
  tran_low_t adst2dct_row[ROW_SIZE] = {0};

  rand_row(row);
  print_row(row, "Random Pixel Values");

  fadst4(row, adst_row);
  print_row(adst_row, "ADST Values");

  fdct4(row, dct_row);
  print_row(dct_row, "DCT Values");

  // Just for validation
  //iadst4_c(adst_row, iadst_row);
  //print_row(iadst_row, "iADST Values");

  adst2dct(adst_row, adst2dct_row);
  print_row(adst2dct_row, "ADST2DCT Values");
}
