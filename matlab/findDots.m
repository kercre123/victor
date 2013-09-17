
function [xcen,ycen] = findDots(img, mask, varargin)

% the multiplier should match the one used to compute the dot radius in
% generateMarkerImage
dotRadiusMultiplier = .125;
squareWidth = []; 

parseVarargin(varargin{:});

if size(img,3)>1
    img = mean(im2double(img),3);
end

if isempty(mask)
    mask = roipoly(img);
end

if isempty(squareWidth)
    squareWidth = sqrt(sum(mask(:)));
end

sigma = (squareWidth * dotRadiusMultiplier)/3; 
hsize = ceil(6*sigma + 1);
LoG = fspecial('log', hsize, sigma);

imgFilter = imfilter(img, LoG);

step = 1; %max(1, round((squareWidth * dotRadiusMultiplier)/12))
localMaxima = find(mask &  ...
    imgFilter > image_right(imgFilter,step) & ...
    imgFilter > image_left(imgFilter,step) & ...
    imgFilter > image_down(imgFilter,step) & ...
    imgFilter > image_up(imgFilter,step));

if length(localMaxima) < 4
   error('Did not find 4 local maxima.'); 
end

% Choose top four local maxima
[~, sortIndex] = sort(imgFilter(localMaxima), 'descend');
dotCenters = localMaxima(sortIndex(1:4));

[ycen,xcen] = ind2sub(size(img), dotCenters);

theta = cart2pol(xcen-mean(xcen),ycen-mean(ycen));
[~,sortIndex] = sort(theta);

xcen = xcen(sortIndex);
ycen = ycen(sortIndex);

% Return as upper left, lower left, upper right, lower rihgt:
xcen = xcen([1 4 2 3]);
ycen = ycen([1 4 2 3]);

if nargout == 0
    hold off, imagesc(img), axis image, hold on
    plot(xcen, ycen, 'r.', 'MarkerSize', 12);
    plot(xcen([1 2 4 3 1]), ycen([1 2 4 3 1]), 'r');
    
    clear xcen ycen
end

end % FUNCTION findDots()
