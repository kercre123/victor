*******************************************
*
* How to add a build server simulation test
*
*******************************************

1) Create a .cpp file in this directory with a class that inherits from CozmoSimTestController.
   The class name should start with CST_. This is just convention and not required for functionality.

2) Make sure you register the class using REGISTER_COZMO_SIM_TEST_CLASS().

3) Use CST_ASSERT() and CST_EXPECT() macros to test conditions of interest. 
   Use CST_EXIT() to properly exit your test.

4) Add the name of the test class and world file to project/build-scripts/webots/webotsTests.cfg.
   The world file should be added to simulator/worlds/ and must contain an instance of each of the following.
  
     BlockWorldComms (i.e. Cozmo engine)
     CozmoBot (i.e. Cozmo robot)
     BuildServerTestController (i.e. The test)
  
   and BuildServerTestController should contain the line
   
     controllerArgs "%COZMO_SIM_TEST%"



TIPS:

i) The tests specified in webotsTest.cfg are run by calling project/build-scripts/webots/webotsTest.py
   By default they run in 'minimized' mode which means there is no Webots window that actually appears.
   Run webotsTest.py with --showGraphics to enable the Webots window. 

ii) The easiest way to debug a single test is to modify the world file to point to the name of your test class 
    instead of %COZMO_SIM_TEST%. (Just don't commit it that way!) Then, in cozmoSimTestController.h, set 
 
      #define DO_NOT_QUIT_WEBOTS 1

    In this way, you can run the test repeatedly without Webots quitting on you.
    
iii) There is some time overhead associated with each test (.cpp) file. It's basically restarting
     Webots and going through the whole connection routine. If it makes sense, it can shave off
     some seconds by combining tests to run in a single .cpp file.
