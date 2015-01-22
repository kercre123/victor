
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
    
    saturationThreshold = 250; % Ignore pixels where any color is higher than this
    
%     templateQuad = [158,68; 158,97; 191,67; 192,98];
%     templateRectangle = [min(templateQuad(:,1)), max(templateQuad(:,1)), min(templateQuad(:,2)), max(templateQuad(:,2))]; % [left, right, top, bottom]
    
    image = getNextImage();
    
    % 1. Capture N images
    images = zeros([size(image), numImages], 'uint8');
    images(:,:,:,1) = image;
    
    for i = 2:numImages
%         imshows(uint8(images(:,:,:,i-1)), 3);
        images(:,:,:,i) = getNextImage();
    end
    
    blurKernel = ones(11, 11);
    blurKernel = blurKernel / sum(blurKernel(:));
    
    blurredImages = imfilter(images, blurKernel);
    
    minImageBlurred = double(min(blurredImages, [], 4));
    maxImageBlurred = double(max(blurredImages, [], 4));
    %thresholdImageBlurred = (maxImageBlurred + minImageBlurred) / 2;
    thresholdImageBlurred = minImageBlurred;

    grayMaxImageBlurred = max(maxImageBlurred, [], 3);
    saturatedInds = find(grayMaxImageBlurred > saturationThreshold);
    unsaturatedInds = find(grayMaxImageBlurred <= saturationThreshold);
    
    diffImagesBlurred = zeros(size(images), 'uint8');

    % Find the per-pixel difference from the threshold
    colors = zeros(3, size(images,4));
    for iImage = 1:size(images,4)
        curBlurred = double(blurredImages(:,:,:,iImage))/2 - thresholdImageBlurred/2;
        
        for iChannel = 1:3
            curChannelImage = curBlurred(:,:,iChannel);
            colors(iChannel, iImage) = mean(curChannelImage(unsaturatedInds));
            
            curChannelImage(saturatedInds) = 0;
            diffImagesBlurred(:,:,iChannel,iImage) = curChannelImage + 128;
        end
    end % for iImage = 1:size(images,4)
    
    % Find which pixels are mostly white (possible shadow on a white-light reflection)
%     grayDiffImagesBlurred = repmat(mean(diffImagesBlurred, 3), [1,1,3,1]);
%     differenceFromGray = squeeze(mean(abs(diffImagesBlurred - grayDiffImagesBlurred), 3));
    
    colorsWithSaturated = squeeze(mean(mean(diffImagesBlurred(100:140,140:180,:,:))));
    
    figure(); subplot(1,2,1); plot(colors(3:-1:1,:)'); subplot(1,2,2); plot(colorsWithSaturated(3:-1:1,:)');
    
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
