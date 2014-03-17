
% markerLibrary = VisionMarkerLibrary.Load();
%
%         numCorners = [3 2 3;
%             2 1 2;
%             3 2 3];
%         
%         angles = [-90   -180   -180;
%             -90     0      90;
%             0     0      90];
%         
%         anki = imread('~/Box Sync/Cozmo SE/VisionMarkers/ankiLogo.png');
     
markerSize_mm = 65;

matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/letters';
fnames = {'A.png', 'B.png', 'C.png', 'D.png';
    'E.png', 'F.png', 'G.png', 'J.png';
    'K.png', 'L.png', 'M.png', 'N.png';
    'P.png', 'R.png', 'S.png', 'T.png'};

%matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/mat'; %WithFiducials/unpadded';
%fnames = getfnames(matMarkerPath, '*.png');

numImages = numel(fnames);
assert(numImages == 16, 'Expecting to find 16 images.');
images = cell(4,4);
for i = 1:numImages
    images{i} = imread(fullfile(matMarkerPath, fnames{i}));
end

[xgrid,ygrid] = meshgrid(linspace(-200,200,4));
angles = zeros(4,4);
numCorners = zeros(4,4); % TODO: remove
        
createWebotsMat(images, numCorners', xgrid', ygrid', angles', ...
    markerSize_mm, fnames); %,'FiducialColor', [27 130 193]/255, ...
    %'ForegroundColor', zeros(1,3), 'BackgroundColor', ones(1,3));

%markerLibrary.Save();

