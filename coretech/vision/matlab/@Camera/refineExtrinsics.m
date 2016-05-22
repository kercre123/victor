
function F = refineExtrinsics(camera, F_init, u, v, X, Y, Z)
% Parameters
mu = 0; % 0 --> pure Gauss-Newton

F = F_init;
while 1
    % Transform the 3D points by the current Frame
    [Xp,Yp,Zp] = F.applyTo(X,Y,Z);
    [up,vp] = camera.projectPoints(Xp,Yp,Zp);
    
    r = [u - up; v - vp];
    
    A = J'*J + mu*eye(size(J,2));
    b = -J'*r;
    
    wv_update = A\b;
    
    Fupdate = expSE3(wv_update);
    
    F.update( Fupdate );

end

end

function T = expSE3(wv)

w = wv(1:3);
v = wv(4:6);

theta = norm(w);
if theta<eps
    R = eye(3);
    t = zeros(3,1);
else
    sinTheta = sin(theta);
    cosTheta = cos(theta);
    
    w_skew  = skew_symmetric(w);
    
    R = eye(3) + sinTheta/theta * w_skew + (1-cosTheta)/theta^2 * w_skew * w_skew;
    V = eye(3) + (1-cosTheta)/theta^2 * w_skew + (theta-sinTheta)/theta^3 * w_skew * w_skew;
    t = V*v(:);
end

T = [R t; zeros(1,3) 1];

end


