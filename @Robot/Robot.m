classdef Robot < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        position = zeros(3,1);
        orientation = eye(3);
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        handles;        
        
    end % PROPERTIES (get-public, set-protected)
    
    methods(Access = 'public')
        
        function this = Robot()
            
        end
        
    end % METHODS (public)
    
end % CLASSDEF Robot