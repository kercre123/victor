% function [trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages()

function [trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages()

cozmoShortDirectory = 'Z:/Documents/Box Documents/Cozmo SE/cozmoShort/';
trackingImageFilenames = {};

for i = 0:49
    trackingImageFilenames{end+1} = [cozmoShortDirectory, sprintf('cozmo_2014-01-24_16-13-28_%d.png', i)];
end

for i = 0:49
    trackingImageFilenames{end+1} = [cozmoShortDirectory, sprintf('cozmo_2014-01-24_16-16-40_%d.png', i)];
end

centeredImageFilename = [cozmoShortDirectory, 'cozmo_2014-01-24_16-29-47_0.png'];
tiltedImageFilename = [cozmoShortDirectory, 'cozmo_2014-01-24_16-29-15_0.png'];
shiftedImageFilename = [cozmoShortDirectory, 'cozmo_2014-01-24_16-30-40_0.png'];

tic
trackingImages = cell(length(trackingImageFilenames), 1);
for i = 1:length(trackingImageFilenames);
    trackingImages{i} = imread(trackingImageFilenames{i});
end

centeredImage = imread(centeredImageFilename);
tiltedImage = imread(tiltedImageFilename);
shiftedImage = imread(shiftedImageFilename);
toc

