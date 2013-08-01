function [x_obs, x_hat, X, dx_dX, dx_dom, dx_dT, bookkeeping] = reproject(this)

%  x_obs  - 2D observed marker corners (observations)
%  x_hat  - reprojected 3D marker corners (predictions, according to pose)
%  X      - 3D marker locations
%  dx_dX  - deriv. (of *x_hat*) with respect to 3D coordinates
%  dx_dom - deriv. wrt rotation vector "omega"
%  dx_dT  - deriv. wrt translation vector T

world = this.robot.world;
camera = this.robot.camera;
invObsFrame = inv(this.frame);

% Reproject the blocks and the markers onto the image:
x_obs = cell(this.numMarkers,1);
X = cell(this.numMarkers,1);
bookkeeping = zeros(this.numMarkers,2);

for i_marker2D = 1:this.numMarkers
    
    marker2D = this.markers{i_marker2D};
    
    bType = marker2D.blockType;
    block = world.blocks{bType};
    
    assert(~isempty(block), ...
        ['If there is an observed marker, block should have been ' ...
        'instantiated and thus not empty.']);
    
    x_obs{i_marker2D} = marker2D.corners;
    
    fType = marker2D.faceType;
    marker3D = block.getFaceMarker(fType);
    X{i_marker2D} = marker3D.P;
    
    bookkeeping(i_marker2D,:) = [i_marker2D bType fType];
    
end

x_obs = vertcat(x_obs{:});
X = vertcat(X{:});

X = invObsFrame.applyTo(X);

% TODO: integrate the Jacobians as optional outputs from the Camera 
% object's projection function
% [u,v] = camera.projectPoints(P);

[x_hat, dx_dX, dx_dom, dx_dT] = project_points3(X', this.frame.Rvec, ...
    this.frame.T, camera.focalLength, camera.center, ...
    camera.distortionCoeffs, camera.alpha);
      
x_hat = x_hat';

% Sanity checks:
numObs = size(x_obs,1);
assert(isequal(size(x_obs), size(x_hat)), ...
    'x_obs and x_hat should be the same size.');
assert(isequal(size(X), [numObs 3]), 'X should be [numObs 3].');
assert(isequal(size(dx_dX), [2*numObs 3*numObs]), 'dx_dX should be [numObs numObs].');
assert(isequal(size(dx_dom), [2*numObs 3]), 'dx_dom should be [numObs 3].');
assert(isequal(size(dx_dT), [2*numObs 3]), 'dx_dT should be [numObs 3].');

end

            
                 
        