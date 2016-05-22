% function allMarkerImages = visionMarker_exhaustiveMatch_loadallMarkerImages(TrainingImageDirs)

% allMarkerImages = visionMarker_exhaustiveMatch_loadallMarkerImages(VisionMarkerTrained.TrainingImageDir);

function allMarkerImages = visionMarker_exhaustiveMatch_loadallMarkerImages(TrainingImageDirs)
    
    allMarkerImages = {};
    
    for iDir = 1:length(TrainingImageDirs)
        curDir = TrainingImageDirs{iDir};
        
        if ispc()
            curDir = strrep(curDir, '~/', 'z:/');
        end
        
        curDir = strrep(curDir, 'rotated', '');
        
        filenames = dir([curDir, '*.png']);
        
        for iFile = 1:length(filenames)
            [img, ~, alpha] = imread([curDir,filenames(iFile).name]);
            img = mean(im2double(img),3);
            img(alpha < .5) = 1;
            
            img(img<0.5) = 0;
            img(img>0) = 1;
            
            allMarkerImages{end+1} = {img, filenames(iFile).name(1:(end-4))}; %#ok<AGROW>
        end
    end
    