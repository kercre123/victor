function [blockType, faceType, isValid, keyOrient] = decodeBlockMarker(img, varargin)

n = 5;
corners = [];
cropFactor = 0.5;
tform = [];
method = 'warpImage'; % 'warpImage' or 'warpProbes'

parseVarargin(varargin{:});

mid = (n+1)/2;

assert(~isempty(img), 'Image should not be empty!');

assert(ismatrix(img), 'Grayscale image required.');

% Get means either by warping the image according to a given transform, or
% by using the whole image (or extracting a portion of it according to the
% corners)
switch(method)
    case 'warpImage'
        
        darkCorners = [];
        
        if ~isempty(corners)
            
            % Not sure this makes sense with square fiducial instead of dots
            %         % Want to make sure there's some "dark stuff" inside the region will
            %         % pull out, to help avoid trying to decode a blank quadrilateral
            %         cornerIndex = sub2ind(size(img), round(corners(:,2)), round(corners(:,1)));
            %         darkCorners = mean(img(cornerIndex));
            
            % This assumes the corners are in "clockwise" direction (i.e. the 2nd
            % and third corners are clockwise wrt each other
            
            %resolution = ceil(sqrt(sum((corners(1,:)-corners(4,:)).^2))/sqrt(2));
            resolution = sqrt( max( sum((corners(1,:)-corners(2,:)).^2), ...
                sum((corners(1,:)-corners(3,:)).^2)) );
            
            if isempty(tform)
                [xgrid,ygrid] = meshgrid(linspace(1,n,resolution));
                
                tform = cp2tform(corners, [cropFactor cropFactor;
                    cropFactor n+(1-cropFactor); n+(1-cropFactor) (1-cropFactor);
                    n+cropFactor n+cropFactor], 'projective');
            else
                [xgrid,ygrid] = meshgrid(linspace(cropFactor,n-cropFactor, resolution)/n);
            end
            
            [xi,yi] = tforminv(tform, xgrid, ygrid);
            img = interp2(img, xi, yi, 'nearest');
            
        end
        
        if isa(img, 'uint8')
            minValue = 1;
        else
            minValue = 1/256;
        end
        
        % Remove low-frequency variation
        nrows = size(img, 1);
        sigma = 1.5*nrows/n;
        %g = fspecial('gaussian', max(3, ceil(3*sigma)), sigma);
        g = gaussian_kernel(sigma);
        img = min(1, img ./ max(minValue, separable_filter(img, g, g', 'replicate')));
        
        %numBits = n^2 - 2*n + 1;
        
        try 
            squares = imresize(reshape(1:n^2, [n n]), size(img), 'nearest');
        catch E
            keyboard
        end
        
        [xgrid,ygrid] = meshgrid(1:size(squares,2), 1:size(squares,1));
        
        xcen = imresize(reshape(accumarray(squares(:), xgrid(:), [n^2 1], @mean), [n n]), size(img), 'nearest');
        ycen = imresize(reshape(accumarray(squares(:), ygrid(:), [n^2 1], @mean), [n n]), size(img), 'nearest');
        
        weightSigma = nrows/n/6;
        weights = exp(-.5 * ((xgrid-xcen).^2 + (ygrid-ycen).^2)/weightSigma^2);
        
        % namedFigure('DecodeWeights'), subplot 121, imshow(img), overlay_image(weights, 'r');
        
        means = reshape(accumarray(squares(:), weights(:).*img(:), [n^2 1]), [n n]);
        totalWeight = reshape(accumarray(squares(:), weights(:), [n^2 1]), [n n]);
        means = means ./ totalWeight;
        
    case 'warpProbes'
        % Create "probes" for measuring means at the center of each square
        patternOrigin = cropFactor/n;
        patternWidth = (1 - 2*cropFactor/n);
        squareWidth = patternWidth/n;
        squareCenters = linspace(squareWidth/2, patternWidth-squareWidth/2, n) + patternOrigin;
        
        probeGap = .2; % as a fraction of square width
        probeRadius = 4; % in number
        sigma = 1/6;
        [xgrid, ygrid] = meshgrid((linspace(probeGap*squareWidth, (1-probeGap)*squareWidth, 2*probeRadius+1) - squareWidth/2));
        xgrid = row(xgrid);
        ygrid = row(ygrid);
        
        w = exp(-(xgrid.^2 + ygrid.^2) / (2*sigma^2));
        w = w/sum(w);
        
        xProbes = cell(n);
        yProbes = cell(n);
        for i = 1:n
            [yProbes{i,:}] = deal(ygrid + squareCenters(i));
        end
        for j = 1:n
            [xProbes{:,j}] = deal(xgrid + squareCenters(j));
        end
        xProbes = vertcat(xProbes{:}); %/(n+2*cropFactor) + cropFactor;
        yProbes = vertcat(yProbes{:}); %/(n+2*cropFactor) + cropFactor;
        w = w(ones(n^2,1),:);
        
        % Transform the probe locations to match the image coordinates
        [xImg, yImg] = tforminv(tform, xProbes, yProbes);
        
        % Get means
        imgData = interp2(img, xImg, yImg, 'nearest');
        means = reshape(sum(w.*imgData,2), [n n]);
    
        %{
        namedFigure('decodeBlockMarker')
        subplot 121, hold off, imshow(img), hold on
        plot(xImg, yImg, 'r.');
        subplot 122
        imshow(reshape(means, [n n]))
        pause
        %}
        
    otherwise 
        error('Unrecognized method "%s"', method);
        
end % SWITCH(method)


% Now we've got our means for each block.  Proceed with thresholding and
% decoding:

% Not sure if this is gonna be so great in general, especially the
% weighting by the counts when computing the average at the end.  It might
% also be too computationally expensive.
bins = linspace(0,1,20);
counts = hist(img(:), bins);
localMaxima = find(counts > counts([2 1:end-1]) & counts > counts([2:end end-1]));
if length(localMaxima) > 1
    [~,whichMaxima] = sort(counts(localMaxima), 'descend');
    bin1 = bins(localMaxima(whichMaxima(1)));
    w1 = counts(localMaxima(whichMaxima(1)));
    bin2 = bins(localMaxima(whichMaxima(2)));
    w2 = counts(localMaxima(whichMaxima(2)));
    threshold = (w1*bin1+w2*bin2)/(w1+w2);
else
    % Fallback (old, simple method):
    % Compute threshold as halfway b/w darkest and brightest mean
    if isempty(darkCorners)
        threshold = (max(means(:)) + min(means(:)))/2;
    else
        threshold = (max(means(:)) + darkCorners)/2;
    end
end

% namedFigure('DecodeWeights'), subplot 122, imshow(means > threshold), pause

if all(means > threshold)
    % empty square
    blockType = 0;
    faceType = 0;
    isValid = false;
    keyOrient = 'none';
    return
end

upBits    = false(n); upBits(1:(mid-1),mid) = true;
downBits  = false(n); downBits((mid+1):end,mid) = true;
leftBits  = false(n); leftBits(mid,1:(mid-1)) = true;
rightBits = false(n); rightBits(mid,(mid+1):end) = true;

dirMeans = [mean(means(upBits)) mean(means(downBits)) mean(means(leftBits)) mean(means(rightBits))];
[~,whichDir] = max(dirMeans);

dirs = {'up', 'down', 'left', 'right'};
keyOrient = dirs{whichDir};
switch(keyOrient)
    case 'down'
        means = rot90(rot90(means));
    case 'left'
        means = rot90(rot90(rot90(means)));
    case 'right'
        means = rot90(means);
end

valueBits = true(n);
valueBits(:,mid) = false;
valueBits(mid,:) = false;

binaryString = strrep(num2str(row(means(valueBits)) < threshold), ' ', '');
[blockType, faceType, isValid] = decodeIDs(bin2dec(binaryString));

end