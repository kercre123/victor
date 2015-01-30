
% function parseLedCode_dutyCycle(varargin)

function [colorIndex1, numPositive1, colorIndex2, numPositive2] = parseLedCode_dutyCycle(varargin)
    numFramesToTest = 15;
    cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    filenamePattern = '~/Documents/Anki/products-cozmo-large-files/blinkyImages10/red_%05d.png';
    whichImages = 1:100;
    
    processingSize = [120,160];
    
    saturationThreshold = 254; % Ignore pixels where any color is higher than this
    
    lightSquareCenter = [120, 160] * (processingSize(1) / 240);
    lightSquareWidths = [10, 20, 30, 40, 50, 60, 70, 80] * (processingSize(1) / 240);
    %     lightRectangle = round([140,180, 100,140] * (processingSize(1) / 240));
    alignmentRectangle = [110, 210, 70, 170] * (processingSize(1) / 240);
    
    useBoxFilter = false;
    
    showFigures = true;
    
    knownLedColor = [];
    
    parseVarargin(varargin);
    
    images = parseLedCode_captureAllImages(cameraType, filenamePattern, whichImages, processingSize, numFramesToTest, showFigures);
    
    if isempty(images)
        return;
    end
    
    if useBoxFilter
        smallBlurKernel = ones(11, 11);
        smallBlurKernel = smallBlurKernel / sum(smallBlurKernel(:));
    else
%         smallBlurKernel = fspecial('gaussian',[11,11],3);
        smallBlurKernel = fspecial('gaussian',[21,21],6);
    end
    
    blurredImages = imfilter(images, smallBlurKernel);
    
%     figure(3); imshow(blurredImages(:,:,:,1));
    
    % Align the images
    %     largeBlurKernel = ones(31, 31);
    %     largeBlurKernel = largeBlurKernel / sum(largeBlurKernel(:));
    %     toTrackImages = imfilter(blurredImages, largeBlurKernel);
    %
    %     templateImage = rgb2gray(toTrackImages(:,:,:,ceil(size(toTrackImages,4)/2)));
    %     numPyramidLevels = 2;
    %     transformType = bitshift(6,8); % bitshift(2,8); %translation
    %     ridgeWeight = 1e-3;
    %     maxIterations = 50;
    %     convergenceTolerance = 0.05;
    %     homography = eye(3,3);
    %
    %     translationHomographies = cell(size(images,4), 1);
    %     warpedImages = zeros(size(toTrackImages), 'uint8');
    %     for iImage = 1:size(toTrackImages,4)
    %         curImage = rgb2gray(toTrackImages(:,:,:,iImage));
    %         translationHomographies{iImage} = mexTrackLucasKanade(templateImage, double(alignmentRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), curImage, double(maxIterations), double(convergenceTolerance), double(homography));
    %
    %         [warpedImages(:,:,:,iImage), ~, ~] = warpProjective(images(:,:,:,iImage), inv(translationHomographies{iImage}), size(curImage));
    %     end
    
    colors = cell(length(lightSquareWidths), 1);
    colorsHsv = cell(length(lightSquareWidths), 1);
    
    for iWidth = 1:length(lightSquareWidths)
        curWidth = lightSquareWidths(iWidth) / 2;
        yLimits = round([lightSquareCenter(1)-curWidth, lightSquareCenter(1)+curWidth]);
        xLimits = round([lightSquareCenter(2)-curWidth, lightSquareCenter(2)+curWidth]);
        
        colorPatches = blurredImages(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :, :);
        
        [colors{iWidth}, colorsHsv{iWidth}] = extractColors(colorPatches, saturationThreshold);
        
        if showFigures
            figure(1);
            subplot(3, ceil(length(lightSquareWidths)/3), iWidth);
            plot(colors{iWidth}(3:-1:1,:)');
        end
    end
    
    numHistogramBins = 16;
    histogramBlockSize = [24,32];
    [tmp_rawBlockHistogram, ~, ~, ~] = getBlockHistograms(blurredImages(:,:,:,1), numHistogramBins, histogramBlockSize);

    rawBlockHistograms = zeros([size(tmp_rawBlockHistogram), size(blurredImages,4)]);
    normalizedRawBlockHistograms = zeros([size(tmp_rawBlockHistogram), size(blurredImages,4)]);
    maxPooledBlockHistograms = zeros([size(tmp_rawBlockHistogram), size(blurredImages,4)]);
    normalizedMaxPooledBlockHistograms = zeros([size(tmp_rawBlockHistogram), size(blurredImages,4)]);
    for iImage = 1:size(blurredImages,4)
        [rawBlockHistograms(:,:,:,:,iImage), normalizedRawBlockHistograms(:,:,:,:,iImage), maxPooledBlockHistograms(:,:,:,:,iImage), normalizedMaxPooledBlockHistograms(:,:,:,:,iImage)] = getBlockHistograms(blurredImages(:,:,:,iImage), 16, [24,32]);
    end
    
    %     for iImage = 1:size(blurredImages,4)
    %         figure(10+iImage);
    %         plotSpatialHistogram(reorderedHistograms);
    %     end
    
%     figure(4); plotSpatialHistogram(reorderedHistograms(:,1,:,:,1) - reorderedHistograms(:,1,:,:,6));
%     figure(4); plotSpatialHistogram(squeeze(reorderedHistograms(:, :, ceil(end/2), ceil(end/2), :)));

    centerHists = squeeze(normalizedRawBlockHistograms(:, :, ceil(end/2), ceil(end/2), :));
    minHist = min(centerHists, [], 3);
    differenceFromMin = centerHists - repmat(minHist, [1,1,size(normalizedRawBlockHistograms,5)]);
%     figure(4); plotSpatialHistogram(differenceFromMin);
    
    weightedDifferenceFromMean = squeeze(sum(differenceFromMin .* repmat((1:size(differenceFromMin,1))', [1,size(differenceFromMin,2),size(differenceFromMin,3)])));
    
    if showFigures
        figure(4); plot(weightedDifferenceFromMean(3:-1:1,:)');
        curAxis = axis();
        axis([curAxis(1), curAxis(2), 0, curAxis(4)]);
    end
    
   [percentPositive1, colorIndex1, colorName1] = computeDutyCycle(colors{4}, knownLedColor);
   [percentPositive2, colorIndex2, colorName2] = computeDutyCycle(weightedDifferenceFromMean, knownLedColor);
    
%    disp(sprintf('\n\n'));
   
    numPositive1 = round(numFramesToTest*percentPositive1);
    numPositive2 = round(numFramesToTest*percentPositive2);

%     keyboard
    
end % function parseLedCode_lightOnly()

function [percentPositive, colorIndex, colorName] = computeDutyCycle(colors, knownLedColor)
    colorNames = {'red', 'green', 'blue'};
    
     % Finding the single max may not be the most robust?
    rgbThresholds = (max(colors, [] ,2) + min(colors, [] ,2)) / 2;
    
    if isempty(knownLedColor)
        maxRangeRgb = max(colors, [] ,2) - min(colors, [] ,2);
        [~, colorIndex] = max(maxRangeRgb);
    else
        colorIndex = knownLedColor;
    end
    
    percentPositive = length(find(colors(colorIndex,:) > rgbThresholds(colorIndex))) / size(colors,2);
    
    isValid = true;
    
    % Rejection heuristics
    
    %     if maxVal < 5
    %         isValid = false;
    %     end
    
    if isValid
        numFramesToTest = size(colors,2);
        colorName = colorNames{colorIndex};
        disp(sprintf('%s is at %0.2f', colorName, numFramesToTest*percentPositive));
    else
        colorName = 'unknown';
        disp('Unknown color and number');
    end
end

function [colors, colorsHsv] = extractColors(colorImages, saturationThreshold)
    minImageBlurred = double(min(colorImages, [], 4));
    maxImageBlurred = double(max(colorImages, [], 4));
    %thresholdImageBlurred = (maxImageBlurred + minImageBlurred) / 2;
    thresholdImageBlurred = minImageBlurred;
    
    grayMaxImageBlurred = max(maxImageBlurred, [], 3);
    unsaturatedInds = (grayMaxImageBlurred <= saturationThreshold);
    
    % Find the per-pixel difference from the threshold
    colors = zeros(3, size(colorImages,4));
    colorsHsv = zeros(3, size(colorImages,4));
    
    for iImage = 1:size(colorImages,4)
        curBlurred = double(colorImages(:,:,:,iImage))/2 - thresholdImageBlurred/2;
        curBlurredHsv = rgb2hsv(curBlurred);
        
        for iChannel = 1:3
            curChannelImage = curBlurred(:,:,iChannel);
            colors(iChannel, iImage) = mean(curChannelImage(unsaturatedInds));
            
            curChannelImageHsv = curBlurredHsv(:,:,iChannel);
            colorsHsv(iChannel, iImage) = mean(curChannelImageHsv(unsaturatedInds));
        end
    end % for iImage = 1:size(colorImages,4)
end % function colors = extractColors()

function [rawBlockHistogram, normalizedRawBlockHistogram, maxPooledBlockHistogram, normalizedMaxPooledBlockHistogram] = getBlockHistograms(colorImage, numBinsPerColor, blockWidth)
    
    blockStep = blockWidth / 2;
    
    ys = 1:blockStep(1):(size(colorImage,1)-blockWidth(1)+1);
    xs = 1:blockStep(2):(size(colorImage,2)-blockWidth(2)+1);
    
    rawBlockHistogram = zeros([numBinsPerColor, 3, length(ys), length(xs)]);
    normalizedRawBlockHistogram = zeros([numBinsPerColor, 3, length(ys), length(xs)]);
    
    for iy = 1:length(ys)
        y = ys(iy);
        for ix = 1:length(xs)
            x = xs(ix);
            
            for iChannel = 1:3
                curBlock = colorImage(y:(y+blockWidth(1)-1), x:(x+blockWidth(2)-1), iChannel);
                rawBlockHistogram(:,iChannel,iy,ix) = hist(curBlock(:), linspace(0,256,numBinsPerColor));
                normalizedRawBlockHistogram(:,iChannel,iy,ix) = rawBlockHistogram(:,iChannel,iy,ix) / sum(rawBlockHistogram(:,iChannel,iy,ix));
            end
        end
    end
    
    maxPooledBlockHistogram = zeros([numBinsPerColor, 3, length(ys), length(xs)]);
    normalizedMaxPooledBlockHistogram = zeros([numBinsPerColor, 3, length(ys), length(xs)]);
    
    for iy = 2:(length(ys)-1)
        for ix = 2:(length(xs)-1)
            for dy = -1:1
                for dx = -1:1
                    maxPooledBlockHistogram(:,:,iy,ix) = max(maxPooledBlockHistogram(:,:,iy,ix), rawBlockHistogram(:,:,iy+dy,ix+dx));
                end
            end
            
            normalizedMaxPooledBlockHistogram(:,:,iy,ix) = maxPooledBlockHistogram(:,:,iy,ix) / sum(sum(maxPooledBlockHistogram(:,:,iy,ix)));
        end
    end
    
end % function getBlockHistograms()

function plotSpatialHistogram(reorderedHistograms)
    ci = 1;
    for iy = 1:(size(reorderedHistograms, 3))
        for ix = 1:(size(reorderedHistograms, 4))
            subplot(size(reorderedHistograms, 3), size(reorderedHistograms, 4), ci);
            
            plot(reorderedHistograms(:,3:-1:1,iy,ix,1))
%             axis([0, 16, -0.02, 0.02]);
            ci = ci + 1;
        end
    end
end % function plotSpatialHistogram()
