#!/usr/bin/env python3

COZMO_DIR = "../../../"

Paths = {
    "SCRATCH_DIR": COZMO_DIR + "unity/Cozmo/Assets/StreamingAssets/Scratch/",
    "STRINGS_DIR": COZMO_DIR + "unity/Cozmo/Assets/StreamingAssets/LocalizedStrings/en-US",
    "WHATS_NEW_DIR": COZMO_DIR + "unity/Cozmo/Assets/AssetBundles/Shared/Managers-Bundle/",
    "WHATS_NEW_ART_DIR": COZMO_DIR + "unity/Cozmo/Assets/AssetBundles/NeedsHub/Art/WhatsNewIconSprites/",
}

import os, sys, io
import urllib.parse, json
from collections import OrderedDict
from shutil import copyfile
from datetime import datetime

def isAllowedChar(c):
    return c.isalnum() or c == ' '

def UnderscoreName(name):
    return filter(isAllowedChar, name).lower().replace(" ","_")
    
def CamelCaseName(name, firstLower=False):
    name = filter(isAllowedChar, name).title().replace(" ","")
    if firstLower:
        name = name[0].lower() + name[1:]
    return name

log_text = print

## Shamelessly taken from codelab.py. Might break in the future. Probably not.
CODELAB_FILE_HEADER = "CODELAB:"
def load_codelab_file(file_contents):
    unencoded_contents = urllib.parse.unquote_plus(file_contents)

    if not unencoded_contents.startswith(CODELAB_FILE_HEADER):
        log_text("Error: load_codelab_file - file doesn't start with %s" % CODELAB_FILE_HEADER)
        return None

    unencoded_contents = unencoded_contents[len(CODELAB_FILE_HEADER):]

    try:
        json_contents = json.loads(unencoded_contents)
    except json.decoder.JSONDecodeError as e:
        log_text("load_codelab_file: Decode error: %s" % e)
        log_text("unencoded_contents = [%s]" % unencoded_contents)
        return None

    return json_contents
## End of shame

def readCodelabFile(codelabFilePath):
    with open(codelabFilePath) as f:
        codelabJson = load_codelab_file(f.read())
        uuid = codelabJson["ProjectUUID"]
        projectJson = json.loads(codelabJson["ProjectJSON"], object_pairs_hook=OrderedDict)
        log_text('uuid: {}'.format(uuid))

    return (uuid, projectJson)

def createFeaturedProjectFile(name, uuid, projectJson):
    featuredProjectFilePath = "{SCRATCH_DIR}/featuredProjects/{}_en-us.json".format(UnderscoreName(name), **Paths)
    with open(featuredProjectFilePath, "w", encoding="utf8") as f:
        json.dump(OrderedDict([("ProjectUUID",uuid), ("ProjectJSON",projectJson)]), f, sort_keys=False, indent=4, separators=(',', ': '))
    log_text('Created featured project file: {}'.format(featuredProjectFilePath))

def addFeaturedProject(uuid, name, desc, instructions, bgColor, textColor, startDate):
    featuredProjectsJsonPath = "{SCRATCH_DIR}/featured-projects.json".format(**Paths)
    with open(featuredProjectsJsonPath, encoding="utf8") as f:
        featuredProjects = json.load(f, object_pairs_hook=OrderedDict)

    featuredProjects.append(
        OrderedDict([
            ("ProjectUUID", uuid),
            ("ProjectName", "codeLabFeatured{}.projectName".format(CamelCaseName(name))),
            ("DASProjectName", CamelCaseName(name)),
            ("VersionNum", "3"),
            ("IsVertical", "true"),
            ("DisplayOrder", "11"),
            ("FeaturedProjectDescription", "codeLabFeatured{}.projectDescription".format(CamelCaseName(name))),
            ("FeaturedProjectInstructions", "codeLabFeatured{}.projectInstructions".format(CamelCaseName(name))),
            ("FeaturedProjectImageName", CamelCaseName(name)),
            ("FeaturedProjectBackgroundColor", bgColor),
            ("FeaturedProjectTitleTextColor", textColor),
            ("ProjectJSONFile", UnderscoreName(name)),
            ("StartDate", OrderedDict([
                ("Month", str(startDate.month)),
                ("Day", str(startDate.day)),
                ("Year", str(startDate.year))
            ]))
        ]))

    with open(featuredProjectsJsonPath, "w", encoding="utf8") as f:
        json.dump(featuredProjects, f, sort_keys=False, indent=4, separators=(',', ': '))

def addCodeLabStrings(name, desc, instructions):
    stringsPath = "{STRINGS_DIR}/CodeLabStrings.json".format(**Paths)
    with open(stringsPath, encoding="utf8") as f:
        strings = json.load(f, object_pairs_hook=OrderedDict)

    strings["codeLabFeatured{}.projectName".format(CamelCaseName(name))] = {"translation": name}
    strings["codeLabFeatured{}.projectDescription".format(CamelCaseName(name))] = {"translation": desc}
    strings["codeLabFeatured{}.projectInstructions".format(CamelCaseName(name))] = {"translation": instructions}

    with open(stringsPath, "w", encoding="utf8") as f:
        jsondump = json.dumps(strings, sort_keys=False, indent=2, separators=(',', ': '), ensure_ascii=False)
        f.write(jsondump)

def copyImages(name, framingCardPath, activeIconPath, idleIconPath):
    framingCardDestPath = "{SCRATCH_DIR}/extra/images/featured/framing_card{}_feat.png".format(CamelCaseName(name), **Paths)
    copyfile(framingCardPath, framingCardDestPath)
    
    activeIconDestPath = "{SCRATCH_DIR}/images/ui/play-now-modal/icon_{}_active.jpg".format(CamelCaseName(name).lower(), **Paths)
    copyfile(activeIconPath, activeIconDestPath)
    
    idleIconDestPath = "{SCRATCH_DIR}/images/ui/play-now-modal/icon_{}_idle.jpg".format(CamelCaseName(name).lower(), **Paths)
    copyfile(idleIconPath, idleIconDestPath)

def addWhatsNewModalData(name, uuid, leftTint, rightTint, startDate, endDate):
    whatsNewModelDataJsonPath = "{WHATS_NEW_DIR}/whatsNewModalData.json".format(**Paths)
    with open(whatsNewModelDataJsonPath, encoding="utf8") as f:
        modalData = json.load(f, object_pairs_hook=OrderedDict)

    modalData.append(
        OrderedDict([
            ("FeatureID", "code_lab_{}".format(UnderscoreName(name))),
            ("DASEventFeatureID", "code_lab_{}".format(UnderscoreName(name))),
            ("StartDate", OrderedDict([
                ("Month", str(startDate.month)),
                ("Day", str(startDate.day)),
                ("Year", str(startDate.year))
            ])),
            ("EndDate", OrderedDict([
                ("Month", str(endDate.month)),
                ("Day", str(endDate.day)),
                ("Year", str(endDate.year))
            ])),
            ("TitleKey", "whatsNew.CodeLab_{}.title".format(CamelCaseName(name))),
            ("DescriptionKey", "whatsNew.CodeLab_{}.description".format(CamelCaseName(name))),
            ("IconAssetBundleName", "whats_new_icon_sprites"),
            ("IconAssetName", "icon_{}".format(CamelCaseName(name, firstLower=True))),
            ("LeftTintColor", leftTint),
            ("RightTintColor", rightTint),
            ("CodeLabData", OrderedDict([
                ("ShowButton", True),
                ("ProjectGuid", uuid)
            ]))
        ]))

    with open(whatsNewModelDataJsonPath, "w", encoding="utf8") as f:
        json.dump(modalData, f, sort_keys=False, indent=2, separators=(',', ': '))

def addWhatsNewStrings(name, desc):
    stringsPath = "{STRINGS_DIR}/WhatsNewStrings.json".format(**Paths)
    with open(stringsPath, encoding="utf8") as f:
        strings = json.load(f, object_pairs_hook=OrderedDict)

    strings["whatsNew.CodeLab_{}.title".format(CamelCaseName(name))] = {"translation": name}
    strings["whatsNew.CodeLab_{}.description".format(CamelCaseName(name))] = {"translation": desc}

    with open(stringsPath, "w", encoding="utf8") as f:
        jsondump = json.dumps(strings, sort_keys=False, indent=2, separators=(',', ': '), ensure_ascii=False)
        f.write(jsondump)

def copyWhatsNewImage(name, whatsNewIconPath):
    destPath = "{WHATS_NEW_ART_DIR}/whats_new_icon_sprites-SD/Packed/icon_{}.png".format(CamelCaseName(name, firstLower=True), **Paths)
    copyfile(whatsNewIconPath, destPath)

    destPath = "{WHATS_NEW_ART_DIR}/whats_new_icon_sprites-HD/Packed/icon_{}.png".format(CamelCaseName(name, firstLower=True), **Paths)
    copyfile(whatsNewIconPath, destPath)

    destPath = "{WHATS_NEW_ART_DIR}/whats_new_icon_sprites-UHD/Packed/icon_{}.png".format(CamelCaseName(name, firstLower=True), **Paths)
    copyfile(whatsNewIconPath, destPath)


def importCodelabFile(projectName, codelabFilePath, description, instructions, bgColor, textColor, framingCardPath, activeIconPath, idleIconPath,
                      leftTint, rightTint, whatsNewIconPath, startDate, endDate):

    try:
        log_text('Reading codelab file...')
        (uuid, projectJson) = readCodelabFile(codelabFilePath)
        log_text('Creating featured project file...')
        createFeaturedProjectFile(projectName, uuid, projectJson)
        log_text('Adding featured project to featured-projects.json...')
        addFeaturedProject(uuid, projectName, description, instructions, bgColor, textColor, startDate)
        log_text('Adding CodeLab strings...')
        addCodeLabStrings(projectName, description, instructions)
        log_text('Copying CodeLab images...')
        copyImages(projectName, framingCardPath, activeIconPath, idleIconPath)

        log_text('Adding new modal to whatsNewModalData.json...')
        addWhatsNewModalData(projectName, uuid, leftTint, rightTint, startDate, endDate)
        log_text('Adding WhatsNew strings...')
        addWhatsNewStrings(projectName, description)
        log_text('Copying WhatsNew images...')
        copyWhatsNewImage(projectName, whatsNewIconPath)
        log_text('Success!')
    except Exception as e:
        log_text(str(e))
