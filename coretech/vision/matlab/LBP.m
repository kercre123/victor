function [lbp, contrast] = LBP(img, scale, minContrast)
% Compute Local Binary Patterns (LBP)
%
% [lbp, contrast] = LBP(img, scale)
%
%  Each pixel of LBP will have one of the 58 LBP labels. No histogramming 
%  is done over local windows.
%
%  scale defaults to 1 (caller is responsible for smoothing at larger
%  scales)
%
% See also: MultiscaleLBP
% ------------
% Andrew Stein
%

persistent LUT

if isempty(LUT)
    LUT = ComputeUniformLBPTable();
    assert(length(LUT)==256, 'Lookup Table has wrong size.');
end

if nargin < 2 || isempty(scale)
    scale = 1;
end

if nargin < 3
  minContrast = 1;
end


imageRight     = image_right(img, scale);
imageUpRight   = image_upright(img, scale);
imageUp        = image_up(img, scale);
imageUpLeft    = image_upleft(img, scale);
imageLeft      = image_left(img, scale);
imageDownLeft  = image_downleft(img, scale);
imageDown      = image_down(img, scale);
imageDownRight = image_downright(img, scale);

imgAdj = img * minContrast;
lbp = uint8(double(imageRight > imgAdj) + ...
    2*double(imageUpRight     > imgAdj) + ...
    4*double(imageUp          > imgAdj) + ...
    8*double(imageUpLeft      > imgAdj) + ...
    16*double(imageLeft       > imgAdj) + ...
    32*double(imageDownLeft   > imgAdj) + ...
    64*double(imageDown       > imgAdj) + ...
    128*double(imageDownRight > imgAdj));

lbp = LUT(lbp+1);

if nargout > 1
  contrast = im2double(max(cat(3, ...
    abs(imageRight     - img), ...
    abs(imageUpRight   - img), ...
    abs(imageUp        - img), ...
    abs(imageUpLeft    - img), ...
    abs(imageLeft      - img), ...
    abs(imageDownLeft  - img), ...
    abs(imageDown      - img), ...
    abs(imageDownRight - img)), [], 3));
end

if nargout == 0
    imagesc(lbp), axis image
end

end % LBP()


function lut = ComputeUniformLBPTable()
% Helper function for computing LBP

IsUniform = @(bits) nnz(diff(bits([1:end, 1]))) <= 2;

bitValues = 2.^(0:7);

% reserve label 0 for non-uniform
lut = zeros(1, 256);
crntLabel = 1;
for k = 1:256,
    bits = bitand(k-1, bitValues) > 0;
    if IsUniform(bits),
        lut(k)  = crntLabel;
        crntLabel = crntLabel + 1;
    else
        lut(k) = 0;
    end
end

end % ComputeUniformLBPTable()