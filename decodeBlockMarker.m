function [blockType, faceType, isValid, keyOrient] = decodeBlockMarker(img, corners, cropFactor)

n = 5;

if nargin < 2
    corners = [];
end

if nargin < 3
    cropFactor = .5;
end

darkCorners = [];

assert(ismatrix(img), 'Grayscale image required.');

if ~isempty(corners)
    % Want to make sure there's some "dark stuff" inside the region will
    % pull out, to help avoid trying to decode a blank quadrilateral
    cornerIndex = sub2ind(size(img), round(corners(:,2)), round(corners(:,1)));
    darkCorners = mean(img(cornerIndex));
    
    % This assumes the corners are in "clockwise" direction (i.e. the 2nd
    % and third corners are clockwise wrt each other
    
    diagLength = sqrt(sum((corners(1,:)-corners(4,:)).^2));
    [xgrid,ygrid] = meshgrid(linspace(1,n,ceil(diagLength/sqrt(2))));
    
    tform = cp2tform(corners, [cropFactor cropFactor; 
        cropFactor n+(1-cropFactor); n+(1-cropFactor) (1-cropFactor); 
        n+cropFactor n+cropFactor], 'projective');
    [xi,yi] = tforminv(tform, xgrid, ygrid);
    img = interp2(img, xi, yi, 'nearest');
    
    
end



if isa(img, 'uint8')
    minValue = 1;
else
    minValue = 1/256;
end

% Remove low-frequency variation
nrows = size(img, 1);
sigma = 1.5*nrows/n;
%g = fspecial('gaussian', max(3, ceil(3*sigma)), sigma);
g = gaussian_kernel(sigma);
img = min(1, img ./ max(minValue, separable_filter(img, g, g', 'replicate')));

mid = (n+1)/2;
%numBits = n^2 - 2*n + 1;

squares = imresize(reshape(1:n^2, [n n]), size(img), 'nearest');

[xgrid,ygrid] = meshgrid(1:size(squares,2), 1:size(squares,1));

xcen = imresize(reshape(accumarray(squares(:), xgrid(:), [n^2 1], @mean), [n n]), size(img), 'nearest'); 
ycen = imresize(reshape(accumarray(squares(:), ygrid(:), [n^2 1], @mean), [n n]), size(img), 'nearest'); 

weightSigma = nrows/n/6;
weights = exp(-.5 * ((xgrid-xcen).^2 + (ygrid-ycen).^2)/weightSigma^2);

%namedFigure('DecodeWeights'), subplot 121, imshow(img), overlay_image(weights, 'r'); 

means = reshape(accumarray(squares(:), weights(:).*img(:), [n^2 1]), [n n]);
totalWeight = reshape(accumarray(squares(:), weights(:), [n^2 1]), [n n]);
means = means ./ totalWeight;

% Compute threshold as halfway b/w darkest and brightest mean
if isempty(darkCorners)
    threshold = (max(means(:)) + min(means(:)))/2;
else
    threshold = (max(means(:)) + darkCorners)/2;
end

%namedFigure('DecodeWeights'), subplot 122, imshow(means > threshold), pause

if all(means > threshold)
    % empty square
    blockType = 0;
    faceType = 0;
    isValid = false;
    keyOrient = 'none';
    return
end

upBits    = false(n); upBits(1:(mid-1),mid) = true;
downBits  = false(n); downBits((mid+1):end,mid) = true;
leftBits  = false(n); leftBits(mid,1:(mid-1)) = true;
rightBits = false(n); rightBits(mid,(mid+1):end) = true;

dirMeans = [mean(means(upBits)) mean(means(downBits)) mean(means(leftBits)) mean(means(rightBits))];
[~,whichDir] = max(dirMeans);

dirs = {'up', 'down', 'left', 'right'};
keyOrient = dirs{whichDir};
switch(keyOrient)
    case 'down'
        means = rot90(rot90(means));
    case 'left'
        means = rot90(rot90(rot90(means)));
    case 'right'
        means = rot90(means);
end

valueBits = true(n);
valueBits(:,mid) = false;
valueBits(mid,:) = false;

binaryString = strrep(num2str(means(valueBits)' < threshold), ' ', '');
[blockType, faceType, isValid] = decodeIDs(bin2dec(binaryString));

end