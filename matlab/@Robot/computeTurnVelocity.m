function [turnVelocityLeft, turnVelocityRight, headingError] = computeTurnVelocity(~, ...
    goalHeading, currentHeading, K)

headingError = goalHeading - currentHeading;
if goalHeading > currentHeading
    if headingError > pi
        headingError = 2*pi - headingError;
    end
elseif headingError < -pi
    headingError = 2*pi + headingError;
end

turnVelocityLeft  = -K*headingError;
turnVelocityRight = -turnVelocityLeft;
    
end % FUNCTION computeTurnVelocity()