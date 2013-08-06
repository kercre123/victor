
classdef EmbeddedConversionsManager < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        homographyEstimationType;        
    end % PROPERTIES (get-public, set-protected)
    
    methods(Access = 'public')
        
        function this = EmbeddedConversionsManager(varargin)
            homographyEstimationType = 'cp2tform';
            homographyEstimationType_acceptable = {'cp2tform', 'opencv_cp2tform'};
            
            parseVarargin(varargin{:});
            
            isAcceptable(homographyEstimationType_acceptable, homographyEstimationType);           
            this.homographyEstimationType = homographyEstimationType;               
        end
    end % METHODS (public)
end % classdef OptimizationManager < handle

function isOk = isAcceptable(acceptableList, query)
    isOk = false;
    try
        isOk = ~isempty(find(ismember(acceptableList, query), 1));
    catch exception
        exception2 = MException('EmbeddedConversionsManager:isAcceptable', 'The query is not in the list of acceptable queries');
        exception3 = addCause(exception2, exception);
        throw(exception3);
    end
    
    if ~isOk
        exception = MException('EmbeddedConversionsManager:isAcceptable', 'The query is not in the list of acceptable queries');
        throw(exception);    
    end
end