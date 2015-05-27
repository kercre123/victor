rootPath = '~/Code/cozmo-game/lib/anki/products-cozmo-large-files/VisionMarkers/matGears/withFiducials';
fnames = getfnames(rootPath, '*.png');
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