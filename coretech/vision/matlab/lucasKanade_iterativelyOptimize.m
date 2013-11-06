% function lucasKanade_iterativelyOptimize()

function lucasKanade_iterativelyOptimize(A_translationOnly, A_affine, whichScale, maxIterations)

% function converged = trackHelper(this, img, i_scale, translationDone)
            
spacing = 2^(i_scale-1);

xPrev = this.xgrid{i_scale};
yPrev = this.ygrid{i_scale};

iteration = 1;
tform_orig = this.tform;

converged = false;
while ~converged && iteration < this.maxIterations

    xi = this.tform(1,1)*this.xgrid{i_scale} + ...
        this.tform(1,2)*this.ygrid{i_scale} + ...
        this.tform(1,3);

    yi = this.tform(2,1)*this.xgrid{i_scale} + ...
        this.tform(2,2)*this.ygrid{i_scale} + ...
        this.tform(2,3);

    % RMS error between pixel locations and previous locations
    change = sqrt(mean((xPrev(:)-xi(:)).^2 + (yPrev(:)-yi(:)).^2));

    imgi = interp2(img, ...
        xi(:) + this.xcen, yi(:) + this.ycen, 'linear');
    inBounds = ~isnan(imgi);

    if this.useNormalization
        imgi = (imgi - mean(imgi(inBounds)))/std(imgi(inBounds));
    end

    It = imgi - this.target{i_scale}(:);

    %It(isnan(It)) = 0;
    %inBounds = true(size(It));

    if numel(inBounds) < 16
        warning('Template drifted too far out of image.');
        break;
    end

    this.err = mean(abs(It(inBounds)));

    %{
    namedFigure('It')
    imagesc(It), axis image, 
    title(sprintf('Error=%.3f, Previous=%.3f, AbsDiff=%.3f', ...
        err, prevErr, abs(err-prevErr))), pause(.1)
    %}

    %hold off, imagesc(img), axis image, hold on
    %plot(xi, yi, 'rx'); drawnow
    % %                 [nrows,ncols] = size(img);
    % %                 temp = zeros(nrows,ncols);
    % %                 temp(sub2ind([nrows ncols], ...
    % %                     max(1,min(nrows,round(yi))), ...
    % %                     max(1,min(ncols,round(xi))))) = It;
    % %                 hold off, imagesc(temp), axis image, drawnow


    if translationDone
        if this.estimateAffine
            A = this.A_affine{i_scale};
        elseif this.estimateScale
            A = this.A_scale{i_scale};
        else
            error('Should not get here.');
        end
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

    AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';
    AtWA = AtW*A(inBounds,:);% + diag(this.ridgeWeight*ones(1,size(A,2)));                

    b = AtW*It(inBounds);

    update = AtWA\b;
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

        if this.estimateAffine
            tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; zeros(1,3)];

        elseif this.estimateScale
            tformUpdate = [(1+update(3)) 0 update(1); 
                0 (1+update(3)) update(2);
                0 0 1];
        end

        % TODO: hardcode this inverse
        %this.tform = this.tform*inv(tformUpdate);
        this.tform = this.tform / tformUpdate;
    else
        %this.tx = this.tx + update(1);
        %this.ty = this.ty + update(2);
        this.tform(1:2,3) = this.tform(1:2,3) - update;
    end

    iteration = iteration + 1;

    %change = abs(err - prevErr);
    %change = err;
    %change = abs(update - prevUpdate) ./ (abs(prevUpdate)+eps);
    if all(change < this.convergenceTolerance*spacing)
        converged = true;
    end

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

% if ~converged
%     this.tform = tform_orig;
% end
