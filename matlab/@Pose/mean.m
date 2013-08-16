function Pavg = mean(P1, P2)
% Compute the average of two Pose objects.

Pavg = interpolate(P1, P2, .5);

end

