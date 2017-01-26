# TF Merigng and the ADST

In this experiment, we apply the TF merge of Daala to the ADST of AV1. We are
interested in discovering if a 2Nx2N quartet of NxN ADST transformed blocks can
be TF-merged into a 2Nx2N ADST block.

## Experiment

Given the following image:

![Reference Image](https://github.com/luctrudeau/VideoExperiments/raw/master/videos/owl.png)

Split the image into NxN blocks and perform a forward transform on the block.
Apply a TF merge on all 2Nx2N block quartets.Apply an 2Nx2N inverse transform
on the resulting block and reassemble the image.

## Results

![Results](https://github.com/luctrudeau/VideoExperiments/raw/master/tf/adst/sidebyside.png)

## Observations

  * The TF and the ADST are not compatible.
  * Using the DCT as the inverse transform appears to be less noisy.
  * The visual degradation is very similar in multiple blocks, it might be
    possible to remove it.
  * For ADST -> TF -> ADST ripples are visible from the DC outwards.

In the context of CfL, it might be favorable to perform an operation that would
convert the ADST into an approximation of the DCT. According to jmspeex, the
properties of the ADST are not wanted on for the Chroma plane during CfL (i.e.
CfL might not produce a residual with a geomtric offset, in which case the DCT
is a better transform).
