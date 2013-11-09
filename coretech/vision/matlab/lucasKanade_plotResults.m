% function lucasKanade_plotResults(image, corners, homography)

function lucasKanade_plotResults(image, corners, homography)

    cen = mean(corners,1);

    order = [1,2,3,4,1];

    hold off;
    imshow(image);
    hold on;
    plot(corners(order,1), corners(order,2), 'b--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = homography(1,1)*(corners(order,1)-cen(1)) + ...
        homography(1,2)*(corners(order,2)-cen(2)) + ...
        homography(1,3) + cen(1);

    tempy = homography(2,1)*(corners(order,1)-cen(1)) + ...
        homography(2,2)*(corners(order,2)-cen(2)) + ...
        homography(2,3) + cen(2);

    plot(tempx, tempy, 'r', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');
