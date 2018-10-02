**This documentation lists the steps required to add more tests into the Nightly SDK Tests Suite that is run on TeamCity.**

The tests can run any SDK script that can be run on a physical robot. 

The tests open a simulated robot in webots world robot that is specified in `sdkNightlyTests.cfg`

The tests then run the SDK script on this simulated robot and logs the results for that test. 

**The following steps will successfully add a new test into the Test Suite:**

1. Create an SDK python script that runs the required tests on the robot.

2. Save this SDK script in `victor/tools/sdk/vector-sdk/tests/automated/`

3. Once this is saved, open `victor/project/build-scripts/webots/sdkNightlyTests.cfg`

4. Add to the cfg file the name of the test, the sdk world file the tests should run in, 
   and the name of the SDK test script from Step 2 in the following format:

  ```
  [Test_all_messages]
  world_file : ["sdk.wbt"]
  sdk_file : ["test_all_messages.py"]
  ```
   
   Where `Test_all_messages` is replaced with the name of the test,
         `sdk.wbt` is replaced with the world file that needs to be opened,
         and `test_all_messages.py` is replaced with the SDK test script name from Step 2.

   Once the cfg file is edited to add the configurations of the new test, *sdkTest.py* would open the 
   world file specified, and run the test script specified on the simulated robot in that world.

5. Now run *sdkTest.py* locally to test if the automated tests run the new test script that was added by following
   the instructions below.

6. Once it is confirmed that the new tests run successfully, push the changes and create a PR. Teamcity will run the
   tests once the changes are in master.




**The following steps will successfully run sdkTest.py to test if the new test was added successfully:**

1. Change directory back to `victor/`

2. Do a git clean by running `git clean -xffd`

3. Do a debug build for mac by running `./project/victor/build-victor.sh -p mac -f -c Debug`

4. Make GRPC tools by running the following command:
  ```
  ./tools/sdk/scripts/update_proto.sh
  ```

5. Install Vector-SDK by running the following commands:
  ```
  pushd ./tools/sdk/vector-sdk
  python3 -m pip install -e .
  popd
  ```

6. Export the necessary environment varibles for running the SDK with webots by running:
  ```
  export VECTOR_ROOT=$(pwd)
  export VECTOR_ROBOT_NAME_MAC=Vector-Local
  export VECTOR_ROBOT_IP_MAC=localhost
  export VECTOR_ROBOT_CERT_MAC=/tmp/anki/gateway/trust.cert
  ```

7. Run `sdkTest.py` by running `./project/build-scripts/webots/sdkTest.py --password your_password`

   where `your_password` is replaced with the admin password for your computer.

8. Check the output to verify that the new test is being run.

