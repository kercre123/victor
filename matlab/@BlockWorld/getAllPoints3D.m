function [X,Y,Z] = getAllPoints3D(this)

numMarkers = this.numMarkers;

X = zeros(4, numMarkers);
Y = zeros(4, numMarkers);
Z = zeros(4, numMarkers);

for i_marker = 1:numMarkers
    currentPosition = getPosition(this.allMarkers3D{i_marker});
    
    X(:,i_marker) = currentPosition(:,1);
    Y(:,i_marker) = currentPosition(:,2);
    Z(:,i_marker) = currentPosition(:,3);
end

if nargout == 1
    X = [X(:) Y(:) Z(:)];
end

end