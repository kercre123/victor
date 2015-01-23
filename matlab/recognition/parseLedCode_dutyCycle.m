
function parseLedCode_dutyCycle()
    
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    global g_curImageIndex;
    
    numImages = 15;
    g_cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    g_filenamePattern = '~/Documents/Anki/product-cozmo-large-files/blinkyImages10/red_%05d.png';
    g_whichImages = 1:100;
    
    g_processingSize = [240,320];
    g_curImageIndex = 1;
    
    expectedFps = 30;
    
    saturationThreshold = 254; % Ignore pixels where any color is higher than this
    
%     templateQuad = [158,68; 158,97; 191,67; 192,98];
%     templateRectangle = [min(templateQuad(:,1)), max(templateQuad(:,1)), min(templateQuad(:,2)), max(templateQuad(:,2))]; % [left, right, top, bottom]
    templateRectangle = [140,180, 100,140];

%     image = getNextImage();
    
    image = getNextImage();

    % 1. Capture N images
    images = zeros([g_processingSize, 3, numImages], 'uint8');
    images(:,:,:,1) = image;
    
    tic
    for i = 1:numImages
%         imshows(uint8(images(:,:,:,i-1)), 3);
        images(:,:,:,i) = getNextImage();
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
    
%     return
    
    colorPatches = images(templateRectangle(3):templateRectangle(4),templateRectangle(1):templateRectangle(2),:,:);
    
    blurKernel = ones(11, 11);
    blurKernel = blurKernel / sum(blurKernel(:));
    
    blurredColorPatches = imfilter(colorPatches, blurKernel);
    
    minImageBlurred = double(min(blurredColorPatches, [], 4));
    maxImageBlurred = double(max(blurredColorPatches, [], 4));
    %thresholdImageBlurred = (maxImageBlurred + minImageBlurred) / 2;
    thresholdImageBlurred = minImageBlurred;

    grayMaxImageBlurred = max(maxImageBlurred, [], 3);
    saturatedInds = (grayMaxImageBlurred > saturationThreshold);
    unsaturatedInds = (grayMaxImageBlurred <= saturationThreshold);
    
    diffImagesBlurred = zeros(size(blurredColorPatches), 'uint8');

    % Find the per-pixel difference from the threshold
    colors = zeros(3, size(images,4));
    numMax = zeros(3,1);
    for iImage = 1:size(images,4)
        curBlurred = double(blurredColorPatches(:,:,:,iImage))/2 - thresholdImageBlurred/2;
        
        for iChannel = 1:3
            curChannelImage = curBlurred(:,:,iChannel);
            colors(iChannel, iImage) = mean(curChannelImage(unsaturatedInds));
            
            curChannelImage(saturatedInds) = 0;
            diffImagesBlurred(:,:,iChannel,iImage) = curChannelImage + 128; 
        end
    end % for iImage = 1:size(images,4)
    
    % Finding the single max may not be the most robust?
    maxRangeRgb = max(colors, [] ,2) - min(colors, [] ,2); 
    rgbThresholds = (max(colors, [] ,2) + min(colors, [] ,2)) / 2;
    
    [maxVal, maxInd] = max(maxRangeRgb);
    
    colorNames = {'red', 'green', 'blue'};
    
    percentPositive = length(find(colors(maxInd,:) > rgbThresholds(maxInd))) / size(colors,2);
    
    disp(sprintf('%s is at %0.2f', colorNames{maxInd}, numImages*percentPositive));
    
    plot(colors(3:-1:1,:)');
    pause(0.05);
    
%     colorsWithSaturated = squeeze(mean(mean(diffImagesBlurred)));
    
%     figure(); subplot(1,2,1); plot(colors(3:-1:1,:)'); subplot(1,2,2); plot(colorsWithSaturated(3:-1:1,:)');
    
%     keyboard
    
end % function parseLedCode_lightOnly()

function image = getNextImage()
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    global g_curImageIndex;
    persistent cap;
    
    if strcmpi(g_cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(g_cameraType, 'usbcam')
        persistent usbcamStarted; %#ok<TLEV>
        
        cameraId = 0;
        
        if isempty(usbcamStarted)
            usbcamStarted = false;
        end
        
        if ~usbcamStarted
            cap = cv.VideoCapture(cameraId);
            usbcamStarted = true;
        end
        
        image = imresize((cap.read()), g_processingSize);
    elseif strcmpi(g_cameraType, 'offline')
        image = imresize((imread(sprintf(g_filenamePattern, g_whichImages(g_curImageIndex)))), g_processingSize);
        g_curImageIndex = g_curImageIndex + 1;
    else
        assert(false);
    end
    
end % function getNextImage()
