
function simpleTrackingTest()
    
%     trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_14/';
%     firstFilename = 'cozmo1_img_53151.jpg';
%     lastFilename = 'cozmo1_img_57704.jpg';
%     filenameIncrement = 1;
%     templateQuad = [127.728426395939,43.7487309644670;173.048223350254,44.9670050761421;171.829949238579,89.3121827411168;127.728426395939,88.5812182741117];
    
    trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_15KevinRotate/';
    firstFilename = 'cozmo1_img_46168.jpg';
    lastFilename = 'cozmo1_img_50012.jpg';
    filenameIncrement = 1;
    templateQuad = [51.0989847715736,64.7030456852792;183.403553299492,64.9467005076142;181.941624365482,186.774111675127;59.6269035532995,187.992385786802];

    focalLength_x = 374.98139;
    focalLength_y = 371.84817;
    camCenter_x = 155.83712;
    camCenter_y = 117.87848;
    templateWidth_mm = 25.0;
    fiducialSquareWidthFraction = 0.1;
    numPyramidLevels = 4; % TODO: Compute from resolution to get down to a given size?
    maxSamplesAtBaseLevel = 500; % a.k.a. numFiducialEdgeSamples
    numSamplingRegions = 5;
    numFiducialSquareSamples = 500; % a.k.a. numInteriorSamples
    maxIterations = 25;
    convergenceTolerance_angle = 0.25 * pi / 180;
    convergenceTolerance_distance = 0.25;
    verify_maxPixelDifference = 30;
    normalizeImage = false;
    %     radialDistortionCoeffs = [0.02865, 0.14764, -0.00074, 0.00046];
    
    if numFiducialSquareSamples > 0
        scaleTemplateRegionPercent = 1.0 - fiducialSquareWidthFraction;
    else
        scaleTemplateRegionPercent = 1.1;
    end
    
    templateQuadCenter = mean(templateQuad,1);
    
    files = dir([trackingDatabaseDirectory, '*.jpg']);
    
    firstIndex = -1;
    lastIndex = -1;
    for iFile = 1:length(files)
        if strcmpi(files(iFile).name, firstFilename)
            firstIndex = iFile;
        end
        
        if strcmpi(files(iFile).name, lastFilename)
            lastIndex = iFile;
        end
    end % for iFile = 1:length(files)
    
    assert(firstIndex ~= -1);
    assert(lastIndex ~= -1);
    
    fileIndexes = firstIndex:filenameIncrement:lastIndex;
    
    images = cell(length(fileIndexes), 1);
    for iFile = 1:length(fileIndexes)
        images{iFile} = imread([trackingDatabaseDirectory,files(fileIndexes(iFile)).name]);
    end % for iFile = firstFilename:filenameIncrement:lastFilename
    
    %     templateMask = roipoly(images{1}, templateQuad(:,1), templateQuad(:,2));
    
    [samples,...
        angleX,...
        angleY,...
        angleZ,...
        tX,...
        tY,...
        tZ] = mexPlanar6dofTrack(...
        images{1},...
        templateQuad([1,4,2,3],:) - 1,...
        focalLength_x,...
        focalLength_y,...
        camCenter_x,...
        camCenter_y,...
        templateWidth_mm,...
        scaleTemplateRegionPercent,...
        numPyramidLevels,...
        maxSamplesAtBaseLevel,...
        numSamplingRegions,...
        numFiducialSquareSamples,...
        fiducialSquareWidthFraction,...
        normalizeImage);
    
    figure(1);
    hold off;
    imshow(images{1});
    hold on;
    plot(templateQuad([1:end,1],1), templateQuad([1:end,1],2))
    scatter(samples(:,1) + templateQuadCenter(1) + 1, samples(:,2) + templateQuadCenter(2) + 1, 'g.');
    
    for iImage = 2:length(images)
        
        [verify_converged,...
            verify_meanAbsoluteDifference,...
            verify_numInBounds,...
            verify_numSimilarPixels,...
            currentQuad,...
            angleX,...
            angleY,...
            angleZ,...
            tX,...
            tY,...
            tZ,...
            tform] = mexPlanar6dofTrack(...
            images{iImage},...
            maxIterations,...
            convergenceTolerance_angle,...
            convergenceTolerance_distance,...
            verify_maxPixelDifference,...
            normalizeImage);
        
        figure(2);
        hold off;
        imshow(images{iImage});
        hold on;
        plot(currentQuad([1,3,4,2,1],1) + 1, currentQuad([1,3,4,2,1],2) + 1);
        
        pause(0.25);
    end % for iImage = 2:length(images)
    
    
    keyboard