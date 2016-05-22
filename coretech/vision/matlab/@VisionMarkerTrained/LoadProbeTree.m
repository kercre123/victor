function tree = LoadProbeTree()
% TODO: inherit from the VisionMarker class and use derived-class saved
% probe tree.

try
    tree = load(fullfile(VisionMarkerTrained.SavedTreePath, 'probeTree.mat'), 'probeTree');
    tree = tree.probeTree;
catch E
    warning('Could not load probe tree! (%s)', E.message);
    tree = [];
end

fprintf('VisionMarkerTrained decision tree loaded from %s.\n', VisionMarkerTrained.SavedTreePath);

end