
% function W = testBlockWorld(varargin)

% The main function for testing the fiducial detection for both the mat and
% the blocks.

% Usage examples:
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false);

% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'matlab_original', 'computeCharacteristicScaleImageType', 'matlab_original', 'traceBoundaryType', 'matlab_original', 'connectedComponentsType', 'matlab_original'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'opencv_cp2tform', 'computeCharacteristicScaleImageType', 'c_fixedPoint', 'traceBoundaryType', 'c_fixedPoint', 'connectedComponentsType', 'matlab_approximate'));

% <Set BlockMarker2D.UseOutsideOfSquare = true in BlockMarker2D.m!!!>
% close all % Get rid of any instantiated/displayed BlockMarkers
% clear BlockMarker2D BlockMarker3D
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'c_float64', 'computeCharacteristicScaleImageType', 'c_fixedPoint', 'traceBoundaryType', 'matlab_approximate', 'connectedComponentsType', 'matlab_approximate'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'c_float64', 'traceBoundaryType', 'matlab_approximate', 'completeCImplementationType', 'c_singleStep123'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'c_float64', 'traceBoundaryType', 'matlab_approximate', 'completeCImplementationType', 'c_singleStep1234'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'c_float64', 'traceBoundaryType', 'matlab_approximate', 'completeCImplementationType', 'c_singleStep12345'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('traceBoundaryType', 'matlab_approximate', 'connectedComponentsType', 'matlab_approximate', 'computeCharacteristicScaleImageType', 'matlab_boxFilters'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'matlab_inhomogeneous'));
% <Possibly close, clear, and set UseOutsideOfSquares back to false>


function W = testBlockWorld(varargin)

device = 0;
calibration = [];
useMat = true;
matDevice = 1;
matCalibration = [];
frames = {};
matFrames = {};
drawResults = true;
doPause = true;
embeddedConversions = EmbeddedConversionsManager();

parseVarargin(varargin{:});

if ischar(frames) && ischar(matFrames)
    frames = {frames};
    matFrames = {matFrames};
end

if ~isempty(frames) && ~isempty(matFrames)
    assert(length(matFrames)==length(frames), ...
        'frames and matFrames should be same length.');
end

if drawResults
    cla(findobj(namedFigure('BlockWorld 3D'), 'Type', 'axes'))
end

T_update = 0;
T_draw = 0;

if drawResults
    h_fig(1) = namedFigure('BlockWorld 3D');
    h_fig(2) = namedFigure('BlockWorld Reproject');

    set(h_fig, 'CurrentCharacter', ' ');

    chars = [get(h_fig(1), 'CurrentCharacter') ...
        get(h_fig(2), 'CurrentCharacter')];
end

if nargin > 1 && ~isempty(frames)
    % From canned frames
    
    W = BlockWorld('CameraCalibration', calibration, ...
                   'MatCameraCalibration', matCalibration, ...
                   'EmbeddedConversions', embeddedConversions, ...
                   'HasMat', useMat);

    for i = 1:length(frames)
        t = tic;
        if isempty(matFrames)
            W.update(frames{i});
        else
            W.update(frames{i}, matFrames{i});
        end
        T_update = T_update + toc(t);
        
        if drawResults
            t = tic;
            draw(W);
            %title(i)
            T_draw = T_draw + toc(t);
            
            if doPause && i < length(frames)
                % wait for keystroke (not mouse click)
                while waitforbuttonpress ~= 1, end
            end
            
            chars = [get(h_fig(1), 'CurrentCharacter') ...
                get(h_fig(2), 'CurrentCharacter')];
            
            if doPause && any(chars == 'r')
                % Press 'r' to switch to pauseless "Run" mode
                doPause = false;
                
            elseif any(chars == 'q') || any(chars == 27)
                % Press q or ESC to stop
                break;
            end
        end % IF drawResults
    end
    
else
    C = onCleanup(@()clear('mexCameraCapture'));
    
    % From live camera feed
    W = BlockWorld('CameraCalibration', calibration, ...
                   'CameraDevice', device, 'MatCameraDevice', matDevice, ...
                   'MatCameraCalibration', matCalibration, ...
                   'HasMat', useMat, ...
                   'EmbeddedConversions', embeddedConversions);
    
    while ~any(chars==27) % Until ESC is pressed
        
        t = tic;
        W.update();
        T_update = T_update + toc(t);
        
        if drawResults
            t = tic;
            draw(W);
            T_draw = T_draw + toc(t);

            %pause
            chars = [get(h_fig(1), 'CurrentCharacter') ...
                get(h_fig(2), 'CurrentCharacter')];
        end
    end
    
end

T = T_update + T_draw;
fprintf('Updates took a total of %.2f seconds, or %.1f%%.\n', ...
    T_update, 100*T_update/T);
fprintf('Drawing took a total of %.2f seconds, or %.1f%%.\n', ...
    T_draw, 100*T_draw/T);

if nargout == 0
    clear W;
end

end
    
    
    
