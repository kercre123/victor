function inPosition = goToPosition(this, xGoal, yGoal, distanceThreshold)

% TODO: make these properties of the robot?
K_turn  = 2;
K_dist  = 0.05;
maxSpeed = 8;
headingThreshold = 5;

% Get the position of the turning point between front drive wheels:
offset = this.appearance.AxlePosition;
currentOrient = this.headingAngle;
xTurnPoint = this.pose.T(1) + offset*cos(currentOrient);
yTurnPoint = this.pose.T(2) + offset*sin(currentOrient);

% How far away are we?
xDist = xGoal-xTurnPoint;
yDist = yGoal-yTurnPoint;
distance = sqrt(xDist^2 + yDist^2);

goalHeading = atan2(yDist, xDist);

[turnVelocityLeft, turnVelocityRight, headingError] = computeTurnVelocity(...
    this, goalHeading, currentOrient, K_turn);

% Don't start driving towards the goal until we are basically facing it
if abs(headingError) < headingThreshold*pi/180
    distanceVelocity = max(-maxSpeed, min(maxSpeed, K_dist*distance));
else
    distanceVelocity = 0;
end

fprintf('Heading error: %.2f degrees (Goal = %.2f, Current = %.2f), Distance Error: %.2f\n', ...
    headingError * 180/pi, goalHeading*180/pi, currentOrient*180/pi, distance);

leftMotorVelocity  = max(-maxSpeed, min(maxSpeed, turnVelocityLeft  + distanceVelocity));
rightMotorVelocity = max(-maxSpeed, min(maxSpeed, turnVelocityRight + distanceVelocity));

inPosition = distance < distanceThreshold;

this.drive(leftMotorVelocity, rightMotorVelocity);

end % FUNCTION goToPosition


