classdef Camera < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        focalLengthX;
        focalLengthY;
        
        centerX;
        centerY;
        
        distortionCoeffs;
        
    end
        
    methods(Access = 'public')
        
        function this = Camera(calibrationFile)
           
            if ~exist(calibrationFile, 'file')
                error('Specified calibration file does not exist.');
            end
            
            calibration = load(calibrationFile);
            
            this.focalLengthX = calibration.fc(1);
            this.focalLengthY = calibration.fc(2);
            
            this.centerX = calibration.cc(1);
            this.centerY = calibration.cc(2);
            
            this.distortionCoeffs = calibration.kc;            
            
        end
        
    end % METHODS (public)
    
end % CLASSDEF Camera