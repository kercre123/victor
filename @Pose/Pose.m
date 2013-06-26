classdef Pose
    
    properties(GetAccess = 'public', SetAccess = 'protected')
       
        Rmat;
        Rvec;
        T;
        
        % Derivative of the elements of the rotation matrix w.r.t. the
        % elements of the rotation vector
        dRmat_dRvec;
        dT;
        
    end
        
    methods(Access = 'public')
        
        function this = Pose(Rin,Tin)
            
            if nargin==0
                %this.Rmat = eye(3);
                this.Rvec = zeros(3,1);
                [this.Rmat, this.dRmat_dRvec] = rodrigues(this.Rvec);
                this.T = zeros(3,1);
            else
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
            end
        end % FUNCTION Frame() [Constructor]
        
        function P = inv(this)
            P = Pose(this.Rmat', -this.Rmat'*this.T);
        end
        
        
    end % METHODS (public)
    
end %CLASSDEF Pose