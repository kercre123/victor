function UpdatePose(this, varargin)
if nargin == 2
    assert(isa(varargin{1}, 'Pose'), ...
        'LucasKanadeTracker.UpdatePose(): Expecting Pose object.');
    R = varargin{1}.Rmat;
    T = varargin{1}.T;
else
    assert(nargin==3 && isequal(size(varargin{1}), [3 3]) && ...
        isequal(size(varargin{2}), [3 1]), ...
        'LucasKanadeTracker.UpdatePose(): Expecting 3x3 rotation matrix and 3x1 translation vector.');
    
    R = varargin{1};
    T = varargin{2};
end

this.rotationMatrix = R;
this.translation    = T;

% Now that we've updated the pose parameters, we need to update
% the transformation
this.tform = this.Compute6dofTform(R);

end