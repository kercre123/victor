function [quads, quadTforms] = simpleDetector_step4_computeQuads(nrows, ncols, numRegions, indexList, centroid, minQuadArea, cornerMethod, computeTransformFromBoundary, quadRefinementMethod, components2d, minDistanceFromImageEdge, embeddedConversions, DEBUG_DISPLAY)

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

[xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);

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
    
    if strcmp(embeddedConversions.emptyCenterDetection, 'matlab_original')
        
        [~,interiorIndex] = bwfill(regionMap==i_region, ...
            centroid(i_region,1), centroid(i_region,2));
        
        interiorAreaRatio = length(interiorIndex) / length(indexList{i_region});
        expectedRatio = VisionMarker.FiducialInnerArea / VisionMarker.FiducialArea;
        if interiorAreaRatio < 0.25 * expectedRatio
            continue;
        end
            %{
        
        % Check to see if interior of this region is roughly empty: 
        x = xgrid(indexList{i_region});
        y = ygrid(indexList{i_region});
                
        
        
        %xcen = centroid(i_region,1);
        %ycen = centroid(i_region,2);
        xcen = (min(x(:))+max(x(:)))/2;
        ycen = (min(y(:))+max(y(:)))/2;
        x = round(0.25*(x-xcen)+xcen);
        y = round(0.25*(y-ycen)+ycen);

        %{
        % Draw a line from extreme to extreme and let the centroid be the
        % intersection of that line. This gives the _projected_ centroid of
        % the square, not the image-space centroid
        x1 = min(x(:));
        x2 = max(x(:));
        assert(x1 ~= x2, 'Min and max x should not be equal.');
        y1 = min(y(x==x1));
        y2 = max(y(x==x2));
        m1 = (y1 - y2) / (x1 - x2);
        b1 = y1-m1*x1;
                
        [y1,index1] = min(y(:));
        [y2,index2] = max(y(:));
        if x(index1) == x(index2)
            xcen = x(index1);
            ycen = m1*xcen + b1;
        else
            m2 = (y1 - y2) / (x(index1) - x(index2));
            b2 = y1 - m2*x(index1);
            
            xcen = (b2-b1)/(m1-m2);
            ycen = m1*xcen + b1;
        end
        
        x = x(:)-xcen;
        y = y(:)-ycen;
        x = round([0.5*x; 0.25*x]+xcen);
        y = round([0.5*y; 0.25*y]+ycen);
        %}
        
        x = max(x, 1);
        x = min(x, ncols);
        y = max(y, 1);
        y = min(y, nrows);
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
        %}
    end
    
    % % External boundary
    % [rowStart,colStart] = ind2sub([nrows ncols], ...
    %    stats(i_region).PixelIdxList(1));
    
    if strcmp(embeddedConversions.traceBoundaryType, 'matlab_original') || strcmp(embeddedConversions.traceBoundaryType, 'matlab_loops')
        if BlockMarker2D.UseOutsideOfSquare
            % External boundary
            [rowStart, colStart] = find(regionMap == i_region, 1);
        else
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
        end
    end
    
%     figure(2); imshow(regionMap)
    if strcmp(embeddedConversions.traceBoundaryType, 'matlab_original')
%         assert(~BlockMarker2D.UseOutsideOfSquare, ...
%                 ['You need to set constant property ' ...
%                 'BlockMarker2D.UseOutsideOfSquare = false to use this method.']);
            
        try
            boundary = bwtraceboundary(regionMap == i_region, ...
                [rowStart colStart], 'N');
        catch E
            try
                % For a bit of robustness, try starting the trace walking
                % in the opposite direction.  If that still fails, return
                % the warning.
                boundary = bwtraceboundary(regionMap == i_region, ...
                    [rowStart colStart], 'S');
                
            catch E
                warning(E.message);
                continue
            end
        end
    elseif strcmp(embeddedConversions.traceBoundaryType, 'matlab_loops')
        binaryImg = uint8(regionMap == i_region);
        boundary = traceBoundary(binaryImg, ...
                [rowStart colStart], 'N');
    elseif strcmp(embeddedConversions.traceBoundaryType, 'c_fixedPoint')
        binaryImg = uint8(regionMap == i_region);
        boundary = mexTraceBoundary(binaryImg, [rowStart, colStart], 'n');
    elseif strcmp(embeddedConversions.traceBoundaryType, 'matlab_approximate')
        if ~strcmp(embeddedConversions.connectedComponentsType, 'matlab_approximate')
%             disp('embeddedConversions.connectedComponentsType must be a run-length encoding version, like matlab_approximate');
%             keyboard
           components2d = convertConnectedComponentsMapToRunline(regionMap); 
        end
        
        boundary = approximateTraceBoundary(components2d{i_region});
%         keyboard
    else
        keyboard
    end
        
    
    if isempty(boundary)
        continue
    end
    
    %plot(boundary(:,2), boundary(:,1), 'b-', 'LineWidth', 2, 'Parent', h_initialAxes);
    %keyboard
    
    corners = [];
    
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
        g = gaussian_kernel(sigma);
        dg2 = conv(stencil, g);
        r_smooth = imfilter(boundary, dg2(:), 'circular');
        r_smooth = sum(r_smooth.^2, 2);
        
      case 'lineFits'
        useMatlab2015A = false;
        
        boundaryLength = size(boundary,1);
        
        sigma = boundaryLength/64;
        [~,dg] = gaussian_kernel(sigma);
        dB = imfilter(boundary, dg(:), 'circular');
        
        % TODO: verify that these are the same
        if useMatlab2015A
            [counts,~,bin] = histcounts(atan2(dB(:,2), dB(:,1)), linspace(-pi, pi, 16));
        else
            edges = linspace(-pi, pi, 16);
            edges(1) = -Inf;
            edges(end) = Inf;
            
            [counts,bin] = histc(atan2(dB(:,2), dB(:,1)), edges);
        end
        
        [~,maxBins] = sort(counts,'descend');
        p = cell(1,4);
        lineFits = struct('slope', cell(1,4), ...
          'intercept', cell(1,4), ...
          'switched', cell(1,4));
        didFitFourLines = true;
        
        if DEBUG_DISPLAY
          hold off
          imagesc(regionMap == i_region), axis image, hold on
        end
        for iBin = 1:4
          boundaryIndex = find(bin==maxBins(iBin));
          numSide = length(boundaryIndex);
          if numSide > 1
            if all(boundary(boundaryIndex,2)==boundary(boundaryIndex(1),2))
              %A = [boundary(boundaryIndex,1) ones(numSide,1)];
              %p{iBin} = A \ boundary(boundaryIndex,2);
              p{iBin} = [0; boundary(boundaryIndex(1),2)];
              lineFits(iBin).switched = true;
            else
              A = [boundary(boundaryIndex,2) ones(numSide,1)];
              p{iBin} = A \ boundary(boundaryIndex,1);
              lineFits(iBin).switched = false;
            end
            
            lineFits(iBin).slope = p{iBin}(1);
            lineFits(iBin).intercept = p{iBin}(2);
            
            if DEBUG_DISPLAY
              xx = 1:size(regionMap,2);
              yy = p{iBin}(1)*xx + p{iBin}(2);
              plot(xx,yy,'r');
            end
          else
            didFitFourLines = false;
            break;
          end
        end
        
        if didFitFourLines
          corners = {};
          for iLine = 1:4
            for jLine = (iLine+1):4
              if lineFits(iLine).switched == lineFits(jLine).switched
                %xInt = (p{jLine}(2)-p{iLine}(2)) / (p{iLine}(1) - p{jLine}(1));
                xInt = (lineFits(jLine).intercept - lineFits(iLine).intercept) / ...
                  (lineFits(iLine).slope - lineFits(jLine).slope);
                %yInt = p{iLine}(1)*xInt + p{iLine}(2);
                yInt = lineFits(iLine).slope*xInt + lineFits(iLine).intercept;
                
                if lineFits(iLine).switched == true
                  temp = xInt;
                  xInt = yInt;
                  yInt = temp;
                end
                
              elseif lineFits(iLine).switched == false && lineFits(jLine).switched == true
                yInt = (lineFits(iLine).slope*lineFits(jLine).intercept + lineFits(iLine).intercept) / ...
                  (1 - lineFits(iLine).slope*lineFits(jLine).slope);
                xInt = lineFits(jLine).slope*yInt + lineFits(jLine).intercept;
              elseif lineFits(iLine).switched == true && lineFits(jLine).switched == false
                yInt = (lineFits(jLine).slope*lineFits(iLine).intercept + lineFits(jLine).intercept) / ...
                  (1 - lineFits(iLine).slope*lineFits(jLine).slope);
                xInt = lineFits(iLine).slope*yInt + lineFits(iLine).intercept;
              end
              
              if xInt >= 1 && xInt <= size(regionMap,2) && yInt >= 1 && yInt <= size(regionMap,1)
                corners{end+1} = [xInt yInt];
              end
            end
          end
        end % IF didFitFourLines
        
        if length(corners)==4
          corners = vertcat(corners{:});
          % make sure corners are in clockwise order
          cen = mean(corners,1);
          angles = atan2(corners(:,2)-cen(2), corners(:,1)-cen(1));
          [~,sortIndex] = sort(angles);
          corners = corners(sortIndex,:);
          
           % Reorder the indexes to be in the order
           % [corner1, theCornerOppositeCorner1, corner2, corner3]
           corners = corners([1 4 2 3],:);
           
           if DEBUG_DISPLAY
             plot(corners([1 2 4 3 1],1), corners([1 2 4 3 1], 2), 'g--*');
             set(gca, 'XLim', [1 size(regionMap,2)], 'YLim', [1 size(regionMap,1)]);
             %keyboard
           end
        else 
          corners = [];
        end
        
        
      otherwise
        error('Unrecognzed cornerMethod "%s"', cornerMethod)
    end % SWITCH(cornerMethod)
    
    
    if isempty(corners) && ~strcmp(cornerMethod, 'lineFits')
      
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
      % of the square.
      % NOTE: one of the comparisons is >= while the other is >, in order to
      % combat rare cases where we have two responses next to each other that
      % are exactly equal.
      localMaxima = find(r_smooth >= r_smooth([end 1:end-1]) & r_smooth > r_smooth([2:end 1]));
      
      if length(localMaxima)>=4 
        [~,whichMaxima] = sort(r_smooth(localMaxima), 'descend');
        whichMaxima = sort(whichMaxima(1:4), 'ascend');
        
        % Reorder the indexes to be in the order
        % [corner1, theCornerOppositeCorner1, corner2, corner3]
        bin = localMaxima(whichMaxima([1 4 2 3]));
        
        % plot(boundary(index,2), boundary(index,1), 'y+')
        
        corners = fliplr(boundary(bin,:));
      end
    end
    
    if ~isempty(corners)
        %         % Shift the corners out to the cracks (EXPERIMENTAL)
        %         indexBefore = index-1;
        %         indexBefore(indexBefore==0) = size(boundary,1);
        %         indexAfter = index+1;
        %         indexAfter(indexAfter>size(boundary,1)) = 1;
        %         deriv = fliplr((boundary(indexAfter,:) - 2*boundary(index,:) + boundary(indexBefore,:))/2);
        %         corners = corners - deriv;
        
        if DEBUG_DISPLAY
            plot(corners(:,1), corners(:,2), 'gx', 'Parent', h_initialAxes);
        end
        
        % Verify corners are in a clockwise direction, so we don't get an
        % accidental projective mirroring when we do the tranformation below to
        % extract the image.  Can look whether the z direction of cross product
        % of the two vectors forming the quadrilateral is positive or negative.
        A = [corners(2,:) - corners(1,:);
            corners(3,:) - corners(1,:)];
        detA = det(A); % cross product of vectors anchored at corner 1
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
            % each other in the middle or form some sort of weird concave
            % shape).
            B = [corners(3,:) - corners(4,:);
                corners(2,:) - corners(4,:)];
            detB = det(B); % cross product of vectors anchored at corner 4
            
            C = [corners(4,:) - corners(3,:); 
                corners(1,:) - corners(3,:)];
            detC = det(C); % cross product of vectors anchored at corner 3
            
            D = [corners(1,:) - corners(2,:);
                corners(4,:) - corners(2,:)];
            detD = det(D); % cross product of vectors anchored at corner 2
            
            if sign(detA) == sign(detB) && sign(detC) == sign(detD) && ...
                abs(detA) > .001 && abs(detB) > .001 && abs(detC) > .001 && abs(detD)>.001
              
                detA = abs(detA);
                detB = abs(detB);
                
                %detC = abs(detC);
                %detD = abs(detD);
                
                ratioAB = max(detA,detB) / min(detA,detB);
                ratioCD = max(detC,detD) / min(detC,detD);
                
                % Is the quad symmetry below the threshold?
                if min(ratioAB, ratioCD) < 2
                    
%                   % Shift the corners to their presumed locations if we're 
%                   % using curved corners (rely on refinement later to
%                   % fine-tune these positions)
%                   if VisionMarkerTrained.CornerRadiusFraction > 0
%                     curvedCornersScale = 2*(sqrt(2)-1)*VisionMarkerTrained.CornerRadiusFraction + 1;
%                     cen = ones(4,1)*mean(corners,1);
%                     corners = curvedCornersScale*(corners - cen) + cen;
%                   end
                  
                    tform = []; %#ok<NASGU>
                    if computeTransformFromBoundary
                        % Define each side independently:
                        N = size(boundary,1);
                        
                        Nside = ceil(N/4);
                        
%                         if VisionMarkerTrained.CornerRadiusFraction > 0
%                           curvedCornersScale = 2*(sqrt(2)-1)*VisionMarkerTrained.CornerRadiusFraction + 1;
%                           canonicalCorners = [-.5 -.5; -.5 .5; .5 -.5; .5 .5] / curvedCornersScale + 0.5; 
%                         else
                          canonicalCorners = [0 0; 0 1; 1 0; 1 1];
%                         end
                        
                        tformIsInitialized = false;
                        if strcmp(embeddedConversions.homographyEstimationType, 'matlab_original')
                            try
                                tformInit = cp2tform(corners, canonicalCorners, 'projective');
                                tformIsInitialized = true;
                            catch E
                                warning(['While computing tformInit: ' E.message]);
                                tformInit = [];
                            end
                        elseif strcmp(embeddedConversions.homographyEstimationType, 'opencv_cp2tform')
                            tformInit = mex_cp2tform_projective(corners, canonicalCorners);
                            tformIsInitialized = true;
                        elseif strcmp(embeddedConversions.homographyEstimationType, 'c_float64')
                            tformInit = mexEstimateHomography(corners, canonicalCorners);
                            tformIsInitialized = true;
                        elseif strcmp(embeddedConversions.homographyEstimationType, 'matlab_inhomogeneous')
                            tformInit = estimateHomographyInhomogeneous(corners, canonicalCorners);
                            tformIsInitialized = true;
                        end

                        % Test comparing the matlab and openCV versions
%                         homography = mex_cp2tform_projective(corners, [0 0; 0 1; 1 0; 1 1]);
%                         tformIsInitialized = true;
%                         
%                         c1 = tformInit.tdata.T'*[corners,ones(4,1)]';
%                         c1 = c1 ./ repmat(c1(3,:), [3,1]);
%                         
%                         c2 = homography*[corners,ones(4,1)]';
%                         c2 = c2 ./ repmat(c2(3,:), [3,1]);
%                         
%                         keyboard
                        
                        if tformIsInitialized
%                             try
                                canonicalBoundary = ...
                                    [zeros(Nside,1) linspace(0,1,Nside)';  % left side
                                    linspace(0,1,Nside)' ones(Nside,1);  % top
                                    ones(Nside,1) linspace(1,0,Nside)'; % right
                                    linspace(1,0,Nside)' zeros(Nside,1)]; % bottom
                                switch(quadRefinementMethod)
                                    case 'ICP'
                                        if strcmp(embeddedConversions.homographyEstimationType, 'matlab_original')
                                            tform = ICP(fliplr(boundary), canonicalBoundary, ...
                                                'projective', 'tformInit', tformInit, ...
                                                'maxIterations', 10, 'tolerance', .001, ...
                                                'sampleFraction', 1);
                                            
%                                             disp('normal ICP');
%                                             disp(computeHomographyFromTform(tform));
                                        elseif strcmp(embeddedConversions.homographyEstimationType, 'opencv_cp2tform') || strcmp(embeddedConversions.homographyEstimationType, 'c_float64')
                                            tform = ICP_projective(fliplr(boundary), canonicalBoundary, ...
                                                'homographyInit', tformInit, ...
                                                'maxIterations', 10, 'tolerance', .001, ...
                                                'sampleFraction', 1);
%                                             disp('projective ICP');
%                                             disp(tform);
                                            if ~isempty(find(isnan(tform), 1))
                                                tform = tformInit;
                                            end
                                        end                                        
                                    case 'fminsearch'
                                        mag = smoothgradient(img, 1);
                                        %namedFigure('refineHelper')
                                        %hold off, imagesc(mag), axis image off, hold on
                                        %colormap(gray)
                                        options = optimset('TolFun', .1, 'MaxIter', 50);
                                        tform = fminsearch(@(x)refineHelper(x,mag,canonicalBoundary), tformInit.tdata.T, options);
                                        tform = maketform('projective', tform);
                                        
                                    case 'none'
                                        tform = tformInit;
                                        
                                    otherwise
                                        error('Unrecognized quadRefinementMethod "%s"', quadRefinementMethod);
                                end
                                
%                             catch E
%                                 warning(['While refining tform: ' E.message])
%                                 tform = tformInit;
%                             end
                        else
                            tform = [];
                        end
                                                
                        
                        if ~isempty(tform)
                            %if strcmp(quadRefinementMethod, 'ICP') && (strcmp(embeddedConversions.homographyEstimationType, 'opencv_cp2tform') || strcmp(embeddedConversions.homographyEstimationType, 'c_float64'))
                            if ~strcmp(embeddedConversions.homographyEstimationType, 'matlab_original')
                                
%                                 [x,y] = tforminv(tform, [0 0 1 1]', [0 1 0 1]');
                                tformInv = inv(tform);
                                tformInv = tformInv / tformInv(3,3);
                                xy = tformInv*[0,0,1,1;0,1,0,1;1,1,1,1];
                                xy = xy ./ repmat(xy(3,:), [3,1]);
                                x = xy(1,:)';
                                y = xy(2,:)';
                                
                                % Make the tform normal, so it works with
                                % the other pieces
                                tform = maketform('projective', tform');
                            else % Just a normal Matlab tform
                                % The original corners must have been
                                % visible in the image, and we'd expect
                                % them to all still be within the image
                                % after the transformation adjustment.
                                % Also, the area should still be large
                                % enough
                                % TODO: repeat all the other sanity
                                % checks on the quadrilateral here?
                                [x,y] = tforminv(tform, [0 0 1 1]', [0 1 0 1]');
                                
%                                 disp([x,y])
%                                 
%                                 tformInv = inv(computeHomographyFromTform(tform));
%                                 tformInv = tformInv / tformInv(3,3);
%                                 xy = tformInv*[0,0,1,1;0,1,0,1;1,1,1,1];
%                                 xy = xy ./ repmat(xy(3,:), [3,1]);
%                                 x = xy(1,:)';
%                                 y = xy(2,:)';
                            end
                                                        
%                             disp([x,y])
%                             
%                             keyboard
                            
                            area = abs((x(2)-x(1))*(y(3)-y(1)) - (x(3)-x(1))*(y(2)-y(1)));
                            if all(round(x) >= (1+minDistanceFromImageEdge) & round(x) <= (ncols-minDistanceFromImageEdge) &...
                                   round(y) >= (1+minDistanceFromImageEdge) & round(y) <= (nrows-minDistanceFromImageEdge)) && ...
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
                    
                    if ~isempty(tform) % tfrom now required by new BlockMarker2D
                        quads{end+1} = corners; %#ok<AGROW>
                        quadTforms{end+1} = tform; %#ok<AGROW>
                    end
                    
                end % IF areas of parallelgrams are similar
                
            end % IF signs of determinants match
            
        end % IF quadrilateral has enough area
        
    end % IF we have at least 4 local maxima
    
end % FOR each region

% keyboard
