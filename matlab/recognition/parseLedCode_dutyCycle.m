
% function parseLedCode_dutyCycle(varargin)

function [colorIndex, numPositive] = parseLedCode_dutyCycle(varargin)
    numFramesToTest = 15;
    cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    filenamePattern = '~/Documents/Anki/products-cozmo-large-files/blinkyImages10/red_%05d.png';
    whichImages = 1:100;
    
    processingSize = [120,160];
    
    saturationThreshold = 254; % Ignore pixels where any color is higher than this
    
    lightSquareCenter = [120, 160] * (processingSize(1) / 240);
    %     lightSquareWidths = [10, 20, 30, 40, 50, 60, 70, 80] * (processingSize(1) / 240);
    lightSquareWidths = [40] * (processingSize(1) / 240);
    %     lightRectangle = round([140,180, 100,140] * (processingSize(1) / 240));
    
    alignmentRectangle = [110, 210, 70, 170] * (processingSize(1) / 240);
    %     alignmentRectangle = [120, 200, 80, 160] * (processingSize(1) / 240);
    
    %             spatialBlurSigmas = linspace(1,10,9);
    spatialBlurSigmas = linspace(2,6,5);
    
    useBoxFilter = false;
    alignmentType = 'exhaustiveTranslation'; % {'none', 'exhaustiveTranslation', 'exhaustiveTranslation-double'}
    
    parsingType = 'blur'; % {'spatialBlur', 'blur', 'histogram'}
    
    showFigures = true;
    
    knownLedColor = [];
    
    if useBoxFilter
        smallBlurKernel = ones(11, 11);
        smallBlurKernel = smallBlurKernel / sum(smallBlurKernel(:));
    else
        %         smallBlurKernel = fspecial('gaussian',[11,11],3);
        smallBlurKernel = fspecial('gaussian',[21,21],6);
    end
    
    parseVarargin(varargin);
    
    images = parseLedCode_captureAllImages(cameraType, filenamePattern, whichImages, processingSize, numFramesToTest, showFigures);
    
    if isempty(images)
        return;
    end
    
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
    
    if strcmpi(alignmentType, 'exhaustiveTranslation') || strcmpi(alignmentType, 'exhaustiveTranslation-double')
        maxOffset = 9;
        offsetImageSize = [60,80];
        %         offsetImageScale = processingSize(1) / offsetImageSize(1);
        %
        %         imagesTmp = images;
        %         imagesTmp(1:alignmentRectangle(3), :, :, :) = 0;
        %         imagesTmp(alignmentRectangle(4):end, alignmentRectangle(1):alignmentRectangle(2),:,:) = 0;
        %         imagesTmp(:, 1:alignmentRectangle(1),:,:) = 0;
        %         imagesTmp(:, alignmentRectangle(2):end, :, :) = 0;
        
        %         [bestDys,bestDxs] = parseLedCode_computeBestTranslations(imagesTmp, offsetImageSize, maxOffset);
        [bestDys, bestDxs, offsetImageScale] = parseLedCode_computeBestTranslations(images, offsetImageSize, maxOffset, alignmentRectangle, true);
        
        alignedImages = zeros(size(images), 'uint8');
        for iImage = 1:size(images,4)
            alignedImages(:,:,:,iImage) = exhuastiveAlignment_shiftImage(images(:,:,:,iImage), bestDys(iImage), bestDxs(iImage), maxOffset/offsetImageScale);
        end
        
        images = alignedImages;
    end % if strcmpu(alignmentType, 'exhaustiveTranslation')
    
    blurredImages = imfilter(images, smallBlurKernel);
    
    colors = cell(length(lightSquareWidths), 1);
    colorsHsv = cell(length(lightSquareWidths), 1);
    
    for iWidth = 1:length(lightSquareWidths)
        curWidth = lightSquareWidths(iWidth) / 2;
        yLimits = round([lightSquareCenter(1)-curWidth, lightSquareCenter(1)+curWidth]);
        xLimits = round([lightSquareCenter(2)-curWidth, lightSquareCenter(2)+curWidth]);
        
        colorPatches = blurredImages(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :, :);
        
        if strcmpi(alignmentType, 'exhaustiveTranslation-double')
            maxOffsetLocal = 5;
            [bestDys,bestDxs] = parseLedCode_computeBestTranslations(colorPatches, [size(colorPatches,1),size(colorPatches,2)], maxOffsetLocal);
            
            for iImage = 1:size(images,4)
                moreShifted = uint8(exhuastiveAlignment_shiftImage(blurredImages(:,:,:,iImage), bestDys(iImage), bestDxs(iImage), maxOffsetLocal));
                colorPatches(:,:,:,iImage) = moreShifted(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :);
            end
        end
        
        if strcmpi(parsingType, 'spatialBlur')
            if isempty(knownLedColor)
                numPositives = zeros(3,1);
                peakValues = zeros(3,1);
                
                for iColor = 1:3
                    [numPositives(iColor), peakValues(iColor)] = dutyCycle_changingCenter(colorPatches, spatialBlurSigmas, iColor);
                end
                
                [~, colorIndex] = max(peakValues);
                numPositive = numPositives(colorIndex);
            else
                [numPositive, peakValue] = dutyCycle_changingCenter(colorPatches, spatialBlurSigmas, knownLedColor);
                colorIndex = knownLedColor;
            end
            
            return;
        else
            [colors{iWidth}, colorsHsv{iWidth}] = extractColorsAverage(colorPatches, saturationThreshold);
        end
        
        if showFigures
            figure(1);
            subplot(3, ceil(length(lightSquareWidths)/3), iWidth);
            plot(colors{iWidth}(3:-1:1,:)');
        end
    end
    
    if strcmpi(parsingType, 'blur') || strcmpi(parsingType, 'spatialBlur')
        [percentPositive, colorIndex, ~] = computeDutyCycle(colors{1}, knownLedColor);
    elseif strcmpi(parsingType, 'histogram')
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
        
        [percentPositive, colorIndex, ~] = computeDutyCycle(weightedDifferenceFromMean, knownLedColor);
    else
        assert(false);
    end % if strcmpi(parsingType, 'blur') ... else
    
    %    disp(sprintf('\n\n'));
    
    numPositive = round(numFramesToTest*percentPositive);
    
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
    
    %     if isValid
    %         numFramesToTest = size(colors,2);
    %         colorName = colorNames{colorIndex};
    %         disp(sprintf('%s is at %0.2f', colorName, numFramesToTest*percentPositive));
    %     else
    %         colorName = 'unknown';
    %         disp('Unknown color and number');
    %     end
    
    if isValid
        colorName = colorNames{colorIndex};
    else
        percentPositive = 0;
        colorIndex = -1;
        colorName = 'unknown';
    end
end % function computeDutyCycle()

function [colors, colorsHsv] = extractColorsAverage(colorImages, saturationThreshold)
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
end % function colors = extractColorsAverage()

function [numPositive, peakValue] = dutyCycle_changingCenter(colorImages, blurSigmas, whichColor)
    
    grayImages = squeeze(colorImages(:,:,whichColor,:));
    
    %     blurSigmas = linspace(1,10,10);
    blurKernels_small = cell(length(blurSigmas), 1);
    blurKernels_large = cell(length(blurSigmas), 1);
    blurKernels_outsideRing = cell(length(blurSigmas), 1);
    for iBlur = 1:length(blurSigmas)
        filterWidth = blurSigmas(iBlur) * 4;
        blurKernels_small{iBlur} = fspecial('gaussian', 2*[filterWidth,filterWidth], blurSigmas(iBlur));
        blurKernels_large{iBlur} = fspecial('gaussian', 2*[filterWidth,filterWidth], 2*blurSigmas(iBlur));
        
        blurKernels_outsideRing{iBlur} = blurKernels_large{iBlur} - blurKernels_small{iBlur};
        blurKernels_outsideRing{iBlur}(blurKernels_outsideRing{iBlur} < 0) = 0;
        blurKernels_outsideRing{iBlur} = blurKernels_outsideRing{iBlur} / sum(sum(blurKernels_outsideRing{iBlur}));
    end % for iBlur = 1:length(blurSigmas)
    
    blurredImages_small = zeros([size(grayImages),length(blurSigmas)], 'uint8');
    blurredImages_large = zeros([size(grayImages),length(blurSigmas)], 'uint8');
    blurredImages_outsideRing = zeros([size(grayImages),length(blurSigmas)], 'uint8');
    
    %     figure(1);
    %     hold off
    for iBlur = 1:length(blurSigmas)
        blurredImages_small(:,:,:,iBlur) = imfilter(grayImages, blurKernels_small{iBlur});
        blurredImages_large(:,:,:,iBlur) = imfilter(grayImages, blurKernels_large{iBlur});
        blurredImages_outsideRing(:,:,:,iBlur) = imfilter(grayImages, blurKernels_outsideRing{iBlur});
    end % for iBlur = 1:length(blurSigmas)
    
    blurredImages_inside = blurredImages_small - blurredImages_outsideRing;
    blurredImages_inside(blurredImages_inside<0) = 0;
    
    stepUpFilter = [-1, 0, 1]';
    
    blurredImages_inside_reshaped = double(permute(blurredImages_inside, [3,1,2,4]));
    steps = imfilter(blurredImages_inside_reshaped, stepUpFilter, 'circular');
    
    % Per x,y,blur, what is the minimum of: the positive peaks and the negative peaks
    maxSteps = squeeze(min(max(steps), max(-steps)));
    peakValue = max(maxSteps(:));
    inds = find(maxSteps == peakValue, 1);
    [bestY,bestX,bestB] = ind2sub(size(maxSteps), inds);
    
    [~, startPoint] = max(steps(:,bestY,bestX,bestB));
    [~, endPoint] = min(steps(:,bestY,bestX,bestB));
    numPositive = mod(endPoint - startPoint, size(colorImages,4)) + 0.5;
    
    %     clickGui = true;
    clickGui = false;
    if clickGui
        parseLedCode_plot_guiChangingCenter(colorImages, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps);
    end % if clickGui
    %     playVideo({blurredImages(:,:,:,:,a,1), blurredImages(:,:,:,:,a,2), blurredImages(:,:,:,:,a,3)})
end % function colors = dutyCycle_changingCenter()

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

