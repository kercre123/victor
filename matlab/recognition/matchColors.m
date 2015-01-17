
function matchColors()
    
    histogramMetrics = {'Correl', 'ChiSqr', 'Intersect', 'Bhattacharyya'};
    
    cameraId = 0;
    
    imageDirectory = '~/Documents/Anki/products-cozmo-large-files/cars/';
    carNames = {'nuke', 'shock', 'skull', 'thermo'};
    
    roi = [100, 500, 130, 350]; % [x0, x1, y0, x1] 
    
    numCars = length(carNames);
    
%     histogramEdges = {linspace(0,180,30+1), linspace(0,256,32+1)};
%     histogramEdges = {linspace(0,180,15+1), linspace(0,256,16+1)};
%     histogramEdges = {linspace(0,180,64+1), linspace(0,256,64+1)};

    histogramEdges = {linspace(0,180,64+1)};
    
    carImages = cell(numCars,1);
    carMasks = cell(numCars,1);
    carImagesHsv = cell(numCars,1);
    carHistograms = cell(numCars,1);
    for iCar = 1:numCars
        carImages{iCar} = imread([imageDirectory, carNames{iCar}, '.png']);
        carMasks{iCar} = rgb2gray2(imread([imageDirectory, carNames{iCar}, '_mask.png']));
        
        carImages{iCar}(~carMasks{iCar}) = 0;

        carImagesHsv{iCar} = cv.cvtColor(carImages{iCar}, 'RGB2HSV');
        
%         carHistograms{iCar} =  cv.calcHist(carImagesHsv{iCar}(:,:,1:2), histogramEdges, 'Mask', carMasks{iCar});
        carHistograms{iCar} =  cv.calcHist(carImagesHsv{iCar}(:,:,1), histogramEdges, 'Mask', carMasks{iCar});
        carHistograms{iCar} = carHistograms{iCar} / sum(carHistograms{iCar}(:));
        
        % TODO: normalize?
    end
    
    trainingDistances = zeros(numCars, numCars, length(histogramMetrics));
    for iMetric = 1:length(histogramMetrics)
        for iCar = 1:numCars
            for jCar = 1:numCars
            
                trainingDistances(iCar, jCar, iMetric) = cv.compareHist(carHistograms{iCar}, carHistograms{jCar}, 'Method', histogramMetrics{iMetric});
            end
        end
        
        disp(trainingDistances(:,:,iMetric));
    end
    
    videoCapture = cv.VideoCapture(cameraId);
    
    while true
        image = videoCapture.read();
        imageHsv = cv.cvtColor(image, 'RGB2HSV');
        
        newHistogram = cv.calcHist(imageHsv(roi(3):roi(4), roi(1):roi(2), 1:2), histogramEdges);
        
        newHistogram = newHistogram / sum(newHistogram(:));
        
        distances = zeros(numCars, length(histogramMetrics));
        
        for iCar = 1:numCars
            fprintf('%s\t', carNames{iCar});
        end
        disp(' ');
        
        for iMetric = 1:length(histogramMetrics)
            for iCar = 1:numCars
                distances(iCar, iMetric) = cv.compareHist(carHistograms{iCar}, newHistogram, 'Method', histogramMetrics{iMetric});
            end
            
            disp(sprintf('%0.2f\t', distances(:,iMetric)'));
        end
        
        disp(' ');
        
        toShowImage = image;
        
        toShowImage((roi(3)-1):(roi(3)+1), roi(1):roi(2), :) = 255;
        toShowImage((roi(4)-1):(roi(4)+1), roi(1):roi(2), :) = 255;
        toShowImage(roi(3):roi(4), (roi(1)-1):(roi(1)+1), :) = 255;
        toShowImage(roi(3):roi(4), (roi(2)-1):(roi(2)+1), :) = 255;
        
        imshow(toShowImage);
        pause(0.01);
    end % while true
    
    keyboard