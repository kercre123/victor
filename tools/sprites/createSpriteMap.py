#!/usr/bin/env python3
# NOTE: This script assumes it is being run from the repository root
# When there's more time it should be updated to be location agnostic

import os
import string


#Path Definitions 
PATH_TO_ROOT = ""
SPRITE_SEQUENCE_PATHS = [ PATH_TO_ROOT + "resources/config/devOnlySprites/spriteSequences/",
                         PATH_TO_ROOT + "EXTERNALS/animation-assets/sprites/spriteSequences/"]
SPRITE_PATHS = [PATH_TO_ROOT + "EXTERNALS/animation-assets/sprites/independentSprites/", 
                PATH_TO_ROOT + "resources/config/devOnlySprites/independentSprites/"]

PATH_TO_MAP = PATH_TO_ROOT + "resources/assets/cladToFileMaps/spriteMap.json"

PATH_TO_CLAD_FILE = PATH_TO_ROOT + "coretech/vision/clad_src/clad/types/spriteNames.clad"
PATH_TO_CLAD_TEMPLATE = PATH_TO_ROOT + "tools/sprites/clad_template.txt"

#Strings to bulid map
MAP_ENTRY_TEMPLATE = '''
  {
    "CladEvent" : "${clad_event}",
    "SpriteName" : "${sprite_name}"
  }'''

def sprite_to_clad(name):
  return ("".join(c.upper() if ((i == 0) or (name[i -1] == "_")) else c for i, c in enumerate(name)))

def get_sprite_names(paths, folders = False):
  all_sprite_names = []
  for path in paths:
    for root, dirs, files in os.walk(path):
      if(folders):
        for folder in dirs:
          all_sprite_names.append(folder)
      else:
        for file in files:
          extension = file.rfind(".")
          if(extension != 0):
            all_sprite_names.append(file[:extension])

  all_sprite_names.sort()
  return all_sprite_names

def generate_map(sprite_names, sprite_sequence_names):
  map_str = "["
  t = string.Template(MAP_ENTRY_TEMPLATE)
  values = {}
  for name in sprite_names:
    values["clad_event"] = sprite_to_clad(name)
    values["sprite_name"] = name
    map_str += (t.substitute(values) + ",")
  for name in sprite_sequence_names:
    values["clad_event"] = sprite_to_clad(name)
    values["sprite_name"] = name
    map_str += (t.substitute(values) + ",")

  map_str = map_str[:-1]
  map_str += "\n]"

  os.remove(PATH_TO_MAP)
  with open(PATH_TO_MAP, 'w') as outfile:
    outfile.write(map_str)
    outfile.write("\n")


def generate_clad_file(sprite_names, sprite_sequence_names):
  # Build the list of sprite names for the clad file
  sprite_name_str = ""
  is_sequence_str = ""
  for name in sprite_names:
    sprite_name_str += (sprite_to_clad(name) + ",\n  ")
    is_sequence_str += (sprite_to_clad(name) + " = 0,\n  ")
  for name in sprite_sequence_names:
    sprite_name_str += (sprite_to_clad(name) + ",\n  ")
    is_sequence_str += (sprite_to_clad(name) + " = 1,\n  ")
  sprite_name_str += "Count"
  is_sequence_str += "Count = 0"

  values = {}
  values["sprite_name_str"] = sprite_name_str
  values["is_sequence_str"] = is_sequence_str

  # Substitute the values into the clad file template and write the new file
  os.remove(PATH_TO_CLAD_FILE)
  with open(PATH_TO_CLAD_TEMPLATE, 'r') as infile:
    t = string.Template(infile.read())
    with open(PATH_TO_CLAD_FILE, 'w') as outfile:
      outfile.write(t.substitute(values))
      outfile.write("\n")



def update_sprite_map():
  sprite_names = get_sprite_names(SPRITE_PATHS)
  sprite_sequence_names = get_sprite_names(SPRITE_SEQUENCE_PATHS, True)
  generate_clad_file(sprite_names, sprite_sequence_names)
  generate_map(sprite_names, sprite_sequence_names)

if __name__ == "__main__":
  update_sprite_map()
