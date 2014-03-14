
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
     
markerSize_mm = 75;
matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/letters';
fnames = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'J', 'K'};
%fnames = getfnames('../robot/simulator/protos/textures/letters', '*.png', 'useFullPath', true);
numImages = length(fnames);
assert(numImages == 9, 'Expecting to find 9 images.');
images = cell(3,3);
for i = 1:length(fnames)
    images{i} = imread(fullfile(matMarkerPath, [fnames{i} '.png']));
end


xgrid  = [-250 0 250;
    -250 0 250;
    -250 0 250];

ygrid  = [ 250  250  250;
    0    0    0;
    -250 -250 -250];
        
createWebotsMat(images, numCorners', xgrid', ygrid', angles', markerSize_mm, fnames);

%markerLibrary.Save();

