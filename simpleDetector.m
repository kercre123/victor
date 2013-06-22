function markers = simpleDetector(img, varargin)

maxSmoothingFraction = 0.1; % fraction of max dim
downsampleFactor = 2;
usePerimeterCheck = false;
thresholdFraction = 1; % fraction of local mean to use as threshld
minQuadArea = 100; % about 10 pixels per side
computeTransformFromBoundary = true;
quadRefinementMethod = 'ICP'; % 'ICP' or 'fminsearch'
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
DEBUG_DISPLAY = nargout==0;

parseVarargin(varargin{:});

markers = {};

if ischar(img)
    img = imread(img);
end

img = mean(im2double(img),3);

[nrows,ncols] = size(img);

%% Binary Region Detection

% % Simpler method (one window size for whole image)
% averageImg = separable_filter(img, gaussian_kernel(avgSigma));

% Create a set of "average" images with different-sized Gaussian kernels
numScales = round(log(maxSmoothingFraction*max(nrows,ncols)) / log(downsampleFactor));
G = cell(1,numScales+1); 
numSigma = 2.5;
prevSigma = 0.5;
%G{1} = separable_filter(img, gaussian_kernel(prevSigma, numSigma));
%G{1} = imfilter(img, fspecial('gaussian', round(numSigma*prevSigma), prevSigma));
G{1} = mexGaussianBlur(img, prevSigma, numSigma);
for i = 1:numScales
    crntSigma = downsampleFactor^(i-1);
    addlSigma = sqrt(crntSigma^2 - prevSigma^2);
    %G{i+1} = separable_filter(G{i}, gaussian_kernel(addlSigma, numSigma)); 
    %G{i+1} = imfilter(G{i}, fspecial('gaussian', round(numSigma*addlSigma), addlSigma));
    G{i+1} = mexGaussianBlur(G{i}, addlSigma, numSigma);
    prevSigma = crntSigma;
end

% Find characteristic scale using DoG stack (approx. Laplacian)
G = cat(3, G{:});
L = G(:,:,2:end) - G(:,:,1:end-1);
[~,whichScale] = max(abs(L),[],3);
[xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
index = ygrid + (xgrid-1)*nrows + (whichScale)*nrows*ncols; % not whichScale-1 on purpose!
averageImg = G(index);

% Threshold:
binaryImg = img < thresholdFraction*averageImg;

% % Get rid of spurious detections on the edges of the image
% binaryImg([1:minDotRadius end:(end-miTnDotRadius+1)],:) = false;
% binaryImg(:,[1:minDotRadius end:(end-minDotRadius+1)]) = false;

t_binaryRegions = tic;

if usePerimeterCheck
    % Perimeter not supported by mexRegionprops yet.
    statTypes = {'Area', 'PixelIdxList', 'BoundingBox', ...
        'Centroid', 'Perimeter'}; %#ok<UNRCH>
    stats = regionprops(binaryImg, statTypes);
    
    numRegions = length(stats);
    
    indexList = {stats.PixelIdxList};
    area = [stats.area];
    bb = vercat(stats.BoundinBox);
    centroid = vertcat(stats.Centroid);
else
    [regionMap, numRegions] = bwlabel(binaryImg);
    
    [area, indexList, bb, centroid] = mexRegionProps( ...
        uint32(regionMap), numRegions);
    area = double(area);
end

if numRegions == 0
    % Didn't even find any regions in the binary image, nothing to
    % do!
    return;
end


if DEBUG_DISPLAY
    namedFigure('InitialFiltering')
    subplot(2,2,1)
    imshow(binaryImg)
    title(sprintf('Initial Binary Image: %d regions', numRegions))
end

minSideLength = .03*max(nrows,ncols);
maxSideLength = .9*min(nrows,ncols);

minArea = minSideLength^2 - (.8*minSideLength)^2;
maxArea = maxSideLength^2 - (.8*maxSideLength)^2;

tooBigOrSmall = area < minArea | area > maxArea;
updateStats(tooBigOrSmall);

    function updateStats(toRemove)
        binaryImg(vertcat(indexList{toRemove})) = false;
        
        area(toRemove) = [];
        indexList(toRemove) = [];
        bb(toRemove,:) = [];
        centroid(toRemove,:) = [];
        numRegions = numRegions - sum(toRemove);
    end

if DEBUG_DISPLAY
    subplot(2,2,2)
    imshow(binaryImg)
    title(sprintf('After Area check: %d regions', numRegions))
end

bbArea = prod(bb(:,3:4),2);
tooSolid = area./ bbArea > .5;
updateStats(tooSolid);

if DEBUG_DISPLAY
    subplot(2,2,3)
    imshow(binaryImg)
    title(sprintf('After tooSolid Check: %d regions', numRegions))
end

if usePerimeterCheck
    % perimter vs. area
    tooWonky = [stats.Perimeter] ./ [stats.Area] > .5; %#ok<UNRCH>
    binaryImg(vertcat(stats(tooWonky).PixelIdxList)) = false;
    stats(tooWonky) = [];
    
    if DEBUG_DISPLAY
        subplot 224
        imshow(binaryImg)
        title(sprintf('After Perimeter vs. Area: %d regions', length(stats)))
        linkaxes
    end
end

fprintf('Binary region detection took %.2f seconds.\n', toc(t_binaryRegions));

t_squareDetect = tic;

if DEBUG_DISPLAY
    % Show what's left
    namedFigure('SimpleDetector');
    
    h_initialAxes = subplot(221);
    hold off, imshow(img)
    overlay_image(binaryImg, 'r', 0, .3);
    hold on
    
    %centroids = vertcat(stats.WeightedCentroid);
    %plot(centroids(:,1), centroids(:,2), 'b*');
    title(sprintf('%d Initial Detections', numRegions))
end

if strcmp(cornerMethod, 'harrisScore')
    Ix = image_right(img) - img; Iy = image_down(img) - img;
    g = gaussian_kernel(.75);
    Ix2 = separable_filter(Ix.^2, g);
    Iy2 = separable_filter(Iy.^2, g);
    IxIy = separable_filter(Ix.*Iy, g);
    cornerScore = Ix2.*Iy2 - IxIy.^2;
end

quads = {};
quadTforms = {};

regionMap = zeros(nrows,ncols);
for i_region = 1:numRegions
    
    regionMap(indexList{i_region}) = i_region;
    
    % Check to see if interior of this region is roughly empty: 
    x = xgrid(indexList{i_region});
    y = ygrid(indexList{i_region});
    xcen = centroid(i_region,1);
    ycen = centroid(i_region,2);
    x = round(0.5*(x-xcen)+xcen);
    y = round(0.5*(y-ycen)+ycen);
    interiorIdx = sub2ind([nrows ncols], y, x);
    if any(regionMap(interiorIdx) == i_region)
        
        if DEBUG_DISPLAY
            namedFigure('InitialFiltering')
            binaryImg(indexList{i_region}) = 0;
            subplot 224
            imshow(binaryImg)
            title('After Interior Check')
            
            namedFigure('SimpleDetector')
            subplot(h_initialAxes)
            overlay_image(binaryImg, 'r', 0, .3);
        end
        
       continue; 
    end
    
    % % External boundary
    % [rowStart,colStart] = ind2sub([nrows ncols], ...
    %    stats(i_region).PixelIdxList(1));
    
    % Internal boundary
    % Find starting pixel by walking from centroid outward until we hit a
    % pixel in this region:
    rowStart = round(centroid(i_region,2));
    colStart = round(centroid(i_region,1));
    if regionMap(rowStart,colStart) == i_region
        continue;
    end
    while colStart > 1 && regionMap(rowStart,colStart) ~= i_region
        colStart = colStart - 1;
    end
    
    if colStart == 1 && regionMap(rowStart,colStart) ~= i_region
        continue
    end
    
    try
        boundary = bwtraceboundary(regionMap == i_region, ...
            [rowStart colStart], 'N');
    catch E
        warning(E.message);
        continue
    end
    
    if isempty(boundary)
        continue
    end
    
    %plot(boundary(:,2), boundary(:,1), 'b-', 'LineWidth', 2, 'Parent', h_initialAxes);
    %keyboard
    
    switch(cornerMethod)
        case 'harrisScore'
            temp = sub2ind([nrows ncols], boundary(:,1), boundary(:,2));
            r = cornerScore(temp);
        case 'radiusPeaks'
            xcen = mean(boundary(:,2));
            ycen = mean(boundary(:,1));
            [~,r] = cart2pol(boundary(:,2)-xcen, boundary(:,1)-ycen);
            % [~, sortIndex] = sort(theta); % already sorted, thanks to bwtraceboundary
        case 'laplacianPeaks'
            % TODO: vary the smoothing/spacing with boundary length?
            boundaryLength = size(boundary,1);
            sigma = boundaryLength/64;
            spacing = max(3, round(boundaryLength/16)); % spacing about 1/4 of side-length
            stencil = [1 zeros(1, spacing-2) -2 zeros(1, spacing-2) 1];
            dg2 = conv(stencil, gaussian_kernel(sigma));
            r_smooth = imfilter(boundary, dg2(:), 'circular');
            r_smooth = sum(r_smooth.^2, 2);
            
        otherwise
            error('Unrecognzed cornerMethod "%s"', cornerMethod)
    end % SWITCH(cornerMethod)
    
    
    if any(strcmp(cornerMethod, {'harrisScore', 'radiusPeaks'}))
        % Smooth the radial distance from the center according to the
        % total perimeter.
        % TODO: Is there a way to set this magic number in a
        % principaled fashion?
        sigma = size(boundary,1)/64;
        g = gaussian_kernel(sigma); g= g(:);
        r_smooth = imfilter(r, g, 'circular');
    end
    
    % Find local maxima -- these should correspond to the corners
    % of the square
    localMaxima = find(r_smooth > r_smooth([end 1:end-1]) & ...
        r_smooth > r_smooth([2:end 1]));
    if length(localMaxima)>=4
        [~,whichMaxima] = sort(r_smooth(localMaxima), 'descend');
        whichMaxima = sort(whichMaxima(1:4), 'ascend');
        
        index = localMaxima(whichMaxima([1 4 2 3]));
        
        % plot(boundary(index,2), boundary(index,1), 'y+')
        
        corners = fliplr(boundary(index,:));
        
        if DEBUG_DISPLAY
            plot(corners(:,1), corners(:,2), 'gx', 'Parent', h_initialAxes);
        end
        
        % Verfiy corners are in a clockwise direction, so we don't get an
        % accidental projective mirroring when we do the tranformation below to
        % extract the image.  Can look whether the z direction of cross product
        % of the two vectors forming the quadrilateral is positive or negative.
        A = [corners(2,:) - corners(1,:);
            corners(3,:) - corners(1,:)];
        detA = det(A);
        if abs(detA) >= minQuadArea
            
            if detA > 0
                corners([2 3],:) = corners([3 2],:);
                %index([2 3]) = index([3 2]);
                detA = -detA;
            end
            
            % One last check: make sure we've got roughly a symmetric
            % quadrilateral (a parallelogram?) by seeing if the area
            % computed using the cross product (via determinates)
            % referenced to opposite corners are similar (and the signs
            % are in agreement, so that two of the sides don't cross
            % each other in the middle).
            B = [corners(3,:) - corners(4,:);
                corners(2,:) - corners(4,:)];
            detB = det(B);
            if sign(detA) == sign(detB)
                detA = abs(detA);
                detB = abs(detB);
                if max(detA,detB) / min(detA,detB) < 1.5
                    
                    tform = []; %#ok<NASGU>
                    if computeTransformFromBoundary
                        % Define each side independently:
                        N = size(boundary,1);
                        
                        Nside = ceil(N/4);
                        
                        try
                            tformInit = cp2tform(corners, [0 0; 0 1; 1 0; 1 1], 'projective');
                        catch E
                            warning(['While computing tformInit: ' E.message]);
                            tformInit = [];
                        end
                        
                        if ~isempty(tformInit)
                            try
                                
                                canonicalBoundary = ...
                                    [zeros(Nside,1) linspace(0,1,Nside)';  % left side
                                    linspace(0,1,Nside)' ones(Nside,1);  % top
                                    ones(Nside,1) linspace(1,0,Nside)'; % right
                                    linspace(1,0,Nside)' zeros(Nside,1)]; % bottom
                                switch(quadRefinementMethod)
                                    case 'ICP'
                                        tform = ICP(fliplr(boundary), canonicalBoundary, ...
                                            'projective', 'tformInit', tformInit, ...
                                            'maxIterations', 10, 'tolerance', .001, ...
                                            'sampleFraction', 1);
                                        
                                    case 'fminsearch'
                                        mag = smoothgradient(img, 1);
                                        %namedFigure('refineHelper')
                                        %hold off, imagesc(mag), axis image off, hold on
                                        %colormap(gray)
                                        options = optimset('TolFun', .1, 'MaxIter', 50);
                                        tform = fminsearch(@(x)refineHelper(x,mag,canonicalBoundary), tformInit.tdata.T, options);
                                        tform = maketform('projective', tform);
                                        
                                    otherwise
                                        error('Unrecognized quadRefinementMethod "%s"', quadRefinementMethod);
                                end
                                
                            catch E
                                warning(['While refining tform: ' E.message])
                                tform = tformInit;
                            end
                        else
                            tform = [];
                        end
                                                
                        
                        if ~isempty(tform)
                            % The original corners must have been
                            % visible in the image, and we'd expect
                            % them to all still be within the image
                            % after the transformation adjustment.
                            % Also, the area should still be large
                            % enough
                            % TODO: repeat all the other sanity
                            % checks on the quadrilateral here?
                            [x,y] = tforminv(tform, [0 0 1 1]', [0 1 0 1]');
                            area = abs((x(2)-x(1))*(y(3)-y(1)) - (x(3)-x(1))*(y(2)-y(1)));
                            if all(x >= 1 & x <= ncols & y >= 1 & y<= nrows) && ...
                                    area >= minQuadArea
                                
                                corners = [x y];
                                
                                if DEBUG_DISPLAY
                                    plot(corners(:,1), corners(:,2), 'y+', 'Parent', h_initialAxes);
                                end
                            else
                                tform = [];
                            end
                        end
                        
                        
                        %[x,y] = tforminv(tform, ...
                        %    canonicalBoundary(:,1), canonicalBoundary(:,2));
                        %
                        %plot(x, y, 'LineWidth', 2, ...
                        %    'Color', 'm', 'LineWidth', 2);
                        
                        
                    end % IF computeTransformFromBoundary
                    
                    quads{end+1} = corners; %#ok<AGROW>
                    quadTforms{end+1} = tform; %#ok<AGROW>
                    
                end % IF areas of parallelgrams are similar
                
            end % IF signs of determinants match
            
        end % IF quadrilateral has enough area
        
    end % IF we have at least 4 local maxima
    
end % FOR each region

cropFactor = .7;
fprintf('Square detection took %.3f seconds.\n', toc(t_squareDetect));


% namedFigure('InitialFiltering')
% subplot(2,2,4)
% imshow(label2rgb(regionImg, 'jet', 'k', 'shuffle'))


if ~isempty(quads)
    t_quads = tic;
    
    numQuads = length(quads);
    
    if DEBUG_DISPLAY
        namedFigure('SimpleDetector')
        h_quadAxes(1) = subplot(223);
        hold off, imshow(img), hold on
        title(sprintf('%d Quadrilaterals Returned', length(quads)))
        
        if numQuads==1
            quadColors = [1 0 0];
        else
            quadColors = im2double(squeeze(label2rgb((1:numQuads)', ...
                jet(numQuads), 'k', 'shuffle')));
        end
        
        h_quadAxes(2) = subplot(224);
        hold off, imshow(img), hold on
    end
    
    numMarkers = 0;
    numInvalidMarkers = 0;
    
    for i_quad = 1:numQuads
        if DEBUG_DISPLAY
            plot(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
                'Color', quadColors(i_quad,:), 'LineWidth', 1, ...
                'Parent', h_quadAxes(1));
        end
        
        if computeTransformFromBoundary

            % Sanity check:
            corners = quads{i_quad};
            A = [corners(2,:) - corners(1,:);
                corners(3,:) - corners(1,:)];
            assert(abs(det(A)) >= minQuadArea, 'Area of quad too small.');
            

            [blockType, faceType, isValid, keyOrient] = ...
                decodeBlockMarker(img, 'tform', quadTforms{i_quad}, ...
                'corners', quads{i_quad}, ... 
                'method', 'warpProbes', ... 'warpImage' or 'warpProbes' ??
                'cropFactor', cropFactor);
            
        else
            [blockType, faceType, isValid, keyOrient] = ...
                decodeBlockMarker(img, 'corners', quads{i_quad}, ...
                'cropFactor', cropFactor); %#ok<UNRCH>
        end
        
        if isValid
            markers{end+1} = BlockMarker2D(blockType, faceType, ...
                quads{i_quad}, keyOrient); %#ok<AGROW>
        end
        
        if DEBUG_DISPLAY
            if isValid
                draw(markers{end}, 'where', h_quadAxes(2));
                numMarkers = numMarkers + 1;
            else
                numInvalidMarkers = numInvalidMarkers + 1;
            end
            
            
            %{
            drawThisMarker = false;
            if isValid
                edgeColor = 'g';
                numValidMarkers = numValidMarkers + 1;
                drawThisMarker = true;
            elseif drawInvalidMarkers
                edgeColor = 'r';
                drawThisMarker = true;
            end
            
            if drawThisMarker
                patch(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
                    'g', 'EdgeColor', edgeColor, 'LineWidth', 2, ...
                    'FaceColor', 'r', 'FaceAlpha', .3, ...
                    'Parent', h_quadAxes(2));
                if ~strcmp(keyOrient, 'none')
                    topSideLUT = struct('down', [2 4], 'up', [1 3], ...
                        'left', [1 2], 'right', [3 4]);
                    plot(quads{i_quad}(topSideLUT.(keyOrient),1), ...
                        quads{i_quad}(topSideLUT.(keyOrient),2), ...
                        'Color', 'b', 'LineWidth', 3, 'Parent', h_quadAxes(2));
                end
                text(mean(quads{i_quad}([1 4],1)), mean(quads{i_quad}([1 4],2)), ...
                    sprintf('Block %d, Face %d', blockType, faceType), ...
                    'Color', 'b', 'FontSize', 14, 'FontWeight', 'b', ...
                    'BackgroundColor', 'w', ...
                    'Hor', 'center', 'Parent', h_quadAxes(2));
                
                numMarkers = numMarkers + 1;
            end
            %}
        end % IF DEBUG_DISPLAY
        
    end % FOR each quad
    
    if DEBUG_DISPLAY
        title(h_quadAxes(2), sprintf('%d Markers Detected', numMarkers));
    end
    
    fprintf('Quad extraction/decoding took %.2f seconds.\n', toc(t_quads));
    
end % IF any quads found

if DEBUG_DISPLAY
    subplot_expand on
end

end % FUNCTION simpleDetector()

