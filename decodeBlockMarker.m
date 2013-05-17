function [blockType, faceType, isValid, keyOrient] = decodeBlockMarker(img)

n = 5;

if size(img,3) > 1
    img = mean(img,3);
end

if isa(img, 'uint8')
    minValue = 1;
else
    minValue = 1/256;
end

% Remove low-frequency variation
nrows = size(img, 1);
sigma = 1.5*nrows/n;
g = fspecial('gaussian', max(3, ceil(3*sigma)), sigma);
img = min(1, img ./ max(minValue, imfilter(img, g, 'replicate')));

mid = (n+1)/2;
%numBits = n^2 - 2*n + 1;

squares = imresize(reshape(1:n^2, [n n]), size(img), 'nearest');

[xgrid,ygrid] = meshgrid(1:size(squares,2), 1:size(squares,1));

xcen = imresize(reshape(accumarray(squares(:), xgrid(:), [], @mean), [n n]), size(img), 'nearest'); 
ycen = imresize(reshape(accumarray(squares(:), ygrid(:), [], @mean), [n n]), size(img), 'nearest'); 

weightSigma = nrows/n/6;
weights = exp(-.5 * ((xgrid-xcen).^2 + (ygrid-ycen).^2)/weightSigma^2);

means = reshape(accumarray(squares(:), weights(:).*img(:)), [n n]);
totalWeight = reshape(accumarray(squares(:), weights(:)), [n n]);
means = means ./ totalWeight;

% Compute threshold as halfway b/w darkest and brightest mean
threshold = (max(means(:)) + min(means(:)))/2;

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