% function parseLedCode()

function parseLedCode()
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;

    numImages = 20;
    g_cameraType = 'offline'; % 'webots', 'usbcam', 'offline'
    
    g_filenamePattern = '/Users/pbarnum/tmp/image_%d.png';
    g_whichImages = 0:99;
    
    g_processingSize = [240,320];    

    image = getNextImage();

    % 1. Capture N images
    images = zeros([size(image),numImages]);
    images(:,:,1) = image;
    
    for i = 2:numImages
        images(:,:,i) = getNextImage();
    end
    
    markers = cell(numImages, 1);
    for i = 1:numImages
        markers{i} = simpleDetector(images(:,:,i)/255);
    end

    % 2. Compute which images are per-pixel above the mean, and which are below
    amountAboveMeanThreshold = 10;
    percentAboveMeanThreshold = 0.002;

    meanImage = mean(images,3);

    areWhite = zeros(numImages, 1);
    for iImage = 1:numImages
        imshow(uint8(images(:,:,iImage)));
        numAboveMean = length(find(images(:,:,iImage) > (meanImage+amountAboveMeanThreshold)));
        if (numAboveMean / numel(images(:,:,iImage))) > percentAboveMeanThreshold
            areWhite(iImage) = 1;
        end
    end
    
%     areWhite = [1;0;0;0;0;0;0;0;0;1;1;0;0;0;0;0;0;0;1;1;0;0;0;0;0;0;0;0;1;0;0;0;0;0;0;0;0;1;1;0;0;0;0;0;0;0;1;1;0;0];
    
    % 3. Compute transitions from bright to dark
    firstDifferentIndex = find(areWhite ~= areWhite(1), 1, 'first');
    lastDifferentIndex = find(areWhite ~= areWhite(end), 1, 'last');
    
    if isempty(firstDifferentIndex) || isempty(lastDifferentIndex)
        disp('Insufficient number of segments');
        return;
    end
    
    segments = zeros(0,2);
    
    curSegmentColor = areWhite(firstDifferentIndex);
    firstSegmentIndex = firstDifferentIndex;
    for iIndex = (firstDifferentIndex+1):lastDifferentIndex
        if areWhite(iIndex) == curSegmentColor
            continue;
        else
            segments(end+1, :) = [curSegmentColor, iIndex-firstSegmentIndex]; %#ok<AGROW>
            curSegmentColor = areWhite(iIndex);
            firstSegmentIndex = iIndex;
        end
    end % for iIndex = firstDifferentIndex:lastDifferentIndex
    segments(end+1, :) = [curSegmentColor, iIndex-firstSegmentIndex+1]; %#ok<NASGU>
    
    % 4. Chose first N transitions, and parse
    numTransitionsToUse = 2;
    
    if size(segments,1) < numTransitionsToUse
        disp('Insufficient number of segments');
        return;
    end
    
    totalValue = 0;
    totalNum = 0;
    for iTransition = 1:numTransitionsToUse
        totalValue = totalValue + segments(iTransition,1) * segments(iTransition,2);
        totalNum = totalNum + segments(iTransition,2);
    end % for iTransition = 1:numTransitionsToUse
    
    estimatedPercent = totalValue / totalNum;
    
    disp(sprintf('Estimated percent = %0.2f', estimatedPercent));
end % function parseLedCode()
    
%     keyboard
function image = getNextImage()
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    
    if strcmpi(g_cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(g_cameraType, 'usbcam')
        persistent usbcamStarted;
        
        if isempty(usbcamStarted)
            usbcamStarted = false;
        end
        
        if ~usbcamStarted
            cameraId = 0;
            mexCameraCapture(0, cameraId);
            usbcamStarted = true;
        end
        
        image = imresize(rgb2gray2(mexCameraCapture(1, cameraId)), g_processingSize);
    elseif strcmpi(g_cameraType, 'offline')
        persistent curImageIndex;
        
        if isempty(curImageIndex)
            curImageIndex = 1;
        end
        
        image = imresize(rgb2gray2(imread(sprintf(g_filenamePattern, g_whichImages(curImageIndex)))), g_processingSize);
        curImageIndex = curImageIndex + 1;
    else
        assert(false);
    end

end
