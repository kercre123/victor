% function rotationList = getListOfSymmetricMarkers(TrainingImageDirs)

% Check all filenames. 
% 1. Filenames with 4 copies are not symmetric 
% 2. Filenames with 2 copies are 180-degree symmetric 
% 3. Filenames with 1 copy are symmetric 

% once the list is computed, get the number of rotations for 'MARKERNAME' via:
% numRotations = rotationList{strcmp(rotationList(:,1), 'MARKERNAME'), 2};

% rotationList = getListOfSymmetricMarkers({'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbols/withFiducials/', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/letters/withFiducials', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/dice/withFiducials'});

function rotationList = getListOfSymmetricMarkers(TrainingImageDirs)

allFiles = {};

for iDir = 1:length(TrainingImageDirs)
   curFiles = dir([TrainingImageDirs{iDir}, '/rotated/*.png']);
   
   for iFile = 1:length(curFiles)
       allFiles{end+1} = curFiles(iFile).name; %#ok<AGROW>
   end
end

allStrippedFiles = cell(length(allFiles),1);

for iFile = 1:length(allFiles)
    allStrippedFiles{iFile} = upper(strrep(strrep(strrep(strrep(strrep(allFiles{iFile},'.png',''),'_000',''),'_090',''),'_180',''),'_270',''));
end

allUniqueFiles = unique(allStrippedFiles);

rotationList = cell(length(allUniqueFiles), 2);
for iFile = 1:length(allUniqueFiles)
    rotationList{iFile, 1} = allUniqueFiles{iFile};
    rotationList{iFile, 2} = length(find(strcmp(allUniqueFiles{iFile}, allStrippedFiles)));
end

% keyboard