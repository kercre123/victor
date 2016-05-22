/**
File: showVoxels.cpp
Author: Peter Barnum
Created: 2014-12-09

Show a set of voxels

Based on tips at
http://lazyfoo.net/tutorials/SDL/50_SDL_and_opengl_2/index.php
https://www3.ntu.edu.sg/home/ehchua/programming/opengl/CG_Examples.html

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "showVoxels.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>

#include <stdio.h>
#include <string>

#include "anki/common/robot/utilities.h"
#include "anki/common/shared/utilities_shared.h"

namespace Anki
{
  static SDL_Window* sdlWindow = NULL;
  static SDL_GLContext sdlGlContext;

  static GLfloat cameraZ = -7.0f;
  static GLfloat cameraTheta = 0.0f;
  static GLfloat cameraPhi = 0.0f;

  static bool initialized = false;
  static SDL_mutex * voxelMutex = NULL;
  //static std::vector<Voxel> voxels;
  static const VoxelBuffer * restrict voxels;

  static int initSDL(const int windowWidth, const int windowHeight);
  static int initGL(const int windowWidth, const int windowHeight);
  static int reshapeGL(const int windowWidth, const int windowHeight);
  static int render();
  static int handleKeys( unsigned char key, int x, int y );

  int updateVoxels(const VoxelBuffer * const newVoxels)
  {
    if(!initialized)
      return -1;
    
    SDL_LockMutex(voxelMutex);
    
    voxels = newVoxels;
    
    SDL_UnlockMutex(voxelMutex);
    
    return 0;
  }

  int init(const int windowWidth, const int windowHeight)
  {
    if(initialized)
      return 0;
      
    initialized = false;

    if(initSDL(windowWidth, windowHeight) < 0)
      return -1;
    
    if(initGL(windowWidth, windowHeight) < 0)
      return -2;
    
    initialized = true;
      
    return 0;
  } // int init()

  static int initSDL(const int windowWidth, const int windowHeight)
  {
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
      Anki::CoreTechPrint("SDL_Init failed. SDL Error: %s\n", SDL_GetError());
      return -1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // TODO: check opengl error?
    
    sdlWindow = SDL_CreateWindow("Voxels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!sdlWindow) {
      Anki::CoreTechPrint("SDL_CreateWindow failed. SDL Error: %s\n", SDL_GetError());
      return -2;
    }

    sdlGlContext = SDL_GL_CreateContext(sdlWindow);
    if(!sdlGlContext) {
      Anki::CoreTechPrint("SDL_GL_CreateContext failed. SDL Error: %s\n", SDL_GetError());
      return -3;
    }
    
    // vsync on
    if( SDL_GL_SetSwapInterval(1) < 0 ) {
      Anki::CoreTechPrint( "SDL_GL_SetSwapInterval failed. SDL Error: %s\n", SDL_GetError());
      return -4;
    }
    
    voxelMutex = SDL_CreateMutex();
    if (!voxelMutex) {
      Anki::CoreTechPrint( "SDL_CreateMutex failed. SDL Error: %s\n", SDL_GetError());
      return -5;
    }
    
    //SDL_StartTextInput();
    
    return 0;
  } // int initSDL()

  static int initGL(const int windowWidth, const int windowHeight)
  {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
    glClearDepth(1.0f);                   // Set background depth to farthest
    glEnable(GL_DEPTH_TEST);   // Enable depth testing for z-culling
    glDepthFunc(GL_LEQUAL);    // Set the type of depth-test
    glShadeModel(GL_FLAT);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    
    if(reshapeGL(windowWidth, windowHeight) < 0) {
      return -1;
    }
    
    return 0;
  } // static int initGL()

  int close()
  {
    initialized = false;

    //SDL_StopTextInput();
    
    if(voxelMutex)
      SDL_DestroyMutex(voxelMutex);

    //Destroy window	
    SDL_DestroyWindow(sdlWindow);
    sdlWindow = NULL;

    //Quit SDL subsystems
    SDL_Quit();
    
    return 0;
  } // static int close()

  static int reshapeGL(const int windowWidth, const int windowHeight)
  {
    const GLfloat fieldOfView = 45.0f;
    const GLfloat minDistance = 0.01f;
    const GLfloat maxDistance = 100000.0f;
    
    const GLfloat aspect = (GLfloat)windowWidth / (GLfloat)windowHeight;
   
    glViewport(0, 0, windowWidth, windowHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(fieldOfView, aspect, minDistance, maxDistance);
    
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR) {
      Anki::CoreTechPrint( "Error with glGetError: %s\n", gluErrorString(error) );
      return -1;
    }
    
    return 0;
  } // static int reshapeGL()

  static int handleKeys(const unsigned char key, const int x, const int y)
  {
    if(key == 'q') {
      return 1;
    } else if(key == '=') {
      cameraZ += 0.5f;
    } else if(key == '-') {
      cameraZ -= 0.5f;
    } else if(key == 'a') {
      cameraTheta += 5.0f;
    } else if(key == 'd') {
      cameraTheta -= 5.0f;
    } else if(key == 'w') {
      cameraPhi += 5.0f;
    } else if(key == 's') {
      cameraPhi -= 5.0f;
    }
    
  //  cameraZ = MAX(0.0f, MIN(10000.0f, cameraZ));
    
  /*  if(cameraTheta < 0.0f) {
      cameraTheta = cameraTheta + 360.0f;
    } else if(cameraTheta >= 360.0f) {
      cameraTheta = cameraTheta - 360.0f;
    }*/
    
    return 0;
  } // static int handleKeys()

  static void drawVoxel(const Voxel &voxel)
  {
    glTranslatef(voxel.x, voxel.y, voxel.z);

    // Define vertices in counter-clockwise (CCW) order with normal pointing out
    glBegin(GL_QUADS);
      glColor3f(voxel.red, voxel.green, voxel.blue);
    
      // Top face (y = 1.0f)
      glVertex3f( 0.5f, 0.5f, -0.5f);
      glVertex3f(-0.5f, 0.5f, -0.5f);
      glVertex3f(-0.5f, 0.5f,  0.5f);
      glVertex3f( 0.5f, 0.5f,  0.5f);

      // Bottom face (y = -0.5f)
      glVertex3f( 0.5f, -0.5f,  0.5f);
      glVertex3f(-0.5f, -0.5f,  0.5f);
      glVertex3f(-0.5f, -0.5f, -0.5f);
      glVertex3f( 0.5f, -0.5f, -0.5f);

      // Front face  (z = 0.5f)
      glVertex3f( 0.5f,  0.5f, 0.5f);
      glVertex3f(-0.5f,  0.5f, 0.5f);
      glVertex3f(-0.5f, -0.5f, 0.5f);
      glVertex3f( 0.5f, -0.5f, 0.5f);

      // Back face (z = -0.5f)
      glVertex3f( 0.5f, -0.5f, -0.5f);
      glVertex3f(-0.5f, -0.5f, -0.5f);
      glVertex3f(-0.5f,  0.5f, -0.5f);
      glVertex3f( 0.5f,  0.5f, -0.5f);

      // Left face (x = -0.5f)
      glVertex3f(-0.5f,  0.5f,  0.5f);
      glVertex3f(-0.5f,  0.5f, -0.5f);
      glVertex3f(-0.5f, -0.5f, -0.5f);
      glVertex3f(-0.5f, -0.5f,  0.5f);

      // Right face (x = 0.5f)
      glVertex3f(0.5f,  0.5f, -0.5f);
      glVertex3f(0.5f,  0.5f,  0.5f);
      glVertex3f(0.5f, -0.5f,  0.5f);
      glVertex3f(0.5f, -0.5f, -0.5f);
    glEnd();  // End of drawing color-cube
    
    glTranslatef(-voxel.x, -voxel.y, -voxel.z);
  } // void drawVoxel()

  static int render()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers
    glMatrixMode(GL_MODELVIEW);     // To operate on model-view matrix

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, cameraZ);
    
    const float camX = cameraZ * -sinf(cameraTheta*(M_PI/180)) * cosf((cameraPhi)*(M_PI/180));
    const float camY = cameraZ * -sinf((cameraPhi)*(M_PI/180));
    const float camZ = -cameraZ * cosf((cameraTheta)*(M_PI/180)) * cosf((cameraPhi)*(M_PI/180));

    // Set the camera position and lookat point
    gluLookAt(camX,camY,camZ,   // Camera position
              0.0, 0.0, 0.0,    // Look at point
              0.0, 1.0, 0.0);   // Up vector

    SDL_LockMutex(voxelMutex);

    for(s32 i=0; i<voxels->numVoxels; i++) {
        drawVoxel(voxels->voxels[i]);
    }
    
    SDL_UnlockMutex(voxelMutex);

    SDL_GL_SwapWindow(sdlWindow);
    
    return 0;
  } // static int render()

  int gameMain(f64 numSecondsToRun)
  {
    const f64 mouseMoveSpeed = 1.0;
    const f64 mouseScrollSpeed = 1.0;
    
    /*const Uint8 *state =*/ SDL_GetKeyboardState(NULL);
    
    int lastMouseX = -1;
    int lastMouseY = -1;
    
    const f64 startTime = Anki::Embedded::GetTimeF64();
    
    bool isRunning = true;
    while(isRunning) {
      if(numSecondsToRun > 0 && (Anki::Embedded::GetTimeF64() - startTime) > numSecondsToRun) {
        isRunning = false;
        continue;
      }

      //Handle events
      SDL_Event e;
      while( SDL_PollEvent( &e ) != 0 ) {
        //User requests quit
        if( e.type == SDL_QUIT ) {
          //Anki::CoreTechPrint("quit event\n");
          isRunning = false;
          break;
        } else if( e.type == SDL_KEYDOWN) {
          Anki::CoreTechPrint("keydown event\n");
        } else if( e.type == SDL_MOUSEMOTION) {
          //Anki::CoreTechPrint("SDL_MOUSEMOTION event 0x%x (%d,%d)\n", buttonState, x, y);
          
          SDL_PumpEvents();
          int x = 0;
          int y = 0;
          const u32 buttonState = SDL_GetMouseState( &x, &y );
          
          if(lastMouseX < 0 || lastMouseY < 0) {
            lastMouseX = x;
            lastMouseY = y;
            continue;
          }
          
          if(buttonState & 0x1) {
            const int dx = x - lastMouseX;
            const int dy = y - lastMouseY;
            cameraTheta += dx * mouseMoveSpeed;
            cameraPhi += dy * mouseMoveSpeed;
            
            lastMouseX = x;
            lastMouseY = y;
          }
        } // if( e.type == SDL_QUIT ) ... else
        
        else if( e.type == SDL_MOUSEBUTTONDOWN) {
          //Anki::CoreTechPrint("SDL_MOUSEBUTTONDOWN event 0x%x (%d,%d)\n", buttonState, x, y);
          SDL_PumpEvents();
          int x = 0;
          int y = 0;
          SDL_GetMouseState( &x, &y );
          lastMouseX = x;
          lastMouseY = y;
        }
        
        else if( e.type == SDL_MOUSEBUTTONUP) {
          //Anki::CoreTechPrint("SDL_MOUSEBUTTONUP event 0x%x (%d,%d)\n", buttonState, x, y);
          lastMouseX = -1;
          lastMouseY = -1;
        } else if( e.type == SDL_MOUSEWHEEL) {
          //Anki::CoreTechPrint("SDL_MOUSEWHEEL event\n");
          cameraZ += mouseScrollSpeed * e.wheel.y;
          cameraZ = MIN(0, cameraZ);
        }
        else {
          //Anki::CoreTechPrint("Unknown event 0x%x\n", e.type);
        }
      } // while( SDL_PollEvent( &e ) != 0 )

      if(render() < 0)
        isRunning = false;
    } // while(isRunning)

    return 0;
  } // int gameMain()
} //   namespace Anki
