classdef Quaternion
% Quaternion class

% See also:
%  https://bitbucket.org/sinbad/ogre/src/0bba4f7cdb953c083aa9e3e15a13857e0988d48a/OgreMain/src/OgreQuaternion.cpp?at=default
%  http://code.google.com/p/anima-animation-sample/source/browse/trunk/math.cpp

    properties
        q = zeros(4,1);
    end
    
    properties(Dependent = true)
        %a,b,c,d;
        r, v;
        angle;
        axis;
    end
    
    methods(Access = public)
        function Q = Quaternion(varargin)
            
            switch(nargin)
                case 1
                    if isa(varargin{1}, 'Quaternion')
                        Q.q = varargin{1}.q;
                    elseif isvector(varargin{1}) && length(varargin{1})==4
                        Q.q = varargin{1}(:);
                    elseif ismatrix(varargin{1}) && all(size(varargin{1})==3)
                        % Rotation matrix
                        angleAxis = rodrigues(varargin{1});
                        angle = norm(angleAxis);
                        axis  = angleAxis/angle;
                        Q = Quaternion(angle, axis);
                        
                    else 
                        error('Unrecognized inputs for Quaternion constructor.');
                    end
                    
                case 2
                    assert(isscalar(varargin{1}) && ...
                        isvector(varargin{2}) && length(varargin{2})==3, ...
                        'Expecting scalar angle and 3-element axis.');
                    
                    halfAngle = varargin{1}/2;
                    axis  = varargin{2};
                    
                    Q.q(1) = cos(halfAngle);
                    Q.q(2:4) = sin(halfAngle)*axis(:);
                    
                otherwise
                    error('Unrecognized inputs for Quaternion constructor.');
                    
            end % SWITCH(nargin)
                        
        end % CONSTRUCTOR Quaternion()
        
        function Qconj = conj(Q)
            Qconj = Quaternion([Q.q(1); -Q.q(2:4)]);
        end
        
        function n = norm(Q)
            n = norm(Q.q);
        end
       
        function d = dot(this, other)
            assert(isa(other, 'Quaternion'), ...
                'Expecting another Quaternion to compute dot product.');
            
            d = sum(this.q .* other.q);
        end
        
        function Qnorm = normalize(Q)
            n = norm(Q);
            if n > 0
                denom = n;
            else
                denom = 1;
            end
            
            Qnorm = Quaternion(Q.q / denom);
        end
        
        function Qmean = unitMean(Q, varargin)
            assert(all(cellfun(@(x)isa(x,'Quaternion'), varargin)), ...
                'All inputs should be Quaternion objects.');
            
            M = Q.q * Q.q';
            for i = 1:length(varargin)
                M = M + (varargin{i}.q*varargin{i}.q');
            end
            
            [maxEvector, temp] = eigs(M, 1, 'LM'); %#ok<NASGU>
            Qmean = Quaternion(maxEvector);
        end
    end
    
    methods
        function r = get.r(this)
            r = this.q(1);
        end
        function v = get.v(this)
            v = this.q(2:4);
        end
        function theta = get.angle(this)
            assert(abs(norm(this) - 1) < 10*eps, ...
                'Quaternion should be Unit for computing rotation angle.');
            assert(abs(this.q(1))<=1, 'Expecting |q1| <= 1.');
            
            theta = 2*acos(this.q(1));
        end
        function vec = get.axis(this)
            assert(abs(norm(this) - 1) < 10*eps, ...
                'Quaternion should be Unit for computing rotation axis.');
            vec = this.q(2:4)/norm(this.q(2:4));
        end
    end
    
end % CLASSDEF Quaternion