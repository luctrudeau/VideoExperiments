% tabula rasa
clc; clear; close all

% Tested with 4, 8, 16 and 32
block_size = 8;
bm1 = block_size - 1;
% Same N as in https://people.xiph.org/~unlord/spie_cfl.pdf
N = block_size * block_size;

%im = imread('~/Videos/Hamilton.jpg');
%im = imread('~/Videos/Meerkat.jpg');
%im = imread('~/Videos/Owl.jpg');
%im = imread('~/Videos/gamegear.jpg');
im = imread('../../videos/lizard.jpg');
[h w ~] = size(im);
num_pix = h * w;
yuv = rgb2ycbcr(im);

y_img = yuv(:,:,1);
c_img = yuv(:,:,2);
cfl = uint8(zeros(h, w, 1));

k = 1;
as = zeros((h/block_size) * (w/block_size),1);
above = 1;
for y = 1:block_size:h-bm1
  yRange = y:y+bm1;
  left = 1;
  for x = 1:block_size:w-bm1
    xRange = x:x+bm1;

    by = y_img(yRange,xRange);
    bc = c_img(yRange,xRange);

    % The DC should be the sum times some scaling factor.
    % However, we want to avoid signaling beta, so the idea
    % is that beta=DC_PRED. DC_PRED is a prediction of the
    % average pixel value inside the block.

    % For Luma (encoder only), we will always use the
    % average over the entire block.
    by_avg = round(mean(by(:)));

    % For Chroma, we use DC_PRED on both the decoder side
    % and the encoder side.
    above_row = c_img(above, xRange);
    left_col = c_img(yRange, left);
    beta = round(mean([above_row(:); left_col(:)])); % aka DC_PRED

    % Named L and C to refer to CfL paper.
    % However, unline the paper L and C are zero mean.
    L = int16(by) - by_avg;
    C = int16(bc) - beta;

    % We could process Luma to remove outliers
    % L = L + (abs(L) > 4*std(L(:))) * (4*std(L(:)));

    % Sum of Luma == 0, because L is zero mean
    % sL = sum(L(:));
    % This does not old anymore because of rounding
    % assert(sum(L(:)) == 0);

    % This is also true for sC. However, because we use
    % beta=DC_PRED, this will no longer old. However, we
    % will show that sC simplifies out.
    % sC = sum(C(:));
    %assert(sum(C(:)) == 0);

    sLL = sum(L(:).^2);
    sLC = sum(L(:) .* C(:));

    % Because sL == 0, alpha as defined in eq.2
    % of https://people.xiph.org/~unlord/spie_cfl.pdf
    % a = (N * sLC - sL * sC) / (N * sLL - sL.^2)
    % the denominator is
    %den = (N * sLL - sL.^2);
    % it simplifies to
    den = sLL;
    if den != 0
      % the numerator is
      % a = (N * sLC - sL * sC) / den;
      % it simplifies to
      a = sLC / den;
    else
      a = 0;
    end

    as(k) = a;
    % Question: Should we take the rounding error into
    % consideration?
    % Answer: Ignoring the rounding error gives better
    % images and it would appear the alpha is also more
    % robust to the error resulting from using DC_PRED as
    % beta.

    cfl(yRange, xRange) = uint8(round(L * a + beta));
    left = x + bm1;
    k = k + 1;
  end
  above = y + bm1;
end

cfl_err = 127 + (c_img - cfl);
subplot(2,2,1:2); plot(as, 'x'); title('Alpha');
subplot(2,2,3); imshow(cfl); title('CfL');
subplot(2,2,4); imshow(cfl_err); title('Chroma - CfL');

imwrite(cfl, 'dcpred_cfl_int.png');
imwrite(cfl_err, 'dcpred_cfl_err_int.png');

sse = sum((c_img(:) - cfl(:)).^2)
psnr = 20 * log10(255) - 10 * log10(sse/(num_pix))

