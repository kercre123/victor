function testBlockWorld(calibration, frames)

cla
W = BlockWorld('CameraCalibration', calibration);

T_update = 0;
T_draw = 0;

if nargin > 1 && ~isempty(frames)
    for i = 1:length(frames)
        t = tic;
        W.update(frames{i});
        T_update = T_update + toc(t);
        
        t = tic;
        draw(W);
        title(i)
        T_draw = T_draw + toc(t);
        
    end
    
else
    
    h_fig(1) = namedFigure('BlockWorld 3D');
    h_fig(2) = namedFigure('BlockWorld Reproject');
    
    set(h_fig, 'CurrentCharacter', ' ');
    
    while get(h_fig(1), 'CurrentCharacter') ~= 27 && ...
            get(h_fig(2), 'CurrentCharacter') ~= 27
        
        t = tic;
        W.update();
        T_update = T_update + toc(t);
        
        t = tic;
        draw(W);
        T_draw = T_draw + toc(t);
        
        %pause
    end
    
end

T = T_update + T_draw;
fprintf('Updates took a total of %.2f seconds, or %.1f%%.\n', ...
    T_update, 100*T_update/T);
fprintf('Drawing took a total of %.2f seconds, or %.1f%%.\n', ...
    T_draw, 100*T_draw/T);

end
    
    
    
