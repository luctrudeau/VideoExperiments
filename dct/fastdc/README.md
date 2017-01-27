# Fast DCT for Flat Blocks

In this experiment, we build a fast replacement for AV1's DCT for Flat blocks
(blocks where the value of each pixel is the same). This is the case when
DC\_PRED is used, to predict the value of a block.

## Proposed Fast DCT for Flat Blocks

It can be showned that for a flat block of size NxN the DC coefficient produced
by AV1's DCT can be obtained using the following equation DC = 8*N*x,
where x is the pixel value of the flat block. This is true when N < 32,
for N == 32, DC = 128*x.

## Experiment

For each flat block of size NxN within the range 0 to 255, compute AV1's DC
using:
  * AV1's DCT function (AOM\_fdctNxN\_c)
  * AV1's DC function (AOM\_fdctNxN\_1\_c)
  * The proposed Fast DCT for Flat Blocks.

Perform the inverse transform on each transformed block and validate that the
inversed transformed flat block is equivalent to the original flat block.

## Results

While doing the experiment, we noticed a difference between the DC of
AOM\_fdctNxN\_c and the DC of AOM\_fdctNxN\_1\_c. However, we don't see an
change in the inverse transform.

  ![DCT function error](https://github.com/luctrudeau/VideoExperiments/blob/master/dct/fastdc/sidebyside.png)

Otherwise, there is no difference between the proposed method and AOM\_fdctNxN\_1\_c.
