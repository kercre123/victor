classdef VisionMarkerLibrary < handle
    
    properties(Constant = true)
        DEFAULT_ROOT_PATH = '~/Box Sync/Cozmo SE/VisionMarkers';
        DEFAULT_FILENAME  = 'cozmoMarkerLibrary.mat';
    end
        
    properties(SetAccess = 'protected')
       
        knownMarkers;
        nameToKeyLUT;
        keyToNameLUT;
        
    end
    
    properties(SetAccess = 'protected', Dependent = true)
       
        numKnown;
        
    end
    
    methods(Static = true)
        
        function markerLibrary = Load(rootPath)
            
            if nargin==0 || isempty(rootPath)
                rootPath = VisionMarkerLibrary.DEFAULT_ROOT_PATH;
            end
            
            try
                filename = fullfile(rootPath, VisionMarkerLibrary.DEFAULT_FILENAME);
                if ~exist(filename, 'file')
                    markerLibrary = VisionMarkerLibrary();
                else
                    temp = load(filename);
                    markerLibrary = temp.markerLibrary;
                end
            catch E
                error('Unable to load marker library: %s.', E.message);
            end
            
        end % function Load()
        
    end % Static Methods
    
    methods
        function this = VisionMarkerLibrary(varargin)
            
            this.knownMarkers = containers.Map('KeyType', 'char', 'ValueType', 'any');
            this.nameToKeyLUT = containers.Map('KeyType', 'char', 'ValueType', 'char');
            this.keyToNameLUT = containers.Map('KeyType', 'char', 'ValueType', 'any');
            
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
        
        function marker = GetMarkerByName(this, name)
            if this.nameToKeyLUT.isKey(name)
                key = this.nameToKeyLUT(name);
                assert(this.knownMarkers.isKey(key), ...
                    'Key referenced to name is not valid.');
                marker = this.knownMarkers(key);
            else
                warning('No marker named "%s" in library.', name);
                marker = [];
            end
        end
        
        function AddMarker(this, input)
            
            if isa(input, 'VisionMarker')
                marker = input;
            else
                name = '';
                if ischar(input)
                    if ~exist(input, 'file')
                        error('Image file "%s" does not exist.', input);
                    end
                    [~,name,~] = fileparts(input);
                    input = imread(input);                    
                end
                marker = VisionMarker(input, 'Name', name);
            end
            
            key = VisionMarkerLibrary.MarkerToKey(marker);
            this.knownMarkers(key) = marker; %struct(varargin{:});
            this.nameToKeyLUT(marker.name) = key;
            if this.keyToNameLUT.isKey(key)
                temp = this.keyToNameLUT(key);
                if ~any(strcmp(temp, marker.name))
                    temp{end+1} = marker.name;
                    this.keyToNameLUT(key) = temp;
                end
            else
                this.keyToNameLUT(key) = {marker.name};
            end
        end % function AddMarker()
        
        function RemoveMarker(this, marker)
            % library.RemoveMarker(marker)
            % library.RemoveMarker(name)
            
            if isa(marker, 'VisionMarker')
                key = VisionMarkerLibrary.MarkerToKey(marker);
                if this.knownMarkers.isKey(key)
                    % Remove the marker from the known markers
                    this.knownMarkers.remove(key);
                    assert(this.keyToNameLUT.isKey(key), ...
                        ['Specified marker found in library, but no ' ...
                        'corresponding entry was found for it''s name.']);
                    
                    % Remove all names referring to this marker
                    names = this.keyToNameLUT(key);
                    for i = 1:length(names)
                        assert(this.nameToKeyLUT.isKey(names{i}), ...
                            'Expecting "%s" to be in the nameToKeyLUT.', names{i});
                        this.nameToKeyLUT.remove(names{i});
                    end
                else
                    warning('No matching marker found, nothing to remove.');
                end
            else
                name = marker;
                if this.nameToKeyLUT.isKey(name)
                    key = this.nameToKeyLUT(name);
                    assert(this.knownMarkers.isKey(key), ...
                        'Marker name yielded invalid key.');
                    
                    % if this name is the only one referring to this
                    % marker, then remove the marker.  Otherwise, just
                    % remove this particular name
                    assert(this.keyToNameLUT.isKey(key), ...
                        'Name''s key should have entries in the keyToNameLUT.');
                    
                    names = this.keyToNameLUT(key);
                    numNames = length(names);
                    if numNames == 1
                        this.knownMarkers.remove(key);    
                        this.keyToNameLUT.remove(key);
                        assert(strcmp(names{1}, name), ...
                            'Single name found for this key does not match the specified name.');
                        this.nameToKeyLUT.remove(name);
                    else
                        matched = strcmp(name, names);
                        
                        this.nameToKeyLUT.remove(name);
                        this.keyToNameLUT(key) = names(~matched);
                    end
                    
                else
                    warning('No matching name found, nothing to remove.');
                end
            end
        end % function RemoveMarker()
        
        function match = IdentifyMarker(this, marker)
            key = VisionMarkerLibrary.MarkerToKey(marker);
            if this.knownMarkers.isKey(key)
                match = this.knownMarkers(key); 
            else
                %warning('Unrecognized marker.');
                match = [];
            end
        end
        
        function N = get.numKnown(this)
           N = this.knownMarkers.Count; 
        end
        
        function Save(this, rootPath)
            if nargin<2
                rootPath = VisionMarkerLibrary.DEFAULT_ROOT_PATH;
            end
                
            markerLibrary = this; %#ok<NASGU>
            save(fullfile(rootPath, VisionMarkerLibrary.DEFAULT_FILENAME),  'markerLibrary');
           
        end % function Save()
        
    end % methods (public)
    
    methods(Access = 'protected', Static = true)
        
        function key = MarkerToKey(marker)
            key = char(uint8([marker.code(:); marker.cornerCode(:)]));
        end
    end
    
end % classdef VisionMarkerLibrary
