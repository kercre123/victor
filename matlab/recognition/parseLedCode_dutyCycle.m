
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
    
    lightRectangle = round([140,180, 100,140] * (processingSize(1) / 240));
    carRectangle = round([100,220, 60,180] * (processingSize(1) / 240));
    
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
    
    blurKernel = ones(11, 11);
    blurKernel = blurKernel / sum(blurKernel(:));
    
    blurredImages = imfilter(images, blurKernel);
    
%     % Translationally align the images
%     templateImage = rgb2gray(images(:,:,:,ceil(size(blurredImages,4)/2)));
%     numPyramidLevels = 2;
%     transformType = bitshift(2,8); %translation
%     ridgeWeight = 1e-3;
%     maxIterations = 50;
%     convergenceTolerance = 0.05;
%     homography = eye(3,3);
% 
%     translationHomographies = cell(size(images,4), 1);
%     warpedImages = zeros(size(images), 'uint8');
%     for iImage = 1:size(images,4)
%         curImage = rgb2gray(images(:,:,:,iImage));
%         translationHomographies{iImage} = mexTrackLucasKanade(templateImage, double(carRectangle), double(numPyramidLevels), double(transformType), double(ridgeWeight), curImage, double(maxIterations), double(convergenceTolerance), double(homography));
%     
%         [warpedImages(:,:,:,iImage), ~, ~] = warpProjective(images(:,:,:,iImage), inv(translationHomographies{iImage}), size(curImage));
%     end

    colorPatches = blurredImages(lightRectangle(3):lightRectangle(4),lightRectangle(1):lightRectangle(2),:,:);
    
    [colors, colorsHsv] = extractColors(colorPatches, saturationThreshold);
    
    % Finding the single max may not be the most robust?
    maxRangeRgb = max(colors, [] ,2) - min(colors, [] ,2); 
    rgbThresholds = (max(colors, [] ,2) + min(colors, [] ,2)) / 2;
    
    [maxVal, maxInd] = max(maxRangeRgb);
    
    colorNames = {'red', 'green', 'blue'};
    
    percentPositive = length(find(colors(maxInd,:) > rgbThresholds(maxInd))) / size(colors,2);
    
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
    
    figure(1); plot(colors(3:-1:1,:)');
    figure(3); plot(colorsHsv(3:-1:1,:)');
    pause(0.05);
    
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
