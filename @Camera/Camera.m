classdef Camera < handle
    
    properties(GetAccess = 'protected', Constant = true)
        OPEN = 0;
        GRAB = 1;
        CLOSE = 2;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        image;
        pose;
        
    end
        
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        usbDevice;
        nrows;
        ncols; 
        
        focalLength;
                
        center;
        
        distortionCoeffs;
        
        alpha;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        calibrationMatrix;
        
    end
    
    
    methods(Access = 'public')
        
        function this = Camera(varargin)
            device = [];
            resolution = [640 480];
            calibration = struct('fc', [1000 1000], ...
                'cc', resolution/2, 'kc', zeros(5,1), 'alpha_c', 0);
            pose = Pose(); %#ok<PROP>
           
            parseVarargin(varargin{:});
            
            if ~isstruct(calibration)
                if ~exist(calibration, 'file')
                    error('Specified calibration file does not exist.');
                end
                
                calibration = load(calibration);
            end
            
            this.usbDevice = device;
            this.nrows = resolution(2);
            this.ncols = resolution(1);
            
            this.focalLength = calibration.fc;
                        
            this.center = calibration.cc;
                        
            this.distortionCoeffs = calibration.kc; 
                        
            this.alpha = calibration.alpha_c;
            
            this.pose = pose; %#ok<PROP>
            
            if ~isempty(this.usbDevice)
                mexCameraCapture(Camera.OPEN, this.usbDevice, ...
                    this.ncols, this.nrows);
            end
            
        end
        
        function out = grabFrame(this)
            this.image = mexCameraCapture(Camera.GRAB, this.usbDevice);
            this.image = this.image(:,:,[3 2 1]); % BGR to RGB
            if nargout > 0
                out = this.image;
            end
        end
        
        function close(this)
            mexCameraCapture(Camera.CLOSE, this.usbDevice);
        end
        
         
    end % METHODS (public)
    
    methods % Dependent get/set
        
        function K = get.calibrationMatrix(this)
            K = [this.focalLength(1) this.alpha*this.focalLength(1) this.center(1);
                 0 this.focalLength(2) this.center(2);
                 0 0 1];
        end
                
    end % METHODS (Dependent get/set)
    
end % CLASSDEF Camera