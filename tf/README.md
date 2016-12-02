# TF in Daala and in AV1

In this experiment we apply the TF merge of Daala to AV1.

## Experiment

Given an image and a block size of NxN, the image is split into NxN blocks and each block is transformed. A TF merge is performed over four transform blocks (forming a 2Nx2N big block). Only the upper left quadrant of the TF merge is taken. The NxN inverse transform of the block is computed and the image is reassembled.

## Results

![Experimental Results](https://raw.githubusercontent.com/luctrudeau/VideoExperiments/master/tf/sidebyside.png)

## Observation

  * As it is an approximation, TF introduces artifacts. These artifacts are less visible when block sizes are smaller.
    * Although the noise is similar
  * Daala requires scaling, while AV1 does not. 

## Future work

  * Add support for two-stage TF Merge
  * Include ADST to experiment
