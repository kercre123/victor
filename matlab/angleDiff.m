function d = angleDiff(angle1, angle2)
% Compute the signed difference angle1-angle2, in radians, handling wrap.

d = angle1 - angle2;
if angle1 > angle2
    if d > pi
        d = 2*pi - d;
    end
elseif d < -pi
    d = 2*pi + d;
end

end % FUNCTION angleDiff()