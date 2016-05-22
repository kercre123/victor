
% function warpedImage = binaryTracker_warpWithHomography(image, homography)

function warpedImage = binaryTracker_warpWithHomography(image, homography)
 
if ~exist('interpMethod','var')
   interpMethod='linear';
end

scale = 1.0;

% Compute coordinates corresponding to input 
% and transformed coordinates for warpedImage

imageHeight = size(image, 1);
imageWidth = size(image, 2);

imageResizedHeight = imageHeight * scale;
imageResizedWidth = imageWidth * scale;

homographyOffset = [(imageWidth-1)/2, (imageHeight-1)/2];

[x, y] = meshgrid(...
    linspace(-homographyOffset(1), homographyOffset(1), imageResizedWidth), ...
    linspace(-homographyOffset(2), homographyOffset(2), imageResizedHeight));

coords = [x(:)'; y(:)'];

homogeneousCoords = [coords; ones(1,prod([imageHeight, imageWidth]))];
warpedCoords = inv(homography)*homogeneousCoords;

xprime = warpedCoords(1,:) ./ warpedCoords(3,:);
yprime = warpedCoords(2,:) ./ warpedCoords(3,:);

if ndims(image)==3
    warpedImage = zeros([imageHeight, imageWidth, 3]);
    
    for i=1:3
        warpedImageTmp = interp2(x, y, double(image(:,:,i)), xprime, yprime, interpMethod);
        warpedImage(:,:,i) = reshape(warpedImageTmp, [imageHeight, imageWidth]);
    end    
else
    warpedImage = interp2(x, y, double(image), xprime, yprime, interpMethod);
    warpedImage = reshape(warpedImage, [imageHeight, imageWidth]);
end