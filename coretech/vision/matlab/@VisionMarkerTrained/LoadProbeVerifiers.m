function verifiers = LoadProbeVerifiers()
% TODO: inherit from the VisionMarker class and use derived-class saved
% probe tree.

try
    temp = load('probeTree.mat', 'verifiers');
    verifiers = temp.verifiers;
catch E
    warning('Could not load probe verifiers! (%s)', E.message);
    verifiers = [];
end

fprintf('VisionMarkerTrained decision tree verifiers loaded.\n');
end