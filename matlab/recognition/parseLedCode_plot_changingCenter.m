function parseLedCode_plot_changingCenter(x, y, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps)
    
    if x < 1 || x > size(blurredImages_small,2) || y < 1 || y > size(blurredImages_small,1)
        return;
    end
    
    figure(2);
    
    numColorChannels = 1;
    
    plotMultipleColors = false;
    
    if ndims(blurredImages_small) == 5
        plotMultipleColors = true;
        numColorChannels = max(numColorChannels, size(blurredImages_small,3));
    end
    
    if ndims(blurredImages_large)
        plotMultipleColors = true;
        numColorChannels = max(numColorChannels, size(blurredImages_large,3));
    end
    
    if ndims(blurredImages_outsideRing)
        plotMultipleColors = true;
        numColorChannels = max(numColorChannels, size(blurredImages_outsideRing,3));
    end
    
    if ndims(blurredImages_inside) == 5
        plotMultipleColors = true;
        numColorChannels = max(numColorChannels, size(blurredImages_inside,3));
    end
        
    if ~plotMultipleColors
        for iBlur = 1:length(blurSigmas)
            subplot(3,3,iBlur);
            hold off
            plot(squeeze(blurredImages_small(y, x, :, iBlur))', 'b')
            hold on
            plot(squeeze(blurredImages_large(y, x, :, iBlur))', 'g')
            plot(squeeze(blurredImages_outsideRing(y, x, :, iBlur))', 'r')
            plot(squeeze(blurredImages_inside(y, x, :, iBlur))', 'k')
            
            a = axis();
            a(3:4) = [0,260];
            axis(a);
        end
        
        figure(3);
        maxAxis = 0;
        for iBlur = 1:length(blurSigmas)
            curSteps = steps(:,y,x,iBlur);
            
            subplot(3,3,iBlur);
            hold off
            plot(curSteps);
            curAxis = axis();
            
            maxAxis = max(maxAxis, max(abs(curAxis(3:4))));
        end % for iBlur = 1:length(blurSigmas)
    else % if ~plotMultipleColors
        ci = 1;
        for iColor = 1:numColorChannels
            for iBlur = 1:length(blurSigmas)
                heldOff = false;
                subplot(numColorChannels,length(blurSigmas),ci);
                hold off
                
                if ndims(blurredImages_small) == 4
                    plot(squeeze(blurredImages_small(y, x, iColor, iBlur))', 'b');
                    hold on
                else
                    if size(blurredImages_small,3) >= iColor
                        plot(squeeze(blurredImages_small(y, x, iColor, :, iBlur))', 'b');
                        hold on
                    end
                end
                
                if ndims(blurredImages_small) == 4
                    plot(squeeze(blurredImages_large(y, x, iColor, iBlur))', 'g');
                    hold on
                else
                    if size(blurredImages_large,3) >= iColor
                        plot(squeeze(blurredImages_large(y, x, iColor, :, iBlur))', 'g');
                        hold on
                    end
                end
                
                if ndims(blurredImages_small) == 4
                    plot(squeeze(blurredImages_outsideRing(y, x, iColor, iBlur))', 'r');
                    hold on
                else
                    if size(blurredImages_outsideRing,3) >= iColor
                        plot(squeeze(blurredImages_outsideRing(y, x, iColor, :, iBlur))', 'r');
                        hold on
                    end
                end
                
                if ndims(blurredImages_small) == 4
                    plot(squeeze(blurredImages_inside(y, x, iColor, iBlur))', 'k');
                    hold on
                else
                    if size(blurredImages_inside,3) >= iColor
                        plot(squeeze(blurredImages_inside(y, x, iColor, :, iBlur))', 'k');
                        hold on
                    end
                end
                
                a = axis();
                a(3:4) = [0,260];
                axis(a);
                
                ci = ci + 1;
            end % for iBlur = 1:length(blurSigmas)
        end % for iColor = 1:numColorChannels
        
        %
        %         figure(3);
        %         maxAxis = 0;
        %         for iBlur = 1:length(blurSigmas)
        %             curSteps = steps(:,y,x,iBlur);
        %
        %             subplot(3,3,iBlur);
        %             hold off
        %             plot(curSteps);
        %             curAxis = axis();
        %
        %             maxAxis = max(maxAxis, max(abs(curAxis(3:4))));
        %         end % for iBlur = 1:length(blurSigmas)
        %
        %     for iBlur = 1:length(blurSigmas)
        %         subplot(3,3,iBlur);
        %         axis([curAxis(1:2), -maxAxis, maxAxis])
        %     end
    end % if ~plotMultipleColors ... else
    
end % function parseLedCode_plot_changingCenter()
