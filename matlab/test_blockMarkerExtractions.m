
% function test_blockMarkerExtractions()

image = imresize(rgb2gray(imread('C:\Anki\blockImages\testTrainedCodes.png')), [240,320]);
imageSize = size(image);

scaleImage_thresholdMultiplier = .75;
scaleImage_numPyramidLevels = 3;
component1d_minComponentWidth = 0;
component1d_maxSkipDistance = 0;
minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
component_sparseMultiplyThreshold = 1000.0;
component_solidMultiplyThreshold = 2.0;
component_minHollowRatio = 1.0;
quads_minQuadArea = 100 / 4;
quads_quadSymmetryThreshold = 2.0;
quads_minDistanceFromImageEdge = 2;
decode_minContrastRatio = 1.25;
returnInvalidMarkers = 0;

[quads, markerTypes] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, returnInvalidMarkers);


% figure(1); hold off; imshow(image); hold on;

markerLabels = {
    'MARKER_ALL_BLACK',
    'MARKER_ALL_WHITE', 
    'MARKER_ANGRYFACE', 
    'MARKER_ANKILOGO', 
    'MARKER_BATTERIES', 
    'MARKER_BULLSEYE', 
    'MARKER_FIRE', 
    'MARKER_SQUAREPLUSCORNERS', 
    'MARKER_UNKNOWN'};

figure(1); 
hold off; 
imshow(image);
hold on; 
for i = 1:length(quads)     
    plot(quads{i}([1:end,1],1),quads{i}([1:end,1],2));
    text(...
        mean(quads{i}(:,1)), mean(quads{i}(:,2)),...
        markerLabels{markerTypes(i)+1},...
        'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7]);
end

markers = simpleDetector(image);

figure(2); 
hold off; 
imshow(image);
hold on; 
for i = 1:length(markers)     
    plot(markers{i}.corners([1:end,1],1),markers{i}.corners([1:end,1],2));
    text(...
        mean(markers{i}.corners(:,1)), mean(markers{i}.corners(:,2)),...
        markers{i}.name,...
        'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7]);
end



