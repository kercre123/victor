
% function faceDetection_webfaces_createTrainingSet()

% allFaces = faceDetection_webfaces_readLandmarks('/Users/pbarnum/Documents/datasets/webfaces');
% [allDetections, posemap] = faceDetection_webfaces_detectPoses(allFaces, 1:length(allFaces), '~/Documents/datasets/webfaces/webfaces-detectedPoses%d.mat');

% allFaces = faceDetection_webfaces_readLandmarks('/Users/pbarnum/Documents/datasets/webfaces');
% load('~/Documents/datasets/webfaces/webfaces-detectedPoses.mat', 'allDetections', 'posemap');
% faceDetection_webfaces_createTrainingSet(allFaces, allDetections, posemap, '~/Documents/Anki/products-cozmo-large-files/faceDetection/webfacesFrontFaces.dat');

function faceDetection_webfaces_createTrainingSet(allFaces, allDetections, posemap, outputFilename)
    
    showDetections = false;
    rectangleExpansionPercent = 1.75;
    numFacesTotal = 0;
    
    fileId = fopen(outputFilename, 'w');
    
    for iDetection = 1:length(allDetections)
        curImage = rgb2gray2(imread(allDetections{iDetection}{1}));
        
        goodFaces = zeros(0,4);
        
        for iFace = 1:length(allDetections{iDetection}{3})
            % Only use frontal and +-15 degrees faces
            if abs(posemap(allDetections{iDetection}{3}(iFace).c)) > 15
                continue;
            end
            
            x = mean(mean(allDetections{iDetection}{3}(iFace).xy(:,[1,3])));
            y = mean(mean(allDetections{iDetection}{3}(iFace).xy(:,[2,4])));
            
            %             keyboard
            
            curLandmarks = allFaces{iDetection}{2};
            
            bestLandmarkDistance = Inf;
            bestLandmarkIndex = -1;
            for iLandmark = 1:size(curLandmarks,1)
                curDistance = sqrt((x-curLandmarks(iLandmark,5)).^2 + (y-curLandmarks(iLandmark,6)).^2);
                
                if curDistance < bestLandmarkDistance
                    bestLandmarkDistance = curDistance;
                    bestLandmarkIndex = iLandmark;
                end
            end % for iLandmark = minLandmark:maxLandmark
            
            if bestLandmarkIndex ~= -1
                bestLandmark = curLandmarks(bestLandmarkIndex,:);
                bestLandmarkEyeDistance = sqrt((bestLandmark(1)-bestLandmark(3)).^2 + (bestLandmark(2)-bestLandmark(4)).^2);
                
                if sqrt(2)*bestLandmarkEyeDistance > bestLandmarkDistance
                    if showDetections
                        hold off;
                        imshows(curImage);
                        hold on;
                        
                        scatter(bestLandmark(1:2:end), bestLandmark(2:2:end), 'r+');
                    end
                    
                    exactTop = min(bestLandmark(2), bestLandmark(4));
                    exactRect = [bestLandmark(1), exactTop, bestLandmark(3)-bestLandmark(1), bestLandmark(8)-exactTop];
                    
                    expandedRect = round([...
                        exactRect(1) - 0.25*rectangleExpansionPercent*exactRect(3),...
                        exactRect(2) - 0.25*rectangleExpansionPercent*exactRect(4),...
                        rectangleExpansionPercent*exactRect(3:4)]);
                    
                    if showDetections
                        rectangle('Position', expandedRect);
                    end
                    
                    expandedRect = round(expandedRect);
                    expandedRect(1:2) = expandedRect(1:2) - 1; % matlab to c
                    expandedRect(1) = max(expandedRect(1), 0);
                    expandedRect(2) = max(expandedRect(2), 0);
                    
                    if expandedRect(1) + expandedRect(3) >= size(curImage,2)
                        expandedRect(3) =  size(curImage,2) - 1;
                    end
                    
                    if expandedRect(2) + expandedRect(4) >= size(curImage,1)
                        expandedRect(4) =  size(curImage,1) - 1;
                    end
                    
                    goodFaces(end+1, :) = expandedRect; %#ok<AGROW>
                    
                    if showDetections
                        pause(.5);
%                         pause();
                    end
                end
            end % if bestLandmarkIndex ~= -1
        end % for iFace = 1:length(allDetections{iDetection}{3})
        
        numGoodFaces = size(goodFaces,1);
        if numGoodFaces > 0
            ind = strfind(allDetections{iDetection}{1}, 'webfaces/Caltech_WebFaces');
            
            lineOut = sprintf('%s   %d', allDetections{iDetection}{1}(ind:end), numGoodFaces);
            
            numFacesTotal = numFacesTotal + numGoodFaces;
            
            for iFace = 1:numGoodFaces
                
                lineOut = [lineOut, sprintf('   %d %d %d %d', goodFaces(iFace,1), goodFaces(iFace,2), goodFaces(iFace,3), goodFaces(iFace,4))]; %#ok<AGROW>
            end
            
            lineOut = [lineOut, sprintf('\n')]; %#ok<AGROW>
            
            fwrite(fileId, lineOut);
        end % if numGoodFaces > 0
    end % for iDetection = 1:length(allDetections)
    
    fclose(fileId);
    
    disp(sprintf('Num faces total = %d', numFacesTotal));    
    
end % function faceDetection_webfaces_createTrainingSet()


