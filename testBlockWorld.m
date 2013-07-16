function W = testBlockWorld(varargin)

device = 0;
calibration = [];
matDevice = 1;
matCalibration = [];
frames = {};

parseVarargin(varargin{:});

cla(findobj(namedFigure('BlockWorld 3D'), 'Type', 'axes'))

T_update = 0;
T_draw = 0;

C = onCleanup(@()clear('mexCameraCapture'));

if nargin > 1 && ~isempty(frames)
    % From canned frames
    
    W = BlockWorld('CameraCalibration', calibration);

    for i = 1:length(frames)
        t = tic;
        W.update(frames{i});
        T_update = T_update + toc(t);
        
        t = tic;
        draw(W);
        %title(i)
        T_draw = T_draw + toc(t);
        
        if i < length(frames) 
            pause
        end
    end
    
else
    % From live camera feed
    W = BlockWorld('CameraCalibration', calibration, ...
        'CameraDevice', device, 'MatCameraDevice', matDevice, ...
        'MatCameraCalibration', matCalibration);
    
    h_fig(1) = namedFigure('BlockWorld 3D');
    h_fig(2) = namedFigure('BlockWorld Reproject');
    
    set(h_fig, 'CurrentCharacter', ' ');
    
    chars = [get(h_fig(1), 'CurrentCharacter') ...
        get(h_fig(2), 'CurrentCharacter')];
    
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
    
    
    
