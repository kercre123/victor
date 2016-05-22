classdef MatMarker2D < Marker2D
    
    properties(GetAccess = 'public', Constant = true)
        % Original, intended layout (screwed up by using old EncodingBits)
        %         Layout = ['XXOXX';
        %                   'XXCXX';
        %                   'OCCCO';
        %                   'YYCYY';
        %                   'YYOYY'];
              
        Layout = ['XXOYC';
                  'XXYYC';
                  'OXYYO';
                  'XXYYC';
                  'XYOCC'];
              
        IdChars = {'X', 'Y'};
        IdNames = MatMarker2D.IdChars;
        
        % Use Static methods to set the rest of the constant properties
        % from the Layout and ValueChars:
        UpBit    = MatMarker2D.findUpBit(MatMarker2D.Layout);
        DownBit  = MatMarker2D.findDownBit(MatMarker2D.Layout);
        LeftBit  = MatMarker2D.findLeftBit(MatMarker2D.Layout);
        RightBit = MatMarker2D.findRightBit(MatMarker2D.Layout);
        
        CheckBits = MatMarker2D.findCheckBits(MatMarker2D.Layout);
        IdBits    = MatMarker2D.findIdBits(MatMarker2D.Layout, ...
            MatMarker2D.IdChars);
               
        % Probe Setup
        ProbeGap    = .2; 
        ProbeRadius = 3; 
        ProbeSigma  = 1/6;
        CodePadding = 1/10;
        Xprobes = MatMarker2D.createProbes('X', ...
            size(MatMarker2D.Layout,1), MatMarker2D.ProbeGap, ...
            MatMarker2D.ProbeRadius, MatMarker2D.CodePadding);
        Yprobes = MatMarker2D.createProbes('Y', ...
            size(MatMarker2D.Layout,1), MatMarker2D.ProbeGap, ...
            MatMarker2D.ProbeRadius, MatMarker2D.CodePadding);
        ProbeWeights = MatMarker2D.createProbeWeights( ...
            size(MatMarker2D.Layout,1), MatMarker2D.ProbeGap, ...
            MatMarker2D.ProbeRadius, MatMarker2D.ProbeSigma, ...
            MatMarker2D.CodePadding);
        
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        X;
        Y;   
        centroid;
    end
     
    % Static methods required by abstract base class:
    methods(Static = true)
        
        checksum = computeChecksum(binaryX, binaryY);
        
        binaryCode = encodeIDs(Xpos, Ypos);
    end
    
    methods(Access = 'public')
       
        function this = MatMarker2D(img, corners, tform, varargin)
          
            this@Marker2D(img, corners, tform, varargin{:});
            
        end
         
    end % METHODS (public)
    
    methods
        function x = get.X(this)
            x = this.ids(1);
        end
        function y = get.Y(this)
            y = this.ids(2);
        end
        function cen = get.centroid(this)
           cen = mean(this.corners,1); 
        end
    end

end % CLASSDEF MatMarker2D