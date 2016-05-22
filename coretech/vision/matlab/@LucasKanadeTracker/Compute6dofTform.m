function tform = Compute6dofTform(this, R)

if nargin < 2
    cx = cos(this.theta_x);
    cy = cos(this.theta_y);
    cz = cos(this.theta_z);
    
    sx = sin(this.theta_x);
    sy = sin(this.theta_y);
    sz = sin(this.theta_z);
    
    r11 = cy*cz;
    r12 = cx*sz + sx*sy*cz;
    %r13 = sx*sz - cx*sy*cz;
    r21 = -cy*sz;
    r22 = cx*cz - sx*sy*sz;
    %r23 = sx*cz + cx*sy*sz;
    r31 = sy;
    r32 = -sx*cy;
    %r33 = cx*cy;
    
else
    r11 = R(1,1);
    r12 = R(1,2);
    r21 = R(2,1);
    r22 = R(2,2);
    r31 = R(3,1);
    r32 = R(3,2);
end

tform = [this.K(1,1)*[r11 r12 this.tx];
    this.K(2,2)*[r21 r22 this.ty];
    [r31 r32 this.tz]];

end % Compute6dofTform()