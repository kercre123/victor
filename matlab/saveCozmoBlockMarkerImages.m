function saveCozmoBlockMarkerImages(faceImg, name, markerLibrary, rootPath)

if nargin < 4
    rootPath = '~/Box Sync/Cozmo SE/VisionMarkers';
end

if nargin < 3 || isempty(markerLibrary)
    markerLibrary = VisionMarkerLibrary.Load(rootPath);
end

if ~iscell(faceImg)
    faceImg = {faceImg};
end

numFaces = length(faceImg);
markerImg = cell(1, numFaces);
for i = 1:numFaces
    markerImg{i} = VisionMarker.DrawFiducial('Image', faceImg{i});
end

if numFaces == 1
    temp = markerImg{1};
    markerImg = cell(1,6);
    [markerImg{:}] = deal(temp);
end

% Create a temporary Block object, which will in turn create all the faces'
% VisionMarkers
B = Block(name, 'faceImages', markerImg);

outputDir = fullfile(rootPath, 'blocks', name);
if ~isdir(outputDir)
    mkdir(outputDir);
end

for i = 1:B.numMarkers
    % Add each face's marker to the library
    markerLibrary.AddMarker(B.markers{i});
    
    % Save the marker image
    imwrite(imresize(markerImg{i}, [512 512]), ...
        fullfile(outputDir, sprintf('face%d.png',i)));
end

% Save the updated marker library
markerLibrary.Save(rootPath);

