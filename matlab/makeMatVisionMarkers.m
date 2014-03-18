
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

% Letters:
matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials/unpadded';
fnames = {'A', 'B', 'C', 'D';
    'E', 'F', 'G', 'J';
    'K', 'L', 'M', 'N';
    'P', 'R', 'T', 'Y'};
fnames = cellfun(@(name)fullfile(matMarkerPath, [name '.png']), fnames, 'UniformOutput', false);
angles = zeros(4);
fidColors = zeros(1,3);

% % Cozmo/Anki Logos
% matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/matWithFiducials/unpadded'; 
% fnames = getfnames(matMarkerPath, '*.png', 'useFullPath', true);
% fnames = fnames([1:2:end 2:2:end]);
% angles = [zeros(2,4); 180*ones(2,4)];
% fidColors = permute(reshape(repmat([78 82 89; 27 130 193]/255, [8 1]), [4 4 3]), [2 1 3]);

numImages = numel(fnames);
assert(numImages == 16, 'Expecting to find 16 images.');
images = fnames; %cell(4,4);
% for i = 1:numImages
%     images{i} = imread(fullfile(matMarkerPath, fnames{i}));
% end

[xgrid,ygrid] = meshgrid(linspace(-200,200,4));
numCorners = zeros(4,4); % TODO: remove
        
createWebotsMat(images, numCorners', xgrid', ygrid', angles', ...
    markerSize_mm, fnames,'FiducialColor', fidColors, ...
    'ForegroundColor', zeros(1,3), 'BackgroundColor', ones(1,3));

%markerLibrary.Save();

