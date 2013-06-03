function detections = simpleDetector(img, varargin)

usePerimeterCheck = false;
thresholdFraction = 0.75; % fraction of local mean to use as threshld
minQuadArea = 100; % about 10 pixels per side
computeTransformFromBoundary = true;
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
DEBUG_DISPLAY = nargout==0;

parseVarargin(varargin{:});

detections = {};

if ischar(img)
    img = imread(img);
end

img = mean(im2double(img),3);

[nrows,ncols] = size(img);

%% Binary Region Detection

% % Simpler method (one window size for whole image)
% averageImg = separable_filter(img, gaussian_kernel(avgSigma));

% Create a set of "average" images with different-sized Gaussian kernels
numScales = 6;
G = cell(1,numScales); 
G{1} = img;
% TODO: make the filtering progressive (filter previous level an add'l sigma)
for i = 1:(numScales-1)
    G{i+1} = separable_filter(img, gaussian_kernel(2^(i-1))); 
end

% Find characteristic scale using DoG stack (approx. Laplacian)
G = cat(3, G{:});
L = G(:,:,2:end) - G(:,:,1:end-1);
[~,whichScale] = max(abs(L),[],3);
[xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
index = ygrid + (xgrid-1)*nrows + (whichScale)*nrows*ncols;
averageImg = G(index);

% Threshold:
binaryImg = img < thresholdFraction*averageImg;

% % Get rid of spurious detections on the edges of the image
% binaryImg([1:minDotRadius end:(end-minDotRadius+1)],:) = false;
% binaryImg(:,[1:minDotRadius end:(end-minDotRadius+1)]) = false;

if DEBUG_DISPLAY
    namedFigure('InitialFiltering')
    subplot(2,2,1)
    imshow(binaryImg)
    title('Initial Binary Image')
end

t_squareDetect = tic;
statTypes = {'Area', 'PixelIdxList', 'BoundingBox', 'Centroid'};
if usePerimeterCheck
    statTypes{end+1} = 'Perimeter'; %#ok<UNRCH>
end
stats = regionprops(binaryImg, statTypes);

if isempty(stats)
    % Didn't even find any regions in the binary image, nothing to
    % do!
    return;
end

minSideLength = .05*max(nrows,ncols);
maxSideLength = .9*min(nrows,ncols);

minArea = minSideLength^2 - (.8*minSideLength)^2;
maxArea = maxSideLength^2 - (.8*maxSideLength)^2;

area = [stats.Area];
tooBigOrSmall = area < minArea | area > maxArea;
binaryImg(vertcat(stats(tooBigOrSmall).PixelIdxList)) = false;
stats(tooBigOrSmall) = [];

if DEBUG_DISPLAY
    subplot(2,2,2)
    imshow(binaryImg)
    title('Area check')
end

bb = vertcat(stats.BoundingBox);
bbArea = prod(bb(:,3:4),2);
tooSolid = [stats.Area]'./ bbArea > .5;
binaryImg(vertcat(stats(tooSolid).PixelIdxList)) = false;
stats(tooSolid) = [];

if DEBUG_DISPLAY
    subplot(2,2,3)
    imshow(binaryImg)
    title('tooSolid Check')
end

if usePerimeterCheck
    % perimter vs. area
    tooWonky = [stats.Perimeter] ./ [stats.Area] > .5; %#ok<UNRCH>
    binaryImg(vertcat(stats(tooWonky).PixelIdxList)) = false;
    stats(tooWonky) = [];
    
    if DEBUG_DISPLAY
        subplot 224
        imshow(binaryImg)
        title('Perimeter vs. Area')
        linkaxes
    end
end

if DEBUG_DISPLAY
    % Show what's left
    namedFigure('SimpleDetector');
    
    h_initialAxes = subplot(221);
    hold off, imshow(img)
    overlay_image(binaryImg, 'r', 0, .3);
    hold on
    
    %centroids = vertcat(stats.WeightedCentroid);
    %plot(centroids(:,1), centroids(:,2), 'b*');
    title(sprintf('%d Initial Detections', length(stats)))
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

regionImg = zeros(nrows,ncols);
for i_region = 1:length(stats)
    
    regionImg(stats(i_region).PixelIdxList) = i_region;
    
    % % External boundary
    % [rowStart,colStart] = ind2sub([nrows ncols], ...
    %    stats(i_region).PixelIdxList(1));
    
    % Internal boundary
    rowStart = round(stats(i_region).Centroid(2));
    colStart = round(stats(i_region).Centroid(1));
    if regionImg(rowStart,colStart) == i_region
        continue;
    end
    while colStart > 1 && regionImg(rowStart,colStart) ~= i_region
        colStart = colStart - 1;
    end
    
    if colStart == 1 && regionImg(rowStart,colStart) ~= i_region
        continue
    end
    
    try
        boundary = bwtraceboundary(regionImg == i_region, ...
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
            sigma = size(boundary,1)/64;
            dg2 = conv([1 0 0 0 -2 0 0 0 1], gaussian_kernel(sigma));
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
                            canonicalBoundary = ...
                                [zeros(Nside,1) linspace(0,1,Nside)';  % left side
                                linspace(0,1,Nside)' ones(Nside,1);  % top
                                ones(Nside,1) linspace(1,0,Nside)'; % right
                                linspace(1,0,Nside)' zeros(Nside,1)]; % bottom
                            tform = ICP(fliplr(boundary), canonicalBoundary, ...
                                'projective', 'tformInit', tformInit, ...
                                'maxIterations', 10, 'tolerance', .01);
                        catch E
                            warning(E.message)
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
                'method', 'warpImage', ... % or 'warpProbes' ??
                'cropFactor', cropFactor);
            
        else
            [blockType, faceType, isValid, keyOrient] = ...
                decodeBlockMarker(img, 'corners', quads{i_quad}, ...
                'cropFactor', cropFactor); %#ok<UNRCH>
        end
        
        if isValid
            detections{end+1} = BlockDetection(blockType, faceType, ...
                quads{i_quad}, keyOrient); %#ok<AGROW>
        end
        
        if DEBUG_DISPLAY
            if isValid
                draw(detections{end}, h_quadAxes(2));
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

