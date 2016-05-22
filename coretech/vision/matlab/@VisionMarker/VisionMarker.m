classdef VisionMarker
        
    properties(Constant = true)
                    
        ProbeParameters = struct( ...
            'Number', 9, ...          % There will be a Number x Number 2D array of probes
            'WidthFraction', 1/3, ... % As a fraction of square width
            'NumAngles', 8, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        FiducialPaddingFraction = 1; % as a Fraction of SquareWidth
        
        SquareWidth = 1 / (VisionMarker.ProbeParameters.Number + 2 + 2*VisionMarker.FiducialPaddingFraction);
        
        FiducialInnerArea = (1-2*VisionMarker.SquareWidth)^2;
        FiducialArea = 1 - VisionMarker.FiducialInnerArea;
                
        XProbes = VisionMarker.CreateProbes('X');
        YProbes = VisionMarker.CreateProbes('Y');
        
        % X-Z Plane: 
        Canonical3dCorners = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
        % X-Y Plane: Canonical3dCorners = [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
        
    end % Constant Properties
    
    methods(Static = true)
        
        img = DrawFiducial(varargin);
        
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        probes = CreateProbes(coord);
        
        [code, corners, cornerCode, H] = ProbeImage(img, corners, doRefinement);
        
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
        
        name;
        pose;
        size;
        
        isValid;
        
    end % Properties
    
    properties(SetAccess = 'protected', Dependent = true)
        
        byteArray;
        
    end % Dependent, protected properties
            
    methods 

        function this = VisionMarker(img, varargin)
            
            Corners = [];
            Pose = [];
            Name = '';
            Size = 1;
            RefineCorners = true;
            
            parseVarargin(varargin{:});
            
            [this.code, this.corners, this.cornerCode, this.H] = ...
                VisionMarker.ProbeImage(img, Corners, RefineCorners);
            
            this.isValid = ~isempty(this.code);
            
            this.pose = Pose;
            this.name = Name;
            this.size = Size;
            
        end % Constructor VisionMarker()
       
        h = Draw(this, varargin);
        
        % For backwards compatibility to lowercase "draw" in Marker2D
        function h = draw(this, varargin)
            where = [];
            
            DrawArgs = parseVarargin(varargin{:});
            
            h = Draw(this, 'Parent', where, DrawArgs{:});
        end
        
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
        
        function pos3d = GetPosition(this, wrtPose)
           
            % A square with corners (+/- 1, +/- 1):
            canonicalSquare = this.size/2 * VisionMarker.Canonical3dCorners;
            if nargin < 2
                P = this.pose;
            else
                P = this.pose.getWithRespectTo(wrtPose);
            end
            
            pos3d = P.applyTo(canonicalSquare);
            
        end
        
        function b = get.byteArray(this)
            if this.isValid
                temp = [this.code(:); this.cornerCode(:)];
            else
                temp = zeros(VisionMarker.ProbeParameters.Number^2 + 4, 1);
            end
            numBytes = ceil(length(temp)/8);
            temp = [temp; zeros(numBytes*8-length(temp),1)];
            b = uint8(bin2dec(num2str(reshape(temp, [], 8))));
        end
        
    end % Public Methods
    
    methods(Static = true)
        
        
        outputString = GenerateEmbeddedMarkerDefinitionCode(varargin);
        
    end % Static Methods
    
end % CLASSDEF Marker2D
