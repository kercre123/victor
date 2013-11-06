function markers = simpleDetector(img, varargin)

downsampleFactor = 2;
usePyramid = true;
usePerimeterCheck = false;
minQuadArea = 100; % about 10 pixels per side
computeTransformFromBoundary = true;
quadRefinementMethod = 'none'; % 'ICP' or 'fminsearch' or 'none'
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
decodeDownsampleFactor = 1; % use lower resolution image for decoding
minDistanceFromImageEdge = 2; % if a quad has an corner that is too close to the edge, reject it
DEBUG_DISPLAY = nargout==0;
embeddedConversions = EmbeddedConversionsManager; % 1-cp2tform, 2-opencv_cp2tform
showTiming = false;
thresholdFraction = 1; % fraction of local mean to use as threshold
maxSmoothingFraction = 0.1; % fraction of max dim

parseVarargin(varargin{:});

% Use different defaults for 'matlab_boxFilters'
if strcmp(embeddedConversions.computeCharacteristicScaleImageType, 'matlab_boxFilters')
    thresholdFraction = 0.75; % fraction of local mean to use as threshold
    maxSmoothingFraction = 0.025; % fraction of max dim
    parseVarargin(varargin{:});
end

if ischar(img)
    img = imread(img);
end

img = mean(im2double(img),3);

[nrows,ncols] = size(img);

if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345') || strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345_mediumMemory')
    assert(BlockMarker2D.UseOutsideOfSquare, ...
                    ['You need to set constant property ' ...
                    'BlockMarker2D.UseOutsideOfSquare = false to use this method.']);

    if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345')
        scaleImage_useWhichAlgorithm = 0;
        scaleImage_numPyramidLevels = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));
    elseif strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345_mediumMemory')
        scaleImage_useWhichAlgorithm = 1;
        scaleImage_numPyramidLevels = 4;
    end

    scaleImage_thresholdMultiplier = 0.75;

    component1d_minComponentWidth = 0;
    component1d_maxSkipDistance = 0;
    minSideLength = (0.03*max(size(img,1),size(img,2)));
    maxSideLength = (0.9*min(size(img,1),size(img,2)));
    component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    component_sparseMultiplyThreshold = 1000;
    component_solidMultiplyThreshold = 2;
    component_percentHorizontal = 3/4;
    component_percentVertical = 3/4;
    quads_minQuadArea = minQuadArea;
    quads_quadSymmetryThreshold = 1.5;
    quads_minDistanceFromImageEdge = 2;
    decode_minContrastRatio = 1.25;

    [quads, blockTypes, faceTypes, orientations] = mexSimpleDetectorSteps12345(im2uint8(img), scaleImage_useWhichAlgorithm, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_percentHorizontal, component_percentVertical, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio);

    markers = cell(0,0);
    for i = 1:length(quads)
        homography = mexEstimateHomography(quads{i}, [0 0; 0 1; 1 0; 1 1]);
        markers{i} = BlockMarker2D(zeros(size(img)), quads{i}, maketform('projective', homography), 'ExplictInput', 'ids', [blockTypes(i), faceTypes(i)], 'upAngle', orientations(i));

%         keyboard
    end

else % if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345')
    if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234') || strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234_mediumMemory')
        assert(BlockMarker2D.UseOutsideOfSquare, ...
                    ['You need to set constant property ' ...
                    'BlockMarker2D.UseOutsideOfSquare = false to use this method.']);

        if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234')
            scaleImage_useWhichAlgorithm = 0;
            scaleImage_numPyramidLevels = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));
        elseif strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234_mediumMemory')
            scaleImage_useWhichAlgorithm = 1;
            scaleImage_numPyramidLevels = 4;
        end

        scaleImage_thresholdMultiplier = 0.75;
        component1d_minComponentWidth = 0;
        component1d_maxSkipDistance = 0;
        minSideLength = (0.03*max(size(img,1),size(img,2)));
        maxSideLength = (0.9*min(size(img,1),size(img,2)));
        component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
        component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
        component_sparseMultiplyThreshold = 1000;
        component_solidMultiplyThreshold = 2;
        component_percentHorizontal = 3/4;
        component_percentVertical = 3/4;
        quads_minQuadArea = minQuadArea;
        quads_quadSymmetryThreshold = 1.5;
        quads_minDistanceFromImageEdge = 2;

        [quads, quadTforms] = mexSimpleDetectorSteps1234(im2uint8(img), scaleImage_useWhichAlgorithm, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_percentHorizontal, component_percentVertical, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge);

        for i = 1:length(quadTforms)
            quadTforms{i} = maketform('projective', inv(quadTforms{i}'));
        end

    else % if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234')
        if strcmp(embeddedConversions.completeCImplementationType, 'matlab_original')
            binaryImg = simpleDetector_step1_computeCharacteristicScale(maxSmoothingFraction, nrows, ncols, downsampleFactor, img, thresholdFraction, usePyramid, embeddedConversions, showTiming, DEBUG_DISPLAY);

            if showTiming, t_binaryRegions = tic; end

            [numRegions, area, indexList, bb, centroid, components2d] = simpleDetector_step2_computeRegions(binaryImg, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY);

            if numRegions == 0
                % Didn't even find any regions in the binary image, nothing to
                % do!
                markers = {};
                return;
            end

            [numRegions, indexList, centroid, components2d] = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, components2d, embeddedConversions, DEBUG_DISPLAY);
        elseif strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep123') || strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep123_mediumMemory')
            if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep123')
                scaleImage_useWhichAlgorithm = 0;
                scaleImage_numPyramidLevels = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));
            elseif strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep123_mediumMemory')
                scaleImage_useWhichAlgorithm = 1;
                scaleImage_numPyramidLevels = 4;
            end

            scaleImage_thresholdMultiplier = 0.75;
            component1d_minComponentWidth = 0;
            component1d_maxSkipDistance = 0;
            minSideLength = (0.03*max(size(img,1),size(img,2)));
            maxSideLength = (0.9*min(size(img,1),size(img,2)));
            component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
            component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
            component_sparseMultiplyThreshold = 1000;
            component_solidMultiplyThreshold = 2;

        %     keyboard
    %         tic
            components2d = mexSimpleDetectorSteps123(im2uint8(img), scaleImage_useWhichAlgorithm, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold);
    %         toc

            for i = 1:length(components2d)
                components2d{i} = components2d{i} + 1; % Convert from c to matlab indexing
            end

            numRegions = length(components2d);

            % Add in the matlab intermediate results
            indexList = cell(1, length(components2d));
            centroid = zeros(length(components2d), 2);
            for iComponent = 1:length(components2d)
                area = sum(components2d{iComponent}(:,3) - components2d{iComponent}(:,2) + 1);
                indexList{iComponent} = zeros(area, 1);

                ci = 1;
                for iSegment = 1:size(components2d{iComponent}, 1)
                    y = components2d{iComponent}(iSegment,1);
                    xs = components2d{iComponent}(iSegment,2):components2d{iComponent}(iSegment,3);
                    indexList{iComponent}(ci:(ci+length(xs)-1)) = sub2ind(size(img), repmat(y, [1,length(xs)]), xs);
                    ci = ci + length(xs);
                end

                indexList{iComponent} = sort(indexList{iComponent});

                [ys, xs] = ind2sub(size(img), indexList{iComponent});

                centroid(iComponent, :) = [mean(xs), mean(ys)];
            end
        end

        if showTiming
            fprintf('Binary region detection took %.2f seconds.\n', toc(t_binaryRegions));
        end

        if showTiming, t_squareDetect = tic; end

        [quads, quadTforms] = simpleDetector_step4_computeQuads(nrows, ncols, numRegions, indexList, centroid, minQuadArea, cornerMethod, computeTransformFromBoundary, quadRefinementMethod, components2d, minDistanceFromImageEdge, embeddedConversions, DEBUG_DISPLAY);
    end % if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep1234') ... else


    if showTiming
        fprintf('Square detection took %.3f seconds.\n', toc(t_squareDetect));
    end

    % if length(quadTforms) > 0
    %     disp(sprintf('Found %d quads', length(quadTforms)));
    %     disp(quadTforms{1}.tdata.T')
    % end

    % Optionally do the decoding a lower resolution (which necessitates
    % adjusting the quad positions and their transforms to match that new
    % downsampled resolution)
    if decodeDownsampleFactor > 1
        img_decode = imresize(img, 1/decodeDownsampleFactor, 'bilinear');
        [nrows_ds, ncols_ds] = size(img_decode);
        minQuadArea = minQuadArea / decodeDownsampleFactor^2;

        % Compute the downsampling transform (which is relative to the image
        % center, based on the way imresize downsamples)
        T_center = [1 0 -(ncols+1)/2; 0 1 -(nrows+1)/2; 0 0 1];
        T_ds     = [1/decodeDownsampleFactor 0 0;
                    0 1/decodeDownsampleFactor 0;
                    0 0 1];
        T_center_ds = [1 0 (ncols_ds+1)/2; 0 1 (nrows_ds+1)/2; 0 0 1];

        T = maketform('affine', (T_center_ds * T_ds * T_center)');
        Tinv = fliptform(T);

        quads_decode = cell(size(quads));
        for i = 1:length(quads)
            [x,y] = tformfwd(T, quads{i}(:,1), quads{i}(:,2));
            quads_decode{i} = [x(:) y(:)];

            if ~isempty(quadTforms{i})
                quadTforms{i} = maketform('composite', quadTforms{i}, Tinv);
            end
        end
    else
        img_decode = img;
        quads_decode = quads;
    end

    % Decode to find BlockMarker2D objects
    [markers, validQuads] = simpleDetector_step5_decodeMarkers(img_decode, quads_decode, quadTforms, minQuadArea, embeddedConversions, showTiming, DEBUG_DISPLAY);

    % % Undo the downsampling so that the markers are relative to original
    % % resolution, if necessary.
    % if decodeDownsampleFactor > 1
    %     for i = 1:length(markers)
    %         markers{i} = applytform(markers{i}, Tinv);
    %     end
    % end

    % Put the original corners back, since those were (more accurately)
    % computed at original resolution
    quads = quads(validQuads);
    if decodeDownsampleFactor > 1
        for i = 1:length(markers)
            markers{i} = updateCorners(markers{i}, quads{i});
        end
    end
end % if strcmp(embeddedConversions.completeCImplementationType, 'c_singleStep12345')'

end % FUNCTION simpleDetector()

