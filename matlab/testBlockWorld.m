function W = testBlockWorld(varargin)

device = 0;
calibration = [];
matDevice = 1;
matCalibration = [];
deviceType = 'usb'; % 'usb' or 'webot'
frames = {};
matFrames = {};
calibToolboxPath = '~/Code/3rdparty/toolbox_calib';
cameraCapturePath = '~/Code/CameraCapture';

parseVarargin(varargin{:});

if isdir(calibToolboxPath)
    addpath(calibToolboxPath);
end

if isdir(cameraCapturePath)
    addpath(cameraCapturePath);
end

if ~isempty(frames) && ~isempty(matFrames)
    assert(length(matFrames)==length(frames), ...
        'frames and matFrames should be same length.');
end

cla(findobj(namedFigure('BlockWorld 3D'), 'Type', 'axes'))

T_update = 0;
T_draw = 0;

h_fig(1) = namedFigure('BlockWorld 3D');
h_fig(2) = namedFigure('BlockWorld Reproject');

set(h_fig, 'CurrentCharacter', ' ');

chars = [get(h_fig(1), 'CurrentCharacter') ...
    get(h_fig(2), 'CurrentCharacter')];

if nargin > 1 && ~isempty(frames)
    % From canned frames
    
    W = BlockWorld('CameraCalibration', calibration, ...
        'MatCameraCalibration', matCalibration);

    doPause = true;
   
    for i = 1:length(frames)
        t = tic;
        if isempty(matFrames)
            W.update(frames{i});
        else
            W.update(frames{i}, matFrames{i});
        end
        T_update = T_update + toc(t);
        
        t = tic;
        draw(W);
        %title(i)
        T_draw = T_draw + toc(t);
        
        chars = [get(h_fig(1), 'CurrentCharacter') ...
            get(h_fig(2), 'CurrentCharacter')];
        
        if any(chars == 'q') || any(chars == 27)
            break;
        end
        
        if doPause
            if i < length(frames)
                waitforbuttonpress;
            end
            
            % Press 'r' to switch to pauseless "Run" mode
            if any(chars == 'r')
                doPause = false;
            end
        end
            
    end
    
else
    C = onCleanup(@()clear('mexCameraCapture'));
    
    % From live camera feed
    W = BlockWorld('CameraCalibration', calibration, ...
        'CameraDevice', device, 'MatCameraDevice', matDevice, ...
        'MatCameraCalibration', matCalibration);
    
    while ~any(chars==27)
        
        t = tic;
        W.update();
        T_update = T_update + toc(t);
        
        t = tic;
        draw(W);
        T_draw = T_draw + toc(t);
        
        %pause
        chars = [get(h_fig(1), 'CurrentCharacter') ...
            get(h_fig(2), 'CurrentCharacter')];
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
    
    
    
