function parseLedCode_plot_changingCenter(x, y, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps)
    figure(2);
    for iBlur = 1:length(blurSigmas)
        subplot(3,3,iBlur);
        hold off
        plot(squeeze(blurredImages_small(y, x, :, iBlur))', 'b')
        hold on
        plot(squeeze(blurredImages_large(y, x, :, iBlur))', 'g')
        plot(squeeze(blurredImages_outsideRing(y, x, :, iBlur))', 'r')
        plot(squeeze(blurredImages_inside(y, x, :, iBlur))', 'k')

        a = axis();
        a(3:4) = [0,200];
        axis(a);
    end

    figure(3);
    maxAxis = 0;
    for iBlur = 1:length(blurSigmas)
%         curRing = double(squeeze(blurredImages_inside(y,x,:,iBlur)));
%         curSteps = imfilter(curRing, stepUpFilter, 'circular');

        curSteps = steps(:,y,x,iBlur);

        subplot(3,3,iBlur);
        hold off
        plot(curSteps);
        curAxis = axis();

        maxAxis = max(maxAxis, max(abs(curAxis(3:4))));
    end

    for iBlur = 1:length(blurSigmas)
        subplot(3,3,iBlur);
        axis([curAxis(1:2), -maxAxis, maxAxis])
    end
end % function parseLedCode_plot_changingCenter()
