classdef VisionMarkerLibrary < handle
    
    properties(SetAccess = 'protected')
       
        knownMarkers;
        
    end
    
    properties(SetAccess = 'protected', Dependent = true)
       
        numKnown;
        
    end
      
    
    methods
        function this = VisionMarkerLibrary(varargin)
            
            this.knownMarkers = containers.Map('KeyType', 'char', 'ValueType', 'any');
            
            if nargin > 0
                fnames = varargin{1};
                
                for i = 1:length(fnames)
                    [~,name,~] = fileparts(fnames{i});
                    otherArgs = {};
                    for i_arg = 2:2:nargin
                        assert(ischar(varargin{i_arg}), 'Expecting name/value pairs.');
                        otherArgs{end+1} = varargin{i_arg};
                        
                        assert(iscell(varargin{i_arg+1}) && ...
                            length(varargin{i_arg+1})==length(fnames), ...
                            ['Expecting cell array of properties with ' ...
                            'same number of entries as the number of files.']);
                        otherArgs{end+1} = varargin{i_arg+1}{i};
                    end
                    
                    this.AddMarker(fnames{i}, 'Name', name, otherArgs{:});
                end
            end
            
        end % constructor VisionMarkerLibrary()
        
        function AddMarker(this, img, varargin)
            if ischar(img)
                if ~exist(img, 'file')
                    error('Image file "%s" does not exist.', img);
                end
                img = imread(img);
            end
            
            marker = VisionMarker(img);
            key = VisionMarkerLibrary.CodeToKey(marker.code);
            this.knownMarkers(key) = struct(varargin{:});
        end
        
        function markerProperties = IdentifyMarker(this, marker)
            key = VisionMarkerLibrary.CodeToKey(marker.code);
            if this.knownMarkers.isKey(key)
                markerProperties = this.knownMarkers(key); 
            else
                warning('Unrecognized marker.');
                markerProperties = [];
            end
        end
        
        function N = get.numKnown(this)
           N = this.knownMarkers.Count; 
        end
        
    end % methods (public)
    
    methods(Access = 'protected', Static = true)
        
        function key = CodeToKey(code)
            assert(islogical(code), 'Code should be LOGICAL.');
            key = char(uint8(code));
        end
    end
    
end % classdef VisionMarkerLibrary
