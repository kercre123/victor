
function parseLedCode_lightOnly()
    
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    global g_curImageIndex;
    
    numImages = 30;
    g_cameraType = 'offline'; % 'webots', 'usbcam', 'offline'
    
    g_filenamePattern = 'c:/tmp/red_%05d.png';
    g_whichImages = 1:100;
    
    g_processingSize = [240,320];
    g_curImageIndex = 1;
    
%     templateQuad = [158,68; 158,97; 191,67; 192,98];
%     templateRectangle = [min(templateQuad(:,1)), max(templateQuad(:,1)), min(templateQuad(:,2)), max(templateQuad(:,2))]; % [left, right, top, bottom]
    
    image = getNextImage();
    
    % 1. Capture N images
    images = zeros([size(image), numImages], 'uint8');
    images(:,:,:,1) = image;
    
    for i = 2:numImages
        imshows(uint8(images(:,:,:,i-1)), 3);
        images(:,:,:,i) = getNextImage();
    end
    
    blurKernel = ones(11, 11);
    blurKernel = blurKernel / sum(blurKernel(:));
    
    blurredImages = imfilter(images, blurKernel);
    
%     thresholdImage = mean(images, 4);

%     minImage = min(images, [], 4);
%     maxImage = max(images, [], 4);
%     thresholdImage = (maxImage + minImage) / 2;
    
    minImageBlurred = double(min(blurredImages, [], 4));
    maxImageBlurred = double(max(blurredImages, [], 4));
    thresholdImageBlurred = (maxImageBlurred + minImageBlurred) / 2;

%     diffImages = zeros(size(images));
    diffImagesBlurred = zeros(size(images));
%     diffImagesBlurred2 = zeros(size(images));
    
%     binaryImages = zeros(size(images));
%     thresholdFraction = 0.99;
%     downsampleFactor = 2;
%     maxSmoothingFraction = 0.025;

    % Find the per-pixel difference from the threshold
    for iImage = 1:size(images,4)
        curBlurred = double(blurredImages(:,:,:,iImage))/2 - thresholdImageBlurred/2;
        diffImagesBlurred(:,:,:,iImage) = curBlurred + 128;
        
%         curBlurredSome = curBlurred;
%         curBlurredSome(abs(curBlurredSome) < 5) = 0;
        
%         diffImagesBlurred2(:,:,:,iImage) = curBlurredSome + 128;
        
%         for iColor = 1:3
%             binaryImages(:,:,iColor,iImage) = simpleDetector_step1_computeCharacteristicScale(maxSmoothingFraction, size(blurredImages,1), size(blurredImages,2), downsampleFactor, diffImagesBlurred(:,:,iColor,iImage), thresholdFraction, true, EmbeddedConversionsManager(), false, false);
%         end
        
%         imshows(uint8(diffImage));
%         pause();
    end
    
    % Find which pixels are mostly white (possible shadow on a white-light reflection)
%     grayDiffImagesBlurred = repmat(mean(diffImagesBlurred, 3), [1,1,3,1]);
    
%     differenceFromGray = squeeze(mean(abs(diffImagesBlurred - grayDiffImagesBlurred), 3));
    
    colors = squeeze(mean(mean(diffImagesBlurred(100:140,140:180,:,:))));
%     plot(colors(3:-1:1,:)')
    
    keyboard
    
end % function parseLedCode_lightOnly()

function image = getNextImage()
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    global g_curImageIndex;
    
    if strcmpi(g_cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(g_cameraType, 'usbcam')
        persistent usbcamStarted; %#ok<TLEV>
        
        cameraId = 0;
        
        if isempty(usbcamStarted)
            usbcamStarted = false;
        end
        
        if ~usbcamStarted
            mexCameraCapture(0, cameraId);
            usbcamStarted = true;
        end
        
        image = imresize((mexCameraCapture(1, cameraId)), g_processingSize);
    elseif strcmpi(g_cameraType, 'offline')
        image = imresize((imread(sprintf(g_filenamePattern, g_whichImages(g_curImageIndex)))), g_processingSize);
        g_curImageIndex = g_curImageIndex + 1;
    else
        assert(false);
    end
    
end % function getNextImage()
