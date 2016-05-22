function showprim(filename)

  f = fopen(filename, 'r');

  resolution = fscanf(f, 'resolution_m: %f\n')
  numAngles = fscanf(f, 'numberofangles: %d\n')
  numTotalPrims = fscanf(f, 'totalnumberofprimitives: %d')

  X = cell(numAngles, 1);
  Y = cell(numAngles, 1);

  for i = 1:numTotalPrims
    primID = fscanf(f, '\nprimID: %d\n');
    startAngle = fscanf(f, 'startangle_c: %d\n');
    endpose = fscanf(f, 'endpose_c: %d %d %d\n');
    costMulti = fscanf(f, 'additionalactioncostmult: %d\n');
    numPoses = fscanf(f, 'intermediateposes: %d\n');

    poses = fscanf(f, '%f', [3 numPoses])';

    X{startAngle+1} = [X{startAngle+1}, poses(:,1)];
    Y{startAngle+1} = [Y{startAngle+1}, poses(:,2)];
  end

  fclose(f);

  figure;
  hold on;

  for a = 1:numAngles
    plot(X{a},Y{a});

    minX = -0.25; %floor(min(X{a}(:)) / resolution) * resolution;
    maxX = 0.25; %ceil(max(X{a}(:)) / resolution) * resolution;
    minY = -0.25; %floor(min(Y{a}(:)) / resolution) * resolution;
    maxY = 0.25; %ceil(max(Y{a}(:)) / resolution) * resolution;

    axis([minX maxX minY maxY], 'equal');
    set(gca, 'xtick', [minX:resolution:maxX]);
    set(gca, 'ytick', [minY:resolution:maxY]);
    grid('on');
  end

end
