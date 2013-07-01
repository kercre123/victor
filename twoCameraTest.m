function twoCameraTest

frontCamera = Camera('device', 1);
downCamera  = Camera('device', 0);

h_fig = namedFigure('TwoCameraRobot');
h_frontAxes = subplot(1,2,1, 'Parent', h_fig);
h_front = imshow(zeros(480,640,3), 'Parent', h_frontAxes);

h_downAxes = subplot(1,2,2, 'Parent', h_fig);
h_down = imshow(zeros(480,640,3), 'Parent', h_downAxes);

title(h_frontAxes, 'Front');
title(h_downAxes, 'Down');

set(h_fig, 'CurrentCharacter', ' ');

while get(h_fig, 'CurrentCharacter') ~= 27 
    t = tic;
    front = im2uint8(frontCamera.grabFrame());
    down  = im2uint8(downCamera.grabFrame());
    
    set(h_front, 'CData', front);
    set(h_down,  'CData', down);
    
    drawnow
    pause(max(0, 1/30 - toc(t)));
end

close(frontCamera);
close(downCamera);

end