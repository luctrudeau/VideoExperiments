# Daala's 4x4 DCT in AV1

In this experiment, we compare AV1's 4x4 DCT with Daala's 4x4 DCT. Our objective
is to replace AV1's DCT with Daala's. To do so, we must match the scaling or else
the scalar quantizer will not quantize properly.

## Introduction

At a glance, AV1's 1D 4 point DCT and Daala's 1d 4 point DCT appear to be
similar. They are both type-II DCTs, and given the following input
[103, 198, 105, 115]

  * AV1's DCT produces: [368, 25, -60, -91]
  * Daala's DCT produces: [260, 17, -43, -63]

## The First Big Difference

The first big difference is when we perform the inverse DCT

  * AV1's iDCT produces: [206, 397, 209, 230]
  * Daala's iDCT produces: [103, 198, 105, 115]

Daala's DCT allows for a perfect reconstruction, while the AV1 reconstruction
is scaled by a factor of 2. Notice that the odd values in AV1's
coefficients [206, **397**, **209**, 230] will cause rounding error.

## The Hack

AV1 compensates for this with the following hack:

```
if (i == 0 && temp_in[0]) temp_in[0] += 1;
```

Which adds 1 to non-zero DCs. When you apply both DCTs on every column followed
by another DCT over every row over a flat 127 valued block, you end up with:

** AV1's 2D DCT: **

|      |     |     |     |
| ---  | --- | --- | --- |
|1017  | 2   | 2   | 1   |
|2     | 2   | 2   | 1   |
|2     | 2   | 2   | 1   |
|1     | 1   | 1   | 1   |

** Daala 2D DCT: **

|      |     |     |     |
| ---  | --- | --- | --- |
|508 | 0 | 0 | 0 |
|0   | 0 | 0 | 0 |
|0   | 0 | 0 | 0 |
|0   | 0 | 0 | 0 |

A flat block should only have a DC value (i.e. zeros everywhere exept the top
left).

## Pre-Shifting

Since both DCTs are used with 4 bit pre-shift, this does not cause a real
problem. Once the 127 is shifted by 4 and after shifting the output by -4, you
get:

** AV1's 2D DCT: **

|      |     |     |     |
| ---  | --- | --- | --- |
| 1015 | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |

** Daala 2D DCT: **

|      |     |     |     |
| ---  | --- | --- | --- |
| 508  |  0  | 0   | 0   |
| 0    |  0  | 0   | 0   |
| 0    |  0  | 0   | 0   |
| 0    |  0  | 0   | 0   |

## The Hack Reloaded

Don't worry, AV1's actually adds another +1 right before the negative shift, so
you would actualy get 1016.

To be more precise, AV1's does not actually shift by -4, it shifts by -2. So
the actual output is:

AOM 2D DCT:

|      |     |     |     |
| ---  | --- | --- | --- |
| 4064 | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |
| 0    | 0   | 0   | 0   |

## Matching the Scale

It easily follows that since the output of AV1's 4 point DCT is pre-shifted by
4 (i.e. 2⁴=16), scaled by 2 and that the it's output is only shifted by -2
(i.e. 2^⁻2=1/4),  we can use a 3 bit pre-shift (i.e. 2^3 = 8 = 16 * 2 / 4), on
Daala's 4 point DCT and obtain the same scale.

Having the same scale is important or else the scalar quantizer will not perform
correctly. Note that it would also be possible to change the scalar quantizer,
but that will be the subject of another experiment.

Because we changed the pre-shift from 4 bits to 3 bits, the same change must be
done after the post-shift after the inverse transform. By doing so, we can use
Daala's DCT in AV1 without modifying the scalar quantizer.

## An Example

The following is a step by step breakdown of AV1's DCT and Daala's DCT for a 
randomly generated set of pixels

Input Pixels:

|      |     |     |     |
| ---  | --- | --- | --- |
| 81   | 255 | 74  | 236 |
| 41   | 205 | 186 | 171 |
| 242  | 251 | 227 | 70  |
| 124  | 194 | 84  | 248 |

AOM DCT COLUMNS (first pass):

|      |       |       |       |
| ---- | ----- | ----- | ----- |
| 5522 | 10239 |  6460 | 8202  |
| -1865|  620  | -399  | 441   |
| -882 | -79   | -2885 | 2749  |
| 2708 | 1053  | 545   | -1566 |

DAALA DCT COLUMNS (first pass):

|      |      |      |      |
| ---- | ---- | ---- | ---- |
| 1952 | 3620 | 2284 | 2900 |
| -660 | 219  | -141 | 156  |
| -312 | -28  | -1020| 972  |
| 958  | 372  | 192  | -554 |

AOM 2D DCT:

|      |      |      |       |
| ---- | ---- | ---- | ----- |
| 5378 | -258 | -526 | -1129 |
| -213 | -435 | -291 | -456  |
| -194 | -570 | 854  | -996  |
| 484  | 1036 | -81  | 291   |

Daala 2D DCT:

|      |      |      |       |
| ---  | ---- | ---- | ----- |
| 5378 | -258 | -526 | -1129 |
| -213 | -435 | -291 | -456  |
| -194 | -570 | 854  | -996  |
| 484  | 1037 | -80  | 291   |

AOM reconstruction:

|      |     |     |     |
| ---  | --- | --- | --- |
| 81   | 255 | 74  | 236 |
| 41   | 205 | 186 | 171 |
| 242  | 251 | 227 | 70  |
| 124  | 194 | 84  | 248 |

DAALA reconstruction:

|      |     |     |     |
| ---  | --- | --- | --- |
| 81   | 255 | 74  | 236 |
| 41   | 205 | 186 | 171 |
| 242  | 251 | 227 | 70  |
| 124  | 194 | 84  | 248 |
