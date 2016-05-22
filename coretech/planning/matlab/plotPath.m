function state = plotPath(filename, animate, state, color)

if nargin == 0
   filename = 'path.txt'
   animate = true
   state = {};
   color = 'g.-'
elseif nargin == 1
  animate = true
  state = {};
  color = 'g.-'
elseif nargin == 2
  state = {};
  color = 'g.-'
elseif nargin == 3
  color = 'g.-';
end

path = [];

if exist(filename, 'file')
   path = reshape(textread(filename), 3, [])';
   hold on;

   if animate
      ## minX = min(path(:,1));
      ## maxX = max(path(:,1));
      ## minY = min(path(:,2));
      ## maxY = max(path(:,2));

      ## Xrange = maxX - minX;
      ## Yrange = maxY - minY;

      ## minX = minX - 0.1*Xrange;
      ## maxX = maxX + 0.1*Xrange;
      ## minY = minY - 0.1*Yrange;
      ## maxY = maxY + 0.1*Yrange;

      ## ax = axis()
      ## if(minX < ax(1))
      ##   ax(1) = minX;
      ## end
      ## if(maxX > ax(2))
      ##   ax(2) = maxX;
      ## end
      ## if(minY < ax(3))
      ##   ax(3) = minY;
      ## end
      ## if(minY > ax(4))
      ##   ax(4) = minY;
      ## end

      ## axis(ax);

      if length(state) == 0
        disp('first state')
        state{1} = plot(path(1,1), path(1,2), color);
        state{2} = plotRobot(path(1,1), path(1,2), path(1,3));
        state{3} = 1;
      end

      for idx = state{3}:size(path,1)
          sleep(0.01);
          set(state{1}, 'XData', path(1:idx, 1));
          set(state{1}, 'YData', path(1:idx, 2));
          plotRobot(path(idx,1), path(idx,2), path(idx,3), state{2});
      end

      state{3} = size(path,1);
   else
     plot(path(:,1), path(:,2), color);
     plotRobot(path(end,1), path(end,2), path(end,3));
   end
end


end
