function converged = trackHelper(this, images, i_scale, translationDone)
% Protected helper to do actual tracking optimization at a specified scale.

assert(iscell(images));
assert(length(images) == size(this.target,1));

spacing = 2^(i_scale-1);

xPrev = 0; %this.xgrid{i_scale};
yPrev = 0; %this.ygrid{i_scale};

iteration = 1;
tform_orig = this.tform;

converged = false;
while iteration < this.maxIterations
    
    [xi, yi] = this.getImagePoints(i_scale);
    
    % RMS error between pixel locations according to current
    % transformation and previous locations.
    change = sqrt(mean((xPrev(:)-xi(:)).^2 + (yPrev(:)-yi(:)).^2));
    if change < this.convergenceTolerance*spacing
        converged = true;
        break;
    end
    
    
    if translationDone
        numDof = size(this.A_full{1},2);   
    else
        numDof = size(this.A_trans{1},2);
    end
    
    all_AtWA = zeros(numDof, numDof); 
    all_b = zeros(numDof, 1);    
    
    for i_image = 1:length(images)
        imgi = interp2(images{i_image}, xi(:), yi(:), 'linear');
        inBounds = ~isnan(imgi);

        if this.useNormalization
            imgi = (imgi - mean(imgi(inBounds)))/std(imgi(inBounds));
        end

%         figure(4); imshow(this.target{i_image, i_scale});
%         figure(5); imshow(interp2(images{i_image}, xi, yi, 'linear'));
        It = imgi - this.target{i_image, i_scale}(:);

        %It(isnan(It)) = 0;
        %inBounds = true(size(It));

        if numel(inBounds) < 16
            warning('Template drifted too far out of image.');
            break;
        end

        this.err = mean(abs(It(inBounds)));

        if translationDone
            A = this.A_full{i_image, i_scale};
        else
            A = this.A_trans{i_image, i_scale};
        end

        AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';
        AtWA = AtW*A(inBounds,:) + diag(this.ridgeWeight*ones(1,size(A,2)));

        b = AtW*It(inBounds);
        
        all_AtWA = all_AtWA + AtWA;
        all_b = all_b + b;
    end % for i_image = 1:length(firstImages)
    
%     update = AtWA\b;
    update = all_AtWA \ all_b;

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
    else
        %this.tx = this.tx + update(1);
        %this.ty = this.ty + update(2);
        this.tform(1:2,3) = this.tform(1:2,3) - update;
    end
    
%     disp(this.tform)
    
    iteration = iteration + 1;
    
    %change = abs(err - prevErr);
    %change = err;
    %change = abs(update - prevUpdate) ./ (abs(prevUpdate)+eps);
    
    
    if this.debugDisplay
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
    
    %prevErr = err;
    xPrev = xi;
    yPrev = yi;
    
end % WHILE not converged

if ~converged
    this.tform = tform_orig;
end

end % function trackHelper