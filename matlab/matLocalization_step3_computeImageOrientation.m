function [orient] = matLocalization_step3_computeImageOrientation(orient, mag, embeddedConversions, DEBUG_DISPLAY)

% Bin into 1-degree bins, using linear interpolation
orient = 180/pi*orient(:);
leftBin = floor(orient);
leftBinWeight = 1-(orient-leftBin);
rightBin = leftBin + 1;
rightBin(rightBin==181) = 1;
%counts = row(accumarray(round(180/pi*orient(:))+1, mag(:)));
assert(all(leftBinWeight>=0 & leftBinWeight <= 1), ...
    'Bin weights should be [0,1]');
counts = row(accumarray(leftBin+1, leftBinWeight.*mag(:), [181 1]));
counts = counts + row(accumarray(rightBin+1, (1-leftBinWeight).*mag(:), [181 1]));
%counts = imfilter(counts, gaussian_kernel(1), 'circular');

% First and last bin (0 and 180) are same thing
counts(1) = counts(1) + counts(181);
counts = counts(1:180);

% Find peaks
localMaxima = find(counts > counts([end 1:end-1]) & counts > counts([2:end 1]));
if length(localMaxima) < 2
    error('Could not find at least 2 peaks in orientation histogram.');
end

[~,sortIndex] = sort(counts(localMaxima), 'descend');
localMaxima = localMaxima(sortIndex(1:2));

orient = -(localMaxima-1)*pi/180; % subtract 1 because of +1 in accumarray above
if orient(2) > orient(1)
    orient = orient([2 1]);
    localMaxima = localMaxima([2 1]);
end
orientDiff = orient(1) - orient(2); 
if abs(orientDiff - pi/2) > 5*pi/180
    warning('Found two orientation peaks, but they are not ~90 degrees apart.');
end

% Fit a parabola to the peak orientation to get sub-bin orientation
% precision
assert(length(counts)==180, 'Expecting 180 orientation bins');
if localMaxima(1)==1
    bins = [180 1 2]';
elseif localMaxima(1)==180
    bins = [179 180 1]';
else
    bins = localMaxima(1) + [-1 0 1]';
end
% solve for parameters of parabola:
A = [bins.^2 bins ones(3,1)];
p = linsolve(A, counts(bins)');


% Find max location of the parabola *in terms of bins*.  Remember to
% subtract 1 again because of +1 in accumarray above.  Then convert to
% radians.
orient = -(-p(2)/(2*p(1)) - 1) * pi/180; 
