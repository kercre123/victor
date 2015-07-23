
% function testPoseEstimation()

% badQuads = testPoseEstimation('~/Documents/Anki/products-cozmo-large-files/systemTestsData/kitty.png');

% [angleX, angleY, angleZ, tX, tY, tZ] = 0.015088522806764  -0.015327397733927  -0.000901891326066  -7.035226345062256   8.196249008178711  72.933975219726562

function badQuadsToReturn = testPoseEstimation(blockFilename)
    
    global badQuads;
    
    testImageSize = [240, 320];
    
    fiducialSize = [128, 128];
    
    algorithmParameters.normalizeImage = false;
    
    algorithmParameters.whichAlgorithm = 'c_6dof';
    
    % Initialize the track
    algorithmParameters.init_scaleTemplateRegionPercent  = 0.9;
    algorithmParameters.init_numPyramidLevels            = 3;
    algorithmParameters.init_maxSamplesAtBaseLevel       = 500;
    algorithmParameters.init_numSamplingRegions          = 5;
    algorithmParameters.init_numFiducialSquareSamples    = 500;
    algorithmParameters.init_fiducialSquareWidthFraction = 0.1;
    
    % Update the track with a new image
    algorithmParameters.track_maxIterations                 = 50;
    algorithmParameters.track_convergenceTolerance_angle    = 0.05 * 180 / pi;
    algorithmParameters.track_convergenceTolerance_distance = 0.05;
    algorithmParameters.track_verify_maxPixelDifference     = 30;
    algorithmParameters.track_maxSnapToClosestQuadDistance  = 0; % If >0, snap to the closest quad, and re-initialize
    
    % Only for track_snapToClosestQuad == true
    minSideLength = round(0.01*max(testImageSize(1),testImageSize(2)));
    maxSideLength = round(0.97*min(testImageSize(1),testImageSize(2)));
    algorithmParameters.detection.useIntegralImageFiltering = true;
    algorithmParameters.detection.scaleImage_thresholdMultiplier = 1.0;
    algorithmParameters.detection.scaleImage_numPyramidLevels = 3;
    algorithmParameters.detection.component1d_minComponentWidth = 0;
    algorithmParameters.detection.component1d_maxSkipDistance = 0;
    algorithmParameters.detection.component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    algorithmParameters.detection.component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    algorithmParameters.detection.component_sparseMultiplyThreshold = 1000.0;
    algorithmParameters.detection.component_solidMultiplyThreshold = 2.0;
    algorithmParameters.detection.component_minHollowRatio = 1.0;
    algorithmParameters.detection.quads_minQuadArea = 100 / 4;
    algorithmParameters.detection.quads_minLaplacianPeakRatio = 5;
    algorithmParameters.detection.quads_quadSymmetryThreshold = 2.0;
    algorithmParameters.detection.quads_minDistanceFromImageEdge = 2;
    algorithmParameters.detection.decode_minContrastRatio = 0; % EVERYTHING has contrast >= 1, by definition
    algorithmParameters.detection.refine_quadRefinementMinCornerChange = 0.005;
    algorithmParameters.detection.refine_quadRefinementMaxCornerChange = 2;
    algorithmParameters.detection.refine_numRefinementSamples = 100;
    algorithmParameters.detection.refine_quadRefinementIterations = 25;
    algorithmParameters.detection.useMatlabForAll = false;
    algorithmParameters.detection.useMatlabForQuadExtraction = false;
    algorithmParameters.detection.matlab_embeddedConversions = EmbeddedConversionsManager();
    algorithmParameters.detection.showImageDetectionWidth = 640;
    algorithmParameters.detection.showImageDetections = false;
    algorithmParameters.detection.preprocessingFunction = []; % If non-empty, preprocessing is applied before compression
    algorithmParameters.detection.imageCompression = {'none', 0}; % Applies compression before running the algorithm
    
    % Display and compression options
    algorithmParameters.showImageDetectionWidth = 640;
    algorithmParameters.showImageDetections = false;
    algorithmParameters.preprocessingFunction = []; % If non-empty, preprocessing is applied before compression
    algorithmParameters.imageCompression = {'none', 0}; % Applies compression before running the algorithm
    
    % Shake the image horizontally, as it's being tracked
    algorithmParameters.shaking = {'none', 0};
    
    % Change the grayvalue of all pixels + or - this percent
    algorithmParameters.grayvalueShift = {'none', 0};
    
    algorithmParametersOrig = algorithmParameters;
    
    cameraCalibration.ncols = testImageSize(2);
    cameraCalibration.skew = 0;
    cameraCalibration.nrows = testImageSize(1);
    % 	cameraCalibration.center_x = 155.83712;
    % 	cameraCalibration.center_y = 117.87848;
    % 	cameraCalibration.focalLength_y = 371.84817;
    % 	cameraCalibration.focalLength_x = 374.98139;
    cameraCalibration.center_x = testImageSize(2) / 2;
    cameraCalibration.center_y = testImageSize(1) / 2;
    cameraCalibration.focalLength_y = 375;
    cameraCalibration.focalLength_x = 375;
    
    templateWidth_mm = 25.0;
    
    liftWidth_mm = 35.0;
    
    %
    % Do the actual testing
    %
    
    fiducialImage = imresize(rgb2gray2(imread(blockFilename)), fiducialSize, 'bilinear');
    
    placePosition = round((testImageSize-fiducialSize) / 2);
    
    testImage = 255*ones(testImageSize, 'uint8');
    
    testImage(placePosition(1):(placePosition(1)+fiducialSize(1)-1), placePosition(2):(placePosition(2)+fiducialSize(2)-1)) = fiducialImage;
    
    assert(min(size(testImage) == testImageSize) == 1);
    
    currentQuad = [...
        placePosition(2), placePosition(1);
        placePosition(2), placePosition(1)+fiducialSize(1);
        placePosition(2)+fiducialSize(2), placePosition(1);
        placePosition(2)+fiducialSize(2), placePosition(1)+fiducialSize(1);
        ];
    
    expected.angleX = 0;
    expected.angleY = 0;
    expected.angleZ = 0;
    expected.tX = 0;
    expected.tY = 0;
    expected.tZ = cameraCalibration.focalLength_x / fiducialSize(2) * templateWidth_mm;
    
    numIterations = 10000;
    pixelsToJitter = [0,1,2,4,8,16];
    
    distances = cell(length(pixelsToJitter),1);
    
    for iPixel = 1:length(pixelsToJitter)
        tic
        distances{iPixel} = computePermutedDistance(testImage, currentQuad, cameraCalibration, templateWidth_mm, liftWidth_mm, algorithmParameters, expected, pixelsToJitter(iPixel), numIterations);
        disp(sprintf('Jitter %0.2f pixels in %f seconds', pixelsToJitter(iPixel), toc()));
        disp(distances{iPixel});
    end
    
    disp('badQuads:');
    disp(badQuads);
    
    %     round([angleX*180/pi, angleY*180/pi, angleZ*180/pi, tX, tY, tZ])
    
    badQuadsToReturn = badQuads;
    
    for iPixel = 1:length(pixelsToJitter)
        disp(sprintf('Jitter %0.2f pixels in %f seconds', pixelsToJitter(iPixel), toc()));
        disp(distances{iPixel});
    end
    
    keyboard
end % function testPoseEstimation()

function distance = computePermutedDistance(testImage, currentQuad, cameraCalibration, templateWidth_mm, liftWidth_mm, algorithmParameters, expected, pixelsToJitter, numIterations)
    global badQuads;
    
    useOpenCV = false;
    
    if isempty(badQuads) || iscell(badQuads)
        badQuads = zeros(4,2,0);
    end
    
    distance.angleX = 0;
    distance.angleY = 0;
    distance.angleZ = 0;
    distance.tX = 0;
    distance.tY = 0;
    distance.tZ = 0;
    distance.approximateZError = 0;
    
    if useOpenCV
        cameraMatrix = [...
            cameraCalibration.focalLength_x,0,cameraCalibration.center_x;
            0,cameraCalibration.focalLength_y,cameraCalibration.center_y;
            0,0,1];
        
        objectPoints = [...
            -templateWidth_mm/2, -templateWidth_mm/2, 0;...
            -templateWidth_mm/2, templateWidth_mm/2, 0;...
            templateWidth_mm/2,  -templateWidth_mm/2, 0;...
            templateWidth_mm/2,  templateWidth_mm/2, 0];
    end
    
    progressbar = ProgressBar(sprintf('Computing Jitter %d',pixelsToJitter));
    progressbar.set_increment(100/numIterations);
    %     progressbar.showTimingInfo = true;
    
    numBadQuads = 0;
    for iteration = 1:numIterations
        jitteredQuad = currentQuad + rand(4,2) * pixelsToJitter;
        
        if mod(iteration, 100) == 0
            progressbar.increment();
        end
        
        if useOpenCV
            pnPvariant = 'P3P'; % Options are: {'Iterative', 'P3P', 'EPnP'}
            [rvec, tvec] = cv.solvePnP(reshape(objectPoints,[1,4,3]), reshape(jitteredQuad, [1,4,2]), cameraMatrix, single([0,0,0,0,0]'), 'Flags', pnPvariant);
            angleX = rvec(1);
            angleY = rvec(2);
            angleZ = rvec(3);
            tX = tvec(1);
            tY = tvec(2);
            tZ = tvec(3);
        else
            [~, angleX, angleY, angleZ, tX, tY, tZ, ~] = initTracker(testImage, jitteredQuad, cameraCalibration, templateWidth_mm, algorithmParameters);
            
            if isempty(angleX)
                disp(sprintf('initTracker failed on quad:'));
                disp(jitteredQuad);
                badQuads(:,:,end+1) = jitteredQuad; %#ok<AGROW>
                numBadQuads = numBadQuads + 1;
                continue;
            end % if isempty(angleX)
        end
        
        distance.angleX = distance.angleX + abs(expected.angleX - angleX);
        distance.angleY = distance.angleY + abs(expected.angleY - angleY);
        distance.angleZ = distance.angleZ + abs(expected.angleZ - angleZ);
        distance.tX = distance.tX + abs(expected.tX - tX);
        distance.tY = distance.tY + abs(expected.tY - tY);
        distance.tZ = distance.tZ + abs(expected.tZ - tZ);
    end % for iteration = 1:numIterations
    
    distance.angleX = distance.angleX / (numIterations+numBadQuads);
    distance.angleY = distance.angleY / (numIterations+numBadQuads);
    distance.angleZ = distance.angleZ / (numIterations+numBadQuads);
    distance.tX = distance.tX / (numIterations+numBadQuads);
    distance.tY = distance.tY / (numIterations+numBadQuads);
    distance.tZ = distance.tZ / (numIterations+numBadQuads);
    
    % TODO: is this approximate equation reasonable?
    distance.approximateZError = distance.tZ + abs(liftWidth_mm/2 * sin(max([distance.angleX, distance.angleY, distance.angleZ])));
    
    clear('progressbar');
end

function [samples,...
        angleX,...
        angleY,...
        angleZ,...
        tX,...
        tY,...
        tZ,...
        trackerIsValid] = initTracker(image, currentQuad, cameraCalibration, templateWidth_mm, algorithmParameters)
    
    try
        [samples,...
            angleX,...
            angleY,...
            angleZ,...
            tX,...
            tY,...
            tZ] = mexPlanar6dofTrack(...
            image,...
            currentQuad,...
            cameraCalibration.focalLength_x,...
            cameraCalibration.focalLength_y,...
            cameraCalibration.center_x,...
            cameraCalibration.center_y,...
            templateWidth_mm,...
            algorithmParameters.init_scaleTemplateRegionPercent,...
            algorithmParameters.init_numPyramidLevels,...
            algorithmParameters.init_maxSamplesAtBaseLevel,...
            algorithmParameters.init_numSamplingRegions,...
            algorithmParameters.init_numFiducialSquareSamples,...
            algorithmParameters.init_fiducialSquareWidthFraction,...
            algorithmParameters.normalizeImage);
        
        trackerIsValid = true;
    catch exception
        disp(getReport(exception));
        samples = [];
        angleX = [];
        angleY = [];
        angleZ = [];
        tX = [];
        tY = [];
        tZ = [];
        trackerIsValid = false;
        %         keyboard
    end % try ... catch
end % function initTracker()
