classdef Camera < handle
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        image;
        
    end
        
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        focalLength;
                
        center;
        
        distortionCoeffs;
        
        alpha;
        
        frame_c2w; % _coordinate_ frame, not image!
        frame_w2c;
    end
        
    methods(Access = 'public')
        
        function this = Camera(calibration)
           
            if ~isstruct(calibration)
                if ~exist(calibration, 'file')
                    error('Specified calibration file does not exist.');
                end
                
                calibration = load(calibration);
            end
            
            this.focalLength = calibration.fc;
                        
            this.center = calibration.cc;
                        
            this.distortionCoeffs = calibration.kc;            
            
            this.alpha = calibration.alpha_c;
            
            % Camera to world frame:
            %  Rotation -90 degrees around x axis:
            Rcw = [1 0 0; 0 0 1; 0 -1 0];
            this.frame_c2w = Frame(Rcw, zeros(3,1));
            this.frame_w2c = inv(this.frame_c2w);
        end
        
        function grabFrame(this, usbDevice)
            if nargin < 2
                usbDevice = 0;
            end
            
            this.image = mexCameraCapture(usbDevice);
        end
        
         
    end % METHODS (public)
    
end % CLASSDEF Camera