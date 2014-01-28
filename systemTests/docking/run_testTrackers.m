function run_testTrackers()

groundTruthFilename = 'Z:/Documents/Box Documents/Cozmo SE/cozmoShort/labeledCorners.mat';

[trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages();

% test various template sizes

keyboard