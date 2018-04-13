subdirs = {'charger', 'lightCubeSquare', 'lightCubeCircle', 'lightCubeK_lightOnDark', 'customSDK'};

for iSubDir = 1:length(subdirs)
  
  rootPath = fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.VictorImageDir, subdirs{iSubDir});
  % '~/Code/cozmo-game/lib/anki/products-cozmo-large-files/VisionMarkers/matGears/withFiducials';
  fnames = getfnames(rootPath, '*.png');
  
  fprintf('Creating rotated versions of %d markers in %s\n', length(fnames), rootPath);
  
  if ~isdir(fullfile(rootPath, 'rotated'))
    mkdir(fullfile(rootPath, 'rotated'))
  end
  
  for i = 1:length(fnames)
    [img, ~, alpha] = imread(fullfile(rootPath, fnames{i}));
    for angle = [0 90 180 270]
      [~,basename,~] = fileparts(fnames{i});
      imwrite(imrotate(img, angle), ...
        fullfile(rootPath, 'rotated', sprintf('%s_%.3d.png', basename, angle)), ...
        'Alpha', imrotate(alpha, angle, 'bilinear'));
    end
  end
  
end