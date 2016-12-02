# TF in Daala and in AV1

In this experiment, we apply the TF merge of Daala to AV1. In order to implement CfL into AV1, TF merging is required. When 4:2:0 chroma subsampling is used, the AV1 encoder can decide to split a 2Nx2N block into 4 NxN blocks, it can code the chroma planes as NxN blocks. In this situation, if CfL takes one of the 4 NxN as the prediction, it will not cover the right range. To cover the whole 2Nx2N range, the 4 NxN must be merged using TF. CfL can make an accurate prediction of the NxN chroma blocks by only using the top left NxN part of the TF merge.

## Experiment

Given an image and a block size of NxN, split the image into NxN blocks and transform each block using an NxN transform. Perform a TF merge over four transform blocks (forming a 2Nx2N big block). Keep only the upper left quadrant of the TF merge, apply the inverse NxN transform to it and reassemble the image.

## Results

![Experimental Results](ttps://raw.githubusercontent.com/luctrudeau/VideoExperiments/master/tf/sidebyside.png)

## Observation

  * As TF is an approximation, it introduces artifacts.
    * These artifacts are less visible when block sizes are smaller.
    * Although the noise is similar it is not identical (Because the transform are different).
  * Daala requires scaling, while AV1 does not...

## Future work

  * Add support for two-stage TF Merge
  * Include ADST to experiment
