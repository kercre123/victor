function binaryImg = simpleDetector_step1_computeCharacteristicScale(maxSmoothingFraction, nrows, ncols, downsampleFactor, img, thresholdFraction, embeddedConversions, DEBUG_DISPLAY)

% % Simpler method (one window size for whole image)
% averageImg = separable_filter(img, gaussian_kernel(avgSigma));

% Create a set of "average" images with different-sized Gaussian kernels
numScales = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));
G = cell(1,numScales+1); 
numSigma = 2.5;
prevSigma = 0.5 / numSigma;
%G{1} = separable_filter(img, gaussian_kernel(prevSigma, numSigma));
%G{1} = imfilter(img, fspecial('gaussian', round(numSigma*prevSigma), prevSigma));

% Use mexGaussianBlur if available, otherwise fall back on Matlab filtering
if exist('mexGaussianBlur', 'file')
    blurFcn = @mexGaussianBlur;
else
    blurFcn = @(img_, addlSigma_, numSigma_)separable_filter(img_, gaussian_kernel(addlSigma_, numSigma_));
end
    
G{1} = blurFcn(img, prevSigma, numSigma);
for i = 1:numScales
    crntSigma = downsampleFactor^(i-1) / numSigma;
    addlSigma = sqrt(crntSigma^2 - prevSigma^2);
    G{i+1} = blurFcn(G{i}, addlSigma, numSigma);
    %G{i+1} = separable_filter(G{i}, gaussian_kernel(addlSigma, numSigma)); 
    %G{i+1} = imfilter(G{i}, fspecial('gaussian', round(numSigma*addlSigma), addlSigma));
    %G{i+1} = mexGaussianBlur(G{i}, addlSigma, numSigma);
    prevSigma = crntSigma;
end

% Find characteristic scale using DoG stack (approx. Laplacian)
G = cat(3, G{:});
L = G(:,:,2:end) - G(:,:,1:end-1);
[~,whichScale] = max(abs(L),[],3);
[xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
index = ygrid + (xgrid-1)*nrows + (whichScale)*nrows*ncols; % not whichScale-1 on purpose!
averageImg = G(index);

% Threshold:
binaryImg = img < thresholdFraction*averageImg;

% % Get rid of spurious detections on the edges of the image
% binaryImg([1:minDotRadius end:(end-miTnDotRadius+1)],:) = false;
% binaryImg(:,[1:minDotRadius end:(end-minDotRadius+1)]) = false;
