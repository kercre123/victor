function code = createCodeFromImage(img, corners, NumProbes, NumBits)

if ischar(img)
    if ~exist(img, 'file')
        error('Image filename not found.');
    end
    img = imread(img);
end

img = im2double(img);
[nrows,ncols,nbands] = size(img);
if nbands > 1
    img = mean(img,3);
end

% Make square
if nrows > ncols
    temp = ones(nrows,nrows);
    pad = round((nrows-ncols)/2);
    temp(:,pad+(1:ncols)) = img;
    img = temp;
    ncols = nrows;
elseif ncols > nrows
    temp = ones(ncols,ncols);
    pad = round((ncols-nrows)/2);
    temp(pad+(1:nrows),:) = img;
    img = temp;
    nrows = ncols;
end

[xgrid,ygrid] = meshgrid( (0.5/NumProbes) : 1/NumProbes : (1-0.5/NumProbes));
xgrid = xgrid(:);
ygrid = ygrid(:);

xgrid = ncols*[(xgrid-0.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes)];
ygrid = nrows*[[ygrid ygrid ygrid]-0.25/NumProbes ygrid ygrid ygrid [ygrid ygrid ygrid]+.25/NumProbes];

% Adjust the probe locations accordingly, using a homography
if isempty(corners)
    corners = [0 ncols 0 ncols; 0 0 nrows nrows]' + 0.5;
end

assert(isequal(size(corners), [4 2]), 'Corners should be a 4x2 matrix.');
H = cp2tform(corners, [0 ncols 0 ncols; 0 0 nrows nrows]', 'projective');
[x,y] = tforminv(H, xgrid, ygrid);

% Get the interpolated values for this perturbation
code = interp2(img, x, y, 'bilinear');
W = double(~isnan(code));
code(isnan(code))=0;

code = reshape(sum(W.*code,2)./sum(W,2), [NumProbes NumProbes]);

code = round(NumBits * code);

if nargout == 0
    clf
    
    subplot 121
    imagesc(img), axis image
    hold on
    plot(xgrid, ygrid, 'r.', 'MarkerSize', 20);
    
    subplot 122
    imagesc(code), axis image
    
    colormap(gray)
end

end % FUNCTION createCodeFromImage()

%{
% OLD CODE

NumProbes = 6; % probe pattern is NumProbes x NumProbes
NumBits = 1; % 1 for binary, 2 for four grayscale levels, ...
CornerBits = 11; % Bits to represent each x and y for each corner
NumPerturbations = 100;
parseVarargin(varargin{:});

[xgrid,ygrid] = meshgrid( (0.5/NumProbes) : 1/NumProbes : (1-0.5/NumProbes));
xgrid = xgrid(:);
ygrid = ygrid(:);

xgrid = [(xgrid-0.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes)];
ygrid = [[ygrid ygrid ygrid]-0.25/NumProbes ygrid ygrid ygrid [ygrid ygrid ygrid]+.25/NumProbes];

img = im2double(img);
if size(img,3)>1
    img = mean(img,3);
end

[nrows,ncols] = size(img);

% Make square
if nrows > ncols
    temp = ones(nrows,nrows);
    pad = round((nrows-ncols)/2);
    temp(:,pad+(1:ncols)) = img;
    img = temp;
    ncols = nrows;
elseif ncols > nrows
    temp = ones(ncols,ncols);
    pad = round((ncols-nrows)/2);
    temp(pad+(1:nrows),:) = img;
    img = temp;
    nrows = ncols;
end

codes = cell(1, NumPerturbations);
for i_perturb = 1:NumPerturbations
    % Randomly perturb the image corners
    xcorners = [0 1 0 1] + 0.025*randn(1,4);
    ycorners = [0 0 1 1] + 0.025*randn(1,4);
    
    % Adjust the probe locations accordingly, using a homography
    H = cp2tform([xcorners; ycorners]', [0 1 0 1; 0 0 1 1]', 'projective');
    [x,y] = tforminv(H, xgrid, ygrid);
    
    % Scale into image coordinates
    x = x * ncols;
    y = y * nrows;

    % Get the interpolated values for this perturbation
    code = interp2(img, x, y, 'bilinear');
    W = double(~isnan(code));
    if any(all(W==0,2))
        error('Perturbation so large that an entire probe was outside the image.');
    end
    code(isnan(code))=0;
    
    codes{i_perturb} = round(NumBits * sum(W.*code,2)./sum(W,2));
end

% Compute the Gaussian representation for this code
codes = [codes{:}];

codeMean = reshape(mean(codes,2), [NumProbes NumProbes]);
codeVar  = reshape(var(codes,0,2), [NumProbes NumProbes]);

codeLength = 4*2*CornerBits + NumProbes*NumProbes*NumBits;

if nargout == 0
    clf
    
    subplot 131
    imagesc(img), axis image
    hold on
    plot(xgrid*ncols, ygrid*nrows, 'r.', 'MarkerSize', 20);
    title({sprintf('Full Covariance Representation = %d values', NumProbes^2 + NumProbes^4), ...
        sprintf('Diagonal Covariance Representation = %d values', 2*NumProbes^2)})
    
    subplot 132
    imagesc(codeMean), axis image
    title({'Code Mean', sprintf('Code Length = %d bits (%d bytes)', codeLength, ceil(codeLength/8))})
    
    subplot 133
    imagesc(sqrt(codeVar)), axis image
    title('Code Std. Dev.')
    
    colormap(gray)
end

end % FUNCTION createCodeFromImage()

%}