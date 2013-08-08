function [imgUndistorted, xgrid, ygrid, imgCen] = matLocalization_step1_undoRadialDistortion(camera, img, embeddedConversions, DEBUG_DISPLAY)


if ~isempty(camera)
    % Undo radial distortion according to camera calibration info:
    assert(isa(camera, 'Camera'), 'Expecting a Camera object.');
    [imgUndistorted, xgrid, ygrid] = camera.undistort(img, 'nearest');
    imgCen = camera.center;    
    xgrid = xgrid - imgCen(1);
    ygrid = ygrid - imgCen(2);
else
    [nrows,ncols,~] = size(img);
    imgCen = [ncols nrows]/2;
    [xgrid,ygrid] = meshgrid((1:ncols)-imgCen(1), (1:nrows)-imgCen(2));
    imgUndistorted = img;
end

