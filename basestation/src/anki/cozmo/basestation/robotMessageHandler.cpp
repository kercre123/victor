/*

// TODO: Remove this once we get rid of MessageVisionMarker fully
// Convert a MessageVisionMarker into a VisionMarker object and hand it off
// to the BlockWorld
Result RobotMessageHandler::ProcessMessage(Robot* robot, const MessageVisionMarker& msg)
{
  Result retVal = RESULT_FAIL;

  PRINT_NAMED_ERROR("RobotMessageHandler.ProcessMessage.MessageVisionMarker",
    "We should not be receiving this message from anywhere.");

  return retVal;
} // ProcessMessage(MessageVisionMarker)



// For processing imu data chunks arriving from robot.
// Writes the entire log of 3-axis accelerometer and 3-axis
// gyro readings to a .m file in kP_IMU_LOGS_DIR so they
// can be read in from Matlab. (See robot/util/imuLogsTool.m)
Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageIMUDataChunk const& msg)
{
  static u8 imuSeqID = 0;
  static u32 dataSize = 0;
  static s8 imuData[6][1024];  // first ax, ay, az, gx, gy, gz

  // If seqID has changed, then start over.
  if (msg.seqId != imuSeqID) {
    imuSeqID = msg.seqId;
    dataSize = 0;
  }

  // Msgs are guaranteed to be received in order so just append data to array
  memcpy(imuData[0] + dataSize, msg.aX.data(), IMU_CHUNK_SIZE);
  memcpy(imuData[1] + dataSize, msg.aY.data(), IMU_CHUNK_SIZE);
  memcpy(imuData[2] + dataSize, msg.aZ.data(), IMU_CHUNK_SIZE);

  memcpy(imuData[3] + dataSize, msg.gX.data(), IMU_CHUNK_SIZE);
  memcpy(imuData[4] + dataSize, msg.gY.data(), IMU_CHUNK_SIZE);
  memcpy(imuData[5] + dataSize, msg.gZ.data(), IMU_CHUNK_SIZE);

  dataSize += IMU_CHUNK_SIZE;

  // When dataSize matches the expected size, print to file
  if (msg.chunkId == msg.totalNumChunks - 1) {

    // Make sure image capture folder exists
    std::string imuLogsDir = _dataPlatform->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.ProcessIMUDataChunk.CreateDirFailed","%s", imuLogsDir.c_str());
    }

    // Create image file
    char logFilename[564];
    snprintf(logFilename, sizeof(logFilename), "%s/robot%d_imu%d.m", imuLogsDir.c_str(), robot->GetID(), imuSeqID);
    PRINT_STREAM_INFO("RobotMessageHandler.ProcessMessage.MessageIMUDataChunk", "Printing imu log to " << logFilename << " (dataSize = " << dataSize << ")");

    std::ofstream oFile(logFilename);
    for (u32 axis = 0; axis < 6; ++axis) {
      oFile << "imuData" << axis << " = [";
      for (u32 i=0; i<dataSize; ++i) {
        oFile << (s32)(imuData[axis][i]) << " ";
      }
      oFile << "];\n\n";
    }
    oFile.close();
  }

  return RESULT_OK;
}

Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageSyncTimeAck const& msg)
{
  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageSyncTimeAck", "SyncTime acknowledged");
  robot->SetSyncTimeAcknowledged(true);

  return RESULT_OK;
}

Result RobotMessageHandler::ProcessMessage(Robot* robot, MessageBlockIDFlashStarted const& msg)
{
  printf("TODO: MessageBlockIDFlashStarted at time %d ms\n", msg.timestamp);
  return RESULT_OK;
}

*/
