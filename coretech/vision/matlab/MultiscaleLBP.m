function [lbp, constrast] = MultiscaleLBP(img, varargin)
% Creates LBP at multiple scales
%
% [lbp, weights] = MultiscaleLBP(img, varargin)
%
%   Creates LBP labels for each pixel at multiscales scales.
%
%   Options:
%     'numScales' [1]      - How many scales 
%     'scaleFactor' [2]    - Create numScales levels at this power 
%     'doBlurring'  [true] - Blur between scales
%     'minContrast' [1]    - Passed directly to LBP(). See also: LBP
%
%   Returns cell arrays, one element per scale.  Each element will be at 
%   the image's original resolution.
%
% See also: LBP
% ------------
% Andrew Stein
%

numScales = 1;
doBlurring = true;
minContrast = 1;
scaleFactor = 2;

parseVarargin(varargin{:});
 
lbp = cell(1, numScales);
constrast = cell(1, numScales);

for iScale = 1:numScales
    scale = scaleFactor^(iScale-1);
    if scale > 1 && doBlurring
        sigma = (0.5*(scale-1) + 1)/6;
        g = gaussian_kernel(sigma);
        imgScaled = separable_filter(img, g);
    else
        imgScaled = img;
    end
    
    [lbp{iScale}, constrast{iScale}] = LBP(imgScaled, scale, minContrast);
    
    if nargout == 0
      subplot(1,numScales,iScale)
      imagesc(lbp{iScale}), axis image
    end
    
end % FOR each scale



end % MultiscaleLBP()


