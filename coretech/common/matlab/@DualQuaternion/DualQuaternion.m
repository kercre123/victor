classdef DualQuaternion
    
    properties
        real; % real or rotation quaternion
        dual; % dual or displacement quaternion
    end
    
    properties(Dependent = true, SetAccess = 'protected')
        rotationVector;
        rotationMatrix;
        translation;

        conj;
        inv;
    end
    
    methods(Access = public)
        
        function this = DualQuaternion(varargin)
            switch(nargin)
                case 0
                    this.real = Quaternion([0 0 0 1]);
                    this.dual = Quaternion([0 0 0 0]);
                    
                case 1
                    assert(isa(varargin{1}, 'Pose'), ...
                        'Expecting a Pose object.');
                    
                    P = varargin{1};
                    this = DualQuaternion(Quaternion(P.angle, P.axis), P.T);
                    
                    assert(sqrt(sum( (this.translation - P.T).^2)) < 1e-9, ...
                        'Translation of DualQuaternion does not match Pose translation.');
                    
                case 2
                    if isa(varargin{1}, 'Quaternion') && ...
                            isa(varargin{2}, 'Quaternion')
                        this.real = varargin{1};
                        this.dual = varargin{2};
                    elseif isa(varargin{1}, 'Quaternion') && isvector(varargin{2})
                        rotation = varargin{1};
                        translation = varargin{2};
                        
                        assert(length(translation)==3, ...
                            'Expecting 3-element translation vector.');
                        
                        tx = translation(1);
                        ty = translation(2);
                        tz = translation(3);
                        
                        qw = rotation.q(1);
                        qx = rotation.q(2);
                        qy = rotation.q(3);
                        qz = rotation.q(4);                        
                        
                        this.real = varargin{1}; % Rotation quaternion
                        this.dual = Quaternion( ...
                            0.5*[-tx*qx - ty*qy - tz*qz;
                                  tx*qw + ty*qz - tz*qy;
                                 -tx*qz + ty*qw + tz*qx; 
                                  tx*qy - ty*qx + tz*qw] );
                        
                    else
                        error('Unrecognized inputs for DualQuaternion constructor.');
                        
                    end
                otherwise
                    error('Unrecognized inputs for DualQuaternion constructor.');
            end % SWITCH(nargin)
                            
        end % CONSTRUCTOR DualQuaternion() 
        
        %{
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
                            this.Qr = Quaternion(this.Qr);
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
                        this.Qr = Quaternion([cos(theta/2); sin(theta/2)*w(:)]);
                        
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
        %}
        
        function Q = normalize(this)
            mag = norm(this.real);
            if mag > 0
                realN = (1/mag)*this.real;
                dualN = (1/mag)*this.dual;
                Q = DualQuaternion(realN, dualN - realN*dot(realN,dualN));
            else
                Q = this;
            end
        end
        
        function Qmean = mean(Q, varargin)
            reals = cellfun(@(x)x.real, varargin, 'UniformOutput', false);
            %duals = cellfun(@(x)x.dual, varargin, 'UniformOutput', false);
            rotation = unitMean(Q.real, reals{:});
            %dualMean = mean(Q.dual, duals{:});
                                          
            trans = mean(cellfun(@(x)x.translation, varargin),2);
            tx = trans(1);  ty = trans(2);  tz = trans(3);
            
            qw = rotation.q(1);
            qx = rotation.q(2);
            qy = rotation.q(3);
            qz = rotation.q(4);
            dualMean = Quaternion( ...
                0.5*[-tx*qx - ty*qy - tz*qz;
                      tx*qw + ty*qz - tz*qy;
                     -tx*qz + ty*qw + tz*qx;
                      tx*qy - ty*qx + tz*qw] );
                  
            Qmean = DualQuaternion(rotation, dualMean);
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
            q = this.real.q;
            w = q(1);
            x = q(2);
            y = q(3);
            z = q(4);
            
            R = [(1 - 2*(y*y + z*z))  2*(x*y - w*z)        2*(x*z + w*y);
                 2*(x*y + w*z)        (1 - 2*(x*x + z*z))  2*(y*z - w*x);
                 2*(x*z - w*y)        2*(y*z + w*x)        (1 - 2*(x*x + y*y))];
        end
        
        function Rvec = get.rotationVector(this)
            vec = this.real.q(1:3)/norm(this.real.q(1:3));
            theta = 2*acos(this.real.q(4));
            Rvec = theta*vec;
        end
        
        function t = get.translation(this)
            qr = this.real.q;
            qd = this.dual.q;
            wr = qr(1); xr = qr(2); yr = qr(3); zr = qr(4);
            wd = qd(1); xd = qd(2); yd = qd(3); zd = qd(4);
            
            % Return a translation vector
            t = 2*[-wd*xr + xd*wr - yd*zr + zd*yr;
                   -wd*yr + xd*zr + yd*wr - zd*xr;
                   -wd*zr - xd*yr + yd*xr + zd*wr];
        end
        
        function C = get.conj(this)
            % TODO: cache so that this computes only once?
            C = DualQuaternion(conj(this.Qr), conj(this.Qd));
        end
        
        function thisInv = get.inv(this)
            % Is this correct?  Copied from:
            % https://github.com/gravitino/dualQuaternion/blob/master/quaternion.cpp
            
            dot1 = dot(this.real, this.real);
            dot2 = dot(this.real, this.dual);
            
            newReal = (1/dot1) * this.real.conj;
            newDual = (1/dot1) * this.dual.conj + (-2*dot2/dot1)*newReal;
            
            thisInv = DualQuaternion(newReal, newDual);
        end
        
    end
    
end % CLASSDEF DualQuaternion