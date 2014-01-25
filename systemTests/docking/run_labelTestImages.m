
function run_labelTestImages()

cozmoShortDirectory = 'Z:/Documents/Box Documents/Cozmo SE/cozmoShort/';
outputFilename = 'Z:/Documents/Box Documents/Cozmo SE/cozmoShort/labeledCorners.mat';

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
ims = cell(length(trackingImageFilenames), 1);
for i = 1:length(trackingImageFilenames);
    ims{i} = imread(trackingImageFilenames{i});
end

centeredImage = imread(centeredImageFilename);
tiltedImage = imread(tiltedImageFilename);
shiftedImage = imread(shiftedImageFilename);
toc

% frameNums = 1:length(trackingImageFilenames);
frameNums = [1,60,65,70,75,80,85,90];
% frameNums = [1];
trackingImageCorners = cell(0, 1);
for i = frameNums
    trackingImageCorners{end+1} = labelImageCorners(ims{i});
    save(outputFilename, 'trackingImageCorners', 'frameNums'); % Keep saving as we go
end

centeredImageCorners = labelImageCorners(centeredImage);
tiltedImageCorners = labelImageCorners(tiltedImage);
shiftedImageCorners = labelImageCorners(shiftedImage);

save(outputFilename, 'trackingImageCorners', 'frameNums', 'centeredImageCorners', 'tiltedImageCorners', 'shiftedImageCorners');

close all
for i = 1:length(frameNums)
    figure();
    title(sprintf('Frame %d', frameNums(i)));
    imshow(ims{frameNums(i)})
    hold on;
    scatter(trackingImageCorners{i}(1,:), trackingImageCorners{i}(2,:), 'r+');
end

keyboard