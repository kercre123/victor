classdef UnitQuaternion < Quaternion
    
    methods
        function this = UnitQuaternion(varargin)
            
            switch(nargin)
                case 1
                    q = varargin{1};
                    
                case 2
                    % Create a unit quaternion from an angle, theta, and an 
                    % axis, w
                    theta = varargin{1};
                    w     = varargin{2};
                    
                    assert(isscalar(theta) && isvector(w) && length(w)==3, ...
                        'Expecting scalar angle 3-element vector axis.');
                    
                    wLength = norm(w);
                    if wLength ~= 1
                        warning('Given axis was not a unit vector. Normalizing.');
                        w = w/wLength;
                    end
                    
                    q = [cos(theta/2); sin(theta/2)*w(:)];
            end
                        
            this = this@Quaternion(q);
                    
            % Make sure we're unit length
            this.q = this.q / norm(this);
            
        end % CONSTRUCTOR UnitQuaternion()
    end
    
    methods(Static = true)
        Qi = LinearBlend(P,Q,t); % QLB
        Qi = SphericalLinearInterp(P,Q,t); % SLERP
    end
    
end % CLASSDEF UnitQuaternion