
function run_labelTestImages()

outputFilename = 'Z:/Documents/Box Documents/Cozmo SE/cozmoShort/labeledCorners.mat';

[trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages();

% frameNums = 1:length(trackingImageFilenames);
frameNums = [1,60,65,70,75,80,85,90];
% frameNums = [1];
trackingImageCorners = cell(0, 1);
for i = frameNums
    trackingImageCorners{end+1} = labelImageCorners(trackingImages{i});
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
    imshow(trackingImages{frameNums(i)})
    hold on;
    scatter(trackingImageCorners{i}(1,:), trackingImageCorners{i}(2,:), 'r+');
end

keyboard