classdef BlockWorld
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        blocks = {};
        robot = Robot();
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numBlocks;
        
    end % DEPENDENT PROPERTIES (get-public, set-protected)
    
    
    methods(Access = 'public')
        
        function this = BlockWorld()
            
            
        end % FUNCTION BlockWorld()
        
    end % METHODS (public)
    
    methods % Dependent SET/GET
        
        function N = get.numBlocks(this)
           N = length(this.blocks); 
        end
        
    end % METHODS (dependent)
    
end % CLASSDEF BlockWorld