#pragma once

#include "cozmoAnim/doom/d_event.h"

#define printf(...) PRINT_NAMED_WARNING("DOOM", __VA_ARGS__)

extern std::vector<std::string>	wadfiles;

extern bool _gDoomPortRun;
extern bool _gMainLoopStarted;
extern unsigned int _gDoomPortWidth;
extern unsigned int _gDoomPortHeight;
extern std::string _gDoomResourcePath;


#include <memory>
namespace sf {
  class RenderWindow;
}
extern std::unique_ptr<sf::RenderWindow> window;

void D_AddFile (const std::string& file);

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain (void);

//
// BASE LEVEL
//
void D_PageTicker (void);
void D_PageDrawer (void);
void D_AdvanceDemo (void);
void D_StartTitle (void);
