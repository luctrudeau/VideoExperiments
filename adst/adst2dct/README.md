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
s7 = wraplow(x0 - x2 + x3);

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
d0 = a0 + a3;
d1 = a1 + a2;
d2 = a1 - a2;
d3 = a0 - a3;
```


**Stage 2**

```
o0 = d0 * cospi_16_64 + d1 * cospi_16_64;
o1 = d1 * -cospi_16_64 + d0 * cospi_16_64;
o2 = d2 * cospi_24_64 + d3 * cospi_8_64;
o3 = d3 * cospi_24_64 + d2 * -cospi_8_64;

```

## Combining the inverse ADST with the forward DCT

**Step 0:** We move the _WRAPLOW(dct_const_round_shift())_ from a0, a1, a2 and a3 to d0, d1, d2 and d3.
We can now work on expanding and simplifying d0, d1, d2 and d3.

**Step 1:** Expanding d2 and d3 reveals that no simplifications can be made. So we will focus on d0 and d1. 

```
d0 => a0 + a3
   => s0 + s3
    + s0 + s1 - s3
   => 2 * s0 + s1

d1 => a1 + a2
   => s1 + s2 + s3

d2 => a1 - a2
   => s1 - s2 + s3

d3 => a0 - a3
   => s0 + s3
     -s0 - s1 + s3
   => -s1 + 2 * s3
```

So we simplified s3 out of d0 and s0 out of d1.

**Step 2:** We can perform some simplifications on stage 2
We combine d0 + d1 and d0 - d1

```
t0 => d0 + d1
   => 2 * s0 + s1 + s1 + s2 + s3
   => 2 * (s0 + s1) + s2 + s3

t1 => d0 - d1
   => 2 * s0 + s1 - s1 - s2 - s3
   => 2 * s0 - s2 - s3
```
In both cases we managed to simplify s1.


**Step 3:** Extracting common terms
First, we rename t0 and t1 back to d0 and d1. Next, we extract common terms from d0, d1, d2 and d3

```

s0 = s0 * 2

t0 = s2 + s3

d0 => s0 + 2 * s1 + t0

d1 => s0 - t0

d2 => s1 - s2 + s3

d3 => -s1 + 2 * s3

```

