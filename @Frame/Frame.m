classdef Frame
    
    properties(GetAccess = 'public', SetAccess = 'protected')
       
        Rmat;
        Rvec;
        T;
        
    end
        
    methods(Access = 'public')
        
        function this = Frame(Rin,Tin)
            
            if nargin==0
                this.Rmat = eye(3);
                this.Rvec = rodrigues(this.Rmat);
                this.T = zeros(3,1);
            else
                assert(isvector(Tin) && length(Tin)==3, ...
                    'T should be a 3-element vector.');
                
                this.T = Tin(:);
                
                if isvector(Rin)
                    this.Rvec = Rin;
                    this.Rmat = rodrigues(Rin);
                    
                elseif ismatrix(Rin)
                    assert(isequal(size(Rin), [3 3]), ...
                        'Rotation matrix should be 3x3.');
                    this.Rmat = Rin;
                    this.Rvec = rodrigues(Rin);
                   
                else
                    error('Unrecognized form for rotation.');
                end
            end
        end % FUNCTION Frame() [Constructor]
        
        function F = inv(this)
            F = Frame(this.Rmat', -this.T);
        end
        
        
    end % METHODS (public)
    
end %CLASSDEF Frame