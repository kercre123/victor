function [turnVelocityLeft, turnVelocityRight, headingError] = computeTurnVelocity(~, ...
    goalHeading, currentHeading, K)

headingError = angleDiff(goalHeading, currentHeading);

turnVelocityLeft  = -K*headingError;
turnVelocityRight = -turnVelocityLeft;
    
end % FUNCTION computeTurnVelocity()

