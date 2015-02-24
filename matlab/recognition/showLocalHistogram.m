% function showLocalHistogram()

function showLocalHistogram()
    showCrosshair = false;
    
    showBox = true;
    
    histogramRectangle = [70,90,50,70]; % l,r,t,b
    
    runningMeans = zeros(100,3);
    while true
        [image, ~] = parseLedCode_getNextImage('usbcam', [], [], [120,160], []);
        
        imageToShow = image;
        
        if showCrosshair
            centerY = ceil(size(imageToShow,1) / 2);
            centerX = ceil(size(imageToShow,2) / 2);
            imageToShow((centerY-1):(centerY+1), :, :) = 0;
            imageToShow(:, (centerX-1):(centerX+1), :) = 0;
            imageToShow(centerY, :, :) = 255;
            imageToShow(:, centerX, :) = 255;
        end
        
        if showBox
            imageToShow(histogramRectangle(3):histogramRectangle(4), histogramRectangle(1):histogramRectangle(2), :) = 0;
            imageToShow((histogramRectangle(3)+1):(histogramRectangle(4)-1), (histogramRectangle(1)+1):(histogramRectangle(2)-1), :) = 255;
            imageToShow((histogramRectangle(3)+2):(histogramRectangle(4)-2), (histogramRectangle(1)+2):(histogramRectangle(2)-2), :) = 0;
            imageToShow((histogramRectangle(3)+3):(histogramRectangle(4)-3), (histogramRectangle(1)+3):(histogramRectangle(2)-3), :) = image((histogramRectangle(3)+3):(histogramRectangle(4)-3), (histogramRectangle(1)+3):(histogramRectangle(2)-3), :);
        end
        
        figure(1);
        imshow(imageToShow);
        
        
        red = image(histogramRectangle(3):histogramRectangle(4), histogramRectangle(1):histogramRectangle(2), 1);
        red = sort(red(:));
        green = image(histogramRectangle(3):histogramRectangle(4), histogramRectangle(1):histogramRectangle(2), 2);
        green = sort(green(:));
        blue = image(histogramRectangle(3):histogramRectangle(4), histogramRectangle(1):histogramRectangle(2), 3);
        blue = sort(blue(:));
        
        numItems = length(blue);
        
        figure(10);
        hold off;
        plot(red, 'r');
        hold on;
        plot(green, 'g');
        plot(blue, 'b');
        
        %         plot([0,numItems], [mean(red),mean(red)], 'r');
        %         plot([0,numItems], [mean(green),mean(green)], 'g');
        %         plot([0,numItems], [mean(blue),mean(blue)], 'b');
        
        a = axis();
        axis([a(1:2), 0, 260]);
        
        runningMeans(1:(end-1),:) = runningMeans(2:end,:);
        runningMeans(end,:) = [mean(red), mean(green), mean(blue)];
        
        figure(11);
        hold off;
        plot(runningMeans(:,1), 'r');
        hold on;
        plot(runningMeans(:,2), 'g');
        plot(runningMeans(:,3), 'b');
        
        
    end
end % function showLocalHistogram()
