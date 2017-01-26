/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "av1/common/cfl.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


void tf_merge_and_subsample(CFL_CONTEXT *const cfl, tran_low_t *const dst,
    int dstride, const tran_low_t *const src, int sstride, int y_tx_size,
    int uv_tx_size) {

  //Size of the 2x2 group of nxn blocks.
  const int total_size = uv_tx_size << 1;

  // Double buffer setup
  tran_high_t *sbuf = cfl->buf1;
  tran_high_t *dbuf = cfl->buf2;
  tran_high_t out;
  int i, j;

  // Copy 16 bit src over to 32 bits
  assert(total_size <= MAX_SB_SIZE);
  for (j = 0; j < total_size; j++) {
    for (i = 0; i < total_size; i++) {
      sbuf[MAX_SB_SIZE * j + i] = src[sstride * j + i];
    }
  }

  // TF merge until the prediction is the required size
  while (y_tx_size < uv_tx_size) {
    int next_y_tx_size = y_tx_size << 1;
    int row = 0;

    // swap buffers
    tran_high_t *tmp = sbuf;
    sbuf = dbuf;
    dbuf = tmp;

    for (j = 0; j < total_size; j += next_y_tx_size) {
      for (i = 0; i < total_size; i += next_y_tx_size) {
        od_tf_up_hv(sbuf + row + i, MAX_SB_SIZE, dbuf + row + i,
            MAX_SB_SIZE, y_tx_size);
      }
      row += MAX_SB_SIZE;
    }
    y_tx_size = next_y_tx_size;
  }
  assert(y_tx_size == uv_tx_size);

  // TF merge last level and keep top left quadrant (this compensates for
  // chroma subsampling).
  od_tf_up_hv_lp(dbuf, MAX_SB_SIZE, sbuf, MAX_SB_SIZE, uv_tx_size, uv_tx_size,
        uv_tx_size);

  for (j = 0; j < uv_tx_size; j++) {
    for (i = 0; i < uv_tx_size; i++) {
      // FIXME Still not sure why this scaling is required (TF being unit scale)
      // I also need to figure out, if we need to increase scaling by the number
      // of TF operations performed.
      out = dbuf[MAX_SB_SIZE * j + i] >>= 1;
      // Clip as TF on bigger transforms can overflow
      dst[dstride * j + i] = (out < INT16_MAX) ? out : INT16_MAX;
    }
  }
}

void cfl_load_predictor(CFL_CONTEXT *const cfl, int blk_row, int blk_col,
		tran_low_t *const ref_coeff, int uv_tx_size) {

  const int scale = 4; // Needs to be adjusted to support 4:4:4
  const int y_tx_size = cfl->luma_tx_blk_size;
  const tran_low_t *y_coeff;
  const tran_low_t dc = ref_coeff[0];
  int coeff_offset;

  // Check that tx_sizes are valid.
  assert(y_tx_size > 0 && y_tx_size <= MAX_TX_SIZE);
  assert(uv_tx_size > 0 && uv_tx_size <= MAX_TX_SIZE);

  // Adjusting row and cols for 4:2:0. It is important to note that the
  // prediction is always the first AC coeffs (not the collocated coeffs).
  blk_row = (blk_row * scale * 2) / y_tx_size * y_tx_size;
  blk_col = (blk_col * scale * 2) / y_tx_size * y_tx_size;

  coeff_offset = blk_row * MAX_SB_SIZE + (blk_col);

  // Check that the last coeff offset is smaller than the max superblock size
  assert(coeff_offset
      + ((uv_tx_size-1) * MAX_SB_SIZE + (uv_tx_size-1)) < MAX_SB_SQUARE);

  y_coeff = &cfl->luma_coeff[coeff_offset];

  if (y_tx_size > uv_tx_size) {
    // When the CfL prediction is bigger than what is needed, we only take the
    // part that is needed.
    int shift = (y_tx_size / uv_tx_size) >> 1;
    int i, j, k = 0;

    // 1 and 2 are the only scale changes supported for now.
    assert(shift == 1 || shift == 2);

    // Don't scale the 32x32, it's already scaled.
    if (uv_tx_size > 16) shift--;

    for (j = 0; j < uv_tx_size; j++) {
      for (i = 0; i < uv_tx_size; i++) {
        // Scale coefficients as the inverse transform is half the size of the
        // forward transform
        ref_coeff[k++] = y_coeff[MAX_SB_SIZE * j + i] >> shift;
      }
    }
  } else {
    tf_merge_and_subsample(cfl, ref_coeff, uv_tx_size, y_coeff, MAX_SB_SIZE,
        y_tx_size, uv_tx_size);
  }
  // CfL does not apply to dc (only ac)
  ref_coeff[0] = dc;
}

/* Store the values of the luma plane to predict the values of the chroma plane
 *  * If the AC values are coded the dequantized transformed coefficients are
 *    used
 *  * If the AC values are skipped, the intra luma prediction is used.
 */
void cfl_store_predictor(CFL_CONTEXT *const cfl, int blk_row, int blk_col,
		int tx_blk_size, const tran_low_t *const ref_coeff,
		const tran_low_t *const dqcoeff, int ac_dc_coded) {

  const int scale = 4; // Needs to be adjusted to support 4:4:4
  const int coeff_offset = scale * blk_row * MAX_SB_SIZE + (scale * blk_col);
  const tran_low_t *src;
  tran_low_t *const luma_coeff = &cfl->luma_coeff[coeff_offset];
  int i,j,k = 0;

  if (blk_row != 0 || blk_col != 0) {
    // Check that all luma parts are the same size
    assert(cfl->luma_tx_blk_size == tx_blk_size);
  } else {
    // We store the Luma transform size as it can differ from the chroma
    // transform size. We assume that Luma transform size is constant inside a
    // partition.
    cfl->luma_tx_blk_size = tx_blk_size;

    if (luma_coeff == cfl->luma_coeff) {
      // Zero out luma_coeff on first store. This is important as frame
      // boundary situations can cause reading from stale memory.
      memset(luma_coeff, 0, sizeof(tran_low_t) * MAX_SB_SQUARE);
    }
  }

  // Check that the last coeff offset is smaller than the max superblock size
  assert(coeff_offset
    + ((tx_blk_size-1) * MAX_SB_SIZE + (tx_blk_size-1)) < MAX_SB_SQUARE);


  switch(ac_dc_coded) {
    case 0: // DC and AC are skipped
    case 1: // DC is coded and AC is skipped
      // AC is not coded: copy the AC prediction
      src = ref_coeff;
      break;
    case 2: // AC is coded and DC is skipped.
    case 3: // AC is coded and DC is coded
      // AC is coded: copy Luma transform dequantized coefficients
      src = dqcoeff;
      break;
    default:
      fprintf(stderr, "AC_DC_CODED value %d is > 3\n", ac_dc_coded);
      assert(0);
  }

  for (j = 0; j < tx_blk_size; j++) {
    for (i = 0; i < tx_blk_size; i++) {
      luma_coeff[j * MAX_SB_SIZE + i] = src[k++];
    }
  }
}

/*Increase horizontal and vertical frequency resolution of an entire block and
   return the LF quarter.*/
void od_tf_up_hv_lp(tran_high_t *const dst, int dstride,
 const tran_high_t *const src, int sstride, int dx, int dy, int n) {
  int x;
  int y;
  for (y = 0; y < n >> 1; y++) {
    int vswap;
    vswap = y & 1;
    for (x = 0; x < n >> 1; x++) {
      int32_t ll;
      int32_t lh;
      int32_t hl;
      int32_t hh;
      int hswap;
      ll = src[y*sstride + x];
      lh = src[y*sstride + x + dx];
      hl = src[(y + dy)*sstride + x];
      hh = src[(y + dy)*sstride + x + dx];
      /*We swap lh and hl for compatibility with od_tf_up_hv.*/
      OD_HAAR_KERNEL(ll, hl, lh, hh);
      hswap = x & 1;
      dst[(2*y + vswap)*dstride + 2*x + hswap] = ll;
      dst[(2*y + vswap)*dstride + 2*x + 1 - hswap] = lh;
      dst[(2*y + 1 - vswap)*dstride + 2*x + hswap] = hl;
      dst[(2*y + 1 - vswap)*dstride + 2*x + 1 - hswap] = hh;
    }
  }
}

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
