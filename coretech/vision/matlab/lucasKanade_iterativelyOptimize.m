% function lucasKanade_iterativelyOptimize()

function lucasKanade_iterativelyOptimize(A_translationOnly, A_affine, whichScale, maxIterations)

% function converged = trackHelper(this, img, i_scale, translationDone)

scale = 2^(whichScale-1);

% xPrev = this.xgrid{i_scale};
% yPrev = this.ygrid{i_scale};

iteration = 1;
% tform_orig = this.tform;

converged = false;
while ~converged && iteration < maxIterations

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

    It = this.target{i_scale}(:) - imgi;

    if numel(inBounds) < 16
        warning('Template drifted too far out of image.');
        break;
    end

    this.err = mean(abs(It(inBounds)));

    if translationDone
        if this.estimateAffine
            A = this.A_affine{i_scale};
        elseif this.estimateScale
            A = this.A_scale{i_scale};
        else
            error('Should not get here.');
        end
    else
        A = this.A_trans{i_scale};
    end

    AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';
    AtWA = AtW*A(inBounds,:);

    b = AtW*It(inBounds);

    update = AtWA\b;

    % Compose the update with the current transformation
    if translationDone

        if this.estimateAffine
            tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; zeros(1,3)];

        elseif this.estimateScale
            tformUpdate = [(1+update(3)) 0 update(1);
                0 (1+update(3)) update(2);
                0 0 1];
        end

        this.tform = tformUpdate*this.tform;
    else
        this.tform(1:2,3) = this.tform(1:2,3) + update;
    end

    iteration = iteration + 1;

    if all(change < this.convergenceTolerance*spacing)
        converged = true;
    end

    xPrev = xi;
    yPrev = yi;

end % WHILE not converged

% if ~converged
%     this.tform = tform_orig;
% end