function findSmoothPath()

    hold off
    rectangle('Pos', [0 0 10 10]);
    hold on
    axis equal, grid on
    
    h_start = imline;
    h_end   = imline;
    
    setColor(h_start, 'g');
    setColor(h_end, 'r');
    
    startPos = getPosition(h_start);
    endPos   = getPosition(h_end);
    h_path  = plot([startPos(1,1) endPos(1,1)], [startPos(1,2) endPos(1,2)], ...
        'b', 'LineWidth', 2);
    
    set(h_path, 'HitTest', 'off')
    
    findSmoothPathCallback();
    
    addNewPositionCallback(h_start, @findSmoothPathCallback);
    addNewPositionCallback(h_end,   @findSmoothPathCallback);

    function findSmoothPathCallback(~)
                
        poseStart = computePoseHelper(h_start);
        poseEnd   = computePoseHelper(h_end);
        
        [xx, yy] = findSmoothPathCompute(poseStart, poseEnd, 3);
        
        set(h_path, 'XData', xx, 'YData', yy);
        drawnow
        
    end

end
    
function pose = computePoseHelper(h)
pos = getPosition(h);
x = pos(:,1);
y = pos(:,2);
v = [x(2)-x(1) y(2)-y(1)];
pose = [x(1) y(1) atan2(v(2), v(1)) norm(v)];
end

function [xx, yy] = findSmoothPathCompute(poseStart, poseEnd, factor)

usePrecomputedInverse = true;

x_start = poseStart(1);
y_start = poseStart(2);
theta_start = poseStart(3);
lambda_start = factor*poseStart(4);

x_end = poseEnd(1);
y_end = poseEnd(2);
theta_end = poseEnd(3);
lambda_end = factor*poseEnd(4);

% RHS of the constraint equations
bx = [x_start x_end lambda_start*[cos(theta_start) cos(theta_end)]]';
by = [y_start y_end lambda_end*[sin(theta_start) sin(theta_end)]]';

if usePrecomputedInverse
    
    invA = [0.2500   -0.2500    0.2500    0.2500;
            0         0        -0.2500    0.2500;
           -0.7500    0.7500   -0.2500   -0.2500;
            0.5000    0.5000    0.2500   -0.2500];
    
    px = invA*bx;
    py = invA*by;
    
else
    A = [(-1)^3 (-1)^2 -1 1;
        1^3    1^2   1 1;
        3*(-1)^2 2*(-1) 1 0;
        3*(1)^2 2*1 1 0];
    
    px = A\bx;
    py = A\by;
    
end

tt = linspace(-1,1,100);
xx = polyval(px, tt);
yy = polyval(py, tt);

end
