function d = angleDiff(angle1, angle2, useDegrees)
% Compute the signed difference angle1-angle2, in radians, handling wrap.
%
% d = angleDiff(angle1, angle2, <useDegrees>)
%

if nargin < 3 || isempty(useDegrees)
    useDegrees = false;
end

if useDegrees
    PI = 180;
else
    PI = pi;
end
TWO_PI = 2*PI;

angle1 = mod(angle1, TWO_PI);
angle2 = mod(angle2, TWO_PI);

d = angle1 - angle2;

if isscalar(d)
    
    if angle1 > angle2
        if d > PI
            d = d - TWO_PI;
        end
    elseif d < -PI
        d = TWO_PI + d;
    end
    
else
    
    case1 = angle1 > angle2 & d > PI;
    d(case1) = d(case1) - TWO_PI;
    
    case2 = angle1 < angle2 & d < -PI;
    d(case2) = TWO_PI + d(case2);

end

end % FUNCTION angleDiff()