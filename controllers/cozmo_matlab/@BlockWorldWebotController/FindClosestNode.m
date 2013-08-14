function [closestNode, closestP] = FindClosestNode(this, name, pose)
% Find the node with 

minDist = inf;
closestNode = [];

for i_node = 1:this.numSceneNodes
    
    sceneObject = wb_supervisor_field_get_mf_node(this.sceneNodes, i_node-1);
    nameField = wb_supervisor_node_get_field(sceneObject, 'name');
    if ~nameField.isNull
        objName = wb_supervisor_field_get_sf_string(nameField);
        
        if strcmp(objName, name)
            crntPose = GetNodePose(this, sceneObject);
            dist = compare(pose, crntPose);
            
            if dist < minDist
                minDist     = dist;
                closestP    = crntPose;
                closestNode = sceneObject;
            end
            
        end
    end
end % FOR each scene node

if isempty(closestNode)
    warning('No nodes found with name "%s".', name);
end

end % FUNCTION findClosestNode()