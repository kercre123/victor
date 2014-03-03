function probeList = GetProbeList(node, probeList, workingList)
if nargin == 1
    probeList = struct();
    workingList = [];
end

if ~isempty(node)
    if isfield(node, 'labelName')
        probeList.(['label_' node.labelName]) = workingList;
    elseif isfield(node, 'whichProbe')
        workingList = [workingList node.whichProbe];
        if isfield(node, 'leftChild')
            probeList = GetProbeList(node.leftChild, probeList, workingList);
        end
        if isfield(node, 'rightChild')
            probeList = GetProbeList(node.rightChild, probeList, workingList);
        end
    end
end

end