classdef DualQuaternion
    
    properties
        Qr; % real or rotation quaternion
        Qd; % dual or displacement quaternion
    end
    
    properties(Dependent = true, SetAccess = 'protected')
        %rotationVector;
        rotationMatrix;
        translation;

        conj;
    end
    
    methods(Access = public)
        
        function this = DualQuaternion(varargin)
            switch(nargin)
                case 2
                    if isa(varargin{2}, 'Quaternion')
                        this.Qd = varargin{2};
                        
                        if isa(varargin{1}, 'UnitQuaternion') 
                            this.Qr = varargin{1};
                        elseif isa(varargin{1}, 'Quaternion')
                            this.Qr = varargin{1};
                            mag = norm(this.Qr);
                            this.Qr = UnitQuaternion(this.Qr);
                            this.Qd = 1/mag * this.Qd;
                        else
                            error('Unrecognized inputs for constructing a DualQuaternion.');
                        end
                        
                    elseif isvector(varargin{1}) && length(varargin{1})==3 && ...
                            isvector(varargin{2}) && length(varargin{2})==3
                        % Angle/axis rotation vector + translation vector
                        theta = norm(varargin{1});
                        w     = varargin{1}/theta;
                        t     = varargin{2};
                        this.Qr = UnitQuaternion([cos(theta/2); sin(theta/2)*w(:)]);
                        
                        % Note the cast to Quaternion, to ensure that the
                        % product does not get normalized to another
                        % UnitQuaternion
                        this.Qd = .5*Quaternion([0;t(:)])*Quaternion(this.Qr.q);
                        
                    else
                        error(['Unrecognized input arguments for ' ...
                            'constructing a DualQuaternion.']);
                    end
                    
                otherwise
                    error(['Unrecognized number of input arguments for ' ...
                        'constructing a DualQuaternion.']);
            end
            
           
        end
        
        function Q = normalize(this)
            mag = norm(this.Qr);
            Q = DualQuaternion(1/mag * this.Qr, 1/mag * this.Qd);
        end
        
        
%         function tf = isUnit(this)
%             M = this * this.conj;
%             
%             tf = norm(M.Qr) == 1 && ...
%                 (conj(this.Qr)*this.Qd + conj(this.Qd)*this.Qr) == 0;
%         end
    end % METHODS Public
    
    methods(Static = true)
        Qi = LinearBlend(P,Q,t); % Dual Linear Blending
    end
    
    methods
%         function R = get.rotationVector(this)
%             error('Not correct yet.');
%             % Return a rotation vector (angle * unit axis)
%             R = this.Qr.q(1)*this.Qr.q(2:4);
%         end
        
        function R = get.rotationMatrix(this)
            q = this.Qr.q;
            w = q(1);
            x = q(2);
            y = q(3);
            z = q(4);
            
            R = [(1 - 2*y*y - 2*z*z)  (2*x*y - 2*w*z)      (2*x*z + 2*w*y);
                 (2*x*y + 2*w*z)      (1 - 2*x*x - 2*z*z)  (2*y*z - 2*w*x);
                 (2*x*z - 2*w*y)      (2*y*z + 2*w*x)      (1 - 2*x*x - 2*y*y)];
        end
        
        function t = get.translation(this)
            qr = this.Qr.q;
            qd = this.Qd.q;
            wr = qr(1); xr = qr(2); yr = qr(3); zr = qr(4);
            wd = qd(1); xd = qd(2); yd = qd(3); zd = qd(4);
            
            % Return a translation vector
            t = 2*[-wd*xr + xd*wr - yd*zr + zd*yr;
                   -wd*yr + xd*zr + yd*wr - zd*xr;
                   -wd*zr - xd*yr + yd*xr + zd*wr];
        end
        
        function C = get.conj(this)
            C = DualQuaternion(conj(this.Qr), conj(this.Qd));
        end
        
    end
    
end % CLASSDEF DualQuaternion