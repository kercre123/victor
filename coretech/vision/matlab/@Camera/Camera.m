classdef Camera < handle
    
    properties(GetAccess = 'protected', Constant = true)
        OPEN = 0;
        GRAB = 1;
        CLOSE = 2;
        
        % Resolution (as in downsample factor) of the lookup tables for
        % radial distortion (use 0 to disable the use LUTs and just compute
        % the distorted coordinates dynamically, 1 to get full size tables,
        % 2 to get halfsize tables, etc.)  Must be integer.
        DistortionLUTres = 20;
        
        % For creating blurry simulated images
        WebotBlurKernel = fspecial('gaussian', 5, 3);
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
        DistortionLUT_xCoords;
        DistortionLUT_yCoords;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
       
        image;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        calibrationMatrix;
        FOV_vertical, FOV_horizontal;
        
    end
    
    
    methods(Access = 'public')
        
        function this = Camera(varargin)
            % Constructs a calibrated Camera object.
            %
            % cam = Camera(<Prop1, val1>, <Prop2, val2>, ...)
            % 
            %   A Camera object knows how to take images (from a USB or
            %   Webot simulated camera), knows its pose in the world, and
            %   knows its calibration parameters.
            %
            %   Properties available to set at construction include (with
            %   defaults in []):
            % 
            %   'device', [<empty>]
            %     The USB device number or Webot camera handle/tag.
            %
            %   'deviceType', ['usb']
            %     Either 'usb' or 'webot', to specify which kind of camera
            %     is being used.
            %
            %   'timeStep', [64]
            %     For Webot cameras, in milliseconds.
            %
            %   'resolution', [640 480]
            %     The size of the images the camera takes.
            %
            %   'calibration' [<struct,seebelow>]
            %     A struct containing fields corresponding to Bouguet's
            %     calibration naming convention:
            %      'fc', [1000 1000], x and y focal length
            %      'cc', [resolution/2], x and y camera center
            %      'kc', [zeros(5,1)], radial distortion coefficients
            %      'alpha_c', [0], skew
            %
            %   'Pose', [Pose()]
            %     A Pose object reporesenting the camera's coordinate
            %     frame.
            %
            % ------------
            % Andrew Stein
            %
            
            device = [];
            timeStep = 30; % in ms, for webot cameras
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
            
            if Camera.DistortionLUTres > 0
                this.DistortionLUT_xCoords = linspace(1, this.ncols, ...
                    ceil(this.ncols/Camera.DistortionLUTres));
                this.DistortionLUT_yCoords = linspace(1, this.nrows, ...
                    ceil(this.nrows/Camera.DistortionLUTres));
                [xgrid,ygrid] = meshgrid( ...
                    this.DistortionLUT_xCoords, this.DistortionLUT_yCoords);
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
            assert(~isempty(this.deviceID), ...
                'This Camera has no device and thus cannot grab frames.');
            
            switch(this.deviceType)
                case 'usb'
                    this.image = mexCameraCapture(Camera.GRAB, this.deviceID);
                    this.image = this.image(:,:,[3 2 1]); % BGR to RGB
                case 'webot'
                    this.image = wb_camera_get_image(this.deviceID);
                    
                    if ~isempty(Camera.WebotBlurKernel)
                        this.image = imfilter(this.image, ...
                            Camera.WebotBlurKernel, 'replicate');
                    end
                    
                otherwise
                    error('Unrecognized deviceType "%s".', this.deviceType);
                    
            end % SWITCH(deviceType)
            
            if nargout > 0
                out = this.image;
            end
        end
        
        function close(this)
            if ~isempty(this.deviceID)
                switch(this.deviceType)
                    case 'usb'
                        mexCameraCapture(Camera.CLOSE, this.deviceID);
                    case 'webot'
                        wb_camera_disable(this.deviceID);
                    otherwise
                        error('Unrecognized deviceType "%s".', this.deviceType);
                end % SWITCH(deviceType)
            end
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
        
        function fov = get.FOV_vertical(this)
            fov = 2*atan2(this.nrows, 2*this.focalLength(2));
        end
        
        function fov = get.FOV_horizontal(this)
            aspect = this.ncols / this.nrows;
            fov = aspect * this.FOV_vertical;
        end
                
    end % METHODS (Dependent get/set)
    
end % CLASSDEF Camera