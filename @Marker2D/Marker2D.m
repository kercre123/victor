classdef Marker2D
    
    properties(GetAccess = 'public', Constant = true)
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        
    end
    
    properties(GetAccess = 'public', Constant = true, Abstract = true)
        % Subclasses must define the layout and values!
        % 'O' is a reserved character in the layout, meaning Orientation.
        %  The orientation bits are always the four ends of the "plus sign"
        %  in the square.
        % 'C' is a reserved character for specifying checsum bits.
        Layout;
        IdChars; % cell array
        
        % In sub-classes, set the following using the static methods
        % defined below, passing in the above Constant properties.
        %NumValues;
        UpBit, DownBit, LeftBit, RightBit;
        CheckBits;
        IdBits;
        
        % TODO: remove these.  For now, supports legacy (printed) codes.
        EncodingBits; % check bits + all value bits
        
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
        
        function bits = getEncodingBits(checkBits, valueBits)
            bits = [valueBits.values checkBits];
            bits = vertcat(bits{:});
        end
        
        w = createProbeWeights(n, probeGap, probeRadius, probeSigma, cropFactor)
        probes = createProbes(dir, n, probeGap, probeRadius, cropFactor)
    end
        
    properties(GetAccess = 'public', SetAccess = 'protected')   
        corners;
        isValid;
        ids;
        threshold;
        %topOrient;   
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numIDs;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        handles;
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end    
           
    methods(Abstract = true, Static = true)
        % Each subclass must implement a method for computing a checksum
        % from a binaryCode input.
        checksum = computeChecksum(binaryCode);
        
        % Each subclas must implement a method for encoding IDs into a
        % 2D binary code. length(varargin) should == numIDs
        binaryCode = encodeIDs(varargin);
    end
        
    methods(Access = 'public')
        
        function this = Marker2D(img, corners_, tform)
                               
            % Transform the probe locations to match the image coordinates
            [xImg, yImg] = tforminv(tform, this.Xprobes, this.Yprobes);
        
            % Get means
            assert(ismatrix(img), 'Image should be grayscale.');
            [nrows,ncols] = size(img);
            xImg = max(1, min(ncols, xImg));
            yImg = max(1, min(nrows, yImg));
            index = round(yImg) + (round(xImg)-1)*size(img,1);
            img = img(index);
            means = reshape(sum(this.ProbeWeights.*img,2), size(this.Layout));
        
            assert(isequal(size(means), size(this.Layout)), ...
                'Means size does not match layout size.');
            
            this.corners = corners_;
            this.ids = zeros(1, this.numIDs);
            [this, binaryString] = orientAndThreshold(this, means);
            this = decodeIDs(this, binaryString);
            
        end % CONSTRUCTOR Marker2D()
                
    end 
    
    methods
        function n = get.numIDs(this)
            n = length(this.IdChars);
        end    
            
    end
    
end % CLASSDEF Marker2D