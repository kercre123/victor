function [u,v] = getAllPoints2D(this, numMarkers3D)

u = -ones(4,numMarkers3D);
v = -ones(4,numMarkers3D);

numMarkers2D = this.numMarkers;

world = this.robot.world;
for i_marker = 1:numMarkers2D
    marker2D = this.markers{i_marker};
    
    block = world.getBlock(marker2D.blockType);
    marker3D = block.getFaceMarker(marker2D.faceType);
    
    assert(marker3D.ID <= numMarkers3D, 'BlockMarker3D has ID out of range.');
    
    u(:,marker3D.ID) = marker2D.corners(:,1);
    v(:,marker3D.ID) = marker2D.corners(:,2);
    
end

if nargout == 1
    u = [u(:) v(:)];
end

end