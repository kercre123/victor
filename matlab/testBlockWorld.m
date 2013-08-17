
% function W = testBlockWorld(varargin)

% The main function for testing the fiducial detection for both the mat and
% the blocks.

% Usage examples:
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true);
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false);
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', false, 'doPause', false);

% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'opencv_cp2tform', 'computeCharacteristicScaleImageType', 'matlab_original'));
% testBlockWorld('calibration', calibration, 'matCalibration', matCalibration, 'frames', frames, 'matFrames', matFrames, 'drawResults', true, 'doPause', false, 'embeddedConversions', EmbeddedConversionsManager('homographyEstimationType', 'opencv_cp2tform', 'computeCharacteristicScaleImageType', 'matlab_loopsAndFixedPoint'));

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
embeddedConversions = EmbeddedConversionsManager('homographyEstimationType', 'matlab_cp2tform');

parseVarargin(varargin{:});

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
    
    
    
