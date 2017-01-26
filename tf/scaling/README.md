# DCT Scaling in Daala and in AV1 for Flat blocks

In this experiment, we compare the DCT of Daala and the one of AV1 for flat blocks. Afterward, we apply TF to each.

## Experiment

Given a flat block of size NxN, Apply the DCT to it four different times, in order to produce a 2Nx2N big block. Perform a TF merge over the big block. Keep the upper left quadrant of the merge and apply the inverse NxN transform to it.

## Results

### 4x4 Daala

#### Pixel Values

|   |   |   |   |
| --- | --- | --- | --- |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |

#### Transformed Coefficients

|   |   |   |   |   |   |   |   |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 508 | 0 | 0 | 0 | 508 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 508 | 0 | 0 | 0 | 508 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

#### TF

|   |   |   |   |
| --- | --- | --- | --- |
| 1016 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |

### 4x4 AOM

#### Pixel Values

|   |   |   |   |
| --- | --- | --- | --- |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |
| 127 | 127 | 127 | 127 |

#### Transformed Coefficients

|   |   |   |   |   |   |   |   |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 4064 | 0 | 0 | 0 | 4064 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 4064 | 0 | 0 | 0 | 4064 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

#### TF

|   |   |   |   |
| --- | --- | --- | --- |
| 8128 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 0 |

### Other sizes

#### Daala

| Size | DCT DC | TF DC |
|---|---|----|
| 4x4 | 508 | 1016 |
| 8x8 | 1015 | 2030 |
| 16x16 | 2032 | 4064 |
| 32x32 | 4062 | 8124 |

#### AV1
| Size | DCT DC | TF DC |
|---|---|----|
| 4x4 | 4064  | 8128 |
| 8x8 | 8128 | 16256  |
| **16x16** | **16257** | **32514** |
| **32x32** | **16257** | **32514** |
