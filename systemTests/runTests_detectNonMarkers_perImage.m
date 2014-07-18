function runTests_detectNonMarkers_perImage(workQueue, algorithmParameters)
    
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
        
        [detectedQuads, detectedQuadValidity, detectedMarkers] = extractMarkers(image, algorithmParameters); %#ok<NASGU,ASGLU>
        
        save(workQueue{iWork}.outputFilename, 'detectedQuads', 'detectedQuadValidity', 'detectedMarkers', 'imageSize', 'scale');
        
        if mod(iWork, 100) == 0
            disp(sprintf('Finished %d/%d in %f seconds', iWork, length(workQueue), toc(perImageTic)));
            perImageTic = tic();
        end
    end