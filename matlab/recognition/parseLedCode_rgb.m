
% function parseLedCode_rgb(varargin)

function [colorIndex, numPositive] = parseLedCode_rgb(varargin)
    numFramesToTest = 15;
    cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    filenamePattern = '~/Documents/Anki/products-cozmo-large-files/rgb1/images_%05d.png';
    whichImages = 1:100;
    
    processingSize = [120,160];
    
    lightSquareCenter = [120, 160] * (processingSize(1) / 240);
    lightSquareWidths = [40] * (processingSize(1) / 240);
    alignmentRectangle = [110, 210, 70, 170] * (processingSize(1) / 240);
    
    spatialBlurSigmas = linspace(2,6,5);
    
    useBoxFilter = false;
    alignmentType = 'exhaustiveTranslation'; % {'none', 'exhaustiveTranslation'}
    
    parsingType = 'spatialBlur'; % {'spatialBlur'}
    
    showFigures = true;
    
    knownLedColor = [];
    
    if useBoxFilter
        smallBlurKernel = ones(11, 11);
        smallBlurKernel = smallBlurKernel / sum(smallBlurKernel(:));
    else
        smallBlurKernel = fspecial('gaussian',[21,21],6);
    end
    
    parseVarargin(varargin);
    
    images = parseLedCode_captureAllImages(cameraType, filenamePattern, whichImages, processingSize, numFramesToTest, showFigures);
    
    if isempty(images)
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
    
    blurredImages = imfilter(images, smallBlurKernel);
    
    colors = cell(length(lightSquareWidths), 1);
    
    for iWidth = 1:length(lightSquareWidths)
        curWidth = lightSquareWidths(iWidth) / 2;
        yLimits = round([lightSquareCenter(1)-curWidth, lightSquareCenter(1)+curWidth]);
        xLimits = round([lightSquareCenter(2)-curWidth, lightSquareCenter(2)+curWidth]);
        
        colorPatches = blurredImages(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :, :);
        
        if strcmpi(parsingType, 'spatialBlur')
            if isempty(knownLedColor)
                assert(false);
            else
                [numPositive, peakValue] = dutyCycle_changingCenter(colorPatches, spatialBlurSigmas, knownLedColor);
                colorIndex = knownLedColor;
            end
            
            return;
        else % if strcmpi(parsingType, 'spatialBlur')
            assert(false);
        end % if strcmpi(parsingType, 'spatialBlur') ... else
    end % for iWidth = 1:length(lightSquareWidths)
    
    if strcmpi(parsingType, 'spatialBlur')
        [percentPositive, colorIndex, ~] = computeDutyCycle(colors{1}, knownLedColor);
    else
        assert(false);
    end % if strcmpi(parsingType, 'blur') ... else
    
    numPositive = round(numFramesToTest*percentPositive);
    
    %     keyboard
    
end % function parseLedCode_rgb()

function [numPositive, peakValue] = dutyCycle_changingCenter(colorImages, blurSigmas, whichColors)
    
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
    
    % Find the best R, G, and B (seeds), then grow them out
    numSamples = size(bestColors,2);
    
    colorLabels = zeros(numSamples, 1);
    
    codeIsValid = true;
    for iColor = 1:3
        [~, ind] = max(bestColors(iColor,:));
        ind = ind(ceil(length(ind)/2));
        
        otherColors = [1,2,3];
        otherColors = otherColors(3 ~= otherColors);
        if bestColors(iColor,ind) < bestColors(otherColors(1),ind) || bestColors(iColor,ind) < bestColors(otherColors(2),ind)
            disp('This is an invalid code');
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
    
    % TODO: check if colorLabels is contiguous, except for the lowest 2 (or 3) values
    %       If something is missing, then the code is INVALID
    
    highestColors = zeros(size(bestColors,2), 1);
    highestColors(bestColors(1,:) > bestColors(2,:) & bestColors(1,:) > bestColors(3,:)) = 1;
    highestColors(bestColors(2,:) > bestColors(1,:) & bestColors(2,:) > bestColors(3,:)) = 2;
    highestColors(bestColors(3,:) > bestColors(1,:) & bestColors(3,:) > bestColors(2,:)) = 3;
    
    keyboard
    
    stepUpFilter = [-1, 0, 1]';
    
    blurredImages_inside_reshaped = double(permute(blurredImages_inside, [4,1,2,3,5]));
    steps = imfilter(blurredImages_inside_reshaped, stepUpFilter, 'circular');
    
    % Per x,y,blur, what is the minimum of: the positive peaks and the negative peaks
    maxSteps = squeeze(min(max(steps), max(-steps)));
    peakValue = max(maxSteps(:));
    inds = find(maxSteps == peakValue, 1);
    [bestY,bestX,bestB] = ind2sub(size(maxSteps), inds);
    
    [~, startPoint] = max(steps(:,bestY,bestX,bestB));
    [~, endPoint] = min(steps(:,bestY,bestX,bestB));
    numPositive = mod(endPoint - startPoint, size(colorImages,4)) + 0.5;
    
    clickGui = true;
    %     clickGui = false;
    if clickGui
        parseLedCode_plot_guiChangingCenter(colorImages, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps);
    end % if clickGui
    %     playVideo({blurredImages(:,:,:,:,a,1), blurredImages(:,:,:,:,a,2), blurredImages(:,:,:,:,a,3)})
end % function colors = dutyCycle_changingCenter()


