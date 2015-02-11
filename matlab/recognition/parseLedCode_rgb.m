
% function parseLedCode_rgb(varargin)

function [whichColors, numPositive] = parseLedCode_rgb(varargin)
    numFramesToTest = 15;
    cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    filenamePattern = '~/Documents/Anki/products-cozmo-large-files/rgb1/images_%05d.png';
    whichImages = 1:100;
    
    processingSize = [120,160];
    
    lightSquareCenter = [120, 160] * (processingSize(1) / 240);
    lightSquareWidth = 40 * (processingSize(1) / 240);
    alignmentRectangle = [110, 210, 70, 170] * (processingSize(1) / 240);
    
    spatialBlurSigmas = linspace(2,5,4);
    
    %     useBoxFilter = false;
    alignmentType = 'exhaustiveTranslation'; % {'none', 'exhaustiveTranslation'}
    
    parsingType = 'spatialBlur'; % {'spatialBlur'}
    
    scaleDetectionMethod = 'temporalBox'; % { 'originalGaussian', 'temporalGaussian', 'temporalBox'}
    
    redBlueOnly = false;
    
    showFigures = true;
    displayText = false;
    
    knownLedColor = [];
    
    %     if useBoxFilter
    %         smallBlurKernel = ones(11, 11);
    %         smallBlurKernel = smallBlurKernel / sum(smallBlurKernel(:));
    %     else
    %         smallBlurKernel = fspecial('gaussian',[21,21],6);
    %     end
    
    parseVarargin(varargin);
    
    images = parseLedCode_captureAllImages(cameraType, filenamePattern, whichImages, processingSize, numFramesToTest, showFigures, displayText);
    
    if isempty(images)
        whichColors = [];
        numPositive = [];
        return;
    end
    
    if strcmpi(alignmentType, 'exhaustiveTranslation')
        maxOffset = 9;
        offsetImageSize = [60,80];
        
        [bestDys, bestDxs, offsetImageScale] = parseLedCode_computeBestTranslations(images, offsetImageSize, maxOffset, alignmentRectangle, true);
        
        alignedImages = zeros(size(images), 'uint8');
        for iImage = 1:size(images,4)
            alignedImages(:,:,:,iImage) = exhuastiveAlignment_shiftImage(images(:,:,:,iImage), bestDys(iImage), bestDxs(iImage), maxOffset/offsetImageScale);
        end
        
        images = alignedImages;
    end % if strcmpu(alignmentType, 'exhaustiveTranslation')
    
    %     blurredImages = imfilter(images, smallBlurKernel);
    blurredImages = images;
    
    lightSquareHalfWidth = lightSquareWidth / 2;
    yLimits = round([lightSquareCenter(1)-lightSquareHalfWidth, lightSquareCenter(1)+lightSquareHalfWidth]);
    xLimits = round([lightSquareCenter(2)-lightSquareHalfWidth, lightSquareCenter(2)+lightSquareHalfWidth]);
    
    colorPatches = blurredImages(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :, :);
    
    if strcmpi(parsingType, 'spatialBlur')
        if isempty(knownLedColor)
            if redBlueOnly
                [whichColors, numPositive] = dutyCycle_changingCenter_redBlue(colorPatches, spatialBlurSigmas, scaleDetectionMethod);
            else
                [whichColors, numPositive] = dutyCycle_changingCenter(colorPatches, spatialBlurSigmas);
            end
        else
            assert(false);
        end
        
        return;
    else % if strcmpi(parsingType, 'spatialBlur')
        assert(false);
    end % if strcmpi(parsingType, 'spatialBlur') ... else
    
    
    %     keyboard
    
end % function parseLedCode_rgb()

function [whichColors, numPositive] = dutyCycle_changingCenter(colorImages, blurSigmas)
    findHsvPeak = false;
    
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
    
    if findHsvPeak
        hsvImages = zeros(size(colorImages));
        for i = 1:size(colorImages,4)
            hsvImages(:,:,:,i) = rgb2hsv(colorImages(:,:,:,i));
        end
        
        blurredSize = [size(colorImages,1), size(colorImages,2), 4, size(colorImages,4), length(blurSigmas)];
        
        blurredImages_small = zeros(blurredSize, 'double');
        blurredImages_large = zeros(blurredSize, 'double');
        blurredImages_outsideRing = zeros(blurredSize, 'double');
        
        for iBlur = 1:length(blurSigmas)
            blurredImages_small(:,:,1:3,:,iBlur)       = imfilter(colorImages, blurKernels_small{iBlur});
            blurredImages_large(:,:,1:3,:,iBlur)       = imfilter(colorImages, blurKernels_large{iBlur});
            blurredImages_outsideRing(:,:,1:3,:,iBlur) = imfilter(colorImages, blurKernels_outsideRing{iBlur});
            
            blurredImages_small(:,:,4,:,iBlur)       = max(blurredImages_small(:,:,1:3,:,iBlur), [], 3);
            blurredImages_large(:,:,4,:,iBlur)       = max(blurredImages_large(:,:,1:3,:,iBlur), [], 3);
            blurredImages_outsideRing(:,:,4,:,iBlur) = max(blurredImages_outsideRing(:,:,1:3,:,iBlur), [], 3);
        end % for iBlur = 1:length(blurSigmas)
        
        blurredImages_inside = zeros([size(colorImages,1), size(colorImages,2), 5, size(colorImages,4), length(blurSigmas)]);
        blurredImages_inside(:,:,1:4,:,:) = blurredImages_small - blurredImages_outsideRing;
        blurredImages_inside(:,:,5,:,:) = max(blurredImages_inside(:,:,1:3,:,:), [], 3);
        blurredImages_inside(blurredImages_inside<0) = 0;
        
        peakValues = zeros(5, 1);
        bestYs = zeros(5, 1);
        bestXs = zeros(5, 1);
        bestBs = zeros(5, 1);
        
        for iChannel = 1:5
            blurredImages_onMax = permute(squeeze(blurredImages_inside(:,:,iChannel,:,:)), [3,1,2,4]);
            blurredImages_onMax = blurredImages_onMax - repmat(min(blurredImages_onMax), [size(blurredImages_onMax,1),1,1,1]);
            blurredImages_onMax_mean = squeeze(mean(blurredImages_onMax));
            peakValues(iChannel) = max(blurredImages_onMax_mean(:));
            
            inds = find(blurredImages_onMax_mean == peakValues(iChannel), 1);
            [bestYs(iChannel),bestXs(iChannel),bestBs(iChannel)] = ind2sub(size(blurredImages_onMax_mean), inds);
        end
        
        bestColors = zeros(size(colorImages,4), 4);
        for iChannel = 1:4
            %             bestColors(:,iChannel) = squeeze(blurredImages_small(bestYs(iChannel), bestXs(iChannel), iChannel, :, bestBs(iChannel)));
            bestColors(:,iChannel) = squeeze(blurredImages_small(bestYs(1), bestXs(1), iChannel, :, bestBs(1)));
        end
        
        %         whichChannel = 1; % hue
        
        
        
        keyboard
    else
        blurredSize = [size(colorImages,1), size(colorImages,2), 4, size(colorImages,4), length(blurSigmas)];
        blurredImages_small = zeros(blurredSize, 'uint8');
        blurredImages_large = zeros(blurredSize, 'uint8');
        blurredImages_outsideRing = zeros(blurredSize, 'uint8');
        
        for iBlur = 1:length(blurSigmas)
            blurredImages_small(:,:,1:3,:,iBlur)       = imfilter(colorImages, blurKernels_small{iBlur});
            blurredImages_large(:,:,1:3,:,iBlur)       = imfilter(colorImages, blurKernels_large{iBlur});
            blurredImages_outsideRing(:,:,1:3,:,iBlur) = imfilter(colorImages, blurKernels_outsideRing{iBlur});
            
            blurredImages_small(:,:,4,:,iBlur)       = max(blurredImages_small(:,:,1:3,:,iBlur), [], 3);
            blurredImages_large(:,:,4,:,iBlur)       = max(blurredImages_large(:,:,1:3,:,iBlur), [], 3);
            blurredImages_outsideRing(:,:,4,:,iBlur) = max(blurredImages_outsideRing(:,:,1:3,:,iBlur), [], 3);
        end % for iBlur = 1:length(blurSigmas)
        
        blurredImages_inside = zeros([size(colorImages,1), size(colorImages,2), 5, size(colorImages,4), length(blurSigmas)]);
        blurredImages_inside(:,:,1:4,:,:) = blurredImages_small - blurredImages_outsideRing;
        blurredImages_inside(:,:,5,:,:) = max(blurredImages_inside(:,:,1:3,:,:), [], 3);
        blurredImages_inside(blurredImages_inside<0) = 0;
        
        blurredImages_onMax = permute(squeeze(blurredImages_inside(:,:,5,:,:)), [3,1,2,4]);
        blurredImages_onMax = blurredImages_onMax - repmat(min(blurredImages_onMax), [size(blurredImages_onMax,1),1,1,1]);
        blurredImages_onMax_mean = squeeze(mean(blurredImages_onMax));
        peakValue = max(blurredImages_onMax_mean(:));
        
        inds = find(blurredImages_onMax_mean == peakValue, 1);
        [bestY,bestX,bestB] = ind2sub(size(blurredImages_onMax_mean), inds);
        
        bestColors = squeeze(blurredImages_small(bestY,bestX, 1:3, :, bestB));
    end % if findHsvPeak ... else
    
    % Find the best R, G, and B (seeds), then grow them out
    numSamples = size(bestColors,2);
    
    numOffFrames = 3;
    
    colorLabels = zeros(numSamples, 1);
    
    codeIsValid = true;
    for iColor = 1:3
        [~, ind] = max(bestColors(iColor,:));
        ind = ind(ceil(length(ind)/2));
        
        otherColors = [1,2,3];
        otherColors = otherColors(iColor ~= otherColors);
        if bestColors(iColor,ind) < bestColors(otherColors(1),ind) || bestColors(iColor,ind) < bestColors(otherColors(2),ind)
            %             disp('This is an invalid code');
            codeIsValid = false;
            break;
        end
        
        colorLabels(ind) = iColor;
        
        % Check right
        %         disp('right');
        for iSample = [(ind+1):numSamples, 1:(ind-1)]
            if bestColors(iColor,iSample) < bestColors(otherColors(1),iSample) || bestColors(iColor,iSample) < bestColors(otherColors(2),iSample)
                break;
            else
                colorLabels(iSample) = iColor;
                %                 disp(colorLabels')
            end
        end
        
        % Check left
        %         disp('left');
        for iSample = [(ind-1):-1:1, numSamples:-1:(ind+1)]
            if bestColors(iColor,iSample) < bestColors(otherColors(1),iSample) || bestColors(iColor,iSample) < bestColors(otherColors(2),iSample)
                break;
            else
                colorLabels(iSample) = iColor;
                %                 disp(colorLabels')
            end
        end
    end % for iColor = 1:3
    
    % Various validity heuristics
    
    % The lowest three points must be contiguous (TODO: is there a counterexample for this?)
    maxBestColors = max(bestColors);
    bestColorsSorted = sortrows([maxBestColors;1:length(maxBestColors)]', 1);
    minInds = double(bestColorsSorted(1:numOffFrames,2));
    
    % Set the min values to color 4 (not RGB)
    colorLabels(bestColorsSorted(1:numOffFrames,2)) = 4;
    
    minInds = sort(minInds);
    
    % TODO: support other numSamples and numOffFrames
    assert(numSamples == 15);
    assert(numOffFrames == 3);
    if minInds(1) == 13
        minInds = minInds - 15;
    elseif minInds(2) == 14
        minInds(2:3) = minInds(2:3) - 15;
    elseif minInds(3) == 15
        minInds(3) = minInds(3) - 15;
    end
    
    minInds = sort(minInds);
    
    if minInds(2) ~= (minInds(1)+1) || minInds(2) ~= (minInds(3)-1)
        codeIsValid = false;
    end
    
    % If any labels are zero, it means that the max labels are not continuous
    if(min(colorLabels) == 0)
        codeIsValid = false;
    end
    
    % If we didn't see red, green, and blue, it's invalid
    if isempty(find(colorLabels==1,1)) || isempty(find(colorLabels==2,1)) || isempty(find(colorLabels==3,1))
        codeIsValid = false;
    end
    
    if ~codeIsValid
        whichColors = [1,1,1];
        numPositive = [0,0,0];
        return;
    end
    
    % shift the pattern
    fourInds = find(colorLabels == 4);
    
    for iInd = 1:length(fourInds)
        curInd = fourInds(iInd);
        
        if curInd == numSamples
            curInd = 0;
        end
        
        if colorLabels(curInd+1) ~= 4
            startIndex = curInd + 1;
            break;
        end
    end
    
    for iInd = 1:length(fourInds)
        curInd = fourInds(iInd);
        
        if curInd == 1
            curInd = numSamples+1;
        end
        
        if colorLabels(curInd-1) ~= 4
            endIndex = curInd - 1;
            break;
        end
    end
    
    if endIndex < startIndex
        shiftedColorLabels = colorLabels([startIndex:end, 1:endIndex]);
    else
        shiftedColorLabels = colorLabels(startIndex:endIndex);
    end
    
    shiftedColorLabelsTmp = shiftedColorLabels;
    whichColors = zeros(3,1);
    numPositive = zeros(3,1);
    for iColor = 1:3
        inds = find(shiftedColorLabelsTmp > 0);
        inds = find(shiftedColorLabelsTmp == shiftedColorLabelsTmp(inds(1)));
        whichColors(iColor) = shiftedColorLabelsTmp(inds(1));
        numPositive(iColor) = length(inds);
        shiftedColorLabelsTmp(inds) = 0;
    end
    
    %     keyboard
    
    %     highestColors = zeros(size(bestColors,2), 1);
    %     highestColors(bestColors(1,:) > bestColors(2,:) & bestColors(1,:) > bestColors(3,:)) = 1;
    %     highestColors(bestColors(2,:) > bestColors(1,:) & bestColors(2,:) > bestColors(3,:)) = 2;
    %     highestColors(bestColors(3,:) > bestColors(1,:) & bestColors(3,:) > bestColors(2,:)) = 3;
    
    %     clickGui = true;
    clickGui = false;
    if clickGui
        parseLedCode_plot_guiChangingCenter(colorImages, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps);
    end % if clickGui
end % function colors = dutyCycle_changingCenter()


function [whichColors, numPositive] = dutyCycle_changingCenter_redBlue(colorImages, blurSigmas, scaleDetectionMethod)
    
    if strcmpi(scaleDetectionMethod, 'originalGaussian') || strcmpi(scaleDetectionMethod, 'temporalGaussian')
        % Gaussian kernels
        blurKernels_small = cell(length(blurSigmas), 1);
        blurKernels_large = cell(length(blurSigmas), 1);
        blurKernels_outsideRing = cell(length(blurSigmas), 1);
        for iBlur = 1:length(blurSigmas)
            filterWidth = blurSigmas(iBlur) * 4;
            blurKernels_small{iBlur} = fspecial('gaussian', 2*[filterWidth,filterWidth] + 1, blurSigmas(iBlur));
            blurKernels_large{iBlur} = fspecial('gaussian', 2*[filterWidth,filterWidth] + 1, 2*blurSigmas(iBlur));
            
            blurKernels_outsideRing{iBlur} = blurKernels_large{iBlur} - blurKernels_small{iBlur};
            blurKernels_outsideRing{iBlur}(blurKernels_outsideRing{iBlur} < 0) = 0;
            blurKernels_outsideRing{iBlur} = blurKernels_outsideRing{iBlur} / sum(sum(blurKernels_outsideRing{iBlur}));
        end % for iBlur = 1:length(blurSigmas)
    else % if strcmpi(scaleDetectionMethod, 'originalGaussian') || strcmpi(scaleDetectionMethod, 'temporalGaussian')
        % Box kernels
        blurKernels_small = cell(length(blurSigmas), 1);
        blurKernels_large = cell(length(blurSigmas), 1);
        blurKernels_outsideRing = cell(length(blurSigmas), 1);
        for iBlur = 1:length(blurSigmas)
            smallFilterWidth = 2*blurSigmas(iBlur) + 1;
            largeFilterWidth = 4*blurSigmas(iBlur) + 1;
            
            blurKernels_small{iBlur} = ones(smallFilterWidth, smallFilterWidth);
            blurKernels_small{iBlur} = blurKernels_small{iBlur} / sum(blurKernels_small{iBlur}(:));
            
            blurKernels_large{iBlur} = ones(largeFilterWidth, largeFilterWidth);
            blurKernels_large{iBlur} = blurKernels_large{iBlur} / sum(blurKernels_large{iBlur}(:));
            
            blurKernels_outsideRing{iBlur} = ones(largeFilterWidth, largeFilterWidth);
            blurKernels_outsideRing{iBlur}((ceil(end/2)-blurSigmas(iBlur)):(ceil(end/2)+blurSigmas(iBlur)), (ceil(end/2)-blurSigmas(iBlur)):(ceil(end/2)+blurSigmas(iBlur))) = 0;
            blurKernels_outsideRing{iBlur} = blurKernels_outsideRing{iBlur} / sum(blurKernels_outsideRing{iBlur}(:));
        end % for iBlur = 1:length(blurSigmas)
    end % if strcmpi(scaleDetectionMethod, 'originalGaussian') || strcmpi(scaleDetectionMethod, 'temporalGaussian') ... else
    
    blurredSize = [size(colorImages,1), size(colorImages,2), 3, size(colorImages,4), length(blurSigmas)];
    blurredImages_small = zeros(blurredSize, 'uint8');
    blurredImages_large = zeros(blurredSize, 'uint8');
    blurredImages_outsideRing = zeros(blurredSize, 'uint8');
    
    for iBlur = 1:length(blurSigmas)
        blurredImages_small(:,:,1:2,:,iBlur)       = imfilter(colorImages(:,:,[1,3],:), blurKernels_small{iBlur});
        blurredImages_large(:,:,1:2,:,iBlur)       = imfilter(colorImages(:,:,[1,3],:), blurKernels_large{iBlur});
        blurredImages_outsideRing(:,:,1:2,:,iBlur) = imfilter(colorImages(:,:,[1,3],:), blurKernels_outsideRing{iBlur});

        blurredImages_small(:,:,3,:,iBlur)       = max(blurredImages_small(:,:,1:2,:,iBlur), [], 3);
        blurredImages_large(:,:,3,:,iBlur)       = max(blurredImages_large(:,:,1:2,:,iBlur), [], 3);
        blurredImages_outsideRing(:,:,3,:,iBlur) = max(blurredImages_outsideRing(:,:,1:2,:,iBlur), [], 3);
    end % for iBlur = 1:length(blurSigmas)
    
    if strcmpi(scaleDetectionMethod, 'originalGaussian')
        blurredImages_inside = zeros([size(colorImages,1), size(colorImages,2), 4, size(colorImages,4), length(blurSigmas)]);
        blurredImages_inside(:,:,1:3,:,:) = blurredImages_small - blurredImages_outsideRing;
        blurredImages_inside(:,:,4,:,:) = max(blurredImages_inside(:,:,1:2,:,:), [], 3);
        blurredImages_inside(blurredImages_inside<0) = 0;
        
        blurredImages_onMax = permute(squeeze(blurredImages_inside(:,:,4,:,:)), [3,1,2,4]);
        blurredImages_onMax = blurredImages_onMax - repmat(min(blurredImages_onMax), [size(blurredImages_onMax,1),1,1,1]);
        blurredImages_onMax_mean = squeeze(mean(blurredImages_onMax));
        peakValue = max(blurredImages_onMax_mean(:));
        
        inds = find(blurredImages_onMax_mean == peakValue, 1);
        [bestY,bestX,bestB] = ind2sub(size(blurredImages_onMax_mean), inds);
    elseif strcmpi(scaleDetectionMethod, 'temporalGaussian') || strcmpi(scaleDetectionMethod, 'temporalBox')
        outsideChange = squeeze(max(max(blurredImages_outsideRing(:,:,[1,3],:,:), [], 4) - min(blurredImages_outsideRing(:,:,[1,3],:,:), [], 4), [], 3));
        
        blurredImages_smallMax = permute(squeeze(max(blurredImages_small(:,:,1:2,:,:), [], 3)), [3,1,2,4]);
        
        toSubtract = min(blurredImages_smallMax) + permute(outsideChange, [4,1,2,3]);
        blurredImages_smallMaxScaled = blurredImages_smallMax - repmat(toSubtract, [size(colorImages,4), 1,1,1]);
        
        blurredImages_smallMaxScaled_mean = squeeze(mean(blurredImages_smallMaxScaled));
        
        peakValue = max(blurredImages_smallMaxScaled_mean(:));
        
        inds = find(blurredImages_smallMaxScaled_mean == peakValue, 1);
        [bestY,bestX,bestB] = ind2sub(size(blurredImages_smallMaxScaled_mean), inds);
    end
    
    bestColors = squeeze(blurredImages_small(bestY,bestX, 1:2, :, bestB));
    
    
    % Find the best R, G, and B (seeds), then grow them out
    numSamples = size(bestColors,2);
    
    numOffFrames = 3;
    
    colorLabels = zeros(numSamples, 1);
    
    codeIsValid = true;
    for iColor = 1:2
        [~, ind] = max(bestColors(iColor,:));
        ind = ind(ceil(length(ind)/2));
        
        otherColor = 2 - iColor + 1;
        
        if bestColors(iColor,ind) < bestColors(otherColor,ind)
            %             disp('This is an invalid code');
            codeIsValid = false;
            break;
        end
        
        colorLabels(ind) = iColor;
        
        % Check right
        %         disp('right');
        for iSample = [(ind+1):numSamples, 1:(ind-1)]
            if bestColors(iColor,iSample) < bestColors(otherColor,iSample)
                break;
            else
                colorLabels(iSample) = iColor;
                %                 disp(colorLabels')
            end
        end
        
        % Check left
        %         disp('left');
        for iSample = [(ind-1):-1:1, numSamples:-1:(ind+1)]
            if bestColors(iColor,iSample) < bestColors(otherColor,iSample)
                break;
            else
                colorLabels(iSample) = iColor;
                %                 disp(colorLabels')
            end
        end
    end % for iColor = 1:2
    
    % Various validity heuristics
    
    % The lowest three points must be contiguous (TODO: is there a counterexample for this?)
    maxBestColors = max(bestColors);
    bestColorsSorted = sortrows([maxBestColors;1:length(maxBestColors)]', 1);
    minInds = double(bestColorsSorted(1:numOffFrames,2));
    
    % Set the min values to color 4 (not RGB)
    colorLabels(bestColorsSorted(1:numOffFrames,2)) = 4;
    
    minInds = sort(minInds);
    
    % TODO: support other numSamples and numOffFrames
    assert(numSamples == 15);
    assert(numOffFrames == 3);
    if minInds(1) == 13
        minInds = minInds - 15;
    elseif minInds(2) == 14
        minInds(2:3) = minInds(2:3) - 15;
    elseif minInds(3) == 15
        minInds(3) = minInds(3) - 15;
    end
    
    minInds = sort(minInds);
    
    if minInds(2) ~= (minInds(1)+1) || minInds(2) ~= (minInds(3)-1)
        codeIsValid = false;
    end
    
    % If any labels are zero, it means that the max labels are not continuous
    if(min(colorLabels) == 0)
        codeIsValid = false;
    end
    
    % If we didn't see red, green, and blue, it's invalid
    if isempty(find(colorLabels==1,1)) || isempty(find(colorLabels==2,1))
        codeIsValid = false;
    end
    
    if ~codeIsValid
        whichColors = [1,1];
        numPositive = [0,0];
        return;
    end
    
    % shift the pattern
    fourInds = find(colorLabels == 4);
    
    for iInd = 1:length(fourInds)
        curInd = fourInds(iInd);
        
        if curInd == numSamples
            curInd = 0;
        end
        
        if colorLabels(curInd+1) ~= 4
            startIndex = curInd + 1;
            break;
        end
    end
    
    for iInd = 1:length(fourInds)
        curInd = fourInds(iInd);
        
        if curInd == 1
            curInd = numSamples+1;
        end
        
        if colorLabels(curInd-1) ~= 4
            endIndex = curInd - 1;
            break;
        end
    end
    
    if endIndex < startIndex
        shiftedColorLabels = colorLabels([startIndex:end, 1:endIndex]);
    else
        shiftedColorLabels = colorLabels(startIndex:endIndex);
    end
    
    shiftedColorLabelsTmp = shiftedColorLabels;
    whichColors = zeros(2,1);
    numPositive = zeros(2,1);
    for iColor = 1:2
        inds = find(shiftedColorLabelsTmp > 0);
        inds = find(shiftedColorLabelsTmp == shiftedColorLabelsTmp(inds(1)));
        whichColors(iColor) = shiftedColorLabelsTmp(inds(1));
        numPositive(iColor) = length(inds);
        shiftedColorLabelsTmp(inds) = 0;
    end
    
    %     keyboard
    
    %     highestColors = zeros(size(bestColors,2), 1);
    %     highestColors(bestColors(1,:) > bestColors(2,:) & bestColors(1,:) > bestColors(3,:)) = 1;
    %     highestColors(bestColors(2,:) > bestColors(1,:) & bestColors(2,:) > bestColors(3,:)) = 2;
    %     highestColors(bestColors(3,:) > bestColors(1,:) & bestColors(3,:) > bestColors(2,:)) = 3;
    
    %     clickGui = true;
    clickGui = false;
    if clickGui
        parseLedCode_plot_guiChangingCenter(colorImages, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps);
    end % if clickGui
end % function colors = dutyCycle_changingCenter_redBlue()

