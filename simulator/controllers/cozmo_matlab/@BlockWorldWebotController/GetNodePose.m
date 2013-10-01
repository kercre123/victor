function P = GetNodePose(this, node)
% Get a Pose object for a node with a specified name.

assert(~isempty(node), 'Cannot GetNodePose for an empty name/node.');

if ischar(node)
    name = node;
    if ~isKey(this.nameToNodeLUT, name)
        error('No node named "%s" in this world.', name);
    end
    node = this.nameToNodeLUT(name);
end
    
rotField = wb_supervisor_node_get_field(node, 'rotation');
assert(~rotField.isNull, ...
    'Could not find "rotation" field for node named "%s".', node);
rotation = wb_supervisor_field_get_sf_rotation(rotField);
assert(isvector(rotation) && length(rotation)==4, ...
    'Expecting 4-vector for rotation of node named "%s".', node);

transField = wb_supervisor_node_get_field(node, 'translation');
assert(~transField.isNull, ...
    'Could not find "translation" field for node named "%s".', node);
translation = wb_supervisor_field_get_sf_vec3f(transField);
assert(isvector(translation) && length(translation)==3, ...
    'Expecting 3-vector for translation of node named "%s".', node);

% Note the coordinate change here: Webot has y pointing up,
% while Matlab has z pointing up.  That swap induces a
% right-hand to left-hand coordinate change (I think).  Also,
% things in Matlab are defined in millimeters, while Webot is
% in meters.
%rotAngle = rotation(4);
%rotVec   = [-rotation(1) rotation(3) rotation(2)]
rotation = (rotation(4))*[-rotation(1) rotation(3) rotation(2)];
translation = 1000*[-translation(1) translation(3) translation(2)];
P = Pose(rotation, translation);

end % Function GetNodePose()