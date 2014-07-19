function runTests_detectNonMarkers_perImage(workQueue, algorithmParameters)
    
    showImageDetectionWidth = 640;
    
    perImageTic = tic();
    for iWork = 1:length(workQueue)
        image = imread(workQueue{iWork}.inputFilename);
                
        if size(image,3) ~= 1
            image = rgb2gray(image);
        end
        
        if size(image,1) > algorithmParameters.maxDetectionSize(1) || size(image,2) > algorithmParameters.maxDetectionSize(2)
            scale = min(algorithmParameters.maxDetectionSize./size(image));
            image = imresize(image, size(image)*scale);
        else 
            scale = 1; %#ok<NASGU>
        end
        
        imageSize = size(image); %#ok<NASGU>
        
        [detectedQuads, detectedQuadValidity, detectedMarkers] = extractMarkers(image, algorithmParameters);
        
        save(workQueue{iWork}.outputFilename, 'detectedQuads', 'detectedQuadValidity', 'detectedMarkers', 'imageSize', 'scale');
        
        if showImageDetectionWidth(1) ~= size(image,2)
            showImageDetectionsScale = showImageDetectionWidth(1) / size(image,2);
        else
            showImageDetectionsScale = 1;
        end
        
        toShowResults = {
            workQueue{iWork}.iPattern - 1,...
            workQueue{iWork}.iFilename - 1,...            
            0,...
            0,...
            0,...
            0,...
            0,...
            'none',...
            0,...
            0,...
            0,...
            length(detectedQuads),...
            length(detectedQuads),...
            length(find(detectedQuadValidity == 0))};
        
        markersToDisplay = cell(0,3);        
        
        if ~isempty(detectedMarkers)
            for iMarker = 1:length(detectedMarkers)
                markersToDisplay(end+1,:) = {detectedMarkers{iMarker}.corners, 6, detectedMarkers{iMarker}.name(8:end)}; %#ok<AGROW>
            end
        end
        
        drawnImage = mexDrawSystemTestResults(uint8(image), detectedQuads, detectedQuadValidity, markersToDisplay(:,1), int32(cell2mat(markersToDisplay(:,2))), markersToDisplay(:,3), showImageDetectionsScale, [workQueue{iWork}.inputFilename, '_out.png'], toShowResults);
        drawnImage = drawnImage(:,:,[3,2,1]);
        
        if ~isempty(find(detectedQuadValidity == 0, 1))
            disp(sprintf('Spurrious detection at %d %d on %s', workQueue{iWork}.iPattern, workQueue{iWork}.iFilename, workQueue{iWork}.inputFilename));
        end
        
        if mod(iWork, 100) == 0
            disp(sprintf('Finished %d/%d in %f seconds', iWork, length(workQueue), toc(perImageTic)));
            perImageTic = tic();
        end
    end