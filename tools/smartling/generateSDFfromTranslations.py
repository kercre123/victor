from __future__ import print_function

import sys
import os
import json
import subprocess
import argparse

#anki
sys.path.append(os.path.abspath("tools/build/tools/ankibuild"))
sys.path.append(os.path.abspath("."))
import util
import unity
import configure


def write_translations_to_file(translated_json_dir,output_asset_path):
  json_files = [pos_json for pos_json in os.listdir(translated_json_dir) if pos_json.endswith('.json')]
  
  translation_string = ""
  for js in json_files:
    with open(os.path.join(translated_json_dir, js)) as json_file:
      data = json.load(json_file);
      for key, value in data.items():
        if "translation" in value:
          translation_string = translation_string + value["translation"] + os.linesep
  
  with open(output_asset_path, "w") as text_file:
    text_file.write(translation_string.encode('utf8'))


def get_translated_output_path(project_dir, output_asset_path):
  if output_asset_path is None:
    return os.path.join(project_dir, 'Assets', 'DevTools', 'Editor', 'LanguageHelpers', 'japaneseTranslations.txt')
  return os.path.join(project_dir, output_asset_path)


def get_translated_json_dir(project_dir, json_asset_dir):
  if json_asset_dir is None:
    return os.path.join(project_dir, 'Assets', 'StreamingAssets', 'LocalizedStrings', 'ja-JP')
  return os.path.join(project_dir, json_asset_dir)
  

def get_project_dir(project_dir):
  # Search for unity project - pick the first one.
  if project_dir is None:
    return unity.UnityBuildConfig.find_unity_project(util.Git.repo_root())
  return project_dir


def run(args):
  # Get project directory 
  project_dir = get_project_dir(args.project_dir)
  if project_dir is None:            
    print("Error: Could not find a Unity project")
    return 1
  if not os.path.exists(project_dir):          
    print("Error: Project directory '" + project_dir + "'does not exist")
    return 1

  # Gather all translated strings into one text file
  translated_json_dir = get_translated_json_dir(project_dir, args.translated_json_asset_dir)
  if not os.path.exists(translated_json_dir):          
    print("Error: Translated JSON directory '" + translated_json_dir + "'does not exist")
    return 1

  output_asset_path = get_translated_output_path(project_dir, args.txt_output_asset_path)
  if not os.path.exists(os.path.dirname(output_asset_path)):          
    print("Error: Parent directory of output target '" + output_asset_path + "'does not exist")
    return 1

  write_translations_to_file(translated_json_dir, output_asset_path)
  
  # Set up required config to execute Unity
  unity_exe = os.path.join(configure.find_unity_app_path(), 'Contents', 'MacOS', 'Unity')
  log_file = os.path.join('build','Unity','UnityBuild-JP-font-generation.log')

  # Execute SDF .asset generation code 
  procArgs = [unity_exe, "-batchmode", "-quit", "-nographics"]
  procArgs.extend(["-projectPath", project_dir])
  procArgs.extend(["-executeMethod", 'CreateNeededSDFFont.CreateSDFFontsFromScript'])
  procArgs.extend(["-logFile", log_file])

  if args.verbose:
    print("Running: %s" % ' '.join(procArgs))
  p = subprocess.Popen(procArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  (stdout, stderr) = p.communicate()
  result = p.poll()

  if args.verbose:
    print(stdout)
    fh = open(log_file, 'r')
    print(fh.read())
    fh.close()

  if result == 0:
    print("Unity SDF Generation completed successfully.")
  else:
    print(stderr, file=sys.stderr)
    print("UNITY SDF GENERATION ERROR", file=sys.stderr)

  return result


# Parses arguments for use in generation
def parse_args(argv=[]):

    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s' % message + os.linesep)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(description="Generate SDF .asset files based off of translations")
    parser.add_argument('--project-dir', action='store', default=None,
                        help='Location of project directory')
    parser.add_argument('--translated-json-asset-dir', action='store', default=None,
                        help='Location of translated strings to use as input for font generation. '
                        + 'Should start with Assets.')
    parser.add_argument('--txt-output-asset-path', action='store', default=None,
                        help='Location of file to write to that contains all translated strings, which is then '
                        + 'used to generate SDF. Should start with Assets.')
    parser.add_argument('--verbose', action='store_true', default=False, help='Display Unity log file')
    args = parser.parse_args(argv)
    return args


if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    result = run(args)
    sys.exit(result)


