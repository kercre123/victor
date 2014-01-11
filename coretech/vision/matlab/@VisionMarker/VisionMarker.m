classdef VisionMarker
        
    properties(Constant = true)
                    
        ProbeParameters = struct( ...
            'Number', 9, ...          % There will be a Number x Number 2D array of probes
            'WidthFraction', 1/3, ... % As a fraction of square width
            'NumAngles', 8, ...       % How many samples around ring to sample
            'Method', 'mean');         % How to combine points in a probe
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        FiducialPaddingFraction = 1; % as a Fraction of SquareWidth
        
        SquareWidth = 1 / (VisionMarker.ProbeParameters.Number + 2 + 2*VisionMarker.FiducialPaddingFraction);
        
        XProbes = VisionMarker.CreateProbes('X');
        YProbes = VisionMarker.CreateProbes('Y');
        
    end % Constant Properties
    
    methods(Static = true)
        
        img = Draw(varargin);
        
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        probes = CreateProbes(coord);
        
        [code, corners, cornerCode, H] = ProbeImage(img, corners);
        
        function ShowHideAxesToggle(src, ~)
            if strcmp(get(src, 'SelectionType'), 'open')
                h_axes = findobj(src, 'Type', 'axes');
                if strcmp(get(h_axes(1), 'Visible'), 'on')
                    newSetting = 'off';
                else
                    newSetting = 'on';
                end
                set(h_axes, 'Visible', newSetting);
            end
        end
        
    end % Protected Static Methods
    
    properties(SetAccess = 'protected')
     
        code;
        corners;
        cornerCode;
        H;
        
    end % Properties
    
    methods 

        function this = VisionMarker(img, varargin)
            
            [this.code, this.corners, this.cornerCode, this.H] = ...
                VisionMarker.ProbeImage(img, varargin{:});
            
        end % Constructor VisionMarker()
       
        function [h1, h0] = DrawProbes(this, varargin)
            H = cp2tform([0 0 1 1; 0 1 0 1]', this.corners, 'projective');
            
            X = VisionMarker.XProbes(5:end,1);
            Y = VisionMarker.YProbes(5:end,1);
            %temp = this.H*[X(:) Y(:) ones(numel(X),1)]';
            %x = reshape(temp(1,:)./temp(3,:), [], VisionMarker.ProbeParameters.NumAngles+1);
            %y = reshape(temp(2,:)./temp(3,:), [], VisionMarker.ProbeParameters.NumAngles+1);
            [x,y] = tformfwd(H, X, Y);
            
            h1 = plot(x(this.code,:), y(this.code,:), 'r.', 'MarkerSize', 12, varargin{:});
            h0 = plot(x(~this.code,:), y(~this.code,:), 'b.', 'MarkerSize', 12, varargin{:});
        end
        
    end % Public Methods
    
    
end % CLASSDEF Marker2D
