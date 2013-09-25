function done = liftControlStep(this)

K = .75;
convergenceTolerance = 0.5 * pi/180; 

currentAngle = this.getLiftAngleFcn();
e = this.liftAngle - currentAngle;

done = abs(e) < convergenceTolerance;

if ~done
    if this.liftAngle > currentAngle;
        if e > pi
            e = 2*pi - e;
        end
    elseif e < -pi
        e = 2*pi + e;
    end
    
    this.adjustLift(K*e);
end

end % FUNCTION liftControlStep()