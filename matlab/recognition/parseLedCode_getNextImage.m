function [image, curImageIndex] = parseLedCode_getNextImage(cameraType, filenamePattern, whichImages, processingSize, curImageIndex)
    if strcmpi(cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(cameraType, 'usbcam')
        persistent cap; %#ok<TLEV>
        
        cameraId = 1;
        
        if isempty(cap)
            cap = cv.VideoCapture(cameraId);
            cap.set('framewidth', processingSize(2));
            cap.set('frameheight', processingSize(1));
            cap.set('whitebalanceblueu', 4000);
        end
        
        image = cap.read();
    elseif strcmpi(cameraType, 'offline')
        inFilename = sprintf(filenamePattern, whichImages(curImageIndex));
        image = imresize(imread(inFilename), processingSize);
        curImageIndex = curImageIndex + 1;
    else
        assert(false);
    end
    
end % function parseLedCode_getNextImage()
