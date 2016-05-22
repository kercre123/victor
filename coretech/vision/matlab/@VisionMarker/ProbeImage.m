function [code, corners, cornerCode, H] = ProbeImage(img, corners, doRefinement)
% Static method for probing an image within given corners to produce code.

if nargin < 3
    doRefinement = true;
end

assert(~isempty(img), 'Image is empty!');

[nrows,ncols,nbands] = size(img);
img = im2double(img);
if nbands > 1
    %img = mean(img,3);
    img = min(img,[],3);
end

% No corners provided? Assume the borders of the image
if nargin < 2 || isempty(corners)
    corners = [0 0 ncols ncols; 0 nrows 0 nrows]' + 0.5;
    doRefinement = false;
    %imgOffset = (1+VisionMarker.FiducialPaddingFraction)*VisionMarker.SquareWidth;
    %canonicalCorners = [imgOffset*[1 1] (1-imgOffset)*[1 1]; 
    %    imgOffset (1-imgOffset) imgOffset (1-imgOffset)]';
end

assert(isequal(size(corners), [4 2]), ...
    'Corners should be a 4x2 matrix.');

% Figure out the homography to warp the canonical probe points
% into the image coordinates, according to the corner positions
tform = cp2tform(corners, [0 0 1 1; 0 1 0 1]', 'projective');

if doRefinement
    [corners, H] = LKrefineQuad(img, corners, tform.tdata.Tinv', ...
        'MaxIterations', 10, 'DebugDisplay', false);
else
    H = tform.tdata.Tinv';
end

%[xi,yi] = tforminv(H, VisionMarker.XProbes, VisionMarker.YProbes);
temp = H*[VisionMarker.XProbes(:) VisionMarker.YProbes(:) ones(numel(VisionMarker.XProbes),1)]';
xi = reshape(temp(1,:)./temp(3,:), size(VisionMarker.XProbes));
yi = reshape(temp(2,:)./temp(3,:), size(VisionMarker.YProbes));


% Get the interpolated values for each probe point
means = interp2(img, xi, yi, 'bilinear');

% Compute the average values of the points making each probe
switch(VisionMarker.ProbeParameters.Method)
    case 'mean'
        W = double(~isnan(means));
        means(isnan(means))=0;
        means = sum(W.*means,2)./sum(W,2);
        
    case 'min'
        means(isnan(means)) = inf;
        means = min(means,[],2);
        
    otherwise
        error('Unrecognized method "%s".', Method);
end

orientBits = reshape(means(1:4), [2 2]);
means = reshape(means(5:end), VisionMarker.ProbeParameters.Number*[1 1]);

% Use the four corner bits to orient and threshold
% LowerLeft is the one that's the darkest
[dark,lowerLeftIndex] = min(orientBits(:));
% dark = .5*dark; % HACK: the big thick triangle doesn't get as blurred as is thus darker than stuff in the central code
bright = max(orientBits(:));

if bright < VisionMarker.MinContrastRatio*dark
    % not enough contrast to work with
    warning('Not enough contrast ratio to decode VisionMarker.');
    code = [];
    cornerCode = [];
    return
end

threshold = (bright + dark)/2;

code = means < threshold;

% % DEBUG: Draw color-coded probe locations and corners
% plot(corners(:,1), corners(:,2), 'gx');
% index = [false(4,1); code(:)];
% plot(xi(index,:), yi(index,:), 'r.', 'MarkerSize', 8);
% index = [false(4,1); ~code(:)];
% plot(xi(index,:), yi(index,:), 'b.', 'MarkerSize', 8);

cornerCode = orientBits < threshold;
switch(sum(cornerCode(:)))
    case 0
        warning('No corner markers found!');
        code = [];
        return;
        
    case 1
        % Nothing to do, lowerLeftIndex is already correct
        
    case 2
        if isequal(cornerCode,     [true false; true false])
            lowerLeftIndex = 1;
        elseif isequal(cornerCode, [false false; true true])
            lowerLeftIndex = 2;
        elseif isequal(cornerCode, [true true; false false]) 
            lowerLeftIndex = 3;
        elseif isequal(cornerCode, [false true; false true])
            lowerLeftIndex = 4;
        else
            warning('Diagonal pair of corners indicated. Invalid!')
            code = [];
            return
        end
        
    case 3
        if isequal(cornerCode,     [true true; true false])
            lowerLeftIndex = 1;
        elseif isequal(cornerCode, [true true; false true])
            lowerLeftIndex = 3;
        elseif isequal(cornerCode, [false true; true true])
            lowerLeftIndex = 4;
        elseif isequal(cornerCode, [true false; true true])
            lowerLeftIndex = 2;
        else
            error('Invalid (impossible?!) corner marker configuration.');
        end
        
    otherwise
        warning('All four corners appear marked!');
        code = [];
        return;
        
end % SWITCH(sum(cornerMarkers))

% Reorient the code and the corners to match
reorder = [1 3; 2 4]; % canonical corner ordering
switch(lowerLeftIndex)
    case 1
        code = rot90(code);
        cornerCode = rot90(cornerCode);
        reorder = rot90(reorder);
    case 2
        % nothing to do
    case 3
        code = rot90(rot90(code));
        cornerCode = rot90(rot90(cornerCode));
        reorder = rot90(rot90(reorder));
    case 4
        code = rot90(rot90(rot90(code)));
        cornerCode = rot90(rot90(rot90(cornerCode)));
        reorder = rot90(rot90(rot90(reorder)));
    otherwise
        error('lowerLeft should be 1,2,3, or 4.');
end
corners = corners(reorder(:),:);

if nargout == 0
    subplot 121, hold off, imagesc(img), axis image, hold on
    plot(xi, yi, 'r.', 'MarkerSize', 10);
    
    subplot 122, imagesc(~code), axis image
end

end % function ProbeImage()