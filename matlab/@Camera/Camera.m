classdef Camera < handle
    
    properties(GetAccess = 'protected', Constant = true)
        OPEN = 0;
        GRAB = 1;
        CLOSE = 2;
        
        UseDistortionLUTs = true;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        pose;
        
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        image_;
        
    end
        
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        deviceType;
        deviceID;
        nrows;
        ncols; 
        
        % Calibration parameters:
        focalLength;
        center;
        distortionCoeffs;
        alpha;
        
        xDistortedLUT;
        yDistortedLUT;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
       
        image;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        calibrationMatrix;
        
    end
    
    
    methods(Access = 'public')
        
        function this = Camera(varargin)
            device = [];
            timeStep = 64; % in ms, for webot cameras
            deviceType = 'usb'; %#ok<PROP>
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
            
            this.deviceType = deviceType; %#ok<PROP>
            this.deviceID = device;
            this.nrows = resolution(2);
            this.ncols = resolution(1);
            
            this.focalLength = calibration.fc;
                        
            this.center = calibration.cc;
                        
            this.distortionCoeffs = calibration.kc; 
                        
            this.alpha = calibration.alpha_c;
            
            this.pose = pose; %#ok<PROP>
            
            if Camera.UseDistortionLUTs
                [xgrid,ygrid] = meshgrid(1:this.ncols, 1:this.nrows);
                [this.xDistortedLUT, this.yDistortedLUT] = ...
                    this.distort(xgrid, ygrid, true);
            end
            
            if ~isempty(this.deviceID)
                switch(this.deviceType)
                    case 'usb'
                        % If a USB device was provided, open it.
                        mexCameraCapture(Camera.OPEN, this.deviceID, ...
                            this.ncols, this.nrows);
                    case 'webot'
                        % If a Webot simulatd camera was provided, enable
                        % it.
                        wb_camera_enable(this.deviceID, timeStep);
                    otherwise
                        error('Unrecognized deviceType "%s".', this.deviceType);
                        
                end % SWITCH(deviceType)
            end
            
        end
        
        function out = grabFrame(this)
            switch(this.deviceType)
                case 'usb'
                    this.image = mexCameraCapture(Camera.GRAB, this.deviceID);
                    this.image = this.image(:,:,[3 2 1]); % BGR to RGB
                case 'webot'
                    this.image = wb_camera_get_image(this.deviceID);
                otherwise
                    error('Unrecognized deviceType "%s".', this.deviceType);
                    
            end % SWITCH(deviceType)
            
            if nargout > 0
                out = this.image;
            end
        end
        
        function close(this)
            switch(this.deviceType)
                case 'usb'
                    mexCameraCapture(Camera.CLOSE, this.deviceID);
                case 'webot'
                    wb_camera_disable(this.deviceID);
                otherwise
                    error('Unrecognized deviceType "%s".', this.deviceType);
            end % SWITCH(deviceType)
        end
        
        function delete(this)
            close(this);
        end
        
        % TODO: add calibration function?
         
    end % METHODS (public)
    
    methods % Dependent get/set
        
        function K = get.calibrationMatrix(this)
            % Return calibration parameters in a calibration matrix.
            K = [this.focalLength(1) this.alpha*this.focalLength(1) this.center(1);
                 0 this.focalLength(2) this.center(2);
                 0 0 1];
        end
        
        function I = get.image(this)
            I = this.image_;
        end
        
        function set.image(this, I)
            if ischar(I) 
                if exist(I, 'file')
                    I = imread(I);
                else
                    error('Non-existant image file "%s".', I);
                end
            end
            
            this.image_ = I;
        end
                
    end % METHODS (Dependent get/set)
    
end % CLASSDEF Camera