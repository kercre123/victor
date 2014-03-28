function plotPose(this)

poseSize = 10;

Pbase = [0 1 0 0;
         0 0 1 0;
         0 0 0 1] * poseSize;

R = this.rotationMatrix;

P = R*Pbase + [this.tx; this.ty; this.tz]*ones(1,4);

set(this.h_pose(1), 'XData', P(1,[1 2]), 'YData', P(2,[1 2]), 'ZData', P(3,[1 2]));
set(this.h_pose(2), 'XData', P(1,[1 3]), 'YData', P(2,[1 3]), 'ZData', P(3,[1 3]));
set(this.h_pose(3), 'XData', P(1,[1 4]), 'YData', P(2,[1 4]), 'ZData', P(3,[1 4]));

set(get(get(this.h_pose(1), 'Parent'), 'Title'), 'String', this.poseString);

drawnow

end