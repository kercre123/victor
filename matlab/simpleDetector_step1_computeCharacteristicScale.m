function binaryImg = simpleDetector_step1_computeCharacteristicScale( ...
    maxSmoothingFraction, nrows, ncols, downsampleFactor, img, ...
    thresholdFraction, usePyramid, embeddedConversions, showTiming, DEBUG_DISPLAY)

% % Simpler method (one window size for whole image)
% averageImg = separable_filter(img, gaussian_kernel(avgSigma));

% Create a set of "average" images with different-sized Gaussian kernels
numScales = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));

if usePyramid
    assert(downsampleFactor == 2, ...
        'Using image pyramid for characteristic scale requires downsampleFactor=2.');
    
    if showTiming, t = tic; end
    
    % Note: the default thresholdFraction is different for
    % matlab_boxFilters than the other options
    
    if strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_original')
        averageImg = computeCharacteristicScaleImage(img, numScales);
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_loops')
        averageImg = computeCharacteristicScaleImage_loops(img, numScales);
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_loopsAndFixedPoint')
        averageImg = double(computeCharacteristicScaleImage_loopsAndFixedPoint(img, numScales, false, false)) / (255 * 2^16);
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_loopsAndFixedPoint_mexFiltering')
        averageImg = double(computeCharacteristicScaleImage_loopsAndFixedPoint(img, numScales, false, true)) / (255 * 2^16);
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'c_fixedPoint')
        averageImg = double(mexComputeCharacteristicScale(im2uint8(img), numScales)) / (255 * 2^16);
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_boxFilters')
        binaryImg = computeBinaryCharacteristicScaleImage_boxFilters(img, numScales, thresholdFraction);
%         figure(3); imshow(binaryImg);
        return;
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_boxFilters_multiple')
        binaryImg = computeBinaryCharacteristicScaleImage_boxFilters_multiple(img, numScales, thresholdFraction);
%         figure(5); imshow(binaryImg);
        return;
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_boxFilters_small') 
        binaryImg1 = computeBinaryCharacteristicScaleImage_boxFilters_small(img, numScales, embeddedConversions.smallCharacterisicParameter);
        binaryImg2 = computeBinaryCharacteristicScaleImage_boxFilters(img, [4,8], thresholdFraction);
        binaryImg = binaryImg1 + binaryImg2;
%         figure(6); imshow(binaryImg);
            return;
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_edges') 
        binaryImg = edge(img, 'canny', 0.1, 0.1);
%         figure(6); imshow(binaryImg);
        return
    elseif strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_iterativeBox') 
        boxWidth = 31;
        numIterations = numScales;
        binaryImg = computeBinaryCharacteristicScaleImage_iterativeBox(img, boxWidth, numIterations, thresholdFraction, false);
    else
        assert(false);
    end
    
    if showTiming
        fprintf('computeCharacteristicScaleImage took %f.\n', toc(t));
    end
    
else % Use a stack of smoothed images, not a pyramid
    numSigma = 2.5;
    prevSigma = 0.5/numSigma;
    
    G = cell(1,numScales+1);
    
    [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
    
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
    index = ygrid + (xgrid-1)*nrows + (whichScale)*nrows*ncols; % not whichScale-1 on purpose!
    averageImg = G(index);
    
end % IF usePyramid

% Threshold:
binaryImg = img < thresholdFraction*averageImg;

% figure(2); imshow(binaryImg);

% keyboard

%figure(3); imshow(binaryImg);
% figure(3); imshow(imresize(binaryImg, [120,160]));

% % Get rid of spurious detections on the edges of the image
% binaryImg([1:minDotRadius end:(end-miTnDotRadius+1)],:) = false;
% binaryImg(:,[1:minDotRadius end:(end-minDotRadius+1)]) = false;
