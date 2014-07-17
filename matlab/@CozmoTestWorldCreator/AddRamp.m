function AddRamp(this, varargin)

RampPose  = Pose(0*[0 0 1], [0 0 22]);
RampProto = 'Ramp2x1';
RampName  = '';

parseVarargin(varargin{:});

fid = fopen(this.worldFile, 'at');
cleanup = onCleanup(@()fclose(fid));

T = RampPose.T / 1000;

if ~isempty(RampName)
    rampType = upper(RampName);
    RampName = 'Ramp';
else
    rampType = 0;
    RampName = '';
end
    
fprintf(fid, [ ...
    '%s {\n' ...
    '  name "%s"\n' ...
    '  type "%s"\n' ... 
    '  translation %f %f %f\n' ...
    '  rotation    %f %f %f %f\n', ...
    '}\n\n'], ...
    RampProto, RampName, rampType, T(1), T(2), T(3), ...
    RampPose.axis(1), RampPose.axis(2), RampPose.axis(3), ...
    RampPose.angle);

end