% function counts = integerCounts(image, imageRegionOfInterest)

% NOTE: Has slightly different edges than the C version

% imageRegionOfInterest is a 4x2 quad, with each row [x,y]

% Example:
% image = reshape(0:(15^2 - 1), [15,15])';
% imageRegionOfInterest = 1 + [1, 5; 6, 6; 7, 11; 0, 11];
% counts = integerCounts(image, imageRegionOfInterest);

function counts = integerCounts(image, imageRegionOfInterest)
    assert(ndims(image) == 2);
    assert(min(size(imageRegionOfInterest)==[4,2]) == 1);
    
    mask = roipoly(image, imageRegionOfInterest(:,1), imageRegionOfInterest(:,2));
    
    counts = hist(image(mask), 0:255);