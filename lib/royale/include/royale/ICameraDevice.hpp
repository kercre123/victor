/****************************************************************************\
 * Copyright (C) 2015 Infineon Technologies
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

/**
* \addtogroup royale
* @{
*/

#pragma once

#include <memory>

#include <royale/Status.hpp>
#include <royale/IDepthDataListener.hpp>
#include <royale/IDepthImageListener.hpp>
#include <royale/ISparsePointCloudListener.hpp>
#include <royale/IIRImageListener.hpp>
#include <royale/IExtendedDataListener.hpp>
#include <royale/IEventListener.hpp>
#include <royale/LensParameters.hpp>
#include <royale/ProcessingFlag.hpp>
#include <royale/CallbackData.hpp>
#include <royale/IRecordStopListener.hpp>
#include <royale/IExposureListener.hpp>
#include <royale/IExposureListener2.hpp>
#include <royale/ExposureMode.hpp>
#include <royale/FilterLevel.hpp>
#include <royale/Vector.hpp>
#include <royale/String.hpp>
#include <royale/CameraAccessLevel.hpp>
#include <royale/StreamId.hpp>

namespace royale
{
    /**
     * This is the main interface for talking to the time-of-flight camera system. Typically,
     * an instance is created by the `CameraManager` which automatically detects a connected module.
     * The support access levels can be activated by entering the correct code during creation.
     * After creation, the ICameraDevice is in ready state and can be initialized.
     *
     * Please refer to the provided examples (in the samples directory) for an overview on how to
     * use this class.
     *
     * On Windows please ensure that the CameraDevice object is destroyed before the main() function exits, for example by
     * storing the unique_ptr from CameraManager::createCamera in a unique_ptr that will go out of scope at (or before) the
     * end of main(). Not destroying the CameraDevice before the exit can lead to a deadlock
     * (https://stackoverflow.com/questions/10915233/stdthreadjoin-hangs-if-called-after-main-exits-when-using-vs2012-rc).
     */
    class ICameraDevice
    {
    public:
        virtual ~ICameraDevice() = default;

        // -----------------------------------------------------------------------------------------
        // Level 1: Standard users (Laser Class 1 guaranteed)
        // -----------------------------------------------------------------------------------------
        /**
         *  LEVEL 1
         *  Initializes the camera device and sets the first available use case.
         *
         *  All non-SUCCESS return statuses, except for DEVICE_ALREADY_INITIALIZED, indicate a
         *  non-recoverable error condition.
         *
         *  \return SUCCESS if the camera device has been set up correctly and the default use case
         *  has been be activated.
         *
         *  \return DEVICE_ALREADY_INITIALIZED if this is called more than once.
         *
         *  \return FILE_NOT_FOUND may be returned when this ICameraDevice represents a recording.
         *
         *  \return USECASE_NOT_SUPPORTED if the camera device was successfully opened, but the
         *  default use case could not be activated. This is only expected to happen when bringing
         *  up a new module, so it's not expected at Level 1.
         *
         *  \return CALIBRATION_DATA_ERROR if the camera device has no calibration data (or data that
         *  is incompatible with the processing, requiring a more recent version of Royale); this
         *  device can not be used with Level 1. For bringing up a new module, level 2 access can
         *  either access the hardware by closing this instance, creating a new instance, and calling
         *  setCallbackData (CallbackData::Raw) before calling initialize() or by specifying different
         *  calibration data by calling setCalibrationData (const royale::String &filename) before
         *  calling initialize().
         *
         *  \return Other non-SUCCESS values indicate that the device can not be used.
         */
        virtual royale::CameraStatus initialize () = 0;

        /**
         *  LEVEL 1
         *  Get the ID of the camera device
         *
         *  \param id String container in which the unique ID for the camera device will be written.
         *
         *  \return CameraStatus
         */
        virtual royale::CameraStatus getId (royale::String &id) const = 0;
        /*!
         *  LEVEL 1
         *  Returns the associated camera name as a string which is defined in the CoreConfig
         *  of each module.
         */
        virtual royale::CameraStatus getCameraName (royale::String &cameraName) const = 0;

        /*!
         * LEVEL 1
         * Retrieve further information for this specific camera.
         * The return value is a map, where the keys are depending on the used camera
         */
        virtual royale::CameraStatus getCameraInfo (royale::Vector<royale::Pair<royale::String, royale::String>> &camInfo) const = 0;

        /*!
         *  LEVEL 1
         *  Sets the use case for the camera. If the use case is supported by the connected
         *  camera device SUCCESS will be returned. Changing the use case will also change the
         *  processing parameters that are used (e.g. auto exposure)!
         *
         *  NOTICE: This function must not be called in the data callback - the behavior is
         *  undefined.  Call it from a different thread instead.
         *
         * \param name identifies the use case by an case sensitive string
         *
         * \return SUCCESS if use case can be set
         */
        virtual royale::CameraStatus setUseCase (const royale::String &name) = 0;

        /*!
         *  LEVEL 1
         *  Returns all use cases which are supported by the connected module and valid for the
         *  current selected CallbackData information (e.g. Raw, Depth, ...)
         */
        virtual royale::CameraStatus getUseCases (royale::Vector<royale::String> &useCases) const = 0;

        /*!
         *  LEVEL 1
         *  Get the streams associated with the current use case.
         */
        virtual royale::CameraStatus getStreams (royale::Vector<royale::StreamId> &streams) const = 0;

        /*!
        *  LEVEL 1
        *  Retrieves the number of streams for a specified use case.
        *
        *  \param name use case name
        *  \param nrStreams number of streams for the specified use case
        */
        virtual royale::CameraStatus getNumberOfStreams (const royale::String &name, uint32_t &nrStreams) const = 0;

        /*!
         *  LEVEL 1
         *  Gets the current use case as string
         *
         *  \param useCase current use case identified as string
         */
        virtual royale::CameraStatus getCurrentUseCase (royale::String &useCase) const = 0;

        /*!
         *  LEVEL 1
         *  Change the exposure time for the supported operated operation modes.
         *
         *  For mixed-mode use cases a valid streamId must be passed.
         *  For use cases having only one stream the default value of 0 (which is otherwise not a valid
         *  stream id) can be used to refer to that stream. This is for backward compatibility.
         *
         *  If MANUAL exposure mode of operation is chosen, the user is able to determine set
         *  exposure time manually within the boundaries of the exposure limits of the specific
         *  operation mode.
         *
         *  On success the corresponding status message is returned.
         *  In any other mode of operation the method will return EXPOSURE_MODE_INVALID to indicate
         *  non-compliance with the selected exposure mode.
         *  If the camera is used in the playback configuration a LOGIC_ERROR is returned instead.
         *
         *  WARNING : If this function is used on Level 3 it will ignore the limits given by the use case.
         *
         *  \param exposureTime exposure time in microseconds
         *  \param streamId which stream to change exposure for
         */
        virtual royale::CameraStatus setExposureTime (uint32_t exposureTime, royale::StreamId streamId = 0) = 0;

        /*!
         *  LEVEL 1
         *  Change the exposure mode for the supported operated operation modes.
         *
         *  For mixed-mode use cases a valid streamId must be passed.
         *  For use cases having only one stream the default value of 0 (which is otherwise not a valid
         *  stream id) can be used to refer to that stream. This is for backward compatibility.
         *
         *  If MANUAL exposure mode of operation is chosen, the user is able to determine set
         *  exposure time manually within the boundaries of the exposure limits of the specific
         *  operation mode.
         *
         *  In AUTOMATIC mode the optimum exposure settings are determined the system itself.
         *
         *  The default value is MANUAL.
         *
         *  \param exposureMode mode of operation to determine the exposure time
         *  \param streamId which stream to change exposure mode for
         */
        virtual royale::CameraStatus setExposureMode (royale::ExposureMode exposureMode, royale::StreamId streamId = 0) = 0;

        /*!
         *  LEVEL 1
         *  Retrieves the current mode of operation for acquisition of the exposure time.
         *
         *  For mixed-mode usecases a valid streamId must be passed.
         *  For usecases having only one stream the default value of 0 (which is otherwise not a valid
         *  stream id) can be used to refer to that stream. This is for backward compatibility.
         *
         *  \param exposureMode contains current exposure mode on successful return
         *  \param streamId stream for which the exposure mode should be returned
         */
        virtual royale::CameraStatus getExposureMode (royale::ExposureMode &exposureMode, royale::StreamId streamId = 0) const = 0;

        /*!
         *  LEVEL 1
         *  Retrieves the minimum and maximum allowed exposure limits of the specified operation
         *  mode.  Can be used to retrieve the allowed operational range for a manual definition of
         *  the exposure time.
         *
         *  For mixed-mode usecases a valid streamId must be passed.
         *  For usecases having only one stream the default value of 0 (which is otherwise not a valid
         *  stream id) can be used to refer to that stream. This is for backward compatibility.
         *
         *  \param exposureLimits contains the limits on successful return
         *  \param streamId stream for which the exposure limits should be returned
         */
        virtual royale::CameraStatus getExposureLimits (royale::Pair<uint32_t, uint32_t> &exposureLimits, royale::StreamId streamId = 0) const = 0;

        /**
         *  LEVEL 1
         *  Once registering the data listener, 3D point cloud data is sent via the callback
         *  function.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerDataListener (royale::IDepthDataListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters the data depth listener
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterDataListener() = 0;

        /**
         *  LEVEL 1
         *  Once registering the data listener, Android depth image data is sent via the
         *  callback function.
         *
         *  Consider using registerDataListener and an IDepthDataListener instead of this listener.
         *  This callback provides only an array of depth and confidence values.  The mapping of
         *  pixels to the scene is similar to the pixels of a two-dimensional camera, and it is
         *  unlikely to be a rectilinear projection (although this depends on the exact camera).
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerDepthImageListener (royale::IDepthImageListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters the depth image listener
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterDepthImageListener() = 0;

        /**
         *  LEVEL 1
         *  Once registering the data listener, Android point cloud data is sent via the
         *  callback function.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerSparsePointCloudListener (royale::ISparsePointCloudListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters the sparse point cloud listener
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterSparsePointCloudListener() = 0;

        /**
         *  LEVEL 1
         *  Once registering the data listener, IR image data is sent via the callback function.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerIRImageListener (royale::IIRImageListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters the IR image listener
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterIRImageListener() = 0;

        /**
         *  LEVEL 1
         *  Register listener for event notifications.
         *  The callback will be invoked asynchronously.
         *  Events include things like illumination unit overtemperature.
         */
        virtual royale::CameraStatus registerEventListener (royale::IEventListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters listener for event notifications.
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterEventListener() = 0;

        /**
         *  LEVEL 1
         *  Starts the video capture mode (free-running), based on the specified operation mode.
         *  A listener needs to be registered in order to retrieve the data stream. Either raw data
         *  or processed data can be consumed. If no data listener is registered an error will be
         *  returned and capturing is not started.
         */
        virtual royale::CameraStatus startCapture() = 0;

        /**
         *  LEVEL 1
         *  Stops the video capturing mode.
         *  All buffers should be released again by the data listener.
         */
        virtual royale::CameraStatus stopCapture() = 0;

        /**
         *  LEVEL 1
         *  Returns the maximal width supported by the camera device.
         */
        virtual royale::CameraStatus getMaxSensorWidth (uint16_t &maxSensorWidth) const = 0;

        /**
         *  LEVEL 1
         *  Returns the maximal height supported by the camera device.
         */
        virtual royale::CameraStatus getMaxSensorHeight (uint16_t &maxSensorHeight) const = 0;

        /**
         *  LEVEL 1
         *  Gets the intrinsics of the camera module which are stored in the calibration file
         *
         *  \param param LensParameters is storing all the relevant information (c,f,p,k)
         *
         *  \return CameraStatus
         */
        virtual royale::CameraStatus getLensParameters (royale::LensParameters &param) const = 0;

        /**
         *  LEVEL 1
         *  Returns the information if a connection to the camera could be established
         *
         *  \param connected true if properly set up
         */
        virtual royale::CameraStatus isConnected (bool &connected) const = 0;

        /**
         *  LEVEL 1
         *  Returns the information if the camera module is calibrated. Older camera modules
         *  can still be operated with royale, but calibration data may be incomplete.
         *
         *  \param calibrated true if the module contains proper calibration data
         */
        virtual royale::CameraStatus isCalibrated (bool &calibrated) const = 0;

        /**
         *  LEVEL 1
         *  Returns the information if the camera is currently in capture mode
         *
         *  \param capturing true if camera is in capture mode
         */
        virtual royale::CameraStatus isCapturing (bool &capturing) const = 0;

        /*!
         *  LEVEL 1
         *  Returns the current camera device access level
         */
        virtual royale::CameraStatus getAccessLevel (royale::CameraAccessLevel &accessLevel) const = 0;

        /*!
         *  LEVEL 1
         *  Start recording the raw data stream into a file.
         *  The recording will capture the raw data coming from the imager.
         *  If frameSkip and msSkip are both zero every frame will be recorded.
         *  If both are non-zero the behavior is implementation-defined.
         *
         *  \param fileName full path of target filename (proposed suffix is .rrf)
         *  \param numberOfFrames indicate the maximal number of frames which should be captured
         *                        (stop will be called automatically). If zero (default) is set,
         *                        recording will happen till stopRecording is called.
         *  \param frameSkip indicate how many frames should be skipped after every recorded frame.
         *                   If zero (default) is set and msSkip is zero, every frame will be
         *                   recorded.
         *  \param msSkip indicate how many milliseconds should be skipped after every recorded
         *                frame. If zero (default) is set and frameSkip is zero, every frame will
         *                be recorded.
         */
        virtual royale::CameraStatus startRecording (const royale::String &fileName,
                uint32_t numberOfFrames = 0,
                uint32_t frameSkip = 0,
                uint32_t msSkip = 0) = 0;

        /*!
         *  LEVEL 1
         *  Stop recording the raw data stream into a file. After the recording is stopped
         *  the file is available on the file system.
         */
        virtual royale::CameraStatus stopRecording() = 0;

        /**
         *  LEVEL 1
         *  Once registering a record listener, the listener gets notified once recording
         *  has stopped after specified frames.
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerRecordListener (royale::IRecordStopListener *listener) = 0;

        /**
         *  LEVEL 1
         *  Unregisters the record listener.
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterRecordListener() = 0;

        /*!
         *  LEVEL 1
         *  [deprecated]
         *  Once registering the exposure listener, new exposure values calculated by the
         *  processing are sent to the listener. As this listener doesn't support streams,
         *  only updates for the first stream will be sent.
         *
         *  Only one exposure listener is supported at a time, calling this will automatically
         *  unregister any previously registered IExposureListener or IExposureListener2.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerExposureListener (royale::IExposureListener *listener) = 0;

        /*!
         *  LEVEL 1
         *  Once registering the exposure listener, new exposure values calculated by the
         *  processing are sent to the listener.
         *
         *  Only one exposure listener is supported at a time, calling this will automatically
         *  unregister any previously registered IExposureListener or IExposureListener2.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerExposureListener (royale::IExposureListener2 *listener) = 0;

        /*!
         *  LEVEL 1
         *  Unregisters the exposure listener
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterExposureListener() = 0;

        /*!
         *  LEVEL 1
         *  Set the frame rate to a value. Upper bound is given by the use case.
         *  E.g. Use case with 5 FPS, a maximum frame rate of 5 and a minimum of 1 can be set.
         *  Setting a frame rate of 0 is not allowed.
         *
         *  The framerate is specific for the current use case.
         *  This function is not supported for mixed-mode.
         */
        virtual royale::CameraStatus setFrameRate (uint16_t framerate) = 0;

        /*!
         *  LEVEL 1
         *  Get the current frame rate which is set for the current use case.
         *  This function is not supported for mixed-mode.
         */
        virtual royale::CameraStatus getFrameRate (uint16_t &frameRate) const = 0;

        /*!
         *  LEVEL 1
         *  Get the maximal frame rate which can be set for the current use case.
         *  This function is not supported for mixed-mode.
         */
        virtual royale::CameraStatus getMaxFrameRate (uint16_t &maxFrameRate) const = 0;

        /*!
         *  LEVEL 1
         *  Enable or disable the external triggering.
         *  Some camera modules support an external trigger, they can capture images synchronized with another device.
         *  If the hardware you are using supports it, calling setExternalTrigger(true) will make the camera capture images in this way.
         *  The call to setExternalTrigger has to be done before initializing the device.
         *
         *  The external signal must not exceed the maximum FPS of the chosen UseCase, but lower frame rates are supported.
         *  If no external signal is received, the imager will not start delivering images.
         *
         *  For information if your camera module supports external triggering and how to use it please refer to
         *  the Getting Started Guide of your camera. If the module doesn't support triggering calling this function
         *  will return a LOGIC_ERROR.
         *
         *  Royale currently expects a trigger pulse, not a constant trigger signal. Using a constant
         *  trigger signal might lead to a wrong framerate!
         */
        virtual royale::CameraStatus setExternalTrigger (bool useExternalTrigger) = 0;

        /*!
         *  LEVEL 1
         *  Change the level of filtering that is used during the processing.
         *  This will change the setting of multiple internal filters based on some predefined levels.
         *  Please have a look at the royale::FilterLevel enum to see which levels are available.
         *  FilterLevel::Custom is a special setting which can not be set.
         */
        virtual royale::CameraStatus setFilterLevel (const royale::FilterLevel level, royale::StreamId streamId = 0) = 0;

        /*!
         *  LEVEL 1
         *  Retrieve the level of filtering for the given streamId.
         *  Please have a look at the royale::FilterLevel enum to see which levels are available.
         *  If the processing parameters do not match any of the levels this will return FilterLevel::Custom.
         */
        virtual royale::CameraStatus getFilterLevel (royale::FilterLevel &level, royale::StreamId streamId = 0) const = 0;

        // -----------------------------------------------------------------------------------------
        // Level 2: Experienced users (Laser Class 1 guaranteed) - activation key required
        // -----------------------------------------------------------------------------------------

        /*!
         *  LEVEL 2
         *  Get the list of exposure groups supported by the currently set use case.
         */
        virtual royale::CameraStatus getExposureGroups (royale::Vector< royale::String > &exposureGroups) const = 0;

        /*!
         *  LEVEL 2
         *  Change the exposure time for the supported operated operation modes. If MANUAL exposure mode of operation is chosen, the user
         *  is able to determine set exposure time manually within the boundaries of the exposure limits of the specific operation mode.
         *  On success the corresponding status message is returned.
         *  In any other mode of operation the method will return EXPOSURE_MODE_INVALID to indicate incompliance with the
         *  selected exposure mode. If the camera is used in the playback configuration a LOGIC_ERROR is returned instead.
         *
         *  \param exposureGroup exposure group to be updated
         *  \param exposureTime exposure time in microseconds
         */
        virtual royale::CameraStatus setExposureTime (const String &exposureGroup, uint32_t exposureTime) = 0;

        /*!
         *  LEVEL 2
         *  Retrieves the minimum and maximum allowed exposure limits of the specified operation mode.
         *  Limits may vary between exposure groups.
         *  Can be used to retrieve the allowed operational range for a manual definition of the exposure time.
         *
         *  \param exposureGroup exposure group to be queried
         *  \param exposureLimits pair of (minimum, maximum) exposure time in microseconds
         */
        virtual royale::CameraStatus getExposureLimits (const String &exposureGroup, royale::Pair<uint32_t, uint32_t> &exposureLimits) const = 0;

        /*!
         *  LEVEL 2
         *  Change the exposure times for all sequences.
         *  As it is possible to reuse an exposure group for different sequences it can happen
         *  that the exposure group is updated multiple times!
         *  If the vector that is provided is too long the extraneous values will be discard.
         *  If the vector is too short an error will be returned.
         *
         *  WARNING : If this function is used on Level 3 it will ignore the limits given by the use case.
         *
         *  \param exposureTimes vector with exposure times in microseconds
         *  \param streamId which stream to change exposure times for
         */
        virtual royale::CameraStatus setExposureTimes (const royale::Vector<uint32_t> &exposureTimes, royale::StreamId streamId = 0) = 0;

        /*!
        *  LEVEL 2
        *  Change the exposure times for all exposure groups.
        *  The order of the exposure times is aligned with the order of exposure groups received by getExposureGroups.
        *  If the vector that is provided is too long the extraneous values will be discard.
        *  If the vector is too short an error will be returned.
        *
        *  \param exposureTimes vector with exposure times in microseconds
        */
        virtual royale::CameraStatus setExposureForGroups (const royale::Vector<uint32_t> &exposureTimes) = 0;

        /*!
         *  LEVEL 2
         *  Set/alter processing parameters in order to control the data output.
         *  A list of processing flags is available as an enumeration. The `Variant` data type
         *  can take float, int, or bool. Please make sure to set the proper `Variant` type
         *  for the enum.
         */
        virtual royale::CameraStatus setProcessingParameters (const royale::ProcessingParameterVector &parameters, uint16_t streamId = 0) = 0;

        /*!
         *  LEVEL 2
         *  Retrieve the available processing parameters which are used for the calculation.
         *
         *  Some parameters may only be available on some devices (and may depend on both the
         *  processing implementation and the calibration data available from the device), therefore
         *  the length of the vector may be less than ProcessingFlag::NUM_FLAGS.
         */
        virtual royale::CameraStatus getProcessingParameters (royale::ProcessingParameterVector &parameters, uint16_t streamId = 0) = 0;

        /**
         *  LEVEL 2
         *  After registering the extended data listener, extended data is sent via the callback
         *  function.  If depth data only is specified, this listener is not called. For this case,
         *  please use the standard depth data listener.
         *
         *  \param listener interface which needs to implement the callback method
         */
        virtual royale::CameraStatus registerDataListenerExtended (royale::IExtendedDataListener *listener) = 0;

        /**
         *  LEVEL 2
         *  Unregisters the data extended listener.
         *
         *  It's not necessary to unregister this listener (or any other listener) before deleting
         *  the ICameraDevice.
         */
        virtual royale::CameraStatus unregisterDataListenerExtended() = 0;

        /**
         *  LEVEL 2
         *  Set the callback output data type to one type only.
         *
         *  INFO: This method needs to be called before startCapture(). If is is called while
         *  the camera is in capture mode, it will only have effect after the next stop/start
         *  sequence.
         */
        virtual royale::CameraStatus setCallbackData (royale::CallbackData cbData) = 0;

        /**
         *  LEVEL 2
         *  [deprecated]
         *  Set the callback output data type. Setting multiple types currently isn't supported.
         *
         *  INFO: This method needs to be called before startCapture(). If is is called while
         *  the camera is in capture mode, it will only have effect after the next stop/start
         *  sequence.
         */
        virtual royale::CameraStatus setCallbackData (uint16_t cbData) = 0;

        /**
         *  LEVEL 2
         *  Loads a different calibration from a file. This calibration data will also be used
         *  by the processing!
         *
         *  \param filename name of the calibration file which should be loaded
         *
         *  \return CameraStatus
         */
        virtual royale::CameraStatus setCalibrationData (const royale::String &filename) = 0;

        /**
         *  LEVEL 2
         *  Loads a different calibration from a given Vector. This calibration data will also be
         *  used by the processing!
         *
         *  \param data calibration data which should be used
         *
         *  \return CameraStatus
         */
        virtual royale::CameraStatus setCalibrationData (const royale::Vector<uint8_t> &data) = 0;

        /**
         *  LEVEL 2
         *  Retrieves the current calibration data.
         *
         *  \param data Vector which will be filled with the calibration data
         */
        virtual royale::CameraStatus getCalibrationData (royale::Vector<uint8_t> &data) = 0;

        /**
         *  LEVEL 2
         *  Tries to write the current calibration file into the internal flash of the device.
         *  If no flash is found RESOURCE_ERROR is returned. If there are errors during the flash
         *  process it will try to restore the original calibration.
         *
         *  This is not yet implemented for all cameras!
         *
         *  Some devices also store other data in the calibration data area, for example the product
         *  identifier.  This L2 method will only change the calibration data, and will preserve the
         *  other data; if an unsupported combination of existing data and new data is encountered
         *  it will return an error without writing to the storage.  Only the L3 methods can change
         *  or remove the additional data.
         *
         *  \return CameraStatus
         */
        virtual royale::CameraStatus writeCalibrationToFlash() = 0;

        // -----------------------------------------------------------------------------------------
        // Level 3: Advanced users (Laser Class 1 not (!) guaranteed) - activation key required
        // -----------------------------------------------------------------------------------------

        /*!
         *  LEVEL 3
         *  Writes an arbitrary vector of data on to the storage of the device.
         *  If no flash is found RESOURCE_ERROR is returned.
         *
         *  Where the data will be written to is implementation defined. After using this function,
         *  the eye safety of the device is not guaranteed, even after reopening the device with L1
         *  access.  This method may overwrite the product identifier, and potentially even firmware
         *  in the device.
         *
         *  \param data data that should be flashed
         */
        virtual royale::CameraStatus writeDataToFlash (const royale::Vector<uint8_t> &data) = 0;

        /*!
        *  LEVEL 3
        *  Writes an arbitrary file to the storage of the device.
        *  If no flash is found RESOURCE_ERROR is returned.
        *
        *  Where the data will be written to is implementation defined. After using this function,
        *  the eye safety of the device is not guaranteed, even after reopening the device with L1
        *  access.  This method may overwrite the product identifier, and potentially even firmware
        *  in the device.
        *
        *  \param filename name of the file that should be flashed
        */
        virtual royale::CameraStatus writeDataToFlash (const royale::String &filename) = 0;

        /*!
         *  LEVEL 3
         *  Change the dutycycle of a certain sequence. If the dutycycle is not supported,
         *  an error will be returned. The dutycycle can also be altered during capture
         *  mode.
         *
         *  \param dutyCycle dutyCycle in percent (0, 100)
         *  \param index index of the sequence to change
         */
        virtual royale::CameraStatus setDutyCycle (double dutyCycle, uint16_t index) = 0;

        /**
         * LEVEL 3
         * For each element of the vector a single register write is issued for the connected
         * imager.  Please be aware that any writes that will change crucial parts (starting the
         * imager, stopping the imager, changing the ROI, ...) will not be reflected internally by
         * Royale and might crash the program!
         *
         * If this function is used on Level 4 (empty imager), please be aware that Royale will not
         * start/stop the imager!
         *
         * USE AT YOUR OWN RISK!!!
         *
         * \param   registers   Contains elements of possibly not-unique (String, uint64_t) duplets.
         *                      The String component can consist of:
         *                      a) a base-10 decimal number in the range of [0, 65535]
         *                      b) a base-16 hexadecimal number preceded by a "0x" in the
         *                         range of [0, 65535]
         */
        virtual royale::CameraStatus writeRegisters (const royale::Vector<royale::Pair<royale::String, uint64_t>> &registers) = 0;

        /**
         * LEVEL 3
         * For each element of the vector a single register read is issued for the connected imager.
         * The second element of each pair will be overwritten by the value of the register given
         * by the first element of the pair :
         *
         * \code
           Vector<Pair<String, uint64_t>> registers;
           registers.push_back (Pair<String, uint64_t> ("0x0B0AD", 0));
           camera->readRegisters (registers);
           \endcode
         *
         * will read out the register 0x0B0AD and will replace the 0 with the current value of
         * the register.
         *
         *
         * \param   registers   Contains elements of possibly not-unique (String, uint64_t) duplets.
         *                      The String component can consist of:
         *                      a) a base-10 decimal number in the range of [0, 65535]
         *                      b) a base-16 hexadecimal number preceded by a "0x" in the
         *                         range of [0, 65535]
         */
        virtual royale::CameraStatus readRegisters (royale::Vector<royale::Pair<royale::String, uint64_t>> &registers) = 0;

        /**
         * LEVEL 3
         * Shift the current lens center by the given translation. This works cumulatively (calling
         * shiftLensCenter (0, 1) three times in a row has the same effect as calling shiftLensCenter (0, 3)).
         * If the resulting lens center is not valid this function will return an error.
         * This function works only for raw data readout.
         *
         * \param tx translation in x direction
         * \param ty translation in y direction
         */
        virtual royale::CameraStatus shiftLensCenter (int16_t tx, int16_t ty) = 0;

        /**
         * LEVEL 3
         * Retrieves the current lens center.
         *
         * \param x current x center
         * \param y current y center
         */
        virtual royale::CameraStatus getLensCenter (uint16_t &x, uint16_t &y) = 0;

        // -----------------------------------------------------------------------------------------
        // Level 4: Direct imager access (Laser Class 1 not (!) guaranteed) -
        //          activation key required
        // -----------------------------------------------------------------------------------------

        /**
         * LEVEL 4
         * Initialize the camera and configure the system for the specified use case
         *
         * \param initUseCase identifies the use case by an case sensitive string
         */
        virtual royale::CameraStatus initialize (const royale::String &initUseCase) = 0;
    };
}
/** @}*/
