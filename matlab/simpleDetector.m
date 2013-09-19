function markers = simpleDetector(img, varargin)

maxSmoothingFraction = 0.1; % fraction of max dim
downsampleFactor = 2;
usePyramid = true;
usePerimeterCheck = false;
thresholdFraction = 1; % fraction of local mean to use as threshld
minQuadArea = 100; % about 10 pixels per side
computeTransformFromBoundary = true;
quadRefinementMethod = 'none'; % 'ICP' or 'fminsearch' or 'none'
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
decodeDownsampleFactor = 1; % use lower resolution image for decoding
DEBUG_DISPLAY = nargout==0;
embeddedConversions = EmbeddedConversionsManager; % 1-cp2tform, 2-opencv_cp2tform
showTiming = false;

parseVarargin(varargin{:});

if ischar(img)
    img = imread(img);
end

img = mean(im2double(img),3);

[nrows,ncols] = size(img);

%% Binary Region Detection

binaryImg = simpleDetector_step1_computeCharacteristicScale(maxSmoothingFraction, nrows, ncols, downsampleFactor, img, thresholdFraction, usePyramid, embeddedConversions, showTiming, DEBUG_DISPLAY);

if showTiming, t_binaryRegions = tic; end

[numRegions, area, indexList, bb, centroid, components2d] = simpleDetector_step2_computeRegions(binaryImg, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY);

if numRegions == 0
    % Didn't even find any regions in the binary image, nothing to
    % do!
    return;
end

[numRegions, indexList, centroid, components2d] = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, components2d, embeddedConversions, DEBUG_DISPLAY);

if showTiming
    fprintf('Binary region detection took %.2f seconds.\n', toc(t_binaryRegions));
end

if showTiming, t_squareDetect = tic; end

[quads, quadTforms] = simpleDetector_step4_computeQuads(nrows, ncols, numRegions, indexList, centroid, minQuadArea, cornerMethod, computeTransformFromBoundary, quadRefinementMethod, components2d, embeddedConversions, DEBUG_DISPLAY);

if showTiming
    fprintf('Square detection took %.3f seconds.\n', toc(t_squareDetect));
end

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

end % FUNCTION simpleDetector()

