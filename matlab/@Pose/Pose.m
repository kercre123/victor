classdef Pose
    
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
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        angle;
        axis;
        
    end
        
    methods(Access = 'public')
        
        function this = Pose(varargin)
            
            switch(nargin)
                case 0
                    %this.Rmat = eye(3);
                    this.Rvec = zeros(3,1);
                    [this.Rmat, this.dRmat_dRvec] = rodrigues(this.Rvec);
                    this.T = zeros(3,1);
                    
                case {2,3}
                    Rin = varargin{1};
                    Tin = varargin{2};
                    assert(isvector(Tin) && length(Tin)==3, ...
                        'T should be a 3-element vector.');
                    
                    this.T = Tin(:);
                    
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
                    
                    if nargin==3
                        this.sigma = varargin{3};
                        assert(ismatrix(this.sigma) && size(this.sigma,1)==6 && ...
                            size(this.sigma,2)==6, 'Expecting 6x6 covariance matrix.');
                    end
                        
                otherwise
                    error('Unrecognized number of arguments for constructing a Pose object.');
                    
            end % SWITCH(nargin)
            
            if isempty(this.sigma)
                % TODO: Is a default of infinitite uncertainty what we want?
                this.sigma = diag(inf(1,6));
            end
            
        end % FUNCTION Frame() [Constructor]
        
        function P = inv(this)
            P = Pose(this.Rmat', -this.Rmat'*this.T);
        end
        
        
    end % METHODS (public)
    
    methods
        function theta = get.angle(this)
            theta = norm(this.Rvec);
        end
        
        function v = get.axis(this)
            v = this.Rvec / this.angle;
        end
    end
    
end %CLASSDEF Pose