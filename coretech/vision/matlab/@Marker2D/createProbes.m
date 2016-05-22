function probes = createProbes(dir, n, probeGap, probeRadius, codePadding)

patternOrigin = codePadding;
patternWidth = (1 - 2*codePadding);
squareWidth = patternWidth/n;
squareCenters = linspace(squareWidth/2, patternWidth-squareWidth/2, n) + patternOrigin;

%x = (linspace(probeGap*squareWidth, (1-probeGap)*squareWidth, 2*probeRadius+1) - squareWidth/2);
%xgrid = x(ones(1,2*probeRadius+1),:);
th = linspace(0,2*pi,9);
r = squareWidth*probeGap/2*probeRadius;

probes = cell(n);
switch(lower(dir))
    case 'x'
        %xgrid = row(xgrid);
        xgrid = [0 r*cos(th(1:8))];
        for j = 1:n
            [probes{:,j}] = deal(xgrid + squareCenters(j));
        end

    case 'y'
        %ygrid = row(xgrid');
        ygrid = [0 r*sin(th(1:8))];
        for i = 1:n
            [probes{i,:}] = deal(ygrid + squareCenters(i));
        end
    otherwise
        error('Direction should be x or y.');
end

probes = vertcat(probes{:});

end

