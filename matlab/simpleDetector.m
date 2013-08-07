function markers = simpleDetector(img, varargin)

maxSmoothingFraction = 0.1; % fraction of max dim
downsampleFactor = 2;
usePyramid = true;
usePerimeterCheck = false;
thresholdFraction = 1; % fraction of local mean to use as threshld
minQuadArea = 100; % about 10 pixels per side
computeTransformFromBoundary = true;
quadRefinementMethod = 'ICP'; % 'ICP' or 'fminsearch'
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
DEBUG_DISPLAY = nargout==0;
embeddedConversions = []; % 1-cp2tform, 2-opencv_cp2tform

parseVarargin(varargin{:});

if ischar(img)
    img = imread(img);
end

img = mean(im2double(img),3);

[nrows,ncols] = size(img);

%% Binary Region Detection

binaryImg = simpleDetector_step1_computeCharacteristicScale(maxSmoothingFraction, nrows, ncols, downsampleFactor, img, thresholdFraction, usePyramid, embeddedConversions, DEBUG_DISPLAY);

t_binaryRegions = tic;

[numRegions, area, indexList, bb, centroid] = simpleDetector_step2_computeRegions(binaryImg, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY);

if numRegions == 0
    % Didn't even find any regions in the binary image, nothing to
    % do!
    return;
end

[numRegions, indexList, centroid] = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY);

fprintf('Binary region detection took %.2f seconds.\n', toc(t_binaryRegions));

t_squareDetect = tic;

[quads, quadTforms] = simpleDetector_step4_computeQuads(nrows, ncols, numRegions, indexList, centroid, minQuadArea, cornerMethod, computeTransformFromBoundary, quadRefinementMethod, embeddedConversions, DEBUG_DISPLAY);

fprintf('Square detection took %.3f seconds.\n', toc(t_squareDetect));

markers = simpleDetector_step5_decodeMarkers(img, quads, quadTforms, minQuadArea, embeddedConversions, DEBUG_DISPLAY);

end % FUNCTION simpleDetector()

