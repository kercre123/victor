function trackDemo(varargin)

LKtracker = [];

h_target = [];
corners = [];
cen = [];

CameraCapture('processFcn', @trackHelper, ...
    'doContinuousProcessing', true, varargin{:});

    function trackHelper(img, h_axes, h_img)
        
        if isempty(LKtracker)
            
            axis(h_axes, 'on')
            set(h_axes, 'XColor', 'r', 'YColor', 'r', 'LineWidth', 2, ...
                'Box', 'on', 'XTick', [], 'YTick', []);
            
            set(h_img, 'CData', img); %(:,:,[3 2 1]));
            
            marker = simpleDetector(img);
            
            if isempty(marker) || ~marker{1}.isValid
                return
            end
            
            [nrows,ncols,~] = size(img);
            corners = marker{1}.corners;
            cen = mean(corners,1);
            
            order = [1 2 4 3 1];
            mask = poly2mask(corners(order,1), corners(order,2), nrows, ncols);

            LKtracker = LucasKanadeTracker(img, mask, ...
                'EstimateAffine', false, 'EstimateScale', true, ...
                'DebugDisplay', false, 'UseBlurring', false, ...
                'UseNormalization', true);
                                    
            
            hold(h_axes, 'on');
            h_target = plot(corners(order,1), corners(order,2), 'r', ...
                'LineWidth', 2, 'Tag', 'TrackRect', 'Parent', h_axes);
            
            %pause
            
        else
            
            if isempty(img) || any(size(img)==0)
                warning('Empty image passed in!');
                return
            end
            
            %t = tic;
            converged = LKtracker.track(img, 'MaxIterations', 50, 'ConvergenceTolerance', .25);
            %fprintf('Tracking took %.2f seconds.\n', toc(t));
            
            if ~converged 
                disp('Lost Track!')
                LKtracker = [];
                delete(h_target)
                return
            end
            
            set(h_img, 'CData', img(:,:,[3 2 1]));
            
            title(h_axes, sprintf('Error: %.3f', LKtracker.err));
            if LKtracker.err < 0.3
                set(h_axes, 'XColor', 'g', 'YColor', 'g');
            else
                set(h_axes, 'XColor', 'r', 'YColor', 'r');
            end
            %temp = LKtracker.scale*(corners - cen(ones(4,1),:)) + cen(ones(4,1),:) + ...
            %    ones(4,1)*[LKtracker.tx LKtracker.ty];
            
            tempx = LKtracker.tform(1,1)*(corners(:,1)-cen(1)) + ...
                LKtracker.tform(1,2)*(corners(:,2)-cen(2)) + ...
                LKtracker.tform(1,3) + cen(1);
           
            tempy = LKtracker.tform(2,1)*(corners(:,1)-cen(1)) + ...
                LKtracker.tform(2,2)*(corners(:,2)-cen(2)) + ...
                LKtracker.tform(2,3) + cen(2);
            
            set(h_target, 'XData', tempx([1 2 4 3 1]), ...
                'YData', tempy([1 2 4 3 1]));
%             
%             h_track = findobj(h_axes, 'Tag', 'TrackRect');
%             set(h_track, 'Pos', ...
%                 [LKtracker.tx + targetXcen - LKtracker.scale*targetWidth/2 ...
%                  LKtracker.ty + targetYcen - LKtracker.scale*targetWidth/2 ...
%                  LKtracker.scale*targetWidth*[1 1]]);
             
%             if ~isempty(h_track)
%                                %set(h_track, 'XData', LKtracker.tx, 'YData', LKtracker.ty);
%             else
%                 %plot(LKtracker.tx, LKtracker.ty, 'rx', ...
%                 rectangle('Pos', targetRect + [LKtracker.tx LKtracker.ty 0 0], ...
%                     'EdgeColor', 'r', 'LineWidth', 3, ...
%                     'Parent', h_axes, 'Tag', 'TrackRect');
%             end
            
        end
    end
end
