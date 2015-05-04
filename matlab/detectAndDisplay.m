function detectAndDisplay(img, h_axes, h_img, varargin)

markerLibrary = [];
useMexDetector = false;

simpleDetectorArgs = parseVarargin(varargin{:});

%persistent h_detections;

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end

if ~useMexDetector
  detections = simpleDetector(img, simpleDetectorArgs{:});
  
else
  img = rgb2gray(img);
  imageSize = size(img);
  useIntegralImageFiltering = true;
  scaleImage_thresholdMultiplier = 1.0;
  scaleImage_numPyramidLevels = 3;
  component1d_minComponentWidth = 0;
  component1d_maxSkipDistance = 0;
  minSideLength = round(0.01*max(imageSize(1),imageSize(2)));
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
  quadRefinementIterations = 5;
  numRefinementSamples = 100;
  quadRefinementMaxCornerChange = 5;
  quadRefinementMinCornerChange = .005;
  returnInvalidMarkers = 0;
  [quads, markerTypes, markerNames, markerValidity] = mexDetectFiducialMarkers(img, useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers);
  
  detections = struct('corners', quads, ...
    'markerType', markerTypes,  ...
    'markerName', markerNames, ...
    'validity', markerValidity);
  
end

%%







%fprintf('Detection processing took %.2f seconds.\n', toc(t));

%t = tic;
numDetections = length(detections);

axis(h_axes, 'on');
set(h_axes, 'LineWidth', 5, 'Box', 'on', 'XTick', [], 'YTick', []);
set(h_img, 'CData', img);

h_detections = findobj(h_axes, 'Tag', 'BlockMarker2D');
if ~isempty(h_detections)
    delete(h_detections)
end

if numDetections > 0
    hold(h_axes, 'on')
    for i = 1:numDetections
      
      if isstruct(detections)
        plot(detections(i).corners([1 2 4 3 1],1), detections(i).corners([1 2 4 3 1],2), 'r', 'Tag', 'BlockMarker2D');
      else
        switch(class(detections{i}))
          case 'VisionMarker'
            if isempty(markerLibrary)
              detections{i}.DrawProbes('Tag', 'BlockMarker2D', 'Parent', h_axes);
            else
              match = markerLibrary.IdentifyMarker(detections{i});
              if isempty(match)
                detections{i}.DrawProbes('Tag', 'BlockMarker2D', 'Parent', h_axes);
              else
                detections{i}.Draw('Tag', 'BlockMarker2D', 'Parent', h_axes);
              end
            end
            %draw(detections{i}, 'where', h_axes, 'drawTextLabels', 'short');
          case 'VisionMarkerTrained'
            detections{i}.Draw('Tag', 'BlockMarker2D', 'Parent', h_axes);
          otherwise
            error('Unrecognized detection type.');
        end % switch((class(detection))
        
      end % if isstruct(detection)
    end
    
    set(h_axes, 'XColor', 'g', 'YColor', 'g');
    hold(h_axes, 'off')
    drawnow
else
    set(h_axes, 'XColor', 'r', 'YColor', 'r');
end

%fprintf('Rest of detectAndDisplay took %.2f seconds\n', toc(t));


end
