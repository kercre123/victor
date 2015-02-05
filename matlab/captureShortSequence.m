% function images = captureShortSequence(varargin)

% Capture some images with the default settings
% images = captureShortSequence();

% Capture a sequence as fast as possible, and save the output
% savePattern = 'c:/tmp/image_%05d.png'; images = captureShortSequence('resolution', [120,160], 'numImagesToCapture', 100, 'showImages', false); for i=1:size(images,4) imwrite(images(:,:,:,i), sprintf(savePattern,i-1)); end;

function images = captureShortSequence(varargin)
    captureId = 1;
    resolution = [240,320];
    settingsGui = false; % If true, just set the settings, but don't return the images
    whiteBalanceTemperature = 4000;
    numImagesToCapture = 100;
    showImages = true;
    showCrosshair = true;
    
    parseVarargin(varargin)
    
    disp('Warning: if your needs are time critical, use the C version, captureImages.cpp');
    
    cap = cv.VideoCapture(captureId);
    
    cap.set('framewidth', resolution(2));
    cap.set('frameheight', resolution(1));
    cap.set('whitebalanceblueu', whiteBalanceTemperature);
    
    if settingsGui
        cap.set('settings', 1);
        
        disp('Press enter when you''re done setting the settings');
        
        pause();
        
        % Make sure all the settings are updated
        for i = 1:30
            cap.read();
        end
        
        images = [];
        
        return;
    end
    
    image = cap.read();
    
    images = zeros([size(image), numImagesToCapture], 'uint8');
    
    tic
    for iFrame = 1:numImagesToCapture
        
        images(:,:,:,iFrame) = cap.read();
        
        if showImages
            imageToShow = images(:,:,:,iFrame);
            
            if showCrosshair
                centerY = ceil(size(imageToShow,1) / 2);
                centerX = ceil(size(imageToShow,2) / 2);
                imageToShow((centerY-1):(centerY+1), :, :) = 0;
                imageToShow(:, (centerX-1):(centerX+1), :) = 0;
                imageToShow(centerY, :, :) = 255;
                imageToShow(:, centerX, :) = 255;
            end
            
            imshow(imageToShow)
            pause(0.001);
        end
    end
    timeElapsed = toc();
    
    disp(sprintf('Captured %d images at %0.2f fps', numImagesToCapture, 1/(timeElapsed / numImagesToCapture)));
    