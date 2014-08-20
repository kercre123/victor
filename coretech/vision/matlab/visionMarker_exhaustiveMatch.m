% function visionMarker = visionMarker_exhaustiveMatch(allMarkerImages, exhaustiveSearchMethod, img, corners)

% Example:
% allMarkerImages = visionMarker_exhaustiveMatch_loadallMarkerImages(VisionMarkerTrained.TrainingImageDir);
% im = imread('~/Documents/Anki/products-cozmo-large-files/systemTestsData/images/cozmo_date2014_06_04_time16_53_01_frame0.png');
% corners = [64.4682187605567,45.7997011780586;62.7411426255910,114.884124740094;134.111020988182,45.1725483178793;133.199911871809,115.231167206507];
% visionMarker = visionMarker_exhaustiveMatch(allMarkerImages, {1,'backward'}, im, corners);

function visionMarker = visionMarker_exhaustiveMatch(allMarkerImages, exhaustiveSearchMethod, img, corners)
    exhaustiveSearchThreshold = 0.1;
    
    if exhaustiveSearchMethod{1} == 1
        [codeName, codeID, matchDistance] = visionMarker_exhaustiveMatch_search( ...
            allMarkerImages, img, corners, exhaustiveSearchMethod{2});
    else
        persistent numDatabaseImages databaseImageHeight databaseImageWidth databaseImages databaseLabelIndexes; %#ok<TLEV>
        
        if isempty(numDatabaseImages)
            % TODO: get paths from the right place
            patterns = {'Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/*.png'};
            
            if ~ispc()
                for i = 1:length(patterns)
                    patterns{i} = strrep(patterns{i}, 'Z:/', [tildeToPath(), '/']);
                end
            end
            
            imageFilenames = {};
            for iPattern = 1:length(patterns)
                files = dir(patterns{iPattern});
                for iFile = 1:length(files)
                    imageFilenames{end+1} = [strrep(patterns{iPattern}, '*.png', ''), files(iFile).name]; %#ok<AGROW>
                end
            end
            [numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes] = mexLoadExhaustiveMatchDatabase(imageFilenames);
        end
        
        [markerName, orientation, matchDistance] = mexExhaustiveMatchFiducialMarker(uint8(img*255), corners, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);
        
        if orientation == 0
            codeName = [markerName(8:end), '_000'];
        elseif orientation == 90
            codeName = [markerName(8:end), '_090'];
        elseif orientation == 180
            codeName = [markerName(8:end), '_180'];
        elseif orientation == 270
            codeName = [markerName(8:end), '_270'];
        end
        
        codeID = []; % TODO: must this be set?
    end
        
    if matchDistance < exhaustiveSearchThreshold
        isValid = true;
    else
        isValid = false;
    end
    
    visionMarker = VisionMarkerTrained([], 'Initialize', false);
    
    visionMarker.codeID = codeID;
    visionMarker.codeName = codeName;
    visionMarker.corners = corners;   
    visionMarker.name = codeName;
    visionMarker.fiducialSize = 1;
    visionMarker.isValid = isValid;
    visionMarker.matchDistance = matchDistance;
    
    tform = cp2tform([0 0 1 1; 0 1 0 1]', corners, 'projective');
    visionMarker.H = tform.tdata.T';
    
     
    