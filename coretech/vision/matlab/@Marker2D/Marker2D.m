classdef Marker2D
    
    %% Constant Properties
    properties(GetAccess = 'public', Constant = true)
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
    end
    
    %% Abstract Constant Properties
    properties(GetAccess = 'public', Constant = true, Abstract = true)
        % Subclasses must define the layout and IDs!
        % 'O' is a reserved character in the layout, meaning Orientation.
        %  The orientation bits are always the four ends of the "plus sign"
        %  in the square.
        % 'C' is a reserved character for specifying checksum bits.
        Layout;
        IdChars; % cell array
        IdNames; % cell array (just for drawing)
        
        % In sub-classes, set the following using the static methods
        % defined below, by passing in the above Constant properties.
        UpBit, DownBit, LeftBit, RightBit;
        CheckBits;
        IdBits;
        
        % Probe Parameters:
        ProbeGap;    % as a fraction of code square width (one of the bits)
        ProbeRadius; % in number 
        ProbeSigma;
        CodePadding; % as a fraction of inside fiducial width
        
        % In sub-classes, set up the probes using the static functions
        % below, passing in the above Probe Parameters:
        Xprobes;
        Yprobes;
        ProbeWeights;
    end
          
    %% Static Methods
    methods(Static = true, Access = 'protected')
        
        function S = getSize(layout)
           S = size(layout,1);
           assert(S == size(layout,2), 'Layout should be square.');
           assert(mod(S,2)==1, 'Layout size should be odd.');
        end
        
        function up = findUpBit(layout)
            n   = size(layout,1);
            mid = (n+1)/2;
            up  = (mid-1)*n + 1;
            
            assert(layout(up)=='O', ...
                'UpBit should be "O" in the specified Layout.');
        end
        function down = findDownBit(layout)
            n   = size(layout,1);
            mid = (n+1)/2;
            down  = mid*n;
            
            assert(layout(down)=='O', ...
                'DownBit should be "O" in the specified Layout.');
        end
        function left = findLeftBit(layout)
            n   = size(layout,1);
            mid = (n+1)/2;
            left  = mid;
            
            assert(layout(left)=='O', ...
                'LeftBit should be "O" in the specified Layout.');
        end
        
        function right = findRightBit(layout)
            n   = size(layout,1);
            mid = (n+1)/2;
            right = n^2 - mid + 1;
            
            assert(layout(right)=='O', ...
                'RightBit should be "O" in the specified Layout.');
        end
        
        function bits = findCheckBits(layout)
            bits = find(layout == 'C');
        end
        
        function bits = findIdBits(layout, idChars)
            bits = containers.Map('KeyType', 'char', 'ValueType', 'any');
            for i = 1:length(idChars)
                bits(idChars{i}) = find(layout == idChars{i});
            end
        end
        
        w = createProbeWeights(n, probeGap, probeRadius, probeSigma, cropFactor)
        probes = createProbes(dir, n, probeGap, probeRadius, cropFactor)
    end
        
    %% Get-Public Properties
    properties(GetAccess = 'public', SetAccess = 'protected')   
        corners;
        isValid = true;
        ids;
        threshold;
        means;
        
        upDirection;
        
        % angle needed to rotate given corners so that top side is "up" (in
        % image coordinate frame -- upper left is (0,0))
        upAngle; 
    end
    
    %% Dependent Properties
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numIDs;
        origin;
        unorderedCorners;
    end
    
    %% Protected Properties
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        reorderCorners;
        handles;
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end    
     
    %% Abstract Static Methods
    methods(Abstract = true, Static = true)
        % Each subclass must implement a method for computing a checksum
        % from a binaryCode input.
        checksum = computeChecksum(binaryCode);
        
        % Each subclas must implement a method for encoding IDs into a
        % 2D binary code. length(varargin) should == numIDs
        binaryCode = encodeIDs(varargin);
    end
    
    %% Public Methods
    methods(Access = 'public')
        
        function this = Marker2D(img, corners_, tform, varargin)
            
            % Note that corners are provided in the following order:
            %   1. Upper left
            %   2. Lower left
            %   3. Upper right
            %   4. Lower right
            this.corners = corners_;
            
            if ~isempty(varargin) && strcmpi(varargin{1}, 'ExplictInput')
                ids = [];
                upAngle = -Inf;
                
                parseVarargin(varargin{2:end});
                
                this.ids = ids;
                this.upAngle = upAngle;
                
                [~,this.upDirection] = min(abs(this.upAngle - [0 pi pi/2 3*pi/2]));
                                
                this.isValid = 1;
                            
            else
                this.ids = zeros(1, this.numIDs);

                this.means = probeMeans(this, img, tform);
                [this, binaryString] = orientAndThreshold(this, this.means);
                if this.isValid
                    this = decodeIDs(this, binaryString);
                end
            end
            
        end % CONSTRUCTOR Marker2D()
        
        function thisTformed = applytform(this, T)
            thisTformed = this;
            [x,y] = tformfwd(T, this.corners(:,1), this.corners(:,2));
            thisTformed.corners = [x(:) y(:)];
            warning('Applying transform to Marker2D does not update upAngle!');
        end  
        
        function thisRotated = rotate(this, angle, center)
            thisRotated = this;
            R = [cos(angle) -sin(angle); sin(angle) cos(angle)];
            center = center(ones(4,1),:);
            thisRotated.corners = (R*(thisRotated.corners - center)')' + center;
            thisRotated.upAngle = thisRotated.upAngle + angle;
        end
        
        function updated = updateCorners(this, newCorners)
            assert(isequal(size(newCorners), size(this.corners)), ...
                'Specified corners should be %dx%d.', ...
                size(newCorners,1), size(newCorners,2));
            
            updated = this;
            updated.corners = newCorners(this.reorderCorners,:);
        end
        
        
    end 
    
    %% Dependent Get/Set Methods
    methods
        function n = get.numIDs(this)
            n = length(this.IdChars);
        end    
        
        function o = get.origin(this)
            o = this.corners(1,:);
        end
        
        function c = get.unorderedCorners(this)
            c = zeros(4,2);
            c(this.reorderCorners,:) = this.corners;
        end
            
    end
    
end % CLASSDEF Marker2D