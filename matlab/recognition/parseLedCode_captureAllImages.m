function images = parseLedCode_captureAllImages(cameraType, filenamePattern, whichImages, processingSize, numImages, showFigures, displayText)

    expectedFps = 30;
    
    curImageIndex = 1;
    
    if ~exist('displayText', 'var') || isempty(displayText);
        displayText = false;
    end
    
    [curImage, curImageIndex] = parseLedCode_getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex);
    
    centerPoint = ceil(processingSize / 2);
    
    % 1. Capture N images
    images = zeros([processingSize, 3, numImages], 'uint8');
    images(:,:,:,1) = curImage;
    
    tic
    for i = 2:numImages
        [curImage, curImageIndex] = parseLedCode_getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex);
        images(:,:,:,i) = curImage;
        
        if showFigures
            curImage((centerPoint(1)-1):(centerPoint(1)+1), :, :) = 0;
            curImage(:, (centerPoint(2)-1):(centerPoint(2)+1), :) = 0;
            curImage(centerPoint(1), :, :) = 255;
            curImage(:, centerPoint(2), :) = 255;

            figure(2); imshow(imresize(curImage, [240,320], 'nearest'));
            % figure(2); imshows(imresize(curImage, [240,320], 'nearest'), 2);
        end
    end
    timeElapsed = toc();
    
    %     expectedElapsedTime = numImages * (1/expectedFps);
    expectedElapsedTimeRange = [(numImages-0.75) * (1/expectedFps), (numImages+0.75) * (1/expectedFps)];
    %     expectedFpsRange = expectedFps * (expectedFps/numImages) * [expectedElapsedTime - (0.5/expectedFps), expectedElapsedTime + (0.5/expectedFps)];
    
    if strcmpi(cameraType, 'offline')
        if displayText
            disp(sprintf('Captured %d images from offline.', numImages));
        end
    else
        if displayText
            disp(sprintf('Captured %d images at %0.2f FPS.', numImages, 1/(timeElapsed/numImages)))
        end

        if expectedElapsedTimeRange(1) <= timeElapsed && expectedElapsedTimeRange(2) >= timeElapsed
            if displayText
                disp(sprintf('Good: %0.3f <= %0.3f <= %0.3f', expectedElapsedTimeRange(1), timeElapsed, expectedElapsedTimeRange(2)));
            end
        else
            if displayText
                disp(sprintf('Bad: %0.3f X %0.3f X %0.3f', expectedElapsedTimeRange(1), timeElapsed, expectedElapsedTimeRange(2)));
                disp(' ');
            end
            images = [];
            return;
        end    
    end
end % function parseLedCode_captureAllImages()
