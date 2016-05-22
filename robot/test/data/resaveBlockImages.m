
function resaveBlockImages()
    
    inDirectory1 = 'Z:/Box Sync/Cozmo SE/blockImages/';
    inDirectory2 = 'Z:/Box Sync/Cozmo SE/systemTestImages_all/';
    outDirectory = 'C:/Anki/products-cozmo/robot/test/data/';
    
    im50 = imread([inDirectory1, 'blockImages00050.png']);
    saveImageAsHeader(rgb2gray2(im50), [outDirectory, 'blockImage50.h'], 'blockImage50');
    saveImageAsHeader(imresize(rgb2gray2(im50),[240,320]), [outDirectory, 'blockImage50_320x240.h'], 'blockImage50_320x240');
    
    im189 = imread([inDirectory1, 'blockImages00189.png']);
    saveImageAsHeader(imresize(rgb2gray2(im189),[60,80]), [outDirectory, 'blockImages00189_80x60.h'], 'blockImages00189_80x60');
    
    im190 = imread([inDirectory1, 'blockImages00190.png']);
    saveImageAsHeader(imresize(rgb2gray2(im190),[60,80]), [outDirectory, 'blockImages00190_80x60.h'], 'blockImages00190_80x60');
    
    cozmo_date2014_01_29_time11_41_05_frame10 = imread([inDirectory2, 'cozmo_date2014_01_29_time11_41_05_frame10.png']);
    saveImageAsHeader(imresize(rgb2gray2(cozmo_date2014_01_29_time11_41_05_frame10),[240,320]), [outDirectory, 'cozmo_date2014_01_29_time11_41_05_frame10_320x240.h'], 'cozmo_date2014_01_29_time11_41_05_frame10_320x240');
    
    cozmo_date2014_01_29_time11_41_05_frame12 = imread([inDirectory2, 'cozmo_date2014_01_29_time11_41_05_frame12.png']);
    saveImageAsHeader(imresize(rgb2gray2(cozmo_date2014_01_29_time11_41_05_frame12),[240,320]), [outDirectory, 'cozmo_date2014_01_29_time11_41_05_frame12_320x240.h'], 'cozmo_date2014_01_29_time11_41_05_frame12_320x240');
    
    cozmo_date2014_04_04_time17_40_08_frame0 = imread([inDirectory2, 'cozmo_date2014_04_04_time17_40_08_frame0.png']);
    saveImageAsHeader(rgb2gray2(cozmo_date2014_04_04_time17_40_08_frame0), [outDirectory, 'cozmo_date2014_04_04_time17_40_08_frame0.h'], 'cozmo_date2014_04_04_time17_40_08_frame0');
    
    cozmo_date2014_04_10_time16_15_40_frame0 = imread([inDirectory2, 'cozmo_date2014_04_10_time16_15_40_frame0.png']);
    saveImageAsHeader(rgb2gray2(cozmo_date2014_04_10_time16_15_40_frame0), [outDirectory, 'cozmo_date2014_04_10_time16_15_40_frame0.h'], 'cozmo_date2014_04_10_time16_15_40_frame0');