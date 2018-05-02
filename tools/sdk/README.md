# Victor Python SDK

The SDK itself is now at https://github.com/anki/victor-python-sdk-private

The `victorclad` directory contains boilerplate for creating a separate Python
package for the CLAD dependency.

# Vector Clad
The `vector-clad` directory contains a script to build and copy up the current
sdk-facing clad messages.  This is a pre-requisite for the vector-sdk folder.

# Vector SDK
The `vector-sdk` directory contains a starting point for a victor development
kit.  Currently it consists of a single file which defines a robot similar to
in the cozmo SDK, and invokes some basic driving/turning through the robot
proxy.