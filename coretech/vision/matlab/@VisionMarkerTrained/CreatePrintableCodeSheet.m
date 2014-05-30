function CreatePrintableCodeSheet(varargin)
% Create an tiled image of all the training images, for testing detection.

marginSpacing = 12.5;  % in mm
codeSpacing = 10;     % in mm
pageHeight = 8.5 * 25.4; % in mm
pageWidth  = 11 * 25.4; % in mm
markerImageDir = [];
markerSize = 26; % in mm
numPerCode = 1;

parseVarargin(varargin{:});


% For now, assume all the training images were in a "rotated" subdir and
% we want to create the test image from the parent of that subdir
upOneDir = '';
if isempty(markerImageDir)
    markerImageDir = VisionMarkerTrained.TrainingImageDir;
    upOneDir = '..';
end

numDirs = length(markerImageDir);
for i_dir = 1:numDirs
    fnames = getfnames(fullfile(markerImageDir{i_dir}, upOneDir), 'images', 'useFullPath', true);
    
    if isempty(fnames)
        error('No image files found in directory %s.', markerImageDir{i_dir});
    end
    
    numImages = length(fnames);
    fprintf('Found %d total images.\n', numImages);
    
    numImages = numImages * numPerCode;
    fnames = repmat(fnames(:), [numPerCode 1]);
    
    numRows = floor( (pageHeight - 2*marginSpacing + codeSpacing)/(markerSize + codeSpacing) );
    numCols = floor( (pageWidth  - 2*marginSpacing + codeSpacing)/(markerSize + codeSpacing) );
    
    if numRows*numCols < numImages
        warning('Too many markers for the page. Will leave off %d.', numImages - numRows*numCols);
        numImages = numRows*numCols;
    end
    
    namedFigure(sprintf('VisionMarkerTrained CodeSheet %d', i_dir), ...
        'Color', 'w', 'Units', 'centimeters');
    
    clf
    
    xcenters = linspace(marginSpacing+markerSize/2, pageWidth-marginSpacing-markerSize/2, numCols)/10;
    ycenters = linspace(marginSpacing+markerSize/2, pageHeight-marginSpacing-markerSize/2,numRows)/10;
    
    [xgrid,ygrid] = meshgrid(xcenters,ycenters);
    
    h_axes = axes('Units', 'centimeters', 'Position', [0 0 pageWidth/10 pageHeight/10]); %#ok<*LAXES>
    set(h_axes, 'XLim', [0 pageWidth/10], 'YLim', [0 pageHeight/10], 'Box', 'on');
    hold(h_axes, 'on')
    axis(h_axes, 'ij');
    
    colormap(h_axes, gray);
    
    for iMarker = 1:numImages
        [img, ~, alpha] = imread(fnames{iMarker});
        img = mean(im2double(img),3);
        img(alpha < .5) = 1;
        
        imagesc(xgrid(iMarker)+markerSize/10*[-.5 .5], ...
            ygrid(iMarker)+markerSize/10*[-.5 .5], img);
    end
    
end % FOR each directory

end % FUNCTION CreatePrintableCodeSheet
