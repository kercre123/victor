% function counts = integerCounts(image, imageRegionOfInterest)

% NOTE: Has slightly different edges than the C version

% imageRegionOfInterest is a 4x2 quad, with each row [x,y]

% Example:
% image = reshape(0:(15^2 - 1), [15,15])';
% imageRegionOfInterest = 1 + [1, 5; 6, 6; 7, 11; 0, 11];
% counts = integerCounts(image, imageRegionOfInterest);

% Usage example: Compute the bottom 1%
% countsSum = cumsum(counts); 
% countsSum = countsSum / max(countsSum);
% grayvalue = find(countsSum >= 0.01, 1) - 1;

function counts = integerCounts(image, imageRegionOfInterest)
    assert(ndims(image) == 2);
    
    if ~exist('imageRegionOfInterest', 'var')
        imageRegionOfInterest = [-1,-1; size(image,2)+1,-1; size(image,2)+1,size(image,1)+1; -1,size(image,1)+1];
    end
    
    assert(min(size(imageRegionOfInterest)==[4,2]) == 1);
    
    mask = roipoly(image, imageRegionOfInterest(:,1), imageRegionOfInterest(:,2));
    
    counts = hist(image(mask), 0:255);