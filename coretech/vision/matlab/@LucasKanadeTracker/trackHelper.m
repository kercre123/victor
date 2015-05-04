function [converged, iteration] = trackHelper(this, img, i_scale, translationDone)
% Protected helper to do actual tracking optimization at a specified scale.

spacing = 2^(i_scale-1);

% For PID-style control (when computing update from residual error)
eSum = 0;
useGainScheduling = strcmp(this.tformType, 'planar6dof');
if useGainScheduling
    % Turn down the proportional gain as we get closer (as indicated by tz),
    % to help avoid small oscillations that occur as the target gets large 
    % in our field of view
    Kp_min = 0.1;
    Kp_max = 0.75;
    tz_min = 30;
    tz_max = 150;
else
    Kp = 0.5; %0.25;
end
Ki = 0;%.05; %.05;
Kd = 0;%.05; %0.1;

iteration = 1;
tform_orig = this.tform;

showConvergence = false;

if showConvergence
    if isstruct(this.convergenceTolerance) %#ok<UNRCH>
        assert(strcmp(this.tformType, 'planar6dof'), ...
            'Struct convergenceTolerance requires planar6dof tracker.');
        
        changeHistory = zeros(6,this.maxIterations); %#ok<USENS>
    else
        changeHistory = zeros(4,this.maxIterations);
    end
end

converged = false;
while iteration < this.maxIterations
    
    [xi, yi] = this.getImagePoints(i_scale);
    
    cornersCurrent = this.corners;
        
    if iteration > 1 && ...
            (~strcmp(this.tformType, 'planar6dof') || ~isstruct(this.convergenceTolerance))
        
        % Max change of any one of the corners from it's previous location
        change = sqrt( sum((cornersCurrent - cornersPrevious).^2, 2) );
        
        % if showConvergence
        %    changeHistory(:,iteration) = change;
        % end
                
        %         % Converge once the change is small enough
        %         if change < this.convergenceTolerance*spacing
        %             converged = true;
        %             break;
        %         end
        
        if iteration > 2
            if showConvergence
                changeHistory(:, iteration) = abs(change - changePrev); %#ok<UNRCH>
            end
            
            % Converge once the change stops changing (reaches steady
            % state)
            if all(abs(change - changePrev) < this.convergenceTolerance*spacing)
                converged = true;
                break;
            end
            
        end
        
        changePrev = change;

    end % if checking for corner position convergence
    
    cornersPrevious = cornersCurrent;
    
    imgi = interp2(img, xi(:), yi(:), 'linear');
    inBounds = ~isnan(imgi);
    
    if this.useNormalization
        if ~isempty(this.xgridFull{i_scale})
            [xnorm, ynorm] = this.getImagePoints(this.xgridFull{i_scale}, ...
                this.ygridFull{i_scale});
            normData = interp2(img, xnorm(:), ynorm(:), 'nearest');
            tempInBounds = ~isnan(normData);
            imgi = (imgi - mean(normData(tempInBounds))) / ...
                std(normData(tempInBounds));
        
        else        
            imgi = (imgi - mean(imgi(inBounds))) ./ std(imgi(inBounds));
        end
    end
    
    %figure(3); imshow(this.target{i_scale});
    %figure(4); imshow(interp2(img, xi, yi, 'linear'));
    It = imgi - this.target{i_scale}(:);
    
    %It(isnan(It)) = 0;
    %inBounds = true(size(It));
    
    if numel(inBounds) < 16
        warning('Template drifted too far out of image.');
        break;
    end
    
    %this.err = mean(abs(It(inBounds)));
    
    %namedFigure('It')
    %imagesc(reshape(It, size(this.xgrid{i_scale}))), axis image,
    %title(sprintf('Error=%.3f, Previous=%.3f, AbsDiff=%.3f', ...
    %    this.err, prevErr, abs(err-prevErr))), pause(.1)
    
    
    %hold off, imagesc(img), axis image, hold on
    %plot(xi, yi, 'rx'); drawnow
    % %                 [nrows,ncols] = size(img);
    % %                 temp = zeros(nrows,ncols);
    % %                 temp(sub2ind([nrows ncols], ...
    % %                     max(1,min(nrows,round(yi))), ...
    % %                     max(1,min(ncols,round(xi))))) = It;
    % %                 hold off, imagesc(temp), axis image, drawnow
    
    
    if translationDone
        A = this.A_full{i_scale};
        %{
                    AtWA = this.A_scale{i_scale}(inBounds,:)'*(this.W{i_scale}(:,ones(1,3)).*this.A_scale{i_scale}(inBo
                    AtWA = this.AtWA_scale{i_scale};
                    AtW = this.AtW_scale{i_scale};
        %}
    else
        A = this.A_trans{i_scale};
        %{
                    AtWA = this.AtWA_trans{i_scale};
                    AtW = this.AtW_trans{i_scale};
        %}
    end
    
    % LHS of the update equation
    AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';
    AtWA = AtW*A(inBounds,:) + diag(this.ridgeWeight*ones(1,size(A,2)));
    
    % RHS of the update equation
    b = AtW*It(inBounds);
    
    % Solve for the error / residual
    e = AtWA\b;
       
    if useGainScheduling
        Kp = max(Kp_min, min(Kp_max, (this.tz-tz_min)/(tz_max-tz_min)*(Kp_max-Kp_min) + Kp_min));
    end
    
    % Compute the update ("control signal") from the error signal 
    update = Kp*e; % Proportional Control
    
    if Ki > 0
        % Add integral control
        update = update + Ki*eSum;
        
        eSum = eSum + e; % integrate the error
    end
    
    if Kd > 0 
        if iteration > 1 
            % Add derivative control
            update = update + Kd*(e-ePrev);
        end
        ePrev = e;
    end
    
    %update = least_squares_norm(AtWA, b);
    %update = robust_least_squares(AtWA, b);
    
    %err = norm(b-AtWA*update);
    
    %                 if this.debugDisplay
    %                     edata = [get(this.h_errPlot, 'YData') err];
    %                     xdata = 1:(length(edata));
    %                     set(this.h_errPlot, 'XData', xdata, 'YData', edata);
    %                     set(get(this.h_errPlot, 'Parent'), ...
    %                         'XLim', [1 length(edata)], ...
    %                         'YLim', [0 1.1*max(edata)]);
    %                 end
    
    %update = spacing * update;

    % Compose the update with the current transformation
    if translationDone
        
        if strcmp(this.tformType, 'planar6dof')
            
            if isstruct(this.convergenceTolerance)
                if showConvergence
                    changeHistory(:,iteration) = e; %#ok<UNRCH>
                end
                
                if all(abs(e(1:3)) < this.convergenceTolerance.angle*spacing) && ...
                        all(abs(e(4:6)) < this.convergenceTolerance.distance*spacing)
                    converged = true;
                    break;
                end
                
            end %if isstruct(convergenceTolerance)
                        
            % Subtracting the update because we're using _inverse_
            % compositional LK tracking 
            this.theta_x = this.theta_x - update(1);
            this.theta_y = this.theta_y - update(2);
            this.theta_z = this.theta_z - update(3);
            
            this.tx = this.tx - update(4);
            this.ty = this.ty - update(5);
            this.tz = this.tz - update(6);
                
            % Update the linear transformation matrix based on the updated
            % 6DOF parameters
            this.tform = this.Compute6dofTform();
            
        else
            
            % For all other transformation types, just create the
            % transformation update matrix and compose it with the current
            % one
            
            switch(this.tformType)
                case 'rotation'
                    cosTheta = cos(update(3));
                    sinTheta = sin(update(3));
                    tformUpdate = [cosTheta -sinTheta update(1);
                        sinTheta cosTheta update(2);
                        0 0 1];
                    
                case 'affine'
                    tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; zeros(1,3)];
                    
                case 'scale'
                    tformUpdate = [(1+update(3)) 0 update(1);
                        0 (1+update(3)) update(2);
                        0 0 1];
                    
                case 'homography'
                    tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
                    
                otherwise
                    error('Should not get here.');
            end
            
            % TODO: hardcode this inverse
            %this.tform = this.tform*inv(tformUpdate);
            this.tform = this.tform / tformUpdate;
            
        end % if planar6dof or not
        
    else
        %this.tx = this.tx + update(1);
        %this.ty = this.ty + update(2);
        this.tform(1:2,3) = this.tform(1:2,3) - update;
    end
    
    %disp(this.tform)
    
    iteration = iteration + 1;
    
    %change = abs(err - prevErr);
    %change = err;
    %change = abs(update - prevUpdate) ./ (abs(prevUpdate)+eps);
    
    if false && this.debugDisplay
        edata = [get(this.h_errPlot, 'YData') change];
        xdata = 1:(length(edata));
        set(this.h_errPlot, 'XData', xdata, 'YData', edata);
        set(get(this.h_errPlot, 'Parent'), ...
            'XLim', [1 length(edata)], ...
            'YLim', [0 max(eps,1.1*max(edata))]);
        
        if converged
            h = get(this.h_errPlot, 'Parent');
            hold(h, 'on')
            plot(xdata(end), edata(end), 'go', 'Parent', h);
            hold(h, 'off');
        end
    end
    
end % WHILE not converged

if ~converged && i_scale > 1
    this.tform = tform_orig;
end

if showConvergence && ~converged
    namedFigure('Convergence'), hold off
    
    index = 1:iteration-1;
    
    N = size(changeHistory,1);
    for i = 1:N
        subplot(N,1,i)
        plot(index(2:end), changeHistory(i,index(2:end)));
        grid on
    end
end

end % function trackHelper