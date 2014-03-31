function [xi, yi] = getImagePoints(this, varargin)
% Return (x,y) coordinates according to the tracker's current position.

if nargin==2
    i_scale = varargin{1};
    if i_scale < this.finestScale
        warning(['Finest scale available is %d. Returning ' ...
            'that instead of requested scale %d.'], ...
            this.finestScale, i_scale);
        i_scale = this.finestScale;
    end
    
    x = this.xgrid{i_scale};
    y = this.ygrid{i_scale};

elseif nargin == 3
    
    if strcmp(this.tformType, 'planar6dof')
        x = varargin{1};
        y = varargin{2};
    else
        x = varargin{1} - this.xcen;
        y = varargin{2} - this.ycen;
    end
    
else
    error('Expecting i_scale or (x,y).');
end

xi = this.tform(1,1)*x + this.tform(1,2)*y + this.tform(1,3);

yi = this.tform(2,1)*x + this.tform(2,2)*y + this.tform(2,3);

if any(strcmp(this.tformType, {'homography', 'planar6dof'}))
    zi = this.tform(3,1)*x + this.tform(3,2)*y + this.tform(3,3);
    xi = xi ./ zi;
    yi = yi ./ zi;    
end

xi = xi + this.xcen;
yi = yi + this.ycen;

end