function SetNodePose(~, node, pose, transparency)
% Set the Pose of a node in the world, by name.

assert(~isempty(node), 'Cannot SetNodePose for an empty name.');

if ischar(node)
    name = node;
    if ~isKey(this.nameToNodeLUT, name)
        error('No node named "%s" in this world.', name);
    end
    node = this.nameToNodeLUT(name);
end

if nargin < 4
    transparency = 0.0; % fully visible
end

rotField = wb_supervisor_node_get_field(node, 'rotation');
assert(~rotField.isNull, ...
    'Could not find "rotation" field for node "%s".', name);

translationField = wb_supervisor_node_get_field(node, 'translation');
assert(~translationField.isNull, ...
    'Could not find "translation" field for node "%s".', name);

transparencyField = wb_supervisor_node_get_field(node, 'transparency');
assert(~transparencyField.isNull, ...
    'Could not find "transparency" field for node "%s".', name);

wb_supervisor_field_set_sf_rotation(rotField, ...
    [-pose.axis(1) pose.axis(3) pose.axis(2) pose.angle]);

wb_supervisor_field_set_sf_vec3f(translationField, ...
    [-pose.T(1) pose.T(3) pose.T(2)]/1000);

wb_supervisor_field_set_sf_float(transparencyField, transparency);

end % FUNCTION SetNodePose()