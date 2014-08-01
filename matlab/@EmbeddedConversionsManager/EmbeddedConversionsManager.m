
classdef EmbeddedConversionsManager < handle

    properties(GetAccess = 'public', SetAccess = 'protected')
        homographyEstimationType;
        computeCharacteristicScaleImageType;
        componentRejectionTestsType;
        traceBoundaryType;
        connectedComponentsType;
        completeCImplementationType;
        emptyCenterDetection;
        smallCharacterisicParameter;
        extractFiducialMethod;
        extractFiducialMethodType;
    end % PROPERTIES (get-public, set-protected)

    methods(Access = 'public')

        function this = EmbeddedConversionsManager(varargin)
            homographyEstimationType = 'matlab_original';
            homographyEstimationType_acceptable = {'matlab_original', 'opencv_cp2tform', 'c_float64', 'matlab_inhomogeneous'};

            computeCharacteristicScaleImageType = 'matlab_original';
            computeCharacteristicScaleImageType_acceptable = {'matlab_original', 'matlab_boxFilters', 'matlab_boxFilters_multiple', 'matlab_boxFilters_small', 'matlab_loops', 'matlab_loopsAndFixedPoint', 'matlab_loopsAndFixedPoint_mexFiltering', 'c_fixedPoint', 'matlab_edges'};

            componentRejectionTestsType = 'matlab_original';
            componentRejectionTestsType_acceptable = {'matlab_original', 'off'};
            
            traceBoundaryType = 'matlab_original';
            traceBoundaryType_acceptable = {'matlab_original', 'matlab_loops', 'c_fixedPoint', 'matlab_approximate'};

            connectedComponentsType = 'matlab_original';
            connectedComponentsType_acceptable = {'matlab_original', 'matlab_approximate'};

            completeCImplementationType = 'matlab_original';
            completeCImplementationType_acceptable = {'matlab_original', 'c_DetectFiducialMarkers'};

            emptyCenterDetection = 'matlab_original';
            emptyCenterDetection_acceptable = {'matlab_original', 'off'};
            
            extractFiducialMethod = 'matlab_original';
            extractFiducialMethod_acceptable = {'matlab_original', 'matlab_exhaustive', 'c_exhaustive'};
            
            extractFiducialMethodType = 'backward';
            extractFiducialMethodType_acceptable = {'forward', 'backward', 'backward_noEdges'};

            smallCharacterisicParameter = 0.9;

            parseVarargin(varargin{:});

            isAcceptable(homographyEstimationType_acceptable, homographyEstimationType);
            this.homographyEstimationType = homographyEstimationType; %#ok<*PROP>

            isAcceptable(computeCharacteristicScaleImageType_acceptable, computeCharacteristicScaleImageType);
            this.computeCharacteristicScaleImageType = computeCharacteristicScaleImageType; %#ok<*PROP>
            
            isAcceptable(componentRejectionTestsType_acceptable, componentRejectionTestsType);
            this.componentRejectionTestsType = componentRejectionTestsType; %#ok<*PROP>

            isAcceptable(traceBoundaryType_acceptable, traceBoundaryType);
            this.traceBoundaryType = traceBoundaryType; %#ok<*PROP>

            isAcceptable(connectedComponentsType_acceptable, connectedComponentsType);
            this.connectedComponentsType = connectedComponentsType; %#ok<*PROP>

            isAcceptable(completeCImplementationType_acceptable, completeCImplementationType);
            this.completeCImplementationType = completeCImplementationType; %#ok<*PROP>

            isAcceptable(emptyCenterDetection_acceptable, emptyCenterDetection);
            this.emptyCenterDetection = emptyCenterDetection; %#ok<*PROP>

            this.smallCharacterisicParameter = smallCharacterisicParameter; %#ok<*PROP>
            
            isAcceptable(extractFiducialMethod_acceptable, extractFiducialMethod);
            this.extractFiducialMethod = extractFiducialMethod;
            
            isAcceptable(extractFiducialMethodType_acceptable, extractFiducialMethodType);
            this.extractFiducialMethodType = extractFiducialMethodType;
        end
    end % METHODS (public)
end % classdef OptimizationManager < handle

function isOk = isAcceptable(acceptableList, query)

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

end % FUNCTION isAcceptable()