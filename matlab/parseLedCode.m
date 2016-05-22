% function parseLedCode()

function parseLedCode()
    global g_cameraType;
    global g_filenamePattern;
    global g_whichImages;
    global g_processingSize;
    global g_curImageIndex;
    
    numImages = 50;
    g_cameraType = 'usbcam'; % 'webots', 'usbcam', 'offline'
    
    g_filenamePattern = '~/Documents/Anki/products-cozmo-large-files/blinkyImages/image_%d.png';
    g_whichImages = 100:199;
    
    g_processingSize = [240,320];
    g_curImageIndex = 1;
    
    useCFiducialExtraction = true;
    
    templateQuad = [158,68; 158,97; 191,67; 192,98];
    templateRectangle = [min(templateQuad(:,1)), max(templateQuad(:,1)), min(templateQuad(:,2)), max(templateQuad(:,2))]; % [left, right, top, bottom]
    
%     templateFilename = '~/Documents/Anki/products-cozmo-large-files/blinkyImages/template.png';
%     numPyramidLevels = 2;
%     maxIterations = 50;
%     convergenceTolerance = 0.05;
%     templateImage = rgb2gray2(imread(templateFilename));
    
    image = getNextImage();
    
    % 1. Capture N images
    images = zeros([size(image),numImages]);
    images(:,:,1) = image;
    
    for i = 2:numImages
        imshow(uint8(images(:,:,i-1)));
        images(:,:,i) = getNextImage();
    end
    
    markers = cell(numImages, 1);
    lightHomographies = cell(numImages, 1);
    monsterFaces = cell(numImages, 1);
    for iImage = 1:numImages
        if useCFiducialExtraction
            imageSize = size(images(:,:,iImage));
            useIntegralImageFiltering = true;
            scaleImage_thresholdMultiplier = 1.0;
            scaleImage_numPyramidLevels = 3;
            component1d_minComponentWidth = 0;
            component1d_maxSkipDistance = 0;
            minSideLength = round(0.01*max(imageSize(1),imageSize(2)));
            maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
            component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
            component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
            component_sparseMultiplyThreshold = 1000.0;
            component_solidMultiplyThreshold = 2.0;
            component_minHollowRatio = 1.0;
            minLaplacianPeakRatio = 5;
            quads_minQuadArea = 100 / 4;
            quads_quadSymmetryThreshold = 2.0;
            quads_minDistanceFromImageEdge = 2;
            decode_minContrastRatio = 1.25;
            quadRefinementIterations = 5;
            numRefinementSamples = 100;
            quadRefinementMaxCornerChange = 5.0;
            quadRefinementMinCornerChange = .005;
            returnInvalidMarkers = 0;

            [quads, ~, markerNames] = mexDetectFiducialMarkers(uint8(images(:,:,iImage)), useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, minLaplacianPeakRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers);
            
            markers{iImage} = cell(length(markerNames), 1);
            
            for iMarker = 1:length(markerNames)
                markers{iImage}{iMarker} = struct('codeName', markerNames{iMarker}, 'corners', quads{iMarker});
            end
        else
            markers{iImage} = simpleDetector(images(:,:,iImage)/255);
        end
        
        % Select the largest angryFace marker
        bestMarkerIndex = -1;
        bestMarkerSize = 0;
        for iMarker = 1:length(markers{iImage})
            if strcmpi(markers{iImage}{iMarker}.codeName, 'MARKER_ANGRYFACE')
                markerSize = (max(markers{iImage}{iMarker}.corners(:,1)) - min(markers{iImage}{iMarker}.corners(:,1))) * ...
                    (max(markers{iImage}{iMarker}.corners(:,2)) - min(markers{iImage}{iMarker}.corners(:,2)));
                
                if markerSize > bestMarkerSize
                    bestMarkerSize = markerSize;
                    bestMarkerIndex = iMarker;
                end
            end
        end
        
        if bestMarkerIndex == -1
            lightHomographies{iImage} = {[], []};
            monsterFaces{iImage} = [];
            continue;
        end
        
        T = cp2tform(markers{iImage}{bestMarkerIndex}.corners, [120,120;120,220;220,120;220,220], 'projective');
        H = T.tdata.T';
        H = H / H(3,3);
        warpedImage = warpProjective(images(:,:,iImage), H, size(images(:,:,iImage)));

        % Doesn't seem to help
%             trackHomography = mexTrackLucasKanade(uint8(templateImage), double(templateRectangle), double(numPyramidLevels), double(512), double(0), im2uint8(warpedImage), double(maxIterations), double(convergenceTolerance), double(eye(3,3)));
%             
%             trackHomography(1:2,3) = -trackHomography(1:2,3);
%             H2 = trackHomography*H; 
%             H2 = H2 / H2(3,3);
%             
%             warpedImage2 = warpProjective(images(:,:,iImage), H2, size(images(:,:,iImage)));

        monsterFaces{iImage} = warpedImage(templateRectangle(3):templateRectangle(4), templateRectangle(1):templateRectangle(2));
        lightHomographies{iImage} = {warpedImage, H};
    end
        
    % 2. Compute which images are per-pixel above the mean, and which are below
    amountAboveMeanThreshold = 10;
    percentAboveMeanThreshold = 0.05;
    
    meanImage = zeros([templateRectangle(4)-templateRectangle(3)+1, templateRectangle(2)-templateRectangle(1)+1]);
    numValidFaces = 0;
    for iImage = 1:length(monsterFaces)
        if ~isempty(monsterFaces{iImage})
            meanImage = meanImage + monsterFaces{iImage};
            numValidFaces = numValidFaces + 1;
        end
    end
    
    if numValidFaces < 2
        disp('Insufficient number of segments');
        return;
    end
    
    meanImage = meanImage / numValidFaces;
    
%     meanImage = mean(images,3);
%     areWhite = zeros(numImages, 1);
%     for iImage = 1:numImages
%         imshow(uint8(images(:,:,iImage)));
%         numAboveMean = length(find(images(:,:,iImage) > (meanImage+amountAboveMeanThreshold)));
%         if (numAboveMean / numel(images(:,:,iImage))) > percentAboveMeanThreshold
%             areWhite(iImage) = 1;
%         end
%     end

  
    areWhite = zeros(numImages, 1);
    for iImage = 1:numImages
%         imshow(uint8(images(:,:,iImage)));
%         imshow(monsterFaces{iImage})

        if isempty(monsterFaces{iImage})
            areWhite(iImage) = 2;
        else
            numAboveMean = length(find(monsterFaces{iImage} > (meanImage+amountAboveMeanThreshold)));
            if (numAboveMean / numel(monsterFaces{iImage})) > percentAboveMeanThreshold
                areWhite(iImage) = 1;
            end
        end
    end
    
    % Remove "unknown" values
    unknownInds = find(areWhite == 2);
    if ~isempty(unknownInds)
        knownInds = find(areWhite ~= 2);
        if unknownInds(1) == 1
           areWhite(1) = areWhite(knownInds(1)); 
           unknownInds = unknownInds(2:end);
        end

        for iUnknown = 1:length(unknownInds)
            areWhite(unknownInds(iUnknown)) = areWhite(unknownInds(iUnknown) - 1);
        end
    end % if ~isempty(unknownInds)

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
    global g_curImageIndex;
    
    if strcmpi(g_cameraType, 'webots')
        image = webotsCameraCapture();
    elseif strcmpi(g_cameraType, 'usbcam')
        persistent usbcamStarted;
        
        cameraId = 0;
        
        if isempty(usbcamStarted)
            usbcamStarted = false;
        end
        
        if ~usbcamStarted
            mexCameraCapture(0, cameraId);
            usbcamStarted = true;
        end
        
        image = imresize(rgb2gray2(mexCameraCapture(1, cameraId)), g_processingSize);
    elseif strcmpi(g_cameraType, 'offline')
        persistent curImageIndex;
                
        image = imresize(rgb2gray2(imread(sprintf(g_filenamePattern, g_whichImages(g_curImageIndex)))), g_processingSize);
        g_curImageIndex = g_curImageIndex + 1;
    else
        assert(false);
    end
    
end
