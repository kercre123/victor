function [numRegions, indexList, centroid, components2d] = simpleDetector_step3_simpleRejectionTests(nrows, ncols, numRegions, area, indexList, bb, centroid, usePerimeterCheck, components2d, embeddedConversions, DEBUG_DISPLAY)

solidThreshold = 0.5;
sparseThreshold = .001;
minSideLengthMultiplier = .01; % Was .03
maxSideLengthMultiplier = .97;

if DEBUG_DISPLAY
    namedFigure('InitialFiltering')
    subplot(2,2,1)
    imshow(binaryImg)
    title(sprintf('Initial Binary Image: %d regions', numRegions))
end

minSideLength = minSideLengthMultiplier*max(nrows,ncols);
maxSideLength = maxSideLengthMultiplier*min(nrows,ncols);

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
        
        if ~isempty(components2d)
            components2d(toRemove) = [];
        end
        
        numRegions = numRegions - sum(toRemove);
    end

if DEBUG_DISPLAY
    subplot(2,2,2)
    imshow(binaryImg)
    title(sprintf('After Area check: %d regions', numRegions))
end

if isempty(bb)
   numRegions = 0;
   indexList = zeros(1,0);
   centroid = zeros(0,2);
   components2d = [];
   return;
end

bbArea = prod(bb(:,3:4),2);
tooSolid = area ./ bbArea > solidThreshold;
tooSparse = area ./ bbArea < sparseThreshold;
updateStats(tooSolid | tooSparse);

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

end