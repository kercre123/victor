function CreatePrintableCodeSheet(varargin)
% Create an tiled image of all the trained marker codes, suitable for printing.
%
% CreatePrintableCodeSheet(<ParamName1, value1>, <ParamName2, value2>, ...)
%
%  To print, resize the figure such that the entire page of codes is
%  visible, go to File -> Print Preview, set the following options, and
%  then Print:
%
%   - Set Orientation to Landscape (if specified page size is landscape)
%   - Set Units to Centimeters
%   - Set Placement to Manual, set Left/Top=0, and set Width/Height to
%     match pageSize (see below), converted to cm (8.5"x11" is 
%     width=27.94cm and height=21.59cm)
%
%  Parameters[default]:
%
%  'marginSpacing' [12.5]
%    Border around the edge of the page, in mm.
%
%  'codeSpacing' [10]
%    Distance between markers, in mm.
%
%  'pageWidth' [11*25.4], 'pageHeight' [8.5*25.4]
%    Page size, in mm.
%
%  'markerImageDir' [<empty>]
%    Grab all codes from this directory. Can also be a cell array of 
%    directories, in which case one code sheet per directory is created.
%    If empty (default), one code sheet is created for the directories
%    currently set in VisionMarkerTrained.TrainingImageDir.
%
%  'markerSize' [25]
%    Outside width of the marker fiducial, in mm. 
%    Can also specify both width and height as [width height]
%
%  'numPerCode' [1]
%    Print this many copies of each marker on the sheet (as many as will
%    fit). Use this, for example, to fill up a sheet when there are fewer
%    markers in the directory than will fit on the sheet.
%
%  'outsideBorderSize' [markerSize + 4]
%    Add a light gray, dotted border of this size around each code. If
%    zero, the border will not be printed.
%
%  'backgroundColor' [1 1 1]
%
%  'contrast' [1]
%    Set < 1 to reduce contrast of markers. Can also be a vector to
%    cycle through a set of contrasts.
%
%
% Example Usage:
%
% Windows: VisionMarkerTrained.CreatePrintableCodeSheet('markerImageDir', {'Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/', 'Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials', 'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials'}, 'outsideBorderWidth', [], 'codeSpacing', 5, 'numPerCode', 2, 'nameFilter', '*.*');
%
% OSX: VisionMarkerTrained.CreatePrintableCodeSheet('markerImageDir', {'~/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/', '~/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials', '~/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials'}, 'outsideBorderWidth', [], 'codeSpacing', 5, 'numPerCode', 2, 'nameFilter', '*.*');
%
% ------------
% Andrew Stein
%


nameFilter = 'images';
marginSpacing = 12.5;  % in mm
codeSpacing = 10;     % in mm
pageHeight = 8.5 * 25.4; % in mm
pageWidth  = 11 * 25.4; % in mm
markerImageDir = [];
markerSize = 25; % in mm
numPerCode = 1;
outsideBorderSize = []; % in mm, empty to not use
backgroundColor = [1 1 1];
contrast = 1;

parseVarargin(varargin{:});

assert(isempty(outsideBorderSize) || all(outsideBorderSize==0) || all(outsideBorderSize > markerSize), ...
    'If specified, outsideBorderWidth should be larger than the markerSize.');

if isempty(outsideBorderSize)
    outsideBorderSize = markerSize + 4;
end

if length(markerSize)==1
  markerSize = [markerSize markerSize];
end

if length(outsideBorderSize)==1
  outsideBorderSize = [outsideBorderSize outsideBorderSize];
end

if length(backgroundColor)==1
  backgroundColor = backgroundColor*[1 1 1];
end

% For now, assume all the training images were in a "rotated" subdir and
% we want to create the test image from the parent of that subdir
upOneDir = '';
if isempty(markerImageDir)
    markerImageDir = VisionMarkerTrained.TrainingImageDir;
    upOneDir = '..';
end

if ~iscell(markerImageDir)
    markerImageDir = { markerImageDir };
end    

numDirs = length(markerImageDir);
fnames = cell(1,numDirs);
for iDir = 1:numDirs
    fnames{iDir} = getfnames(fullfile(markerImageDir{iDir}, upOneDir), nameFilter, 'useFullPath', true);
end
fnames = vertcat(fnames{:});

if isempty(fnames)
    error('No image files found.');
end

numImages = length(fnames);
fprintf('Found %d total images.\n', numImages);
   
numImages = numImages * numPerCode;
fnames = repmat(fnames(:), [numPerCode 1]);

numRows = floor( (pageHeight - 2*marginSpacing + codeSpacing)/(markerSize(2) + codeSpacing) );
numCols = floor( (pageWidth  - 2*marginSpacing + codeSpacing)/(markerSize(1) + codeSpacing) );

xcenters = linspace(marginSpacing+markerSize(1)/2, pageWidth-marginSpacing-markerSize(1)/2, numCols)/10;
ycenters = linspace(marginSpacing+markerSize(2)/2, pageHeight-marginSpacing-markerSize(2)/2,numRows)/10;

[xgrid,ygrid] = meshgrid(xcenters,ycenters);
xgrid = xgrid';
ygrid = ygrid';

%numFigures = ceil(numImages / (numRows*numCols));
% if numRows*numCols < numImages
%     warning('Too many markers for the page. Will leave off %d.', numImages - numRows*numCols);
%     numImages = numRows*numCols;
% end

contrast = repmat(contrast, [1 ceil(numImages/length(contrast))]);
  
iFigure = 0;

for iFile = 1:numImages 

    if iFile > iFigure*numRows*numCols
        iFigure = iFigure + 1;
        h_fig = namedFigure(sprintf('VisionMarkerTrained CodeSheet %d', iFigure), ...
          'Color', 'w', 'PaperUnits', 'centimeters', ...
          'PaperPositionMode', 'manual', ...
          'PaperOrientation', 'landscape', ...
          'PaperSize', [pageWidth/10 pageHeight/10], ...
          'PaperPosition', [0 0 pageWidth/10 pageHeight/10]);
        
        delete(findall(gcf, 'Type', 'axes'))
        
        h_axes = axes('Pos', [0 0 1 1], 'Units', 'centimeters', 'Parent', h_fig); %#ok<*LAXES>
        set(h_axes, 'XLim', [0 pageWidth/10], 'YLim', [0 pageHeight/10], 'Box', 'on', 'Units', 'norm');
        hold(h_axes, 'on')
        rectangle('Pos', [0 0 pageWidth pageHeight]/10, 'LineStyle', '--', 'Parent', h_axes)
        axis(h_axes, 'ij', 'off');
        
        colormap(h_axes, gray);
        
        if outsideBorderSize > 0
            borderStr = sprintf('%.1fx%.1fmm Border, ', outsideBorderSize(1), outsideBorderSize(2));
        else
            borderStr = '';
        end
        text(marginSpacing/10, marginSpacing/20, ...
            sprintf('VisionMarkers, %.1fx%.1fmm Size, %s%s', ...
            markerSize(1), markerSize(2), borderStr, datestr(now, 31)));
    end
    
    [img, ~, alpha] = imread(fnames{iFile});
    
    img = mean(im2double(img),3);
    img(alpha < .5) = 1;
    
    currentBackgroundColor = backgroundColor;
    if contrast(iFile) ~= 1
      [nrows,ncols,nchannels] = size(img);
      if nchannels == 1
        img = img(:,:,ones(1,3));
      end
      img = reshape(img, [], 3);
      background = currentBackgroundColor(ones(nrows*ncols,1),:);
      index= find(all(img >= .5,2));
      img(index,:) = contrast(iFile)*(1-currentBackgroundColor) + background(index,:);
      index = find(all(img < .5,2));
      img(index,:) = background(index,:);
      img = reshape(img, nrows, ncols, 3);
    end
    
    iPos = mod(iFile-1, numRows*numCols) + 1;
    
    if any(outsideBorderSize > 0)
      h_rect = rectangle('Position', [xgrid(iPos)-outsideBorderSize(1)/20 ....
        ygrid(iPos)-outsideBorderSize(2)/20 outsideBorderSize/10], ...
        'Parent', h_axes, 'EdgeColor', [0.8 0.8 0.8], ...
        'LineWidth', .5, 'LineStyle', ':');
        set(h_rect, 'FaceColor', currentBackgroundColor);
        if any(currentBackgroundColor ~= 1)
          set(h_rect, 'LineStyle', 'none');
        end
    end
    
    imagesc(xgrid(iPos)+markerSize(1)/10*[-.5 .5], ...
      ygrid(iPos)+markerSize(2)/10*[-.5 .5], img, ...
      'AlphaData', alpha, ...
      'Parent', h_axes);
    
    set(h_axes, 'CLim', [0 1]);
        
end % FOR each File

end % FUNCTION CreatePrintableCodeSheet
