classdef Camera < handle
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        image;
        frame;
        
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
    
    methods(Access = 'public')
        
        function this = Camera(varargin)
            device = [];
            resolution = [640 480];
            calibration = [];
            frame = Frame(); %#ok<PROP>
           
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
            
            this.frame = frame; %#ok<PROP>
            
            if ~isempty(this.usbDevice)
                mexCameraCapture(this.usbDevice, this.ncols, this.nrows);
            end
            
        end
        
        function grabFrame(this)
            this.image = mexCameraCapture;
            this.image = this.image(:,:,[3 2 1]); % BGR to RGB
        end
        
         
    end % METHODS (public)
    
    
end % CLASSDEF Camera