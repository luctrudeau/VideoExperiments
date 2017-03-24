% tabula rasa
clc; clear; close all

% Tested with 4, 8, 16 and 32
block_size = 8;
bm1 = block_size - 1;
% Same N as in https://people.xiph.org/~unlord/spie_cfl.pdf
N = block_size * block_size;

%im = imread('~/Videos/Owl.jpg');
im = imread('~/Videos/Meerkat.jpg');
%im = imread('~/Videos/Hamilton.jpg');
%im = imread('../../videos/lizard.jpg');
[h w ~] = size(im);
num_pix = h * w;
yuv = rgb2ycbcr(im);

% Let's worry about types later
y_img = double(yuv(:,:,1));
c_img = double(yuv(:,:,2));
cfl = double(zeros(h, w, 1));
pred_img = double(zeros(h, w, 1));

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
    above_row = y_img(above, xRange);
    left_col = y_img(yRange, left);
    above_row_c = c_img(above, xRange);
    left_col_c = c_img(yRange, left);
    dc_pred = ones(block_size) * round(mean([above_row(:); left_col(:)]));
    h_pred = ones(block_size) .* left_col;
    v_pred = ones(block_size) .* above_row;

    dc_sse = sum((by(:) - dc_pred(:)).^2);
    h_sse = sum((by(:) - h_pred(:)).^2);
    v_sse = sum((by(:) - v_pred(:)).^2);

    [~, mode] = min([dc_sse, h_sse, v_sse]);
    switch(mode)
    case 1
      y_pred = dc_pred;
      c_pred = ones(block_size) * round(mean([above_row_c(:); left_col_c(:)]));
    case 2
      y_pred = h_pred;
      c_pred = ones(block_size) .* left_col_c;
    case 3
      y_pred = v_pred;
      c_pred = ones(block_size) .* above_row_c;
    end
    % Uncomment this if you want to use dc_pred only for luma
    %y_pred = dc_pred;

    % Named L and C to refer to CfL paper.
    % However, unline the paper L and C are zero mean.
    L = by - y_pred;
    C = bc - c_pred;

    % Sum of Luma == 0, because L is zero mean
    %sL = sum(L(:));
    %assert(sum(L(:)) == 0);
    % This is also true for sC. However, when we use
    % beta = DC_PRED, this will no longer old.
    %sC = sum(C(:));
    %assert(sum(C(:)) == 0);

    sLL = sum(L(:).^2);
    sLC = sum(L(:) .* C(:));

    % Because sL == 0, alpha as defined in eq.2
    % of https://people.xiph.org/~unlord/spie_cfl.pdf
    % a = (N * sLC - sL * sC) / (N * sLL - sL.^2)
    % the denominator does not simplify
    den = sLL;
    if den != 0
      % the numerator should simplifies to
      % a = (N * sLC) / den;
      % however, this works:
      a = sLC / den;
      % but I'm not sure why we should not use N??
      as(k) = a;
      k = k + 1;
    else
      a = 0;
    end

    pred_img(yRange, xRange) = c_pred;
    cfl(yRange, xRange) = c_pred + uint8(round(L * a));
    if any(any(cfl(yRange, xRange) > 200))
      printf('BANG!\n');
      return
    end
    left = x + bm1;
  end
  above = y + bm1;
end
max(cfl(:))

cfl_err = 127 + (c_img - cfl);
subplot(2,2,1:2); plot(as, 'x'); title('Alpha');
subplot(2,2,3); imshow(uint8(cfl)); title('CfL');
subplot(2,2,4); imshow(uint8(cfl_err)); title('Chroma - CfL');

imwrite(uint8(cfl), 'cfl_uvmode.png');
imwrite(uint8(cfl_err), 'cfl_uvmode_err.png');

sse = sum((c_img(:) - cfl(:)).^2)
psnr = 20 * log10(255) - 10 * log10(sse/(num_pix))

sse_pred = sum((c_img(:) - pred_img(:)).^2)
psnr_pred = 20 * log10(255) - 10 * log10(sse_pred/(num_pix))
