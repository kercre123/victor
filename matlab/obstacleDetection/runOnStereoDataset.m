function runOnStereoDataset()
    
    cd('~/Downloads/spsstereo/stereoDataset')
    
    resaveImages = false;
    if resaveImages
        for i = 0:50
            leftIm = imread(sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified_%d.png', i));
            rightIm = imread(sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified_%d.png', i));

            imwrite(leftIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified_%d.pgm', i)); 
            imwrite(rightIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified_%d.pgm', i)); 
            
            leftIm = uint8((double(leftIm(1:2:end,1:2:end)) + double(leftIm(2:2:end,1:2:end)) + double(leftIm(1:2:end,2:2:end)) + double(leftIm(2:2:end,2:2:end))) / 4);
            rightIm = uint8((double(rightIm(1:2:end,1:2:end)) + double(rightIm(2:2:end,1:2:end)) + double(rightIm(1:2:end,2:2:end)) + double(rightIm(2:2:end,2:2:end))) / 4);

            imwrite(leftIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified320_%d.png', i)); 
            imwrite(rightIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified320_%d.png', i)); 
            
            imwrite(leftIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified320_%d.pgm', i)); 
            imwrite(rightIm, sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified320_%d.pgm', i)); 
        end
    end % if resaveImages
           
    stereoColormap = computeStereoColormap(2048);
    stereoColormap = stereoColormap(:,3:-1:1); % For matlab
 
    runWhich = 4;
    
    for i = 0:50
        if runWhich == 1 || runWhich == 2
            tic        
            if runWhich == 1
                system(sprintf('../spsstereo ~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified_%d.png ~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified_%d.png',i ,i));
            else
                system(sprintf('../spsstereo ~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified320_%d.png ~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified320_%d.png',i ,i));
            end
            toc
            
            disp(sprintf('%d/%d in %f', i, 50, toc()))
            
            imBoundary = imread(sprintf('stereo_leftRectified320_%d_boundary.png', i));
            imDisparity = imread(sprintf('stereo_leftRectified320_%d_left_disparity.png', i));

            disparityToShow = bitshift(imDisparity, -4);       

            disparityToShow = disparityToShow + 1;
            disparityToShow(disparityToShow > length(stereoColormap)) = length(stereoColormap);

            colorDisparityToShow = zeros([size(disparityToShow), 3], 'uint8');
            for y = 1:size(disparityToShow,1)
                colorDisparityToShow(y,1:size(disparityToShow,2),:) = stereoColormap(disparityToShow(y,1:size(disparityToShow,2)),:);
            end

            totalImage = uint8([imBoundary, repmat(bitshift(imDisparity, -8),[1,1,3]), colorDisparityToShow]);
            imshows(totalImage)

            imwrite(totalImage, sprintf('totalOutput%d.png', i));
        elseif runWhich == 3
            executable = '~/Downloads/rSGM/src/rSGMCmd ';
            leftFilename = sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified_%d.pgm ', i);
            rightFilename = sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified_%d.pgm ',i);
            dispFilename =  sprintf('~/Downloads/rSGM/stereoDataset/disp_%d.pgm ',i);
            params = '8 5';
            
            tic
            system([executable, leftFilename, rightFilename, dispFilename, params])
            
            disp(sprintf('%d/%d in %f', i, 50, toc()))
            
        elseif runWhich == 4
            executable = '~/Downloads/rSGM/src/rSGMCmd ';
            leftFilename = sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_leftRectified320_%d.pgm ', i);
            rightFilename = sprintf('~/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_rightRectified320_%d.pgm ',i);
            dispFilename =  sprintf('~/Downloads/rSGM/stereoDataset/disp320B_%d.pgm ',i);
            params = '8 4';
            
            tic
            system([executable, leftFilename, rightFilename, dispFilename, params])            
            
            disp(sprintf('%d/%d in %f', i, 50, toc()))
        else
            assert(false)
        end
        
        
        
    end
    
    
