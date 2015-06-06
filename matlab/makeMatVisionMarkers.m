function makeMatVisionMarkers(matType)

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
 
switch(matType)
  case 'letters'
    %% 4x4 Letters:
    gridSize = 4;
    markerSize_mm = 30;
    matMarkerPath = fullfile(fileparts(mfilename('fullpath')), '..', '..', 'products-cozmo-large-files/VisionMarkers/letters/withFiducials');
    fnames = {'A', 'B', 'C', 'D';
      '7', 'F', 'G', 'J';
      'K', 'L', 'M', 'Q';
      'P', 'R', 'T', 'Y'};
    fnames = cellfun(@(name)fullfile(matMarkerPath, [name '.png']), fnames, 'UniformOutput', false);
    % Swap in "?" for D
    fnames{1,4} = '~/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/questionMark.png';
    
    angles = zeros(4);
    fidColors = zeros(1,3);
    
    matDefinitionFile = fullfile(fileparts(mfilename('fullpath')), '..', 'include/anki/cozmo/basestation/Mat_Letters_30mm_4x4.def');
    
  case 'gears'
    %% "Gears"
    
    gridSize = 4;
    markerSize_mm = 30;
    matMarkerPath = fullfile(fileparts(mfilename('fullpath')), '..', '..', 'products-cozmo-large-files/VisionMarkers/matGears/withThinFiducials');
    fnames = {'00', '01', '02', '03';
      '04', '05', '06', '07';
      '08', '09', '10', '11';
      '12', '13', '14', '15'};
    fnames = cellfun(@(name)fullfile(matMarkerPath, ['MatGear' name '.png']), fnames, 'UniformOutput', false);
    fnames{2,2} = fullfile(fileparts(mfilename('fullpath')), '..', '..', 'products-cozmo-large-files/VisionMarkers/matGears/withThinFiducials/inverted/MatGear08.png');
    fnames{4,2} = fullfile(fileparts(mfilename('fullpath')), '..', '..', 'products-cozmo-large-files/VisionMarkers/matGears/withThinFiducials/inverted/MatGear04.png');
    fnames{4,3} = fullfile(fileparts(mfilename('fullpath')), '..', '..', 'products-cozmo-large-files/VisionMarkers/matGears/withThinFiducials/inverted/MatGear15.png');
    
    angles = zeros(4);
    fidColors = zeros(1,3);
    
    matDefinitionFile = fullfile(fileparts(mfilename('fullpath')), '..', 'basestation/include/anki/cozmo/basestation/Mat_Gears_30mm_4x4.def');
    
  case 'ankiLogo'
    %% Anki Logo with 8-bit code
    markerSize_mm = 30;
    matMarkerPath = '~/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat';
    gridSize = 8;
    outputImageSize = 512;
    TransparentColor = [1 1 1];
    
    ankiLogo  = imread(fullfile(matMarkerPath, 'ankiLogo.png'));
    [nrows,ncols,~] = size(ankiLogo);
    bitImages = cell(1,8);
    for i = 1:8
      bitImages{i} = imresize(imread(fullfile(matMarkerPath, sprintf('bit%d.png', i-1))), [nrows ncols], 'bilinear');
    end
    
    fnames = cell(gridSize);
    for iGrid = 1:gridSize^2
      currentImage = ankiLogo;
      whichBits = dec2bin(iGrid, 8);
      for iBit = 1:8
        if whichBits(iBit) == '1'
          currentImage = min(currentImage, bitImages{iBit});
        end
      end
      
      % Create the logo with the fiducial
      [logoWithFiducial, AlphaChannel] = VisionMarkerTrained.AddFiducial(...
        currentImage, ...
        'CropImage', false, ...
        'OutputSize', outputImageSize, ...
        'PadOutside', false, ... 'FiducialColor', [.106 .51 .757], ...
        'TransparentColor', TransparentColor, ...
        'TransparencyTolerance', 0.25);
      
      %subplot(gridSize, gridSize, iGrid)
      %imagesc(logoWithFiducial, 'AlphaData', AlphaChannel), axis image off
      fnames{iGrid} = fullfile(matMarkerPath, 'unpadded', sprintf('ankiLogoWithBits%.3d.png', iGrid));
      imwrite(logoWithFiducial, fnames{iGrid}, 'Alpha', AlphaChannel);
    end
    
    matDefinitionFile = '~/Code/products-cozmo/include/anki/cozmo/basestation/Mat_AnkiLogoPlus8Bits_8x8.def';
end % switch(matType)

%%
[xgrid,ygrid] = meshgrid(linspace(-200,200,gridSize));
numCorners = zeros(gridSize); % TODO: remove
angles = zeros(gridSize);

createWebotsMat(fnames, numCorners', xgrid', ygrid', angles', ...
    markerSize_mm, fnames, ...
    'ForegroundColor', zeros(1,3), 'BackgroundColor', ones(1,3), ...
    'DefinitionFile', matDefinitionFile, ...
    'MarkerSize', markerSize_mm);


