% function createFiducialDetectionTest(outputFilename, inputFilenameRanges)

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00000_frontoParallel_100mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_52_34_frame0.png','cozmo_date2014_06_04_time16_52_43_frame0.png'}, 100, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00001_frontoParallel_100mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_52_57_frame0.png','cozmo_date2014_06_04_time16_53_06_frame0.png'}, 100, 0, 1)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00002_frontoParallel_150mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_53_19_frame0.png','cozmo_date2014_06_04_time16_53_28_frame0.png'}, 150, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00003_frontoParallel_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_53_53_frame0.png','cozmo_date2014_06_04_time16_54_02_frame0.png'}, 200, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00004_frontoParallel_250mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_54_26_frame0.png','cozmo_date2014_06_04_time16_54_36_frame0.png'}, 250, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00005_frontoParallel_250mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_54_49_frame0.png','cozmo_date2014_06_04_time16_54_58_frame0.png'}, 250, 0, 1)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00006_frontoParallel_300mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_55_11_frame0.png','cozmo_date2014_06_04_time16_55_20_frame0.png'}, 300, 0, 0)

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00007_dice_frontoParallel_100mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_00_46_frame0.png','cozmo_date2014_06_10_time13_00_55_frame1.png'}, 100, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00008_dice_frontoParallel_100mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_01_09_frame0.png','cozmo_date2014_06_10_time13_01_18_frame0.png'}, 100, 0, 1)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00009_dice_frontoParallel_150mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_01_31_frame0.png','cozmo_date2014_06_10_time13_01_40_frame0.png'}, 150, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00010_dice_frontoParallel_150mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_01_53_frame0.png','cozmo_date2014_06_10_time13_02_03_frame0.png'}, 150, 0, 1)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00011_dice_frontoParallel_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_02_38_frame0.png','cozmo_date2014_06_10_time13_02_47_frame0.png'}, 200, 0, 0)
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00012_dice_frontoParallel_200mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_10_time13_03_01_frame0.png','cozmo_date2014_06_10_time13_03_10_frame1.png'}, 200, 0, 1)

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00013_sheet2_frontoParallel_50mm_lightOff.json',  'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_07_33_frame0.png','cozmo_date2014_06_16_time13_07_43_frame0.png'}, 50, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00014_sheet2_frontoParallel_50mm_lightOn.json',  'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_08_07_frame0.png','cozmo_date2014_06_16_time13_08_16_frame0.png'}, 50, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00015_sheet2_frontoParallel_100mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_08_29_frame0.png','cozmo_date2014_06_16_time13_08_38_frame0.png'}, 100, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00016_sheet2_frontoParallel_100mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_08_51_frame0.png','cozmo_date2014_06_16_time13_09_01_frame0.png'}, 100, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00017_sheet2_frontoParallel_150mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_09_14_frame0.png','cozmo_date2014_06_16_time13_09_22_frame0.png'}, 150, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00018_sheet2_frontoParallel_150mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_09_47_frame0.png','cozmo_date2014_06_16_time13_09_56_frame1.png'}, 150, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00019_sheet2_frontoParallel_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_10_20_frame0.png','cozmo_date2014_06_16_time13_10_30_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00020_sheet2_frontoParallel_250mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_11_05_frame0.png','cozmo_date2014_06_16_time13_11_14_frame0.png'}, 250, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00021_sheet2_frontoParallel_250mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_11_27_frame0.png','cozmo_date2014_06_16_time13_11_36_frame0.png'}, 250, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00022_sheet2_frontoParallel_300mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_12_01_frame0.png','cozmo_date2014_06_16_time13_12_10_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00023_sheet2_frontoParallel_300mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_12_23_frame0.png','cozmo_date2014_06_16_time13_12_32_frame0.png'}, 300, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00024_sheet2_frontoParallel_350mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_12_45_frame0.png','cozmo_date2014_06_16_time13_12_56_frame0.png'}, 350, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00025_sheet2_frontoParallel_350mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_13_08_frame0.png','cozmo_date2014_06_16_time13_13_16_frame0.png'}, 350, 0, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00026_sheet2_frontoParallel_400mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_13_41_frame0.png','cozmo_date2014_06_16_time13_13_50_frame0.png'}, 400, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00027_sheet2_frontoParallel_400mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_14_03_frame0.png','cozmo_date2014_06_16_time13_14_12_frame0.png'}, 400, 0, 1);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00028_sheet2_45degrees_100mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_36_51_frame0.png','cozmo_date2014_06_16_time13_37_00_frame0.png'}, 100, 45, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00029_sheet2_45degrees_100mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_37_13_frame0.png','cozmo_date2014_06_16_time13_37_22_frame0.png'}, 100, 45, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00030_sheet2_45degrees_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_38_08_frame0.png','cozmo_date2014_06_16_time13_38_17_frame0.png'}, 200, 45, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00031_sheet2_45degrees_200mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_38_31_frame0.png','cozmo_date2014_06_16_time13_38_40_frame0.png'}, 200, 45, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00032_sheet2_45degrees_300mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_39_37_frame0.png','cozmo_date2014_06_16_time13_39_46_frame0.png'}, 300, 45, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00033_sheet2_45degrees_300mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_40_00_frame0.png','cozmo_date2014_06_16_time13_40_09_frame0.png'}, 300, 45, 1);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00034_sheet2_34degrees_100mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_41_51_frame0.png','cozmo_date2014_06_16_time13_42_00_frame0.png'}, 100, 34, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00035_sheet2_34degrees_100mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_42_13_frame0.png','cozmo_date2014_06_16_time13_42_22_frame0.png'}, 100, 34, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00036_sheet2_34degrees_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_42_57_frame0.png','cozmo_date2014_06_16_time13_43_06_frame0.png'}, 200, 34, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00037_sheet2_34degrees_200mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_43_31_frame0.png','cozmo_date2014_06_16_time13_43_40_frame0.png'}, 200, 34, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00038_sheet2_34degrees_300mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_44_28_frame0.png','cozmo_date2014_06_16_time13_44_36_frame0.png'}, 300, 34, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00039_sheet2_34degrees_300mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_45_05_frame0.png','cozmo_date2014_06_16_time13_45_16_frame0.png'}, 300, 34, 1);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00040_sheet2_72degrees_200mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_46_51_frame0.png','cozmo_date2014_06_16_time13_47_00_frame0.png'}, 200, 72, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00041_sheet2_72degrees_200mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_47_13_frame0.png','cozmo_date2014_06_16_time13_47_22_frame0.png'}, 200, 72, 1);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00042_sheet2_72degrees_300mm_lightOff.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_47_57_frame0.png','cozmo_date2014_06_16_time13_48_07_frame0.png'}, 300, 72, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00043_sheet2_72degrees_300mm_lightOn.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_16_time13_48_20_frame0.png','cozmo_date2014_06_16_time13_48_29_frame0.png'}, 300, 72, 1);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00044_blurTest.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'C:/Anki/products-cozmo-large-files/systemTestsData/images/', {'cozmo_date2014_07_21_time17_44_04_frame1.png','cozmo_date2014_07_21_time17_44_33_frame0.png'}, 150, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00045_grayscaleBackground_tone0.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time15_30_26_frame0.png','cozmo_date2014_06_20_time15_30_35_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00046_grayscaleBackground_tone1.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time15_31_11_frame0.png','cozmo_date2014_06_20_time15_31_20_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00047_grayscaleBackground_tone2.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time15_31_33_frame0.png','cozmo_date2014_06_20_time15_31_42_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00048_grayscaleBackground_tone3.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time15_32_06_frame0.png','cozmo_date2014_06_20_time15_32_15_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00049_grayscaleBackground_tone4.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time15_32_28_frame0.png','cozmo_date2014_06_20_time15_32_37_frame0.png'}, 300, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00050_pattern00.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_28_06_frame1.png','cozmo_date2014_06_20_time16_28_11_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00051_pattern01.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_28_24_frame0.png','cozmo_date2014_06_20_time16_28_28_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00052_pattern02.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_28_35_frame1.png','cozmo_date2014_06_20_time16_28_40_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00053_pattern03.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_28_52_frame1.png','cozmo_date2014_06_20_time16_28_57_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00054_pattern04.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_29_10_frame0.png','cozmo_date2014_06_20_time16_29_14_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00055_pattern05.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_29_27_frame1.png','cozmo_date2014_06_20_time16_29_32_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00056_pattern06.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_29_38_frame1.png','cozmo_date2014_06_20_time16_29_43_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00057_pattern07.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_01_frame1.png','cozmo_date2014_06_20_time16_30_06_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00058_pattern08.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_13_frame0.png','cozmo_date2014_06_20_time16_30_18_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00059_pattern09.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_24_frame3.png','cozmo_date2014_06_20_time16_30_29_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00060_pattern10.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_36_frame0.png','cozmo_date2014_06_20_time16_30_41_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00061_pattern11.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_47_frame1.png','cozmo_date2014_06_20_time16_30_52_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00062_pattern12.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_30_59_frame0.png','cozmo_date2014_06_20_time16_31_04_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00063_pattern13.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_31_10_frame1.png','cozmo_date2014_06_20_time16_31_15_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00064_pattern14.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_31_22_frame0.png','cozmo_date2014_06_20_time16_31_27_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00065_pattern15.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_31_33_frame1.png','cozmo_date2014_06_20_time16_31_38_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00066_pattern16.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_31_51_frame0.png','cozmo_date2014_06_20_time16_31_55_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00067_pattern17.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_32_02_frame1.png','cozmo_date2014_06_20_time16_32_07_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00068_pattern18.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_32_14_frame0.png','cozmo_date2014_06_20_time16_32_18_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00069_pattern19.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_32_25_frame1.png','cozmo_date2014_06_20_time16_32_30_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00070_pattern20.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_32_37_frame0.png','cozmo_date2014_06_20_time16_32_42_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00071_pattern21.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_32_48_frame1.png','cozmo_date2014_06_20_time16_32_53_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00072_pattern22.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_33_00_frame0.png','cozmo_date2014_06_20_time16_33_04_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00073_pattern23.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_33_12_frame2.png','cozmo_date2014_06_20_time16_33_16_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00074_pattern24.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_33_28_frame1.png','cozmo_date2014_06_20_time16_33_33_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00075_pattern25.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_33_40_frame0.png','cozmo_date2014_06_20_time16_33_45_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00076_pattern26.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_34_03_frame0.png','cozmo_date2014_06_20_time16_34_08_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00077_pattern27.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_34_20_frame1.png','cozmo_date2014_06_20_time16_34_26_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00078_pattern28.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_20_time16_34_37_frame1.png','cozmo_date2014_06_20_time16_34_42_frame1.png'}, 200, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00079_inPlaneRotations_00degrees_200mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_51_37_frame3.png','cozmo_date2014_07_23_time14_51_40_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00080_inPlaneRotations_00degrees_300mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_52_26_frame2.png','cozmo_date2014_07_23_time14_52_30_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00081_inPlaneRotations_00degrees_400mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_52_53_frame3.png','cozmo_date2014_07_23_time14_52_56_frame4.png'}, 400, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00082_inPlaneRotations_30degrees_200mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_57_00_frame3.png','cozmo_date2014_07_23_time14_57_02_frame1.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00083_inPlaneRotations_30degrees_300mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_58_26_frame2.png','cozmo_date2014_07_23_time14_58_30_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00084_inPlaneRotations_30degrees_400mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time14_59_24_frame1.png','cozmo_date2014_07_23_time14_59_27_frame1.png'}, 400, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00085_inPlaneRotations_45degrees_200mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_02_48_frame0.png','cozmo_date2014_07_23_time15_02_51_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00086_inPlaneRotations_45degrees_300mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_03_33_frame2.png','cozmo_date2014_07_23_time15_03_36_frame2.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00087_inPlaneRotations_45degrees_400mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_04_16_frame0.png','cozmo_date2014_07_23_time15_04_18_frame2.png'}, 400, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00088_inPlaneRotations_60degrees_200mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_08_06_frame1.png','cozmo_date2014_07_23_time15_08_09_frame0.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00089_inPlaneRotations_60degrees_300mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_08_44_frame0.png','cozmo_date2014_07_23_time15_08_47_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00090_inPlaneRotations_60degrees_400mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_09_15_frame0.png','cozmo_date2014_07_23_time15_09_17_frame4.png'}, 400, 0, 0);

% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00091_inPlaneRotations_90degrees_200mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_10_00_frame2.png','cozmo_date2014_07_23_time15_10_03_frame2.png'}, 200, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00092_inPlaneRotations_90degrees_300mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_10_19_frame2.png','cozmo_date2014_07_23_time15_10_23_frame0.png'}, 300, 0, 0);
% createFiducialDetectionTest('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/fiducialDetection_00093_inPlaneRotations_90degrees_400mm.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/images', 'Z:/Box Sync/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_07_23_time15_10_58_frame0.png','cozmo_date2014_07_23_time15_11_02_frame0.png'}, 400, 0, 0);

% tests 94-99 are from the auto-generated tests from C:\Anki\products-cozmo\basestation\test\blockWorldTests on 2014-07-21

% tests 100-123 are from the iphone5s front camera pictures

% tests 124-125 are from challenging simulator images captured by Kevin

function createFiducialDetectionTest(outputFilename, imageCopyPath, inputDirectory, inputFilenameRange, distance, angle, light)

numImagesRequired = 10; % for the .1:.1:1.0 exposures

fullFilename = strrep(mfilename('fullpath'), '\', '/');
slashIndexes = strfind(fullFilename, '/');
fullFilenamePath = fullFilename(1:(slashIndexes(end)));
templateFilename = [fullFilenamePath, 'testTemplate.json'];

if imageCopyPath(end) ~= '/'
    imageCopyPath = [imageCopyPath, '/'];
end

if inputDirectory(end) ~= '/'
    inputDirectory = [inputDirectory, '/'];
end

jsonTestData = loadjson(templateFilename);
 
possibleInputFiles = dir([inputDirectory, '*.png']);

jsonTestData.Poses = [];

firstFile = inputFilenameRange{1};
lastFile = inputFilenameRange{2};

startIndex = -1;
for iIn = 1:length(possibleInputFiles)
    if strcmp(firstFile, possibleInputFiles(iIn).name)
        startIndex = iIn;
        break;
    end
end

endIndex = -1;
for iIn = startIndex:length(possibleInputFiles)
    if strcmp(lastFile, possibleInputFiles(iIn).name)
        endIndex = iIn;
        break;
    end
end

numImageFilesFound = (endIndex - startIndex + 1);
if numImageFilesFound < numImagesRequired
    disp(sprintf('%s has %d image files', outputFilename, numImageFilesFound));
    return;
elseif numImageFilesFound > numImagesRequired
    disp(sprintf('%s has %d image files (perhaps there was a Box Sync error?', outputFilename, numImageFilesFound));
    return;
end

exposure = 0.1;
for iIn = startIndex:endIndex
    curPose.ImageFile = ['../images/', possibleInputFiles(iIn).name];
    curPose.Scene.CameraExposure = exposure;
    curPose.Scene.Distance = distance;
    curPose.Scene.Angle = angle;    
    curPose.Scene.Light = light;
    jsonTestData.Poses{end+1} = curPose;
    exposure = exposure + 0.1;
    
%     if ~strcmpi(inputDirectory, imageCopyPath)
        sceneString = sprintf('Exposure:%f Distance:%dmm Angle:%d Light:%d', exposure, distance, angle, light);
        im = imread([inputDirectory,possibleInputFiles(iIn).name]);
        imwrite(im, [imageCopyPath, possibleInputFiles(iIn).name], 'comment', sceneString);
%     end
end

% For speed, also save a copy with just the filenames
% outputFilename_justFilenames = [outputFilename(1:(end-5)), '_justFilenames.json'];
outputFilename_all = [outputFilename(1:(end-5)), '_all.json'];

% jsonTestDataFilenamesOnly.Poses = cell(length(jsonTestData.Poses), 1);
% for iPose = 1:length(jsonTestDataFilenamesOnly.Poses)
%     jsonTestDataFilenamesOnly.Poses{iPose}.ImageFile = jsonTestData.Poses{iPose}.ImageFile;
% end

disp(sprintf('Saving %s...', outputFilename_all));
% disp(sprintf('Saving %s and %s...', outputFilename_all, outputFilename_justFilenames));
savejson('', jsonTestData, outputFilename_all);
% savejson('', jsonTestDataFilenamesOnly, outputFilename_justFilenames);

% keyboard
