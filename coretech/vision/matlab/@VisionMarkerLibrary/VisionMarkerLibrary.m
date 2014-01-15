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
            
            assert(~isempty(varargin) && mod(length(varargin),2)==0, ...
                ['Expecting name value pairs to create a struct entry ' ...
                'for this marker.']);
            
            if ischar(img)
                if ~exist(img, 'file')
                    error('Image file "%s" does not exist.', img);
                end
                img = imread(img);
            end
            
            marker = VisionMarker(img);
            key = VisionMarkerLibrary.MarkerToKey(marker);
            this.knownMarkers(key) = struct(varargin{:});
        end
        
        function RemoveMarker(this, marker)
           key = VisionMarkerLibrary.MarkerToKey(marker);
           if this.knownMarkers.isKey(key)
               this.knownMarkers.remove(key);
           else
               warning('No matching marker found, nothing to remove.');
           end
        end
        
        function markerProperties = IdentifyMarker(this, marker)
            key = VisionMarkerLibrary.MarkerToKey(marker);
            if this.knownMarkers.isKey(key)
                markerProperties = this.knownMarkers(key); 
            else
                %warning('Unrecognized marker.');
                markerProperties = [];
            end
        end
        
        function N = get.numKnown(this)
           N = this.knownMarkers.Count; 
        end
        
    end % methods (public)
    
    methods(Access = 'protected', Static = true)
        
        function key = MarkerToKey(marker)
            key = char(uint8([marker.code(:); marker.cornerCode(:)]));
        end
    end
    
end % classdef VisionMarkerLibrary
