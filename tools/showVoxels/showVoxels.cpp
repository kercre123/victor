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

#include <SDL.h>
#include <SDL_opengl.h>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>

#include <stdio.h>
#include <string>

static SDL_Window* sdlWindow = NULL;
static SDL_GLContext sdlGlContext;

int init(const int windowWidth, const int windowHeight);
int gameMain();
int close();

static int initSDL(const int windowWidth, const int windowHeight);
static int initGL(const int windowWidth, const int windowHeight);
static int reshapeGL(const int windowWidth, const int windowHeight);
static int render();
static int handleKeys( unsigned char key, int x, int y );

int init(const int windowWidth, const int windowHeight)
{
  if(initSDL(windowWidth, windowHeight) < 0)
    return -1;
  
  if(initGL(windowWidth, windowHeight) < 0)
    return -2;
    
  return 0;
} // int init()

static int initSDL(const int windowWidth, const int windowHeight)
{
	if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
    printf("SDL_Init failed. SDL Error: %s\n", SDL_GetError());
    return -1;
  }
  
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  // TODO: check opengl error?
  
	sdlWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
	if(!sdlWindow) {
    printf("SDL_CreateWindow failed. SDL Error: %s\n", SDL_GetError());
    return -2;
  }

	sdlGlContext = SDL_GL_CreateContext(sdlWindow);
  if(!sdlGlContext) {
    printf("SDL_GL_CreateContext failed. SDL Error: %s\n", SDL_GetError());
    return -3;
  }
  
  // vsync on
	if( SDL_GL_SetSwapInterval(1) < 0 ) {
    printf( "SDL_GL_SetSwapInterval failed. SDL Error: %s\n", SDL_GetError() );
    return -4;
  }
  
  SDL_StartTextInput();
  
  return 0;
} // int initSDL()

static int initGL(const int windowWidth, const int windowHeight)
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
  glClearDepth(1.0f);                   // Set background depth to farthest
  glEnable(GL_DEPTH_TEST);   // Enable depth testing for z-culling
  glDepthFunc(GL_LEQUAL);    // Set the type of depth-test
  glShadeModel(GL_SMOOTH);   // Enable smooth shading
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Nice perspective corrections
  
  if(reshapeGL(windowWidth, windowHeight) < 0) {
    return -1;
  }
  
  return 0;
} // static int initGL()

int close()
{
  SDL_StopTextInput();

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
  const GLfloat minDistance = 0.1f;
  const GLfloat maxDistance = 1000.0f;
  
  const GLfloat aspect = (GLfloat)windowWidth / (GLfloat)windowHeight;
 
  glViewport(0, 0, windowWidth, windowHeight);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  gluPerspective(fieldOfView, aspect, minDistance, maxDistance);
  
  const GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		printf( "Error with glGetError: %s\n", gluErrorString(error) );
    return -1;
	}
  
  return 0;
} // static int reshapeGL()

static int handleKeys(const unsigned char key, const int x, const int y)
{
	if(key == 'q')
	{
    return 1;
	}
  
  return 0;
} // static int handleKeys()

static int render()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers
  glMatrixMode(GL_MODELVIEW);     // To operate on model-view matrix

  // Render a color-cube consisting of 6 quads with different colors
  glLoadIdentity();                 // Reset the model-view matrix
  glTranslatef(1.5f, 0.0f, -7.0f);  // Move right and into the screen

  glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
    // Top face (y = 1.0f)
    // Define vertices in counter-clockwise (CCW) order with normal pointing out
    glColor3f(0.0f, 1.0f, 0.0f);     // Green
    glVertex3f( 1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f,  1.0f);
    glVertex3f( 1.0f, 1.0f,  1.0f);

    // Bottom face (y = -1.0f)
    glColor3f(1.0f, 0.5f, 0.0f);     // Orange
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);

    // Front face  (z = 1.0f)
    glColor3f(1.0f, 0.0f, 0.0f);     // Red
    glVertex3f( 1.0f,  1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f( 1.0f, -1.0f, 1.0f);

    // Back face (z = -1.0f)
    glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);

    // Left face (x = -1.0f)
    glColor3f(0.0f, 0.0f, 1.0f);     // Blue
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);

    // Right face (x = 1.0f)
    glColor3f(1.0f, 0.0f, 1.0f);     // Magenta
    glVertex3f(1.0f,  1.0f, -1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
  glEnd();  // End of drawing color-cube

  // Render a pyramid consists of 4 triangles
  glLoadIdentity();                  // Reset the model-view matrix
  glTranslatef(-1.5f, 0.0f, -6.0f);  // Move left and into the screen

  glBegin(GL_TRIANGLES);           // Begin drawing the pyramid with 4 triangles
    // Front
    glColor3f(1.0f, 0.0f, 0.0f);     // Red
    glVertex3f( 0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);     // Green
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);     // Blue
    glVertex3f(1.0f, -1.0f, 1.0f);

    // Right
    glColor3f(1.0f, 0.0f, 0.0f);     // Red
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);     // Blue
    glVertex3f(1.0f, -1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);     // Green
    glVertex3f(1.0f, -1.0f, -1.0f);

    // Back
    glColor3f(1.0f, 0.0f, 0.0f);     // Red
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);     // Green
    glVertex3f(1.0f, -1.0f, -1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);     // Blue
    glVertex3f(-1.0f, -1.0f, -1.0f);

    // Left
    glColor3f(1.0f,0.0f,0.0f);       // Red
    glVertex3f( 0.0f, 1.0f, 0.0f);
    glColor3f(0.0f,0.0f,1.0f);       // Blue
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glColor3f(0.0f,1.0f,0.0f);       // Green
    glVertex3f(-1.0f,-1.0f, 1.0f);
  glEnd();   // Done drawing the pyramid

  SDL_GL_SwapWindow(sdlWindow);
  
  return 0;
} // static int render()

int gameMain()
{
  bool isRunning = true;
  while(isRunning) {
    //Handle events
    SDL_Event e;
    while( SDL_PollEvent( &e ) != 0 ) {
      //User requests quit
      if( e.type == SDL_QUIT ) {
        isRunning = false;
        break;
      } else if( e.type == SDL_TEXTINPUT ) {
        int x = 0;
        int y = 0;
        SDL_GetMouseState( &x, &y );

        // If the event can't be handled, quit
        if(handleKeys( e.text.text[ 0 ], x, y ) != 0) {
          isRunning = false;
        }
      }
    } // while( SDL_PollEvent( &e ) != 0 )

    if(render() < 0)
      isRunning = false;
  } // while(isRunning)

  return 0;
} // int gameMain()

int main(int argc, char* args[])
{
  const int WINDOW_WIDTH = 640;
  const int WINDOW_HEIGHT = 480;

	//Start up SDL and create window
	if(init(WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
    return -1;
	}
  
  gameMain();
  
	return close();
} // int main(int argc, char* args[])



