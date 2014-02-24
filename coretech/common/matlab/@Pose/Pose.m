classdef Pose < handle
    
    properties
        name;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
       
        Rmat;
        Rvec;
        T;
        
        sigma; % 6x6 covariance matrix
        
        % Derivative of the elements of the rotation matrix w.r.t. the
        % elements of the rotation vector
        dRmat_dRvec;
        dT;
        
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        parent_;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        angle;
        axis;
        treeDepth;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        parent;
    end
    
    
    methods(Access = 'public')
        
        function this = Pose(varargin)
            
            this.name = '';
            this.parent = [];
                        
            switch(nargin)
                case 0
                    this.updateRotation(zeros(3,1));
                    this.updateTranslation(zeros(3,1));
                    
                case {2,3}
                    Rin = varargin{1};
                    Tin = varargin{2};
                    
                    this.updateTranslation(Tin);
                    this.updateRotation(Rin);
                    
                    if nargin==3 && ~isempty(varargin{3})
                        Cin = varargin{3};
                        this.updateCovariance(Cin);
                    end
                        
                otherwise
                    error('Unrecognized number of arguments for constructing a Pose object.');
                    
            end % SWITCH(nargin)
            
        end % FUNCTION Frame() [Constructor]
        
        function P = inv(this)
            % Is this the right thing to do with covariance?
            P = Pose(this.Rmat', -this.Rmat'*this.T, this.sigma);
        end
        
        function update(this, Rnew, Tnew, covNew)
           % Update R and T (and optionally, covariance) without changing 
           % pose tree position (parent/treedepth)
           this.updateRotation(Rnew);
           this.updateTranslation(Tnew);
           
           if nargin > 3 && ~isempty(covNew)
               this.updateCovariance(covNew);
           end
        end
        
    end % METHODS (public)
    
    methods(Access = 'protected')
        % TODO: I could convert all these "udpate" functions to dependent
        % "set" functions...
        function updateRotation(this, Rin)
            if isvector(Rin)
                assert(length(Rin)==3, ...
                    'Rotation vector should have 3 elements.');
                                
                [this.Rmat, this.dRmat_dRvec] = rodrigues(Rin);
                this.Rvec = rodrigues(this.Rmat);
                
            elseif ismatrix(Rin)
                assert(isequal(size(Rin), [3 3]), ...
                    'Rotation matrix should be 3x3.');
                this.Rvec = rodrigues(Rin);
                [this.Rmat, this.dRmat_dRvec] = rodrigues(this.Rvec);
                
            else
                error('Unrecognized form for rotation.');
            end
        end % FUNCTION updateRotation()
        
        function updateTranslation(this, Tin)
            assert(isvector(Tin) && length(Tin)==3, ...
                'T should be a 3-element vector.');
            
            this.T = Tin(:);
        end % FUNCTION updateTranslation()
        
        function updateCovariance(this, Cin)
            assert(ismatrix(Cin) && size(Cin,1)==6 && ...
                size(Cin,2)==6, 'Expecting 6x6 covariance matrix.');
            
            this.sigma = Cin;
        end % FUNCTION updateCovariance()
        
    end % METHODS (protected)
    
    methods(Static = true)
        function isRoot = isRootPose(P)
           isRoot = isempty(P) || any(strcmpi(P, {'World', 'Root'}));
        end
    end
    
    methods
        function theta = get.angle(this)
            % Return angles on the interval (-pi,+pi]
            theta = norm(this.Rvec);
            if theta > pi
                theta = theta - 2*pi;
            end
            assert(theta > -pi && theta <= pi, ...
                'Angle should be on interval (-pi,+pi].');
        end
        
        function v = get.axis(this)
            theta = this.angle;
            if abs(theta) > 0
                v = this.Rvec / theta;
            else
                v = [1 0 0]';
            end
        end
        
        function set.parent(this, P)
            if Pose.isRootPose(P)
                P = [];
                %this.treeDepth = 0;
            else
                %this.treeDepth = P.treeDepth + 1;
            end
            this.parent_ = P;
        end
        
        function depth = get.treeDepth(this)
            depth = 1;
            nextParent = this.parent;
            while ~isempty(nextParent)
                nextParent = nextParent.parent;
                depth = depth + 1;
            end
        end
        
        function p = get.parent(this)
           p = this.parent_; 
        end
    end
    
end %CLASSDEF Pose