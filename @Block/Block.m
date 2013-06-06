classdef Block < handle
    
       
    properties(GetAccess = 'public', SetAccess = 'public')
        % TODO: these should all probably be set-protected
        position;
        orientation;
        
        modelX;
        modelY;
        modelZ;
        
        X;
        Y;
        Z;
        
        color;
        handle;
    end
    
    methods(Access = 'public')
        
        function this = Block(marker)
            [this.modelX, this.modelY, this.modelZ, this.color] = getModel( ...
                marker.blockType, marker.faceType, marker.topOrient);
            
%             this.modelX = this.modelX';
%             this.modelY = this.modelY';
%             this.modelZ = this.modelZ';
            
            this.X = this.modelX;
            this.Y = this.modelY;
            this.Z = this.modelZ;
        end
        
    end % METHODS (public)
    
end % CLASSDEF Block