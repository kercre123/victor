
function homography = computeHomographyFromTform(tform)

homography = tform.tdata(1).tdata.T';
currentTdata = tform.tdata(2);

while length(currentTdata.tdata) == 2
%     homography = currentTdata.tdata(1).tdata.T' * homography;
    homography = homography * currentTdata.tdata(1).tdata.T';
    currentTdata = currentTdata.tdata(2);
%     disp(homography)
end

% homography = currentTdata.tdata.T' * homography;
homography = homography * currentTdata.tdata.T';

homography = homography / homography(3,3);
