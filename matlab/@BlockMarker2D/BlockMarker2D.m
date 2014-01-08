classdef BlockMarker2D < Marker2D
    
    properties(GetAccess = 'public', Constant = true)
        % This is the layout I thought i was using.  But inadvertantly, my
        % use of an "EncodingBits" vector in the old decoding algorithm
        % reordered the bits, yielding the actual layout below.
        %         Layout = ['BCOFB';
        %                   'FBBBC';
        %                   'OC CO';
        %                   'FBCFC';
        %                   'BCOCB'];    
        Layout = ['BBOFC';
                  'BBFCC';
                  'OB CO';
                  'BBFCC';
                  'BFOCC'];

              % % Testing higher-resolution code
              % Layout = ['BBBBOFFFF';
              %           'BBBBBFFFF';
              %           'BBCBBFCFF';
              %           'BBBBBFFFF';
              %           'OBBB FFFO';
              %           'BBCBBFFFF';
              %           'BBBBBFFFF';
              %           'BBBBBFFCC';
              %           'BCBBOFFCC'];

        IdChars = {'B', 'F'}; % O and C are reserved!
        IdNames = {'Block', 'Face'};
        
        % Use Static methods to set the rest of the constant properties
        % from the Layout and ValueChars:
        UpBit    = BlockMarker2D.findUpBit(BlockMarker2D.Layout);
        DownBit  = BlockMarker2D.findDownBit(BlockMarker2D.Layout);
        LeftBit  = BlockMarker2D.findLeftBit(BlockMarker2D.Layout);
        RightBit = BlockMarker2D.findRightBit(BlockMarker2D.Layout);
        
        CheckBits = BlockMarker2D.findCheckBits(BlockMarker2D.Layout);
        IdBits    = BlockMarker2D.findIdBits(BlockMarker2D.Layout, ...
            BlockMarker2D.IdChars);
                
        % Probe Setup
        ProbeGap    = .33; 
        ProbeRadius = 0.8; 
        ProbeSigma  = 1/6;
        UseOutsideOfSquare = true;
        CodePadding = BlockMarker2D.setCodePadding(BlockMarker2D.UseOutsideOfSquare);
                
        Xprobes = BlockMarker2D.createProbes('X', ...
            size(BlockMarker2D.Layout,1), BlockMarker2D.ProbeGap, ...
            BlockMarker2D.ProbeRadius, BlockMarker2D.CodePadding);
        Yprobes = BlockMarker2D.createProbes('Y', ...
            size(BlockMarker2D.Layout,1), BlockMarker2D.ProbeGap, ...
            BlockMarker2D.ProbeRadius, BlockMarker2D.CodePadding);
        ProbeWeights = BlockMarker2D.createProbeWeights( ...
            size(BlockMarker2D.Layout,1), BlockMarker2D.ProbeGap, ...
            BlockMarker2D.ProbeRadius, BlockMarker2D.ProbeSigma, ...
            BlockMarker2D.CodePadding);
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        blockType;
        faceType;   
    end
     
    
    methods(Static = true)
        
        % Static methods required by abstract base class:
        checksum = computeChecksum(binaryBlock, binaryFace);
        binaryCode = encodeIDs(blockType, faceType);
        
        % Specific to this subclass
        function padding = setCodePadding(useOutsideOfSquare)
            if useOutsideOfSquare
                padding = (BlockMarker3D.FiducialSpacing+BlockMarker3D.FiducialSquareWidth)/BlockMarker3D.SquareWidthOutside;
            else
                padding = BlockMarker3D.FiducialSpacing/BlockMarker3D.SquareWidthInside;
            end
        end
        
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker2D(img, corners, tform, varargin)
            
            this@Marker2D(img, corners, tform, varargin{:});    
                        
        end
                 
    end % METHODS (public)
    
    methods
        function b = get.blockType(this)
            b = this.ids(1);
        end
        function f = get.faceType(this)
            f = this.ids(2);
        end
    end
       
end % CLASSDEF BlockMarker2D