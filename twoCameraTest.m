function twoCameraTest

showDown = true;
showFront = true;

clear mexCameraCapture

if showDown
    downCamera  = Camera('device', 0);
    cleanupDown = onCleanup(@()close(downCamera));
end

if showFront
    frontCamera = Camera('device', 1);
    cleanupFront = onCleanup(@()close(frontCamera));
end

h_fig = namedFigure('TwoCameraRobot');
h_frontAxes = subplot(1,2,1, 'Parent', h_fig);
front = zeros(480,640,3);
h_front = imshow(front, 'Parent', h_frontAxes);

h_downAxes = subplot(1,2,2, 'Parent', h_fig);
down = zeros(480,640,3);
h_down = imshow(down, 'Parent', h_downAxes);

title(h_frontAxes, 'Front');
title(h_downAxes, 'Down');

set(h_fig, 'CurrentCharacter', ' ');

while get(h_fig, 'CurrentCharacter') ~= 27 
    t = tic;
    if showFront
        front = im2uint8(frontCamera.grabFrame());
    end
       
    if showDown
        down  = im2uint8(downCamera.grabFrame());
    end
    
    set(h_front, 'CData', front);
    set(h_down,  'CData', down);
    
    drawnow
    pause(max(0, 1/30 - toc(t)));
end

end