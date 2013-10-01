function done = liftControlStep(this)

K = .75;
convergenceTolerance = 0.5 * pi/180; 

e = angleDiff(this.liftAngle, this.getLiftAngleFcn());

done = abs(e) < convergenceTolerance;

if ~done
    this.adjustLift(K*e);
end

end % FUNCTION liftControlStep()