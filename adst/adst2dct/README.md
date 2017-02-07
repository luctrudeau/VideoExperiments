# ADST2DCT for AV1

In this experiment, we look into combining the inverse ADST transform and the
forward DCT transform into a single transform. The motivation is that, once
combined, it might be possible to factor out some parts of these functions.

## Introduction

Let's start by looking at the implementation of AV1's implementation of the
inverse ADST and the forward DCT

### AV1's inverse ADST

First, we apply the ADST to the input vector _input_.

```
x0 = input[0];
x1 = input[1];
x2 = input[2];
x3 = input[3];

if (!(x0 | x1 | x2 | x3)) {
  a0 = a1 = a2 = a3 = 0;
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
a0 = WRAPLOW(dct_const_round_shift(s0 + s3)) >> 1;
a1 = WRAPLOW(dct_const_round_shift(s1 + s3)) >> 1;
a2 = WRAPLOW(dct_const_round_shift(s2)) >> 1;
a3 = WRAPLOW(dct_const_round_shift(s0 + s1 - s3)) >> 1;
```

### AV1's forward DCT

Next, we apply a forward DCT to a0, a1, a2 and a3

**Stage 1**

```
x0 = a0 + a3;
x1 = a1 + a2;
x2 = a1 - a2;
x3 = a0 - a3;
```


**Stage 2**

```
temp = x0 * cospi_16_64 + x1 * cospi_16_64;
o0 = (tran_low_t)fdct_round_shift(temp);
temp = x2 * cospi_24_64 + x3 * cospi_8_64;
o1 = (tran_low_t)fdct_round_shift(temp);
temp = x1 * -cospi_16_64 + x0 * cospi_16_64;
o2 = (tran_low_t)fdct_round_shift(temp);
temp = x3 * cospi_24_64 + x2 * -cospi_8_64;
o3 = (tran_low_t)fdct_round_shift(temp);

```

## Combining the inverse ADST with the forward DCT

**Step 1:** Let's expand the a0, a1, a2 and a3 terms from the inverse ADST (_we will ignore
the wrapping and rounding for now_)


```
a0 => s0 + s3 => s0 + s3 + s5 + s2
   => x0 * sinpi_1_9 + x2 * sinpi_4_9 + x3 * sinpi_2_9 + x1 * sinpi_3_9
   => x0 * sinpi_1_9 + x3 * sinpi_2_9 + x1 * sinpi_3_9 + x2 * sinpi_4_9

a1 => s1 + s3 => s1 - s4 - s6 + s2
   => x0 * sinpi_2_9 - x2 * sinpi_1_9 - x3 * sinpi_4_9 + x1 * sinpi_3_9
   => -x2 * sinpi_1_9 + x0 * sinpi_2_9 + x1 * sinpi_3_9 - x3 * sinpi_4_9

a2 => s2 => (x0 - x2 + x3) * sinpi_3_9

a3 => s0 + s1 - s3 => s0 + s3 + s5 + s1 - s4 - s6 - s2
   => x0 * sinpi_1_9 + x2 * sinpi_4_9 + x3 * sinpi_2_9
     +x0 * sinpi_2_9 - x2 * sinpi_1_9 - x3 * sinpi_4_9 - x1 * sinpi_3_9
   => (x0 - x2) * sinpi_1_9 + (x0 + x3) * sinpi_2_9
     - x1 sinpi_3_9 + (x2 - x3) * sinpi_4_9
```

**Step 2:** We insert the expanded version into the DCT

```
x0 => a0 + a3
   => x0 * sinpi_1_9 + x3 * sinpi_2_9 + x1 * sinpi_3_9 + x2 * sinpi_4_9
     +(x0 - x2) * sinpi_1_9 + (x0 + x3) * sinpi_2_9
     - x1 sinpi_3_9 + (x2 - x3) * sinpi_4_9
   => (2 * x0 - x2) * sinpi_1_9
     +(x0 + 2 * x3) * sinpi_2_9
     +x1 * sinpi_3_9
     +(2 * x2 - x3) * sinpi_4_9

x1 => a1 + a2
   => -x2 * sinpi_1_9 + x0 * sinpi_2_9 + x1 * sinpi_3_9 - x3 * sinpi_4_9
     + (x0 - x2 + x3) * sinpi_3_9
   => -x2 * sinpi_1_9
     + x0 * sinpi_2_9
     + (x0 + x1 - x2 + x3) * sinpi_3_9
     - x3 * sinpi_4_9

x2 => a1 - a2
   => -x2 * sinpi_1_9 + x0 * sinpi_2_9 + x1 * sinpi_3_9 - x3 * sinpi_4_9
     -(x0 - x2 + x3) * sinpi_3_9
   => -x2 * sinpi_1_9
     + x0 * sinpi_2_9
     - (x0 + x1 - x2 + x3) * sinpi_3_9
     + x3 * sinpi_4_9

x3 => a0 - a3
   => x0 * sinpi_1_9 + x3 * sinpi_2_9 + x1 * sinpi_3_9 + x2 * sinpi_4_9
     -(x0 - x2) * sinpi_1_9 - (x0 + x3) * sinpi_2_9
     + x1 sinpi_3_9 - (x2 - x3) * sinpi_4_9
   => -x2 * sinpi_1_9
     - x0 * sinpi_2_9
     + 2 * x1 * sinpi_3_9
     - x3 * sinpi_4_9
```

**Step 3:** We can perform some simplifications on stage 2

```
o0 => (x0 + x1) * cospi_16_64
   => (2 * (x0 - x2) * sinpi_1_9
     + 2 * (x0 + x3) * sinpi_2_9
     + (x0 + 2 * x1 - x2 + x3) * sinpi_3_9
     + 2 * (x2 - x3) * sinpi_4_9) * cospi_16_64
   => (2 * (
           (x0 - x2) * sinpi_1_9
          +(x0 + x3) * sinpi_2_9
          +(x2 - x3) * sinpi_4_9
          )
     + (x0 + 2 * x1 - x2 + x3) * sinpi_3_9) * cospi_16_64

o2 => (x0 - x1) * cospi_16_64
   => 2 * x0 * sinpi_1_9
     + 2 * x3 * sinpi_2_9
     - (x0 - x2 + x3) * sinpi_3_9
     + 2 * x2 * sinpi_4_9

```
