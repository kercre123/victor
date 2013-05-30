function [x,y,r] = detectFiducials(posHist, negHist, img, varargin)

numScales = 8;
scaleFactor = 1.2;
blockSize = 13;
scoreThreshold = -4;
posHist2 = [];
negHist2 = [];
scoreThreshold2 = [];
svmModel = [];

parseVarargin(varargin{:});

assert(mod(blockSize,2)==1, 'Expecting odd blockSize');

blockHalfWidth = (blockSize-1)/2;

[xwin,ywin] = meshgrid(-blockHalfWidth:blockHalfWidth);
xwin = row(xwin);
ywin = row(ywin);

[probes,probes2] = computeProbeLocations('blockHalfWidth', blockHalfWidth, ...
    'numEdgeCenterProbes', 16);
numProbes = size(probes,1);

[probeY, probeX] = ind2sub([blockSize blockSize], probes);
probeX = probeX - (blockHalfWidth+1);
probeY = probeY - (blockHalfWidth+1);

if ~isempty(posHist2)
    numProbes2 = size(probes2,1);
    
    [probeY2, probeX2] = ind2sub([blockSize blockSize], probes2);
    probeX2 = probeX2 - (blockHalfWidth+1);
    probeY2 = probeY2 - (blockHalfWidth+1);
    scoreLUT2 = log(posHist2) - log(negHist2);
end

img = im2double(img);

img = mean(img,3);

scoreLUT = log(posHist) - log(negHist);

numBins = size(posHist,1);

scoreImg = cell(1, numScales);
x = cell(1, numScales);
y = cell(1, numScales);
r = cell(1, numScales);

imgOrig = img;

for i_scale = 1:numScales
    binnedImg = floor((numBins-1)*img) + 1;
    
    [nrows,ncols] = size(img);
    
    inBounds = false(nrows,ncols);
    inBounds((blockHalfWidth+1):(end-blockHalfWidth), ...
        (blockHalfWidth+1):(end-blockHalfWidth)) = true;
    numInBounds = sum(inBounds(:));
    
    [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
    
    X1 = xgrid(inBounds)*ones(1,numProbes) + probeX(:,ones(numInBounds,1))';
    Y1 = ygrid(inBounds)*ones(1,numProbes) + probeY(:,ones(numInBounds,1))';
    
    X2 = xgrid(inBounds)*ones(1,numProbes) + probeX(:,2*ones(numInBounds,1))';
    Y2 = ygrid(inBounds)*ones(1,numProbes) + probeY(:,2*ones(numInBounds,1))';
    
    probeIndex1 = Y1 + (X1-1)*nrows;
    probeIndex2 = Y2 + (X2-1)*nrows;
    
%     indexImg = reshape(1:(nrows*ncols), [nrows ncols]);
%     indexImg = indexImg((blockHalfWidth+1):(end-blockHalfWidth), ...
%         (blockHalfWidth+1):(end-blockHalfWidth));
%     probeIndex1 = indexImg(:) * ones(1,size(probes,1)) + ...
%         probeOffsets(ones(numel(indexImg),1),:);
%     probeIndex2 = indexImg(:) * ones(1,size(probes,1)) + ...
%         probeOffsets(2*ones(numel(indexImg),1),:);

    LUTindex = binnedImg(probeIndex1) + (binnedImg(probeIndex2)-1)*size(scoreLUT,1);
    
    scoreImg{i_scale} = (scoreThreshold-1)*ones(nrows,ncols);
    scoreImg{i_scale}((blockHalfWidth+1):(end-blockHalfWidth), ...
        (blockHalfWidth+1):(end-blockHalfWidth)) = reshape( ...
        min(scoreLUT(LUTindex), [], 2), [nrows ncols]-2*blockHalfWidth);
    
    if isempty(posHist2)
        [regions, numDetections] = bwlabel(scoreImg{i_scale} > scoreThreshold);
    else
        
        detections = find(scoreImg{i_scale} > scoreThreshold);
        numDetections = length(detections);
        
        X1 = xgrid(detections)*ones(1,numProbes2) + probeX2(:,ones(numDetections,1))';
        Y1 = ygrid(detections)*ones(1,numProbes2) + probeY2(:,ones(numDetections,1))';
        
        X2 = xgrid(detections)*ones(1,numProbes2) + probeX2(:,2*ones(numDetections,1))';
        Y2 = ygrid(detections)*ones(1,numProbes2) + probeY2(:,2*ones(numDetections,1))';
        
        probeIndex1 = Y1 + (X1-1)*nrows;
        probeIndex2 = Y2 + (X2-1)*nrows;
        
        LUTindex = binnedImg(probeIndex1) + (binnedImg(probeIndex2)-1)*size(scoreLUT2,1);
       
        scoreImg{i_scale}(detections) = min(scoreLUT2(LUTindex), [], 2);
        
        [regions, numDetections] = bwlabel(scoreImg{i_scale} > scoreThreshold2);
    end
    
    regionIndex = idMap2Index(regions);
    
    %detections{i_scale} = scoreThreshold*ones(nrows,ncols);
    detections = zeros(numDetections,1);
    for i_region = 1:numDetections
        [~,whichPixel] = max(scoreImg{i_scale}(regionIndex{i_region}));
        detections(i_region) = regionIndex{i_region}(whichPixel);
    end
    
    [y{i_scale}, x{i_scale}] = ind2sub([nrows ncols], detections);
    
    if ~isempty(svmModel)
        % Extract the appropriate sized patch around each detection and pass it
        % through the SVM to get final detections
        
        [index, X, Y] = getWindowIndex(x{i_scale}, y{i_scale}, xwin, ywin, nrows);
        windows = 1-img(index); % note the inversion to get *dark* centroid
        
        % find the grayscale centroid of each patch and re-center there:
        totalW = sum(windows,2);
        x{i_scale} = sum(windows.*X,2)./totalW;
        y{i_scale} = sum(windows.*Y,2)./totalW;
        
        index = getWindowIndex(round(x{i_scale}), round(y{i_scale}), ...
            xwin, ywin, nrows);
        windows = img(index);
        
        classifications = svmpredict(zeros(numDetections,1), windows, svmModel, '-q');
        
        detections = classifications > 0;
        numDetections = sum(detections);
        x{i_scale} = x{i_scale}(detections);
        y{i_scale} = y{i_scale}(detections);
        
    end
    
    scale = scaleFactor^(i_scale-1);
    dotRadius = scale*(blockHalfWidth-1);
    
    % Geometric verification
    %DT = delaunayTriangulation();
    quads = geometricCheck(img, x{i_scale}, y{i_scale}, dotRadius, 2*dotRadius*[1.5 4]);
    
    if ~isempty(quads)
        % TODO: resolution of this extraction should not be fixed at 32
        [xgrid,ygrid] = meshgrid(linspace(0,1,64));
        
        namedFigure(sprintf('Quad Extraction %d', i_scale))
        subplot(1, 3, 1:2)
        hold off, imshow(img), hold on
        
        for i_quad = 1:length(quads)
            subplot(1, 3, 1:2)
            plot(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
                'Color', .5 + .5*rand(1,3));
            
            tform = cp2tform(quads{i_quad}, [.5 0; 0 .5; 1 .5; .5 1], 'projective');
            [xi,yi] = tforminv(tform, xgrid, ygrid);
            
            imgPatch = interp2(img, xi, yi);
            
            %subplot(1, 3, 3)
            %hold off, imshow(imgPatch)
            
        end
    end % IF any quads found
    
    % Store detections in original image coordinates/scale:
    x{i_scale} = (x{i_scale}-1)*scale + 1;
    y{i_scale} = (y{i_scale}-1)*scale + 1;
    r{i_scale} = dotRadius * ones(numDetections,1);
    
    img = max(0, min(1, imresize(img, 1/scaleFactor, 'lanczos3')));
    
end

x = vertcat(x{:});
y = vertcat(y{:});
r = vertcat(r{:});

namedFigure('Dot Detections')

hold off, imshow(imgOrig), hold on
for i = 1:length(x)
    rectangle('Pos', [x(i)-r(i) y(i)-r(i) 2*r(i) 2*r(i)], ...
        'Curvature', [1 1], 'EdgeColor', 'r');
end
title(sprintf('%d Total Initial Detections', length(x)));

    
end


function [winIndex, X, Y] = getWindowIndex(xcen, ycen, xwin, ywin, nrows)

winSize = length(xwin);
numPoints = length(xcen);

X = xcen(:,ones(winSize,1)) + xwin(ones(numPoints,1),:);
Y = ycen(:,ones(winSize,1)) + ywin(ones(numPoints,1),:);
winIndex = Y + (X-1)*nrows;

end

