function Rout = plotRobot(x, y, theta, Rin)

arrowLength = 8;
arrowHeadLength = 6;
arrowAngle = 7*pi/8;

if nargin == 4
  Rout = Rin;
  hx = Rin(1);
  ha = Rin(2);
else
  hx = plot(0, 0, 'ro');
  ha = plot(0, 0, 'r-');
  Rout = [hx, ha];
end

% compute arrow points
arrowTipX = x + arrowLength*cos(theta);
arrowTipY = y + arrowLength*sin(theta);

arrowPoints = [x, y;
               arrowTipX, arrowTipY;
               arrowTipX + arrowHeadLength*cos(theta + arrowAngle), ...
               arrowTipY + arrowHeadLength*sin(theta + arrowAngle);
               arrowTipX + arrowHeadLength*cos(theta - arrowAngle), ...
               arrowTipY + arrowHeadLength*sin(theta - arrowAngle);
               arrowTipX, arrowTipY];

set(hx, 'XData', x);
set(hx, 'YData', y);

set(ha, 'XData', arrowPoints(:,1));
set(ha, 'YData', arrowPoints(:,2));
                 

end
