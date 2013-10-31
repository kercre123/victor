function [xcen, ycen] = findFourDotTarget(img, varargin)

squareDiagonal = [];
searchPaddingFactor = 2; % fraction of max(width,height) to add to search region
spacingTolerance = 0.3; % can be this fraction away from expected, i.e. +/- 10%
TrueSquareWidth = [];
TrueDotWidth = [];
TrueDotSpacing = [];
uMask = [];
vMask = [];
doVerify = true;

parseVarargin(varargin{:});

assert(~isempty(uMask) && ~isempty(vMask), 'No uMask and vMask provided.');
assert(~isempty(TrueSquareWidth) && ~isempty(TrueDotSpacing) && ...
    ~isempty(TrueDotWidth), 'True Dot/Square dimensions not provided.');
%assert(~isempty(squareDiagonal), 'No squareDiagonal specified.');

img = im2double(img);
if size(img,3) > 1
    img = mean(img, 3);
end

if isempty(squareDiagonal)
    squareDiagonal = sqrt( (uMask(1)-uMask(4))^2 + (vMask(1)-vMask(4))^2 );
end

%% 

% Create a filter for finding dots of the expected size (this is just a 2D
% Laplacian-of-Gaussian filter)
dotRadiusFraction = (TrueDotWidth/2) / (sqrt(2)*TrueSquareWidth);
dotRadius = squareDiagonal * dotRadiusFraction;
sigma = dotRadius; 
hsize = ceil(6*sigma + 1);
LoG = fspecial('log', hsize, sigma);

% Extract and filter just the image inside the docking target's bounding
% box.  We'll use a padded crop in order to avoid boundary effects.
filterPadding = ceil((hsize+1)/2);
squareWidth = ceil(uMask(3)-uMask(1));
squareHeight = ceil(vMask(2)-vMask(1));
searchPadding = ceil(searchPaddingFactor*max(squareWidth, squareHeight));
padding = filterPadding + searchPadding;
cropRectanglePad = [floor([uMask(1) vMask(1)])-padding ...
    squareWidth+2*padding squareHeight+2*padding];
imgCrop = imcrop(img, cropRectanglePad);

% NOTE: this Laplacian-of-Gaussian filtering step can and should be
% approximated by a difference of two Gaussian filtering steps, since the
% individual Gaussian filtering steps can be done with 1D separable
% filters for speed.
imgFilter = imfilter(imgCrop, LoG);

% Get rid of the filter padding and normalize to be b/w 0 and 1:
cropRectangle = [filterPadding filterPadding cropRectanglePad(3:4)-2*filterPadding];
imgFilter = imcrop(imgFilter, cropRectangle);
imgFilter = imgFilter - min(imgFilter(:));
imgFilter = imgFilter/max(imgFilter(:));


% Get local maxima of the filter result, which should correspond to
% potential dot centers
step = 1; %max(1, round((squareWidth * dotRadiusMultiplier)/12))
localMaxima = find( ... maskCrop &  ...
    imgFilter > image_right(imgFilter,step) & ...
    imgFilter > image_left(imgFilter,step) & ...
    imgFilter > image_down(imgFilter,step) & ...
    imgFilter > image_up(imgFilter,step));

if length(localMaxima)<4
    error('FindDockingTarget:TooFewLocalMaxima', ...
        'We need to find at least 4 local maxima for potential dot locations.');
end

[localMaxima_y, localMaxima_x] = ind2sub(size(imgFilter), localMaxima);


% We still don't know _which_ 4 potential dot centers belong together and
% make up the docking pattern.  To find the best configuration, we 
% effectively filter with this sparse mask:
%    [+1 ..0.. -1 ..0.. +1] 
%    [        ..0..       ]
%    [-1 ..0.. -1 ..0.. -1]
%    [        ..0..       ]
%    [+1 ..0.. -1 ..0.. +1]
% where the spacing between those non-zero filter components corresponds to
% the expected horizontal/vertical spacing between dots.

squareWidth  = uMask(3)-uMask(1); 
squareHeight = vMask(2)-vMask(1);
dotSpacingFrac = TrueDotSpacing/TrueSquareWidth;
dotSpacingHor  = dotSpacingFrac * squareWidth; 
dotSpacingVert = dotSpacingFrac * squareHeight;
stepH = round(dotSpacingHor/2);
stepV = round(dotSpacingVert/2);

imgFilter2 = -imgFilter + ...
    -image_left(imgFilter, stepH) - image_right(imgFilter, stepH) + ...
    -image_up(imgFilter, stepV) - image_down(imgFilter, stepV) + ...
    image_upleft(imgFilter, stepV, stepH) + image_downleft(imgFilter, stepV, stepH) + ...
    image_upright(imgFilter, stepV, stepH) + image_downright(imgFilter, stepV, stepH);

% The peak response of this filter will be the center of the target. We
% will then find the four local maxima from the original filter that are
% closest to the expected dot locations, given that center:
[~,targetCenterIndex] = max(imgFilter2(:)); % This max() still kinda worries me in terms of robustness...
[targetCenter_y, targetCenter_x] = ind2sub(size(imgFilter), targetCenterIndex);
distUL = (localMaxima_x - (targetCenter_x-dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y-dotSpacingVert/2)).^2;
distUR = (localMaxima_x - (targetCenter_x+dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y-dotSpacingVert/2)).^2;
distLL = (localMaxima_x - (targetCenter_x-dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y+dotSpacingVert/2)).^2;
distLR = (localMaxima_x - (targetCenter_x+dotSpacingHor/2)).^2 + ...
    (localMaxima_y - (targetCenter_y+dotSpacingVert/2)).^2;

[~, indexUL] = min(distUL);
[~, indexUR] = min(distUR);
[~, indexLL] = min(distLL);
[~, indexLR] = min(distLR);

if doVerify && length(unique([indexUL indexUR indexLL indexLR]))~=4
    desktop
    keyboard
    error('FindDockingTarget:DuplicateMaximaSelected', ...
        'The same point should not be chosen for multiple dots.');
end

% Select the closest local maxima and put them back in original, uncropped
% coordinates:
xcen = localMaxima_x([indexUL indexLL indexUR indexLR]) + filterPadding + cropRectanglePad(1) - 2;
ycen = localMaxima_y([indexUL indexLL indexUR indexLR]) + filterPadding + cropRectanglePad(2) - 2;

if doVerify
% Make sure the distances between dots are "reasonable":
% (Is this still useful, given the way I am finding/assigning dots now?  Or
% is it sorta guaranteed by the algorithm?)
expectedSpacingFrac = TrueDotSpacing/TrueSquareWidth;

leftLengthFrac = sqrt( (xcen(2)-xcen(1))^2 + (ycen(2)-ycen(1))^2 ) / ...
    sqrt( (uMask(2)-uMask(1))^2 + (vMask(2)-vMask(1))^2);

topLengthFrac = sqrt( (xcen(3)-xcen(1))^2 + (ycen(3)-ycen(1))^2 ) / ...
    sqrt( (uMask(3)-uMask(1))^2 + (vMask(3)-vMask(1))^2);

rightLengthFrac = sqrt( (xcen(3)-xcen(4))^2 + (ycen(3)-ycen(4))^2 ) / ...
    sqrt( (uMask(3)-uMask(4))^2 + (vMask(3)-vMask(4))^2);

bottomLengthFrac = sqrt( (xcen(2)-xcen(4))^2 + (ycen(2)-ycen(4))^2 ) / ...
    sqrt( (uMask(2)-uMask(4))^2 + (vMask(2)-vMask(4))^2);

if ~all(abs(expectedSpacingFrac - ...
        [leftLengthFrac topLengthFrac rightLengthFrac bottomLengthFrac]) / ...
        expectedSpacingFrac < spacingTolerance)
    
    desktop
    keyboard
    
    error('FindFourDotTarget:BadSpacing', ...
        'Detected dots not within spacing tolerances.');
end
end

if nargout == 0
    hold off, imagesc(img), axis image, hold on
    plot(xcen, ycen, 'r.', 'MarkerSize', 12);
    plot(xcen([1 2 4 3 1]), ycen([1 2 4 3 1]), 'r');
    
    clear xcen ycen
end

end % FUNCTION findFourDotTarget()