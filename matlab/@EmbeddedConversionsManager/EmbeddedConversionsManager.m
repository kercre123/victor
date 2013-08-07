
classdef EmbeddedConversionsManager < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        homographyEstimationType;        
    end % PROPERTIES (get-public, set-protected)
    
    methods(Access = 'public')
        
        function this = EmbeddedConversionsManager(varargin)
            homographyEstimationType = 1; % 1-cp2tform, 2-opencv_cp2tform
            parseVarargin(varargin{:});
            
            this.homographyEstimationType = homographyEstimationType;
        end
    end % METHODS (public)
end % classdef OptimizationManager < handle