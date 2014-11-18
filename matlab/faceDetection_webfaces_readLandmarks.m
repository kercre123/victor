% function faceDetection_webfaces_readLandmarks()

% allFaces = faceDetection_webfaces_readLandmarks('/Users/pbarnum/Documents/datasets/webfaces');

function allFaces = faceDetection_webfaces_readLandmarks(webfacesBaseDirectory)
    
    showFaces = false;
    
    groundTruthFilename = [webfacesBaseDirectory, '/WebFaces_GroundThruth.txt'];
    %     textLines = textread(groundTruthFilename, '%s %f %f %f %f %f %f %f %f', 'whitespace', '\n\r');
    textLines = textread(groundTruthFilename, '%s', 'whitespace', '\n\r ');
    
    numFaces = length(textLines) / 9;
    
    imageFilenames = textLines(1:9:end);
    
    landmarks = cell(8,1);
    for i = 1:8
        landmarks{i} = textLines((1+i):9:end);
    end
    
    assert(length(imageFilenames) == numFaces);
    
    allFaces = {};
    
    lastFilename = imageFilenames{1};
    faceLandmarks = zeros(0,8);
    for iFace = 1:numFaces
        if ~strcmp(lastFilename, imageFilenames{iFace})
            allFaces{end+1} = {[webfacesBaseDirectory, '/Caltech_WebFaces/', lastFilename], faceLandmarks}; %#ok<AGROW>
            lastFilename = imageFilenames{iFace};
            faceLandmarks = zeros(0,8);
        end
        
        newLandmarks = zeros(1,8);
        
        for i = 1:8
            newLandmarks(i) = str2num(landmarks{i}{iFace});
        end
        
        faceLandmarks(end+1, :) = newLandmarks; %#ok<AGROW>
    end % for iFace = 1:numFaces
    
    allFaces{end+1} = {[webfacesBaseDirectory, '/Caltech_WebFaces/', lastFilename], faceLandmarks}; %#ok<AGROW>
    
    if showFaces
        for iFiles = 1:length(allFaces)
            im = imread([webfacesBaseDirectory, '/Caltech_WebFaces/', allFaces{iFiles}{1}]);
            
            hold off;
            imshows(im);
            hold on;
            
            for iFace = 1:size(allFaces{iFiles}{2},1)
                curLandmarks = allFaces{iFiles}{2}(iFace,:);
                eyeDistance = curLandmarks(3) - curLandmarks(1);
                rightEyeToMouthDistance = curLandmarks(7) - curLandmarks(1);
                leftEyeToMouthDistance = curLandmarks(3) - curLandmarks(7);
                
                outOfPlaneRotationFactor = abs(rightEyeToMouthDistance - leftEyeToMouthDistance) / eyeDistance;
                
                scatter(curLandmarks(1:2:end), curLandmarks(2:2:end), 'r+');
                text(curLandmarks(5), curLandmarks(6), sprintf('%02f', outOfPlaneRotationFactor));
            end
            
            %         if size(allFaces{iFiles}{2},1) > 1
            %             keyboard
            %         end
            
            pause();
        end
    end % if showFaces
%     
%     keyboard
    
    
    
    