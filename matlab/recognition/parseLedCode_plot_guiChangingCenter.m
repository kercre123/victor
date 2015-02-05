
function parseLedCode_plot_guiChangingCenter(colorImages, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps)
    disp('Click a point');

    while true
        figure(1);
        imshows(colorImages(:,:,:,ceil(end/2)))

        [x,y] = ginput(1);
        x = round(x);
        y = round(y);

        if x < 1 || x > size(blurredImages_small,2) || y < 1 || y > size(blurredImages_small,1)
            continue;
        end
        
        disp(sprintf('Clicked (%d,%d)', x, y));

        parseLedCode_plot_changingCenter(x, y, blurSigmas, blurredImages_small, blurredImages_large, blurredImages_outsideRing, blurredImages_inside, steps);
    end % while true
end % function parseLedCode_plot_guiChangingCenter()
