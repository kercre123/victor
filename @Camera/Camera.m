classdef Camera < handle
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        image;
        frame;
        
    end
        
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        focalLength;
                
        center;
        
        distortionCoeffs;
        
        alpha;
        
    end
    
    methods(Access = 'public')
        
        function this = Camera(varargin)
            calibration = [];
            frame = Frame(); %#ok<PROP>
           
            parseVarargin(varargin{:});
            
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
            
            this.frame = frame; %#ok<PROP>
            
        end
        
        function grabFrame(this, usbDevice)
            if nargin < 2
                usbDevice = 0;
            end
            
            this.image = mexCameraCapture(usbDevice);
        end
        
         
    end % METHODS (public)
    
    
end % CLASSDEF Camera