
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
    
    blurredImages_small = zeros([size(colorImages),length(blurSigmas)], 'uint8');
    blurredImages_large = zeros([size(colorImages),length(blurSigmas)], 'uint8');
    blurredImages_outsideRing = zeros([size(colorImages),length(blurSigmas)], 'uint8');
    
    for iBlur = 1:length(blurSigmas)
        blurredImages_small(:,:,:,:,iBlur) = imfilter(colorImages, blurKernels_small{iBlur});
        blurredImages_large(:,:,:,:,iBlur) = imfilter(colorImages, blurKernels_large{iBlur});
        blurredImages_outsideRing(:,:,:,:,iBlur) = imfilter(colorImages, blurKernels_outsideRing{iBlur});
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


