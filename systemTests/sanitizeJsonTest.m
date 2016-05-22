function jsonData = sanitizeJsonTest(jsonData)
    
    if ~isfield(jsonData, 'Poses')
        assert(false); % Can't correct for this
    end
    
    if isstruct(jsonData.Poses)
        jsonData.Poses = {jsonData.Poses};
    end
    
    if isfield(jsonData, 'Blocks')
        if isstruct(jsonData.Blocks)
            jsonData.Blocks = {jsonData.Blocks};
        end
    
        for i = 1:length(jsonData.Blocks)
            if ~isfield(jsonData.Blocks{i}, 'templateWidth_mm')
                jsonData.Blocks{i}.templateWidth_mm = 0;
            end
        end
    end
    
    if ~isfield(jsonData, 'NumMarkersWithContext')
        jsonData.NumMarkersWithContext = 0;
    end
    
    if ~isfield(jsonData, 'NumPartialMarkersWithContext')
        jsonData.NumPartialMarkersWithContext = 0;
    end
    
    for iPose = 1:length(jsonData.Poses)
        if ~isfield(jsonData.Poses{iPose}, 'ImageFile')
            assert(false); % Can't correct for this
        end
        
        if ~isfield(jsonData.Poses{iPose}, 'VisionMarkers')
            jsonData.Poses{iPose}.VisionMarkers = [];
        end
        
        if ~isfield(jsonData.Poses{iPose}, 'Scene')
            jsonData.Poses{iPose}.Scene.Distance = -1;
            jsonData.Poses{iPose}.Scene.Angle = -1;
            jsonData.Poses{iPose}.Scene.CameraExposure = -1;
            jsonData.Poses{iPose}.Scene.Light = -1;
        end
        
        if ~isfield(jsonData.Poses{iPose}, 'NumMarkers')
            jsonData.Poses{iPose}.NumMarkers = 0;
        end
        
        if isfield(jsonData.Poses{iPose}, 'NumMarkersWithContext')
            jsonData.Poses{iPose} = rmfield(jsonData.Poses{iPose}, 'NumMarkersWithContext');
        end
        
        if isfield(jsonData.Poses{iPose}, 'NumPartialMarkersWithContext')
            jsonData.Poses{iPose} = rmfield(jsonData.Poses{iPose}, 'NumPartialMarkersWithContext');
        end
        
        if ~isfield(jsonData.Poses{iPose}, 'RobotPose')
            jsonData.Poses{iPose}.RobotPose.Angle = 0;
            jsonData.Poses{iPose}.RobotPose.Axis = [1,0,0];
            jsonData.Poses{iPose}.RobotPose.HeadAngle = 0;
            jsonData.Poses{iPose}.RobotPose.Translation = [0,0,0];
        end
        
        if isstruct(jsonData.Poses{iPose}.VisionMarkers)
            jsonData.Poses{iPose}.VisionMarkers = {jsonData.Poses{iPose}.VisionMarkers};
        end
        
        maxMarkerIndex = length(jsonData.Poses{iPose}.VisionMarkers);
        
        if length(jsonData.Poses{iPose}.VisionMarkers) < maxMarkerIndex
            jsonData.Poses{iPose}.VisionMarkers{end+1} = [];
        end
        
        for iMarker = 1:maxMarkerIndex
            if ~isfield(jsonData.Poses{iPose}.VisionMarkers{iMarker}, 'Name')
                jsonData.Poses{iPose}.VisionMarkers{iMarker}.Name = 'MessageVisionMarker';
            end
            
            if ~isfield(jsonData.Poses{iPose}.VisionMarkers{iMarker}, 'markerType')
                jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType = 'MARKER_UNKNOWN';
            end
            
            if ~isfield(jsonData.Poses{iPose}.VisionMarkers{iMarker}, 'timestamp')
                jsonData.Poses{iPose}.VisionMarkers{iMarker}.timestamp = 0;
            end
        end
        
        if isstruct(jsonData.Poses{iPose}.RobotPose)
            jsonData.Poses{iPose}.RobotPose = {jsonData.Poses{iPose}.RobotPose};
        end
    end
    