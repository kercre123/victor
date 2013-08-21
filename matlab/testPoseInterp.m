
P1 = Pose([0 0 0], [1 2 3]);
P2 = Pose(3*pi/2*ones(1,3)/sqrt(3), [80 50 70]);

B = Block(75, 1);
B1 = Block(75, 1);
B2 = Block(75, 1);

cla
B1.pose = P1;
draw(B1, 'FaceAlpha', 0.25);
B2.pose = P2;
draw(B2, 'FaceAlpha', 0.25);
axis equal, grid on
set(gca, 'XLim', [-100 100], 'YLim', [-100 100], 'ZLim', [-100 100]);

B.pose = mean(P1, P2);
draw(B), title('Average Pose')

%%
title('Pose Interpolation')
for i = linspace(0,1,100)
    B.pose = interpolate(P1, P2, i);
    draw(B)
    drawnow
end