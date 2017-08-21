# Combs NeedsHub and HomeHub for unused art assets, and then deletes them
# Assets that are in use are reported in metaReportBasedOnArt
# Assets that are NOT in use are reported in metaReportBasedOnArtUnused
#!/usr/bin/python2

import os
import glob
import json

# Control variables:
# Set to True to skip reporting used files and only report unused ones. 
FAST_REPORT_UNUSED_FILES = True

# Set to True to delete unused files
DELETE_UNUSED_FILES = True

# The directories to get pngs from
ART_DIRECTORY_PATHS = ['Cozmo/Assets/AssetBundles/NeedsHub/Art','Cozmo/Assets/AssetBundles/HomeHub/Art']

# The directory to check for usage of pngs
ASSET_DIRECTORY_PATH = 'Cozmo/Assets/AssetBundles'
FONT_DIRECTORY_NAME = 'Font'
ART_DIRECTORY_NAME = 'Art'
PLATFORM_SPECIFIC_DIRECTORY_NAME = 'PlatformSpecificStartViewUISprites'
LANGUAGE_SPECIFIC_DIRECTORY_NAME = 'LanguageSpecificStartViewUISprites'

# Location of the ui_themes.json file from the Theme System
THEME_JSON_PATH = 'Cozmo/Assets/Resources/ThemeSystem/ui_themes.json'
RESOURCE_KEY_SUFFIX = 'ResourceKey'
RESOURCE_KEY_ART_PATH_DELTA = 'Cozmo/'

# Relative paths to report files; git ignored
REPORT_PATH = 'metaReportBasedOnArt.txt'
UNUSED_REPORT_PATH = 'metaReportBasedOnArtUnused.txt'

PNG_EXTENSION = '.png'
META_EXTENSION = '.meta'

UHD_KEYWORD = 'UHD'
HD_KEYWORD = 'HD'
SD_KEYWORD = 'SD'

# Flattens a JSON object
# From: https://medium.com/@amirziai/flattening-json-objects-in-python-f5343c794b10
def flatten_json(y):
    out = {}

    def flatten(x, name=''):
        if type(x) is dict:
            for a in x:
                flatten(x[a], name + a + '_')
        elif type(x) is list:
            i = 0
            for a in x:
                flatten(a, name + str(i) + '_')
                i += 1
        else:
            out[name[:-1]] = x

    flatten(y)
    return out

# Returns a list of the resource keys that are in use by the Theme system
def get_image_resource_keys():
	# Open and save the themes json file so that we can check if assets are used for skinning
	themeJsonFile = open(THEME_JSON_PATH, 'r')
	themeJsonContents = themeJsonFile.read()
	themeJsonFile.close()

	# Load just the image resource keys so that we can check just that
	imageResourceKeys = []
	themeJsonData = json.loads(themeJsonContents)
	themeJsonDataFlat = flatten_json(themeJsonData)
	for key in themeJsonDataFlat:
		if (key.endswith(RESOURCE_KEY_SUFFIX)):
			if themeJsonDataFlat[key] and themeJsonDataFlat[key].endswith(PNG_EXTENSION):
				imageResourceKeys.append(RESOURCE_KEY_ART_PATH_DELTA + themeJsonDataFlat[key])
	return imageResourceKeys

# Mutates a UHD based string into HD and SD equivalents
def get_hd_sd_from_uhd(uhdString):
	hdString = uhdString.replace(UHD_KEYWORD,HD_KEYWORD)
	sdString = uhdString.replace(UHD_KEYWORD,SD_KEYWORD)
	return hdString,sdString

# Removes .meta from the end of a string
def strip_meta_extension(fileName):
	return fileName[:-5]

# Returns the .png path for UHD, HD, and SD, given the meta files. Assumes that the 
# metas belongs to pngs
def get_png_path_from_meta_path(uhdMetaPath, hdMetaPath, sdMetaPath):
	uhdPngPath = strip_meta_extension(uhdMetaPath)
	hdPngPath = strip_meta_extension(hdMetaPath)
	sdPngPath = strip_meta_extension(sdMetaPath)
	return uhdPngPath,hdPngPath,sdPngPath

# Opens and then grabs the guid from the given meta file
def get_guid(metafilepath):
	metafile = open(metafilepath, 'r')
	# read twice to get the guid line
	metaline = metafile.readline()
	metaline = metafile.readline()
	# grab the guid
	guid = metaline[6:-1]
	return guid

# Returns the guids for the UHD, HD, and SD assets, given the meta file paths
def get_guid_from_meta_path(uhdMetaPath, hdMetaPath, sdMetaPath):
	uhdGuid = get_guid(uhdMetaPath)
	hdGuid = get_guid(hdMetaPath)
	sdGuid = get_guid(sdMetaPath)
	return uhdGuid,hdGuid,sdGuid

# Returns a list of asset names that use the given png, assuming that the meta files
# refers to a png. Does not return full paths to assets.
def get_assets_using_png(uhdMetaPath, hdMetaPath, sdMetaPath):
	# Get GUIDs for UHD, HD, and SD because the project may not always point to UHD assets
	uhdGuid,hdGuid,sdGuid = get_guid_from_meta_path(uhdMetaPath, hdMetaPath, sdMetaPath)

	# Go through all assets looking for references to the GUIDS
	assetUsesPngList = []
	for assetRoot, assetDirs, assetFilenames in os.walk(ASSET_DIRECTORY_PATH):
		# Skip Font and Art folders
		if (FONT_DIRECTORY_NAME not in assetRoot) and (ART_DIRECTORY_NAME not in assetRoot):
			for assetFilename in assetFilenames:
				if META_EXTENSION not in assetFilename and PNG_EXTENSION not in assetFilename:
					assetFile = open(os.path.join(assetRoot, assetFilename), 'r')
					assetContents = assetFile.read()
					if (uhdGuid in assetContents):
						assetUsesPngList.append('    [ ] (UHD) ' + assetFilename + '\n')	
						if FAST_REPORT_UNUSED_FILES: return assetUsesPngList
					if (hdGuid in assetContents):
						assetUsesPngList.append('    [ ] (HD)  ' + assetFilename + '\n')	
						if FAST_REPORT_UNUSED_FILES: return assetUsesPngList
					if (sdGuid in assetContents):
						assetUsesPngList.append('    [ ] (SD)  ' + assetFilename + '\n')	
						if FAST_REPORT_UNUSED_FILES: return assetUsesPngList
					assetFile.close()
	return assetUsesPngList

# Returns True if any of the given assets are in the theme system, otherwise False
def is_in_theme_system(uhdPngPath, hdPngPath, sdPngPath):
	inTheme = False
	for imageKey in imageResourceKeys:
		if imageKey in [uhdPngPath, hdPngPath, sdPngPath]:
			inTheme = True
			break
	return inTheme

# Deletes all the files associated with the UHD files in the given report. Assumes that each line
# is a relative path to an UHD png file. Deletes UHD, HD, SD pngs and their metas.
def delete_unused_files(unusedReportPath):
	with open(unusedReportPath, 'r') as unusedPngList:
		for unusedUhdPngPath in unusedPngList:
			unusedUhdPngPath = unusedUhdPngPath.strip()
			unusedHdPngPath,unusedSdPngPath = get_hd_sd_from_uhd(unusedUhdPngPath)
			for pngPath in [unusedUhdPngPath, unusedHdPngPath, unusedSdPngPath]:
				for png in glob.glob(pngPath):
					os.remove(png)
				metaPath = pngPath + META_EXTENSION
				for meta in glob.glob(metaPath):
					os.remove(meta)

# def main()

# Set up report files
reportFile = open(REPORT_PATH, 'w')
unusedReportFile = open(UNUSED_REPORT_PATH, 'w')

# Get image resource keys from the Theme system
imageResourceKeys = get_image_resource_keys()

for artDirectoryPath in ART_DIRECTORY_PATHS:
	for artRoot, artDirs, artFilenames in os.walk(artDirectoryPath):
		# Use UHD folders as the starting point; skip platform specific assets
		if UHD_KEYWORD in artRoot and PLATFORM_SPECIFIC_DIRECTORY_NAME not in artRoot and LANGUAGE_SPECIFIC_DIRECTORY_NAME not in artRoot:
			for artFilename in artFilenames:
				# Grab the meta files for pngs
				if (PNG_EXTENSION + META_EXTENSION) in artFilename:
					print('Checking asset: ' + artFilename)
					uhdMetaPath = os.path.join(artRoot,artFilename)
					hdMetaPath,sdMetaPath = get_hd_sd_from_uhd(uhdMetaPath)

					# Check if the png is used in the theme system
					uhdPngPath,hdPngPath,sdPngPath = get_png_path_from_meta_path(uhdMetaPath, hdMetaPath, sdMetaPath)
					inTheme = is_in_theme_system(uhdPngPath, hdPngPath, sdPngPath)
					print('  In theme: ' + str(inTheme))

					# Go through all assets looking for references to the GUIDS
					assetUsesPngList = []
					if not (FAST_REPORT_UNUSED_FILES and inTheme):
						print('  Getting list...')
						assetUsesPngList = get_assets_using_png(uhdMetaPath, hdMetaPath, sdMetaPath)
						print('  List length: ' + str(len(assetUsesPngList)))

					# Add to unused report if not seen in list and not in theme
					if (len(assetUsesPngList) is 0) and (inTheme is False):
						unusedReportFile.write(uhdPngPath + '\n')
						print('  WRITING TO UNUSED')

					# Otherwise add to main report
					elif not FAST_REPORT_UNUSED_FILES:
						pngName = strip_meta_extension(artFilename)
						reportFile.write(pngName + '\n')
						reportFile.write('    FULLPATH: ' + uhdPngPath + '\n')
						for assetName in assetUsesPngList:
							reportFile.write(assetName)
						if inTheme:
							reportFile.write('    [ ] (THEME)\n')	
						print('  WRITING TO -USED-')
					else:
					 	print('  SKIP WRITING')

reportFile.close()
unusedReportFile.close()

# Delete all png (UHD,HD,SD) and meta files that are logged in the report
if DELETE_UNUSED_FILES:
	delete_unused_files(UNUSED_REPORT_PATH)

print('Finished.')