function [xMat,yMat,orient1] = matLocalization(img, varargin)

%% Params
orientationSample = 2; % just for speed
derivSigma = 2;
pixPerMM = 6;
squareWidth = 10;
lineWidth = 1.5;
camera = [];
matSize = [1000 1000];
zDirection = 'up'; % 'up' or 'down': use 'down' for Webots
returnMarkerOnly = false;
DEBUG_DISPLAY = false;
embeddedConversions = [];

parseVarargin(varargin{:});

%% Init
if isa(img, 'uint8')
    img = double(img)/255;
end

if size(img,3)>1
    img = mean(img,3);
end

[nrows,ncols] = size(img);

assert(min(img(:))>=0 && max(img(:))<=1, ...
    'Image should be on the interval [0 1].');

%% Image Orientation
imgOrig = img;

[img, xgrid, ygrid, imgCen] = matLocalization_step1_undoRadialDistortion( ...
    camera, img, embeddedConversions, DEBUG_DISPLAY);

[orient1, mag] = matLocalization_step2_downsampleAndComputeGradientAngles( ...
    img, orientationSample, derivSigma, embeddedConversions, DEBUG_DISPLAY);

[orient1] = matLocalization_step3_computeImageOrientation(orient1, mag, ...
    embeddedConversions, DEBUG_DISPLAY);

%% Grid Square localization   

[imgRot] = matLocalization_step4_rotateImage(imgOrig, orient1, imgCen, ...
    xgrid, ygrid, embeddedConversions, DEBUG_DISPLAY);

[xcenIndex, ycenIndex] = matLocalization_step5_findGrideSquareCenter( ...
    imgRot, squareWidth, pixPerMM, lineWidth, embeddedConversions, DEBUG_DISPLAY);


%% Mat Marker Identification
marker = matLocalization_step6_findMarker(imgRot, orient1, squareWidth, ...
    lineWidth, pixPerMM, xcenIndex, ycenIndex, embeddedConversions, DEBUG_DISPLAY);

if returnMarkerOnly
      
    % Hack to allow us to get back just a Mat Marker to give to a simulated
    % robot to pass as a message up to a basestation, leaving the actual
    % localization to be done by the basestation.
    
    if false && ~marker.isValid
        desktop
        keyboard
    end
    
    %{
    hold off
    imshow(imgOrig); hold on
    draw(marker)
    cen = marker.centroid;
    plot(cen(1)+[0 pixPerMM*squareWidth/2*cos(marker.upAngle)], ...
        cen(2)+[0 pixPerMM*squareWidth/2*sin(marker.upAngle)], 'r', 'LineWidth', 3);
    
    title(sprintf('UpAngle=%.1f (orient1=%.1f)', marker.upAngle*180/pi, orient1*180/pi))
   %}

    assert(nargout==1, ...
        'If requesting marker only, exactly one output is required.');
    xMat = marker;
    return;
end

%% Mat Localization

[xMat, yMat, xvecRot, yvecRot, orient1] = matLocalization_step7_localizeOnMat( ...
    marker, squareWidth, pixPerMM, imgCen, xcenIndex, ycenIndex, ...
    matSize, zDirection, embeddedConversions, DEBUG_DISPLAY);


%% Debug output

if nargout == 0 || DEBUG_DISPLAY
    % Location of the center of the square in original (distorted) image
    % coordinates:
    xcen = xgridRot(ycenIndex,xcenIndex);
    ycen = ygridRot(ycenIndex,xcenIndex);

    h_fig = namedFigure('MatLocalization');
    
    % Plot the input image with image/marker centers, or update the
    % existing one:
    h_imgOrig = findobj(h_fig, 'Tag', 'imgOrig');
    if isempty(h_imgOrig)
        h_imgOrigAxes = subplot(2,2,1, 'Parent', h_fig);
        hold(h_imgOrigAxes, 'off')
        imagesc(imgOrig, 'Parent', h_imgOrigAxes, 'Tag', 'imgOrig');
        axis(h_imgOrigAxes, 'xy', 'tight', 'off');
        hold(h_imgOrigAxes, 'on');
        plot(xcen, ycen, 'bx', 'Parent', h_imgOrigAxes, 'Tag', 'imgOrigMarkerCen');
        plot(imgCen(1), imgCen(2), 'r.', 'Parent', h_imgOrigAxes, 'Tag', 'imgOrigCen');
    else
        set(h_imgOrig, 'CData', imgOrig);
        h_imgOrigAxes = get(h_imgOrig, 'Parent');
        set(findobj(h_imgOrigAxes, 'Tag', 'imgOrigMarkerCen'), ...
            'XData', xcen, 'YData', ycen);
        set(findobj(h_imgOrigAxes, 'Tag', 'imgOrigCen'), ...
            'XData', imgCen(1), 'YData', imgCen(2));
    end
       
    % Draw the rotated/warped image, with MatMarker2D and image center, or
    % update the existing ones
    h_imgRot = findobj(h_fig, 'Tag', 'imgRot');
    if isempty(h_imgRot)
        h_imgRotAxes = subplot(2,2,2, 'Parent', h_fig);
        hold(h_imgRotAxes, 'off');
        imagesc(imgRot, 'Parent', h_imgRotAxes, 'Tag', 'imgRot');
        axis(h_imgRotAxes, 'image', 'off');
        hold(h_imgRotAxes, 'on');
        plot(imgCen(1), imgCen(2), 'r.', 'MarkerSize', 12, ...
            'Parent', h_imgRotAxes, 'Tag', 'imgRotCen');
    else
        set(h_imgRot, 'CData', imgRot);
        h_imgRotAxes = get(h_imgRot, 'Parent');
        delete(findobj(h_imgRotAxes, 'Tag', 'MatMarker2D'));
        set(findobj(h_imgRotAxes, 'Tag', 'imgRotCen'), ...
            'XData', imgCen(1), 'YData', imgCen(2));
    end
    draw(marker, 'where', h_imgRotAxes, 'drawTextLabels', 'short');
    title(h_imgRotAxes, sprintf('Orientation = %.1f degrees', orient*180/pi));
    
    % Draw the (thresholded) marker means, or update the existing ones:
    if marker.isValid
        meansRot = imrotate(marker.means, -marker.upAngle * 180/pi, 'nearest');
        meansRot = meansRot > marker.threshold;
        titleStr = sprintf('X = %d, Y = %d', marker.X, marker.Y);
    else
        meansRot = marker.means;
        titleStr = 'Invalid Marker';
    end
    h_meansImg = findobj(h_fig, 'Tag', 'meansImg');
    if isempty(h_meansImg)
        h_meansImgAxes = subplot(2,3,4, 'Parent', h_fig);
        imagesc(meansRot, 'Parent', h_meansImgAxes, 'Tag', 'meansImg', [0 1]);
        axis(h_meansImgAxes, 'image');
    else
        set(h_meansImg, 'CData', meansRot);
        h_meansImgAxes = get(h_meansImg, 'Parent');
    end
    title(h_meansImgAxes, titleStr);
    
    
    % Draw image boundaries on the mat:
    xImg = ncols/2*[-1 1 1 -1 -1] / pixPerMM;
    yImg = nrows/2*[-1 -1 1 1 -1] / pixPerMM;
    xImgRot = xImg*cos(-orient) + yImg*sin(-orient) + xMat;
    yImgRot = -xImg*sin(-orient) + yImg*cos(-orient) + yMat;
    
    % Compute the marker center location on the mat, in mm:
    xMarkerCen = xMat - xvecRot/pixPerMM;
    yMarkerCen = yMat - yvecRot/pixPerMM;
    xMarker = squareWidth/2*[-1 1 1 -1 -1] + xMarkerCen;
    yMarker = squareWidth/2*[-1 -1 1 1 -1] + yMarkerCen;
    
    h_matAxes = subplot(2,3,[5 6], 'Parent', h_fig);
    if ~strcmp(get(h_matAxes, 'Tag'), 'GridMat')
        gridMat('matSize', 1000, 'axesHandle', h_matAxes);
        set(h_matAxes, 'Tag', 'GridMat');
        hold on
        plot(xMat, yMat, 'ro', 'MarkerSize', 12, ...
            'Tag', 'MatPosition', 'Parent', h_matAxes);
        plot(xMat, yMat, 'r.', 'MarkerSize', 12, ...
            'Tag', 'MatPositionHistory', 'Parent', h_matAxes);
        plot(xImgRot, yImgRot, 'r', 'LineWidth', 2, ...
            'Tag', 'MatPositionRect', 'Parent', h_matAxes);
        plot(xMarkerCen, yMarkerCen, 'bx', 'LineWidth', 2, ...
            'Tag', 'MatMarkerCen', 'Parent', h_matAxes);
        plot(xMarker, yMarker, 'g', 'LineWidth', 3, ...
            'Tag', 'MatMarker', 'Parent', h_matAxes);
    else 
        set(findobj(h_matAxes, 'Tag', 'MatPosition'), ...
            'XData', xMat, 'YData', yMat);
        h_history = findobj(h_matAxes, 'Tag', 'MatPositionHistory');
        xMatHistory = column(get(h_history, 'XData'));
        yMatHistory = column(get(h_history, 'YData'));
        set(h_history, ...
            'XData', [xMatHistory((max(1,end-100)):end); xMat], ...
            'YData', [yMatHistory((max(1,end-100)):end); yMat]);
        set(findobj(h_matAxes, 'Tag', 'MatPositionRect'), ...
            'XData', xImgRot, 'YData', yImgRot);
        set(findobj(h_matAxes, 'Tag', 'MatMarkerCen'), ...
            'XData', xMarkerCen, 'YData', yMarkerCen);
        set(findobj(h_matAxes, 'Tag', 'MatMarker'), ...
            'XData', xMarker, 'YData', yMarker);
    end
    
    viewWin = 50;
    set(h_matAxes, 'XLim', [max(0, xMat-viewWin) min(1000, xMat+viewWin)], ...
        'YLim', [max(0, yMat-viewWin) min(1000, yMat+viewWin)]);
end



% keyboard