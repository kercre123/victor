
% function testBinaryLKSolver()

function testBinaryLKSolver()

% 1. A rectangle, with only vertical and horizontal shift
% 2. A diamond, with diagonal gradients
testNumber = 2;

% [ 0,  0, 0, -xi, -yi, -1,  xi*yp,  yi*yp];
% [xi, yi, 1,   0,   0,  0, -xi*xp, -yi*xp];

if testNumber == 1
    vOriginals = [3,3; 4,3; 5,3; 3,7; 4,7; 5,7];
    vWarped =    [3,1; 4,1; 5,1; 3,5; 4,5; 5,5];

    hOriginals = [2,4; 2,5; 7,4; 7,5];
    hWarped =    [1,4; 1,5; 6,4; 6,5];

    numPoints = size(vOriginals,1) + size(hOriginals,1);

    A = zeros(numPoints, 8);
    b = zeros(numPoints, 1);

    ci = 1;
    for i = 1:size(vOriginals,1)
        xi = vOriginals(i,1);
        yi = vOriginals(i,2);

        yp = vWarped(i,2);

        A(ci, :) = [ 0,  0, 0, -xi, -yi, -1,  xi*yp,  yi*yp];

        b(ci) = -yp;

        ci = ci + 1;
    end

    for i = 1:size(hOriginals,1)
        xi = hOriginals(i,1);
        yi = hOriginals(i,2);

        xp = hWarped(i,1);

        A(ci, :) = [xi, yi, 1,   0,  0,   0, -xi*xp, -yi*xp];

        b(ci) = xp;

        ci = ci + 1;
    end

    svdHomography = A \ b;
    svdHomography(9) = 1;
    svdHomography = reshape(svdHomography, [3,3])'

    AtA = A'*A;
    Atb = A'*b;

    L = chol(AtA);

    cholHomography = L \ (L' \ Atb);
    cholHomography(9) = 1;

    cholHomography = reshape(cholHomography, [3,3])'
elseif testNumber == 2
    originals = [...
        2,3;
        2.5,2.5;
        3.5,2.5;
        4,3;
        4,5;
%         3.5,5.5;
        4.5,4.5;
        2,5;
        1.5,4.5];

    warped = [...
        1.5,2.5;
        2,2;
        4,2;
        4.5,2.5;
        3.5,4.5;
%         3,5;
        4,4;
        2.5,4.5;
        2,4];
    
    figure(1);
    hold off;
    plot([3,5,3,1,3], [1,3,5,3,1], 'r');
    hold on;
    plot([3,5,3,1,3], [2,4,6,4,2], 'b');
    for i = 1:size(originals,1)
        plot([originals(i,1),warped(i,1)], [originals(i,2),warped(i,2)], 'g');
    end
    axis equal
    
    numPoints = size(originals,1);
    
    A = zeros(2*numPoints, 8);
    b = zeros(2*numPoints, 1);
    W = zeros(2*numPoints, 2*numPoints);

    ci = 1;
    for i = 1:numPoints
        xi = originals(i,1);
        yi = originals(i,2);

        xp = warped(i,1);
        yp = warped(i,2);

        A(ci,   :) = [ 0,  0, 0, -xi, -yi, -1,  xi*yp,  yi*yp];
        A(ci+1, :) = [xi, yi, 1,   0,  0,   0, -xi*xp, -yi*xp];

        b(ci)   = -yp;
        b(ci+1) = xp;
        
        W(ci,ci)     = sqrt(2)/2;
        W(ci+1,ci+1) = sqrt(2)/2;

        ci = ci + 2;
    end
    
    svdHomography = A \ b;
    svdHomography(9) = 1;
    svdHomography = reshape(svdHomography, [3,3])'

    AtA = A'*A;
    Atb = A'*b;

    L = chol(AtA);

    cholHomography = L \ (L' \ Atb);
    cholHomography(9) = 1;

    cholHomography = reshape(cholHomography, [3,3])'

end % if testNumber == 1 ... else

keyboard
