
% function warpProjective()
%
% image: input image (must be type double)
% H: 3x3 projective transform matrix
% outputImageSize: A 2x1 vector of the size of result, such as outputImageSize=[768,1024]
% regionOfInterestQuad: A 4x2 matrix with [x,y] coordinates of corner points
% interpMethod: Optional interpolation method, one of {'nearest','linear','spline','cubic'}
% result: output image
%
% If a transformed point is outside of the volume, NaN is used
%
% Simple case
% result = warpProjective(ones(60,80), [cos(.1), -sin(.1), 20; sin(.1), cos(.1), 20; 0,0,1], 2*[60,80]);
%
% Add a horizontal box as a regionOfInterest
% result = warpProjective(ones(60,80), [cos(.1), -sin(.1), 20; sin(.1), cos(.1), 20; 0,0,1], 2*[60,80], [30,30;60,30;60,40;30,40]);

function [result, mask, warpedMask] = warpProjective(image, H, outputImageSize, regionOfInterestQuad, interpMethod)

outputImageSize = double(outputImageSize(1:2));
 
if ~exist('regionOfInterestQuad', 'var') || isempty(regionOfInterestQuad)
    regionOfInterestQuad = [0,0; 0,size(image,1); size(image,2),size(image,1); size(image,2),0];
end

if ~exist('interpMethod','var') || isempty(regionOfInterestQuad)
   interpMethod='linear';
end

% Compute coordinates corresponding to input 
% and transformed coordinates for result

[xo,yo] = meshgrid(1:size(image,2), 1:size(image,1));
[x,y] = meshgrid(1:outputImageSize(2), 1:outputImageSize(1));

coords = [x(:)'; y(:)'];

homogeneousCoords = [coords; ones(1,prod(outputImageSize))];
warpedCoords = inv(H)*homogeneousCoords;

xprime = warpedCoords(1,:) ./ warpedCoords(3,:);
yprime = warpedCoords(2,:) ./ warpedCoords(3,:);

mask = roipoly(image, regionOfInterestQuad(:,1), regionOfInterestQuad(:,2));
warpedMask = reshape(interp2(xo, yo, double(mask), xprime, yprime, interpMethod), outputImageSize);

if ndims(image)==3
    warpedMask = repmat(warpedMask, [1,1,3]);
    
    result = zeros([outputImageSize(1), outputImageSize(2), 3]);
    
    for i=1:3
        resultTmp = interp2(xo, yo, double(image(:,:,i)), xprime, yprime, interpMethod);
        result(:,:,i) = reshape(resultTmp, outputImageSize);
    end    
else
    result = interp2(xo, yo, double(image), xprime, yprime, interpMethod);
    result = reshape(result, outputImageSize);
end

result(warpedMask < .01) = nan;

