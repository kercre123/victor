function UpdateClosestBlockObservation(this, blockID, obsPose, transparency)

if ~isKey(this.blockNodes, blockID)
    warning('There is no Block %d in this world, cannot update its observed pose.', blockID);
    return;
end

% Find the closest block with this ID
blocks = this.blockNodes(blockID);
blockPose = GetNodePose(this, blocks{1});
minDist = compare(blockPose, obsPose);
closest = 1;


for i_block = 2:length(blocks)
    crntPose = GetNodePose(this, blocks{i_block});
    dist = compare(crntPose, obsPose);
    
    if dist < minDist
        minDist = dist;
        closest = i_block;
        blockPose = crntPose;
    end
end

closestBlockNode = blocks{closest};

% Update that block's observation node with the specified pose
rotField = wb_supervisor_node_get_field(closestBlockNode, 'obsRotation');
assert(~rotField.isNull, ...
    'Could not find "obsRotation" field for block %d.', blockID);
% rotation = wb_supervisor_field_get_sf_rotation(rotField);

translationField = wb_supervisor_node_get_field(closestBlockNode, 'obsTranslation');
assert(~translationField.isNull, ...
    'Could not find "obsTranslation" field for block %d.', blockID);
% translation = wb_supervisor_field_get_sf_vec3f(translationField);

transparencyField = wb_supervisor_node_get_field(closestBlockNode, 'obsTransparency');
assert(~transparencyField.isNull, ...
    'Could not find "obsTransparency" field for block %d.', blockID);

% The observation is a child of the block in Webots, so its pose is w.r.t. 
% the block's pose.  But the incoming observed pose is w.r.t. the world.
% Adjust accordingly before updating the observed node's pose for display
% in Webots:
obsPose_wrtBlock = inv(blockPose)*obsPose; %#ok<MINV>

wb_supervisor_field_set_sf_rotation(rotField, ...
    [-obsPose_wrtBlock.axis(1) obsPose_wrtBlock.axis(3) ...
    obsPose_wrtBlock.axis(2) obsPose_wrtBlock.angle]);

wb_supervisor_field_set_sf_vec3f(translationField, ...
    [-obsPose_wrtBlock.T(1) obsPose_wrtBlock.T(3) obsPose_wrtBlock.T(2)]/1000);

wb_supervisor_field_set_sf_float(transparencyField, transparency);


end % FUNCTION UpdateClosestBlockObservation()