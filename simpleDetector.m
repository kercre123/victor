function simpleDetector(img, svmModel, varargin)

fiducialType = 'square'; % 'dots' or 'square'
usePerimeterCheck = false;
avgSigma = 30;
thresholdFraction = 0.75; % fraction of local mean to use as threshld
minDotRadius = 1;
maxDotRadiusFraction = .05; % fraction of max image dim
maxViewAngle = 70;
winSize = 13;
paddingFraction = 0.25; % fraction of major axis length to use for padding for SVM classification
codeN = 5;  % number of squares along each dim of 2d barcode
computeTransformFromBoundary = true;
drawInvalidMarkers = false;
cornerMethod = 'laplacianPeaks'; % 'laplacianPeaks', 'harrisScore', or 'radiusPeaks'
DEBUG_DISPLAY = nargout==0;

parseVarargin(varargin{:});

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


switch(fiducialType)
    case 'dots'
        stats = regionprops(binaryImg, 1-img, ...
            {'Area', 'WeightedCentroid', 'PixelIdxList', ...
            'MajorAxisLength', 'MinorAxisLength', 'BoundingBox'});
        
        
        minArea = round(minDotRadius^2 * pi);
        maxArea = round((maxDotRadiusFraction*max(nrows,ncols))^2 * pi);
        
        % Filter out regions that are too big or small
        tooBigOrSmall = [stats.Area] < minArea | [stats.Area] > maxArea;
        binaryImg(vertcat(stats(tooBigOrSmall).PixelIdxList)) = false;
        stats(tooBigOrSmall) = [];
        
        % % Filter out regions that aren't mostly black (a circle inscribed in the
        % % bounding box should have
        % bb = vertcat(stats.BoundingBox);
        % notSolidEnough = [stats.Area]'./prod(bb(:,3:4),2) < .9*pi/4;
        % binaryImg(vertcat(stats(notSolidEnough).PixelIdxList)) = false;
        % stats(notSolidEnough) = [];
        
        % Filter out regions whose centroid isn't dark (regions with concavities)
        centroids = vertcat(stats.WeightedCentroid);
        centerIdx = sub2ind(size(img), round(centroids(:,2)), round(centroids(:,1)));
        notConvex = ~binaryImg(centerIdx);
        binaryImg(vertcat(stats(notConvex).PixelIdxList)) = false;
        stats(notConvex) = [];
        
        % Filter out regions that are too linear (based on the most extreme viewing
        % angle we expect to see a circular dot from).  The maxViewAngle is 0 when
        % viewing a target straight on and increases as you rotate around to either
        % side
        axisRatio = [stats.MinorAxisLength] ./ [stats.MajorAxisLength];
        tooExtreme = axisRatio < cos(maxViewAngle * pi/180);
        binaryImg(vertcat(stats(tooExtreme).PixelIdxList)) = false;
        stats(tooExtreme) = [];
        
        if DEBUG_DISPLAY
            % Show what's left
            namedFigure('SimpleDetector');
            
            subplot 221
            hold off, imshow(img)
            overlay_image(binaryImg, 'r', 0, .3);
            hold on
            
            centroids = vertcat(stats.WeightedCentroid);
            plot(centroids(:,1), centroids(:,2), 'b*');
            title(sprintf('%d Initial Detections', length(stats)))
        end
        
        radii = [stats.MajorAxisLength]' / 2;
        
        
        
        %% SVM Classification
        if ~isempty(svmModel)
            
            % For each remaining region, extract a bounding box around it, scale that
            % to the right size, and classify it via the SVM:
            numRegions = length(stats);
            windows = zeros(numRegions, winSize^2);
            for i_region = 1:numRegions
                bbox = stats(i_region).BoundingBox;
                padding = paddingFraction*stats(i_region).MajorAxisLength;
                bbox = round([bbox(1:2)-padding bbox(3:4)+2*padding]);
                win = imresize(imcrop(img, bbox), [winSize winSize], 'bilinear');
                
                windows(i_region,:) = row(win);
                
            end
            
            classifications = svmpredict(zeros(numRegions,1), windows, svmModel);
            
            binaryImg(vertcat(stats(classifications==-1).PixelIdxList)) = false;
            stats(classifications==-1) = [];
            
            centroids = vertcat(stats.WeightedCentroid);
            radii = [stats.MajorAxisLength]' / 2;
            
            if DEBUG_DISPLAY
                subplot 222
                hold off, imshow(img)
                overlay_image(binaryImg, 'g', 0, .3);
                hold on
                plot(centroids(:,1), centroids(:,2), 'b*');
                for i = 1:length(stats)
                    rectangle('Pos', [centroids(i,:)-radii(i) radii(i)*[2 2]], ...
                        'Curvature', [1 1]);
                end
                title(sprintf('%d Detections After SVM', length(stats)))
            end
            
            % namedFigure('Positive')
            % posIndex = find(classifications==1);
            % numPos = length(posIndex);
            % numPlotRows = ceil(sqrt(numPos));
            % numPlotCols = ceil(numPos/numPlotRows);
            % for i = 1:numPos
            %     subplot(numPlotRows, numPlotCols, i)
            %     imshow(reshape(windows(posIndex(i),:), [winSize winSize]))
            % end
            %
            % namedFigure('Negative')
            % negIndex = find(classifications==-1);
            % numNeg = length(negIndex);
            % numPlotRows = ceil(sqrt(numNeg));
            % numPlotCols = ceil(numNeg/numPlotRows);
            % for i = 1:numNeg
            %     subplot(numPlotRows, numPlotCols, i)
            %     imshow(reshape(windows(negIndex(i),:), [winSize winSize]))
            % end
        end % IF svmModel not empty
        
        %% Geometric/Photometric Verification:
        quads = geometricCheck(img, centroids(:,1), centroids(:,2), radii);
        
        cropFactor = 0.5;
        
    case 'square'
        namedFigure('InitialFiltering')
        subplot(2,2,1)
        imshow(binaryImg)
        title('Initial Binary Image')
        
        t_squareDetect = tic;
        statTypes = {'Area', 'PixelIdxList', 'BoundingBox', 'Centroid'};
        if usePerimeterCheck
            statTypes{end+1} = 'Perimeter';
        end
        stats = regionprops(binaryImg, statTypes);
        
        minSideLength = .05*max(nrows,ncols);
        maxSideLength = .9*min(nrows,ncols);
        
        minArea = minSideLength^2 - (.9*minSideLength)^2;
        maxArea = maxSideLength^2 - (.9*maxSideLength)^2;
        
        area = [stats.Area];
        tooBigOrSmall = area < minArea | area > maxArea;
        binaryImg(vertcat(stats(tooBigOrSmall).PixelIdxList)) = false;
        stats(tooBigOrSmall) = [];
        
        subplot(2,2,2)
        imshow(binaryImg)
        title('Area check')
        
        bb = vertcat(stats.BoundingBox);
        bbArea = prod(bb(:,3:4),2);
        tooSolid = [stats.Area]'./ bbArea > .5;
        binaryImg(vertcat(stats(tooSolid).PixelIdxList)) = false;
        stats(tooSolid) = [];
        
        subplot(2,2,3)
        imshow(binaryImg)
        title('tooSolid Check')
        
%         % re-threshold using mean of region inside bounding box of each
%         % region
%         [~,ordering] = sort([stats.Area], 'ascend');
%         for i_region = row(ordering)
%             bb = round(stats(i_region).BoundingBox);
%             rows = bb(2):(bb(2)+bb(4)-1);
%             cols = bb(1):(bb(1)+bb(3)-1);
%             thresh = (max(column(img(rows,cols))) + min(column(img(rows,cols))))/2;
%             binaryImg(rows,cols) = img(rows,cols) < thresh;
%         end
%         
%         subplot 224
%         imshow(binaryImg)
%         title('Re-thresholded')
        
if usePerimeterCheck
        % perimter vs. area
        tooWonky = [stats.Perimeter] ./ [stats.Area] > .5; 
        binaryImg(vertcat(stats(tooWonky).PixelIdxList)) = false;
        stats(tooWonky) = [];
        
        subplot 224
        imshow(binaryImg)
        title('Perimeter vs. Area')
end

        linkaxes
        
        if DEBUG_DISPLAY
            % Show what's left
            namedFigure('SimpleDetector');
            
            subplot 221
            hold off, imshow(img)
            overlay_image(binaryImg, 'r', 0, .3);
            hold on
            
            %centroids = vertcat(stats.WeightedCentroid);
            %plot(centroids(:,1), centroids(:,2), 'b*');
            title(sprintf('%d Initial Detections', length(stats)))
        end
        
        % A *few* tests indicate that using bwmorph('skel') and sorting 
        % below is slower than using bwtraceboundary.  So I'm leaving the
        % code here for now, but disabling it.
        useSkel = false;
        if useSkel
            binaryImgSkel = bwmorph(binaryImg, 'skel', inf);
            stats = regionprops(binaryImgSkel, 'PixelIdxList');
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
            
%             bb = round(stats(i_region).BoundingBox);
%             rows = bb(2):(bb(2)+bb(4)-1);
%             cols = bb(1):(bb(1)+bb(3)-1);
%             window = img(rows,cols);
%             thresh = (max(window(:)) + min(window(:)))/2;
%             temp = false(nrows,ncols);
%             temp(rows,cols) = window < thresh;
%             regionImg(temp) = i_region;
%             
%             stats(i_region).PixelIdxList = find(temp);
%             [y,x] = ind2sub([nrows,ncols], stats(i_region).PixelIdxList);
%             stats(i_region).Centroid = [mean(x) mean(y)];
            
            
            regionImg(stats(i_region).PixelIdxList) = i_region;
            
            if useSkel
                % If I skeletonized the binaryImage with bwmorph, I can avoid
                % the bwtraceboundary call and just sort by theta to get an
                % ordered list. Is this actually faster?
                [rows,cols] = ind2sub([nrows ncols], ...
                    stats(i_region).PixelIdxList);
                boundary = [rows(:) cols(:)];
                
                xcen = mean(boundary(:,2));
                ycen = mean(boundary(:,1));
                [theta,r] = cart2pol(boundary(:,2)-xcen, boundary(:,1)-ycen);
                [~, sortIndex] = sort(theta);
                r = r(sortIndex);
                boundary = boundary(sortIndex,:);
            else
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
                
                boundary = bwtraceboundary(regionImg == i_region, ...
                    [rowStart colStart], 'N');
                
                if isempty(boundary)
                    continue
                end
                
                %plot(boundary(:,2), boundary(:,1), 'b-', 'LineWidth', 2);
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
                        % TODO: vary the smoothing/spacing with boundary
                        % length?
                        dg2 = conv([1 0 0 0 -2 0 0 0 1], gaussian_kernel(1));
                        r_smooth = imfilter(boundary, dg2(:), 'circular');
                        r_smooth = sum(r_smooth.^2, 2);
                        
                    otherwise
                        error('Unrecognzed cornerMethod "%s"', cornerMethod)
                end % SWITCH(cornerMethod)
            end
            
            if any(strcmp(cornerMethod, {'harrisScore', 'radiusPeaks'}))
                % Smooth the radial distance from the center according to the
                % total perimeter.
                % TODO: Is there a way to set this magic number in a
                % principaled fashion?
                sigma = size(boundary,1)/128;
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

               
                
%                 for i_maxima = 1:4
%                     localWindow = (-1:1) + index(i_maxima);
%                     pre = localWindow < 1;
%                     localWindow(pre) = length(r) + localWindow(pre);
%                     post = localWindow > length(r);
%                     localWindow(post) = localWindow(post) - length(r);
%                     
%                     [~,winIndex] = max(r(localWindow));
%                     index(i_maxima) = localWindow(winIndex);                    
%                 end
                
                corners = fliplr(boundary(index,:));
                
                plot(corners(:,1), corners(:,2), 'gx')
                
%                 % Extract profile from each corner in the direction of the
%                 % center and look for max change from dark to bright
%                 for i_corner = 1:4
%                     [px,py,P] = improfile(img, ...
%                         [corners(i_corner,1) xcen], [corners(i_corner,2) ycen]);
%                     
%                     L = sqrt(sum( (corners(i_corner,:)-[xcen ycen]).^2));
%                     L = max(3,ceil(L/5));
%                     dP = P(2:L) - P(1:(L-1));
%                     
%                     [~,maxIndex] = max(dP);
%                     corners(i_corner,:) = [px(maxIndex) py(maxIndex)];
%                 end 
                
                % Verfiy corners are in a clockwise direction, so we don't get an
                % accidental projective mirroring when we do the tranformation below to
                % extract the image.  Can look whether the z direction of cross product
                % of the two vectors forming the quadrilateral is positive or negative.
                A = [corners(2,:) - corners(1,:);
                    corners(3,:) - corners(1,:)];
                detA = det(A);
                if detA > 0
                    corners([2 3],:) = corners([3 2],:);
                    index([2 3]) = index([3 2]);
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
                        
                        if computeTransformFromBoundary
                            % Define each side independently:
                            N = size(boundary,1);
                            
                            Nside = ceil(N/4);
                            
                            tformInit = cp2tform(corners, [0 0; 0 1; 1 0; 1 1], 'projective');
                            canonicalBoundary = ...
                                [zeros(Nside,1) linspace(0,1,Nside)';  % left side
                                linspace(0,1,Nside)' ones(Nside,1);  % top
                                ones(Nside,1) linspace(1,0,Nside)'; % right
                                linspace(1,0,Nside)' zeros(Nside,1)]; % bottom
                            tform = ICP(fliplr(boundary), canonicalBoundary, ...
                                'projective', 'tformInit', tformInit, ...
                                'maxIterations', 10, 'tolerance', .01);
                            
                            %{
                            if index(1) < index(2)
                                leftSideIndex = index(1):index(2);
                            else
                                leftSideIndex = [index(2):N 1:index(1)];
                            end
                            
                            if index(2) < index(4)
                                topSideIndex = index(2):index(4);
                            else
                                topSideIndex = [index(2):N 1:index(4)];
                            end
                            
                            if index(4) < index(3)
                                rightSideIndex = index(4):index(3);
                            else
                                rightSideIndex = [index(4):N 1:index(3)];
                            end
                            
                            if index(3) < index(1)
                                bottomSideIndex = index(3):index(1);
                            else
                                bottomSideIndex = [index(3):N 1:index(1)];
                            end
                            
                            %{
                        namedFigure('Sides')
                        subplot 121
                        hold off, imshow(regionImg == i_region), hold on
                        plot(boundary(leftSideIndex,2),   boundary(leftSideIndex,1), 'r');
                        plot(boundary(leftSideIndex(1),2),   boundary(leftSideIndex(1),1), 'ro', 'MarkerSize', 10);
                        plot(boundary(leftSideIndex(end),2),   boundary(leftSideIndex(end),1), 'rx', 'MarkerSize', 10);
                        
                        
                        plot(boundary(bottomSideIndex,2), boundary(bottomSideIndex,1), 'g');
                        plot(boundary(bottomSideIndex(1),2),   boundary(bottomSideIndex(1),1), 'go', 'MarkerSize', 10);
                        plot(boundary(bottomSideIndex(end),2),   boundary(bottomSideIndex(end),1), 'gx', 'MarkerSize', 10);
                        
                        plot(boundary(rightSideIndex,2),  boundary(rightSideIndex,1), 'b');
                        plot(boundary(rightSideIndex(1),2),  boundary(rightSideIndex(1),1), 'bo', 'MarkerSize', 10);
                        plot(boundary(rightSideIndex(end),2),  boundary(rightSideIndex(end),1), 'bx', 'MarkerSize', 10);
                        
                        plot(boundary(topSideIndex,2),    boundary(topSideIndex,1), 'm');
                        plot(boundary(topSideIndex(1),2),    boundary(topSideIndex(1),1), 'mo', 'MarkerSize', 10);
                        plot(boundary(topSideIndex(end),2),    boundary(topSideIndex(end),1), 'mx', 'MarkerSize', 10);
                            %}
                            
                            % Define corresponding canonical square:
                            Nleft = length(leftSideIndex);
                            Nright = length(rightSideIndex);
                            Ntop = length(topSideIndex);
                            Nbottom = length(bottomSideIndex);
                            
                            canonicalBoundary = ...
                                [zeros(Nleft,1) linspace(0,1,Nleft)';  % left side
                                linspace(0,1,Ntop)' ones(Ntop,1);  % top
                                ones(Nright,1) linspace(1,0,Nright)'; % right
                                linspace(1,0,Nbottom)' zeros(Nbottom,1)]; % bottom
                            
                            %Ix = [ones(Nleft,1); zeros(Ntop,1); -ones(Nright,1); zeros(Nbottom,1)];
                            %Iy = [zeros(Nleft,1); -ones(Ntop,1); zeros(Nright,1); ones(Nbottom,1)];
                            
                            % Compute transform
                            tform = cp2tform(canonicalBoundary, ...
                                fliplr(boundary([leftSideIndex topSideIndex rightSideIndex bottomSideIndex],:)), ...
                                'projective');
                            %}
                            quadTforms{end+1} = tform;
                            
                            [x,y] = tforminv(tform, [0 0 1 1]', [0 1 0 1]');
                            if all(x >= 1 & x <= ncols & y >= 1 & y<= nrows)
                                corners = [x y];
                                
                                plot(corners(:,1), corners(:,2), 'y+');
                            end
                            
                            
                            %[x,y] = tforminv(tform, ...
                            %    canonicalBoundary(:,1), canonicalBoundary(:,2));
                            %
                            %plot(x, y, 'LineWidth', 2, ...
                            %    'Color', 'm', 'LineWidth', 2);
                           
                            
                            
                        end
                        
                        quads{end+1} = corners;
                        
                    end
                end
                
                
                
%                     B(localMaxima(whichMaxima([1 3 2 4 1])),1)
%                 plot(B(localMaxima(whichMaxima([1 3 2 4 1])),2), ...
%                     B(localMaxima(whichMaxima([1 3 2 4 1])),1), ...
%                     'g.-', 'MarkerSize', 15, 'LineWidth', 2);
            end
        end
        
        cropFactor = .7;
        fprintf('Square detection took %.3f seconds.\n', toc(t_squareDetect));
        
    otherwise
        error('Unrecognized fiducialType "%s"', fiducialType)
        
end % SWITCH(fiducialType)

% namedFigure('InitialFiltering')
% subplot(2,2,4)
% imshow(label2rgb(regionImg, 'jet', 'k', 'shuffle'))


if ~isempty(quads)
    t_quads = tic;
    
    namedFigure('SimpleDetector')
    h_quadAxes(1) = subplot(223);
    hold off, imshow(img), hold on
    title(sprintf('%d Quadrilaterals Returned', length(quads)))
    
    numQuads = length(quads);
    if numQuads==1
        quadColors = [1 0 0];
    else
        quadColors = im2double(squeeze(label2rgb((1:numQuads)', ...
            jet(numQuads), 'k', 'shuffle')));
    end
    
    h_quadAxes(2) = subplot(224);
    hold off, imshow(img), hold on
    
    numMarkers = 0;
    numValidMarkers = 0;
    for i_quad = 1:numQuads
        plot(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
            'Color', quadColors(i_quad,:), 'LineWidth', 1, ...
            'Parent', h_quadAxes(1));
        
        if computeTransformFromBoundary
            
            [blockType, faceType, isValid, keyOrient] = ...
                decodeBlockMarker(img, 'tform', quadTforms{i_quad}, ...
                'corners', quads{i_quad}, ... 
                'method', 'warpImage', ... % or 'warpProbes' ??
                'cropFactor', cropFactor);
            
        else
            [blockType, faceType, isValid, keyOrient] = ...
                decodeBlockMarker(img, 'corners', quads{i_quad}, ...
                'cropFactor', cropFactor);
        end
        
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
        
        
    end % FOR each quad
    
    if drawInvalidMarkers
        title(h_quadAxes(2), sprintf('%d Markers Detected (%d valid)',  ...
            numMarkers, numValidMarkers))
    else
        title(h_quadAxes(2), sprintf('%d Markers Detected', numMarkers));
    end
    
    fprintf('Quad extraction/decoding took %.2f seconds.\n', toc(t_quads));
    
end % IF any quads found

subplot_expand on


