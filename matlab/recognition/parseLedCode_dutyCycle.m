
function parseLedCode_dutyCycle()
    numImages = 15;
    cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    filenamePattern = '~/Documents/Anki/product-cozmo-large-files/blinkyImages10/red_%05d.png';
    whichImages = 1:100;
    
    processingSize = [120,160];
    curImageIndex = 1;
    
    centerPoint = ceil(processingSize / 2);
    
    expectedFps = 30;
    
    saturationThreshold = 254; % Ignore pixels where any color is higher than this
    
    lightSquareCenter = [160, 120] * (processingSize(1) / 240);
    lightSquareWidths = [10, 20, 30, 40, 50, 60, 70, 80] * (processingSize(1) / 240);
%     lightRectangle = round([140,180, 100,140] * (processingSize(1) / 240));
    alignmentRectangle = [110, 210, 70, 170] * (processingSize(1) / 240);


    [image, curImageIndex] = getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex);

    % 1. Capture N images
    images = zeros([processingSize, 3, numImages], 'uint8');
    images(:,:,:,1) = image;
    
    tic
    for i = 1:numImages
        [curImage, curImageIndex] = getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex);
        images(:,:,:,i) = curImage;
        
        curImage((centerPoint(1)-1):(centerPoint(1)+1), :, :) = 0;
        curImage(:, (centerPoint(2)-1):(centerPoint(2)+1), :) = 0;
        curImage(centerPoint(1), :, :) = 255;
        curImage(:, centerPoint(2), :) = 255;
                
%         [gradient, or] = canny(rgb2gray(curImage), 1);
        
%         figure(2); imshows(imresize(gradient/20, [240,320], 'nearest'), 2);
        
        figure(2); imshow(imresize(curImage, [240,320], 'nearest'));
%         figure(2); imshows(imresize(curImage, [240,320], 'nearest'), 2);

%         curImageHsv = rgb2hsv(curImage);
%         figure(3); imshow(curImageHsv(:,:,1));
%         figure(4); imshow(curImageHsv(:,:,2));
%         figure(5); imshow(curImageHsv(:,:,3));
    end
    timeElapsed = toc();
    
%     expectedElapsedTime = numImages * (1/expectedFps);
    expectedElapsedTimeRange = [(numImages-0.75) * (1/expectedFps), (numImages+0.75) * (1/expectedFps)];
%     expectedFpsRange = expectedFps * (expectedFps/numImages) * [expectedElapsedTime - (0.5/expectedFps), expectedElapsedTime + (0.5/expectedFps)];
%     
    disp(sprintf('Captured %d images at %0.2f FPS.', numImages, 1/(timeElapsed/numImages)))
    
    if expectedElapsedTimeRange(1) <= timeElapsed && expectedElapsedTimeRange(2) >= timeElapsed
        disp(sprintf('Good: %0.3f <= %0.3f <= %0.3f', expectedElapsedTimeRange(1), timeElapsed, expectedElapsedTimeRange(2)));
    else
        disp(sprintf('Bad: %0.3f X %0.3f X %0.3f', expectedElapsedTimeRange(1), timeElapsed, expectedElapsedTimeRange(2)));
        disp(' ');
        return;
    end
    
    smallBlurKernel = ones(11, 11);
    smallBlurKernel = smallBlurKernel / sum(smallBlurKernel(:));
    blurredImages = imfilter(images, smallBlurKernel);
    
    figure(3); imshow(blurredImages(:,:,:,1));
    
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
        yLimits = round([lightSquareCenter(2)-curWidth, lightSquareCenter(2)+curWidth]);
        xLimits = round([lightSquareCenter(1)-curWidth, lightSquareCenter(1)+curWidth]);
        
        colorPatches = blurredImages(yLimits(1):yLimits(2), xLimits(1):xLimits(2), :, :);
        
        [colors{iWidth}, colorsHsv{iWidth}] = extractColors(colorPatches, saturationThreshold);
        
        figure(1);
        subplot(3, ceil(length(lightSquareWidths)/3), iWidth);
        plot(colors{iWidth}(3:-1:1,:)');
    end
    pause(0.05);
    
%     return;
    
    % Finding the single max may not be the most robust?
    maxRangeRgb = max(colors{4}, [] ,2) - min(colors{4}, [] ,2); 
    rgbThresholds = (max(colors{4}, [] ,2) + min(colors{4}, [] ,2)) / 2;
    
    [maxVal, maxInd] = max(maxRangeRgb);
    
    colorNames = {'red', 'green', 'blue'};
    
    percentPositive = length(find(colors{4}(maxInd,:) > rgbThresholds(maxInd))) / size(colors{4},2);
    
    % Rejection heuristics
    isValid = true;
    
%     if maxVal < 5
%         isValid = false;
%     end
    
    if isValid
        disp(sprintf('%s is at %0.2f', colorNames{maxInd}, numImages*percentPositive));
    else
        disp('Unknown color and number');
    end
    
%     keyboard
    
end % function parseLedCode_lightOnly()

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
    
function [image, curImageIndex] = getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex)
    if strcmpi(cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(cameraType, 'usbcam')
        persistent cap; %#ok<TLEV>
        
        cameraId = 0;
       
        if isempty(cap)
            cap = cv.VideoCapture(cameraId);
            cap.set('framewidth', processingSize(2));
            cap.set('frameheight', processingSize(1));
            cap.set('whitebalanceblueu', 4000);
        end
        
        image = cap.read();
    elseif strcmpi(cameraType, 'offline')
        image = imresize((imread(sprintf(filenamePattern, whichImages(curImageIndex)))), processingSize);
        curImageIndex = curImageIndex + 1;
    else
        assert(false);
    end
    
end % function getNextImage()
