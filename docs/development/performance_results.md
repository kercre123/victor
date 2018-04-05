# Observations

## 2/21/2018 notes richard@anki.com

### DVT2 - CPU performance

  - Large single function consumers are working with bitmap images
  - Order of expensive functions doesnt change between idle and shaken, some additions
  - vic-anim process, vic-anim thread is consistent between idle and shaken
  - pthreads more prevalent than expected

### DVT1 - CPU performance

  - Large single function consumers are working with bitmap images
  - `_raw_spin_unlock_irq` and thread functions prevalent
  - many memory allocation related functions in sub-1% band
  - Hard to find relationship between DVT1 and Webots performance

### Webots - memory performance

  - finding the jpeg encoder does ~5000 allocations!
  - 1 persistent allocation
  - 300,880 transient allocations in 3.4GB 90% of which are `intel_performance_primitives`
  - `intel_performance_primitives` appears to be running on a different thread, IPC for allocations?

# DVT2 - CPU performance

## PROCESS: vic-engine

### Top 10 functions idle

```
Overhead  Command          Tid    Shared Object             Symbol
19.26%    VisionSystem     12260  libcozmo_engine.so        void Anki::Embedded::ScrollingIntegralImage_u8_s32::FilterRow_innerLoop<unsigned char>(int, int, int, int, int const*, int const*, int const*, int const*, unsigned char*)
11.75%    VisionSystem     12260  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
6.21%     VisionSystem     12261  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
6.11%     VisionSystem     12260  libcozmo_engine.so        Anki::Vision::Image::BoxFilter(Anki::Vision::ImageBase<unsigned char>&, unsigned int) const
6.00%     VisionSystem     12263  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
5.80%     VisionSystem     12262  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
5.05%     VisionSystem     12260  libcozmo_engine.so        Anki::Embedded::ScrollingIntegralImage_u8_s32::ScrollDown(Anki::Embedded::Array<unsigned char> const&, int, Anki::Embedded::MemoryStack)
3.76%     VisionSystem     12260  libcozmo_engine.so        Anki::Embedded::ConnectedComponentsTemplate<unsigned short>::Extract1dComponents(unsigned char const*, short, short, short, Anki::Embedded::FixedLengthList<Anki::Embedded::ConnectedComponentSegment<unsigned short> >&)
3.15%     VisionSystem     12260  libcozmo_engine.so        Anki::Embedded::ecvcs_computeBinaryImage_numFilters3(Anki::Embedded::Array<unsigned char> const&, Anki::Embedded::FixedLengthList<Anki::Embedded::Array<unsigned char> >&, int, int, unsigned char*, bool)
1.71%     VisionSystem     12260  libcozmo_engine.so        Anki::Vision::ImageRGB::FillGray(Anki::Vision::Image&) const
```

### Top 10 functions shaken

```
Overhead  Command          Tid    Shared Object             Symbol
14.68%    VisionSystem     12260  libcozmo_engine.so        void Anki::Embedded::ScrollingIntegralImage_u8_s32::FilterRow_innerLoop<unsigned char>(int, int, int, int, int const*, int const*, int const*, int const*, unsigned char*)
6.17%     VisionSystem     12260  libcozmo_engine.so        OMR_F_DT_0031
5.35%     VisionSystem     12260  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
5.24%     VisionSystem     12262  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
4.46%     VisionSystem     13166  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
4.27%     VisionSystem     12263  libopencv_imgproc.so      (anonymous namespace)::CLAHE_Interpolation_Body<unsigned char, 0>::operator()(cv::Range const&) const
3.98%     VisionSystem     12260  libcozmo_engine.so        Anki::Vision::Image::BoxFilter(Anki::Vision::ImageBase<unsigned char>&, unsigned int) const
3.54%     VisionSystem     12260  libcozmo_engine.so        Anki::Embedded::ScrollingIntegralImage_u8_s32::ScrollDown(Anki::Embedded::Array<unsigned char> const&, int, Anki::Embedded::MemoryStack)
3.01%     CozmoRunner      12205  libcozmo_engine.so        Anki::LineSegment::IntersectsWith(Anki::LineSegment const&) const
2.73%     VisionSystem     12260  libcozmo_engine.so        Anki::Embedded::ConnectedComponentsTemplate<unsigned short>::Extract1dComponents(unsigned char const*, short, short, short, Anki::Embedded::FixedLengthList<Anki::Embedded::ConnectedComponentSegment<unsigned short> >&)
```

### Hierarchical idle

#### THREAD: VisionSystem (64.1%)
```
  69.8% libcozmo_engine
    43.5% Anki::Embedded...FilterRow_innerloop
    13.8% Anki::Vision::Image::BoxFilter
    11.4% Anki::Embedded...ScrollDown
     8.5% ConnectedComponentsTemplate::Extract1Dcomponents
     7.1% ecvcs_computeBinaryImage_numFilters3

  20.5% libopencv_imgproc
    89.9% CLAHE_Interpolation_Body
     8.3% CLAHE_CalcLut_Body

  4.7%  libc
    54.2% memset
    22.4% memcpy
     7.9% pthread_getspecific
```

#### THREAD: CozmoRunner (12.9%)
```
  60.3% libcozmo_engine
    19.8% Anki::LineSegment::IntersectsWith
    10.7% Anki::QuadTreeNode::GetOverlapType
     7.6% Anki::PoseBase::DevAssert_IsValidParentPointer

  24.4% libc
    24.4% pthread_mutex_unlock
    18.4% pthread_mutex_lock
     9.8% pthread_getspecific
     7.4% @plt

   8.6% libc++
    38.2% __release_shared
```

#### THREAD: VisionSystem (7.8%)
```
  ... CLAHE_Interpolation_Body
```

#### THREAD: VisionSystem (7.6%)
```
  ... CLAHE_Interpolation_Body
```

#### THREAD: VisionSystem (7.4%)
```
  ... CLAHE_Interpolation_Body
```

### Hierarchical shaken

#### THREAD: VisionSystem (58.0% < 64.1%)
```
  79.8% libcozmo_engine (> 69.8%)
    32.1% Anki::Embedded...FilterRow_innerloop (< 43.5%)
    13.5% OMR_F_DT_0031 ()
     8.7% Anki::Vision::Image::BoxFilter (< 13.8%)
     7.7% Anki::Embedded...ScrollDown (< 11.4%)
     6.0% ConnectedComponentsTemplate::Extract1Dcomponents (8.5%)
     5.6% OMR_F_DT_0048 ()
     4.8% ecvcs_computeBinaryImage_numFilters3 (7.1%)

  13.8% libopencv_imgproc (< 20.5%)
    67.3% CLAHE_Interpolation_Body (< 89.9%)
     7.1% cv::ColumnSum ()
     5.9% cv::resizeNNInvoker ()
     4.9% CLAHE_CalcLut_Body (<8.3%)

  3.5% libc (<4.7%)
    53.0% memset (<54.2%)
    27.5% memcpy (>22.4%)
     5.1% pthread_getspecific (<7.9%)
```

#### THREAD: CozmoRunner (22.7% > 12.9%)
```
  61.3% libcozmo_engine (~= 60.3%)
    19.8% Anki::LineSegment::IntersectsWith
    10.7% Anki::QuadTreeNode::GetOverlapType
     7.6% Anki::PoseBase::DevAssert_IsValidParentPointer

  21.4% libc (~= 24.4%)
    18.7% pthread_mutex_unlock (< 24.4%)
    13.3% pthread_mutex_lock (< 18.4%)
     8.7% pthread_getspecific (~= 9.8%)
     6.2% @plt (~= 7.4%)

   9.8% libc++ (~= 8.6%)
    43.2% __release_shared (> 38.2%)
```

#### THREAD: VisionSystem (6.5% < 7.8%)
```
  ... CLAHE_Interpolation_Body
```

#### THREAD: VisionSystem (5.6% < 7.6%)
```
  ... CLAHE_Interpolation_Body
```

#### THREAD: VisionSystem (5.3% < 7.4%)
```
  ... CLAHE_Interpolation_Body
```

## Anim process

### Top 10 functions idle

```
Overhead  Command          Tid    Shared Object             Symbol
11.74%    MicDataProc      12190  vic-anim                  .i_loop_no_load
8.74%     MicDataProc      12190  vic-anim                  u139y
6.10%     vic-anim         12183  vic-anim                  Anki::Cozmo::ProceduralFaceDrawer::DrawEye(Anki::Cozmo::ProceduralFace const&, Anki::Cozmo::ProceduralFace::WhichEye, Anki::Util::RandomGenerator const&, Anki::Vision::ImageRGB&, Anki::Rectangle<float>&)
4.87%     MicDataProc      12190  vic-anim                  VGenComplexTone_i16_ansi
4.20%     vic-anim         12183  vic-anim                  Anki::Vision::ImageRGB::ConvertHSV2RGB565(Anki::Vision::ImageRGB565&)
3.55%     vic-anim         12183  libopencv_imgproc.so      void cv::remapNearest<unsigned char>(cv::Mat const&, cv::Mat&, cv::Mat const&, int, cv::Scalar_<double> const&)
3.50%     MicDataProc      12190  libc.so                   memcpy
2.71%     MicDataProc      12190  vic-anim                  ConvertFloatToInt16
2.69%     MicDataProc      12190  vic-anim                  radf4_ps
2.25%     vic-anim         12183  libm.so                   floorf
```

### Top 10 functions shaken

```
Overhead  Command          Tid    Shared Object             Symbol
47.78%    MicDataProc      12190  vic-anim                  u139y
4.85%     MicDataProc      12190  vic-anim                  .i_loop_no_load
3.35%     vic-anim         12183  vic-anim                  Anki::Cozmo::ProceduralFaceDrawer::DrawEye(Anki::Cozmo::ProceduralFace const&, Anki::Cozmo::ProceduralFace::WhichEye, Anki::Util::RandomGenerator const&, Anki::Vision::ImageRGB&, Anki::Rectangle<float>&)
2.46%     MicDataProc      12190  vic-anim                  .loop_8
2.38%     vic-anim         12183  vic-anim                  Anki::Vision::ImageRGB::ConvertHSV2RGB565(Anki::Vision::ImageRGB565&)
2.23%     MicDataProc      12190  vic-anim                  a562j
2.03%     vic-anim         12183  libopencv_imgproc.so      void cv::remapNearest<unsigned char>(cv::Mat const&, cv::Mat&, cv::Mat const&, int, cv::Scalar_<double> const&)
1.99%     MicDataProc      12190  vic-anim                  VGenComplexTone_i16_ansi
1.65%     MicDataProc      12190  vic-anim                  b383i
1.54%     MicDataProc      12190  libc.so                   memcpy
```

### Hierarchical idle

#### THREAD: MicDataProc (64%)
```
90% vic-anim
  20.3% i_loop_no_load
  15.1% u139y
   8.4% VGenComplexTone_i16_ansi
   4.7% ConvertFloatToInt16

7.5% libc
  78.0% memcpy
   3.9% memset
```

#### THREAD: vic-anim (34.7%)
```
38.4% vic-anim
  47.3% DrawEye
  32.6% ConvertHSV2RGB565
   7.9% ApplyScanlines
   4.0% @plt

28.5% libopencv_imgproc
  35.9% cv::remapNearest
  13.6% cv::ColumnSum
  11.4% cv::RowSum
  10.7% cv::LineAA

13.4% libm
  48.0% floorf
  42.1% roundf
   8.0% @plt

12.4% libc
  48.6% memcpy
   7.2% pthread_mutex_lock
   6.8% pthread_mutex_unlock
```

### Hierarchical shaken

#### THREAD: MicDataProc (79.9% > 64%)
```
96.7% vic-anim (> 90%)
  62.2% u139y (> 15.1%)
   6.3% i_loop_no_load (< 20.3%)
   4.4% <other>
   3.2% loop_8
   2.6% VGenComplexTone_i16_ansi (< 8.4%)
   1.4% ConvertFloatToInt16 (4.7%)

2.6% libc (< 7.5%)
  81.9% memcpy (> 78.0%)
   4.1% memset (> 3.9%)
```

#### THREAD: vic-anim (19.9% < 34.7%)
```
38.4% vic-anim (== 38.4%)
  48.3% DrawEye (~= 47.3%)
  34.4% ConvertHSV2RGB565 (~= 32.6%)
   8.3% ApplyScanlines (~= 7.9%)
   3.9% @plt (== 4.0%)

28.4% libopencv_imgproc (== 28.5%)
  38.1% cv::remapNearest (> 35.9%)
  12.4% cv::ColumnSum (> 13.6%)
  10.9% cv::RowSum (< 11.4%)
  11.5% cv::LineAA (< 10.7%)

14.1% libm (~= 13.4%)
  49.6% floorf (~= 48.0%)
  41.4% roundf (~= 42.1%)
   7.7% @plt (~= 8.0%)

11.9% libc (~= 12.4%)
  55.8% memcpy (> 48.6%)
   7.9% pthread_mutex_lock (> 7.2%)
   7.1% pthread_mutex_unlock (> 6.8%)
   7.1% pthread_getspecific ()
```

# DVT1 - CPU performance

## PROCESS: cozmoengined

#### THREAD: VisionSystem
```
  23%    Anki::Embedded::ScrollingIntegralImage_u8_s32::FilterRow_innerLoop
  11%    CLAHE_Interpolation_Body
   7%    Anki::Embedded::ScrollingIntegralImage_u8_s32::ScrollDown
   6%    Anki::Embedded::ExtractLineFitsPeaks
   5%    Anki::Embedded::ConnectedComponentsTemplate
   3%    Anki::Embedded::ecvcs_computeBinaryImage_numFilters3
   2%    Anki::Vision::ImageRGB::FillGray
   1%    CLAHE_CalcLut_Body
   1%    Anki::Embedded::ScrollingIntegralImage_u8_s32::PadImageRow
```

#### THREAD: CozmoRunner
```
   3%    Anki::Cozmo::UiMessageHandler::Update()
   3%    je_arena_malloc_hard
   3%    arena_dalloc_bin_locked_impl
   2%    je_malloc
   2%    arena_run_reg_alloc
   1%    unlock 
   1%    Anki::PoseBase<Anki::Pose3d, Anki::Transform3d>::PoseTreeNode::Dev_AssertIsValidParentPointer
```

#### THREAD: VisionSystem
```
  61%    CLAHE_Interpolation_Body
  20%    _raw_spin_unlock_irq
   7%    CLAHE_CalcLut_Body
   2%    cv::KMeansDistanceComputer::operator()
```

#### THREAD: VisionSystem
```
  62%    CLAHE_Interpolation_Body
  17%    _raw_spin_unlock_irq
   6%    CLAHE_CalcLut_Body
   2%    cv::KMeansDistanceComputer::operator()
```

#### THREAD: VisionSystem
```
  63%    CLAHE_Interpolation_Body
  15%    _raw_spin_unlock_irq
   7%    CLAHE_CalcLut_Body
   2%    cv::KMeansDistanceComputer::operator()
```

#### THREAD: vic-engine
```
  48%    _raw_spin_unlock_irqrestore
   7%    usleep
   2.5%  IsRunning
```

#### THREAD: mm_cam_stream
```
  75%    mm_app_snapshot_notify_cb_raw
  13%    v7_dma_inv_range
```

#### THREAD: FaceRecognizer
```
  40%    _raw_spin_unlock_irqrestore
   4%    FaceRecognizer::Run
   7%    sleep
   2%    thread_mutex_lock
   1%    thread_mutex_unlock
```

#### THREAD: BehaviorServer
```
  36%    _raw_spin_unlock_irqrestore
  11%    mutex
```

#### THREAD: civetweb-master
```
  82%    poll
```

#### THREAD: camera?
```
  51%    v7_dma_flush_range
```


## PROCESS: vic-anim

#### THREAD: MicDataProc
```
  26%    _raw_spin_unlock_irqrestore
   8%    Anki::Cozmo::MicData::MicDataProcessor::ProcessLoop
   4%    sleep
```

#### THREAD: vic-anim
```
  18%    cv::HSV2RGB...
  11%    cv::remapNearest
   9%    Anki::Cozmo::ProceduralFaceDrawer::DrawEye
   6%    cv_vrndq_u32_f32
   6%    memcpy
   3%    cv::LineAA
   3%    cv::WarpAffineInvoker::operator()
   2%    floor
   2%    round
   2%    cv::RGB2RGB5x5::operator()
   1%    cv::fastMalloc
```

#### THREAD: DrawFaceLoop
```
  75%    write
  17%    sleep
   2%    Anki::Cozmo::FaceDisplay::DrawFaceLoop()
```

#### THREAD: civetweb-master
```
  86%    poll
   6%    master_thread
```

#### THREAD: wwise
```
  36%    sleep
```

#### THREAD: wwise
```
  32%    sleep
```

# Webots - CPU performance

## PROCESS: cozmoengined

### THREAD: main thread
```
  99%    CozmoAPI::Update
    87%    Robot::Update
      70%    VisionComponent::Update
        47%    VisionComponent::Update::CaptureImage
        22%    VisionComponent::CompressAndSendImage
    11%    RobotManager::UpdateRobotConnection
```

#### THREAD: VISIONPROCESSORTHREAD
```
  86%    UpdateVisionSystem
  ...
  76%    Anki::Embedded::DetectFiducialMarkers
  54%    Anki::Embedded::ExtraComponentsViaCharacterIsticScale

  ultimately ucvcs_ functions
```

#### THREAD: TASKEXECUTORTHREAD
```
  57%    Anki::Util::RollingFileLogger
    53%    ostream.flush
```

## PROCESS: vic-anim

#### THREAD: MAIN THREAD
```
  51%    AnimationStreamer::Update
    47%    AnimationStreamer::StreamLayers
    45%    AnimationStreamer::BufferFaceToSend
  31%    AnimProcessMessages::Update
    13%    GetNextPacketFromRobot => UDP
    10%    ProcessMessages => UDP
     4%    MicDataProcessor
```

#### THREAD: MIC PROCESSOR
```
  38%    MicDataProcessor::ProcessResampledAudio
  35%    MicDataProcessor::ResampleAudioChunk
  23%    SpeechRecognizerTHF
```

#### THREAD: WWISE

#### THREAD: FACE DISPLAY
```
  65%   sleeping
```

# Webots - memory performance

#### Cozmo process
```
  69%,         422MB, 1910594   Cozmo::Robot::UpdateAllRobots
    68%,       420MB, 1885073     Cozmo::Robot::Update
    48%,       298MB,  160492       Cozmo::VisionComponent::Update
      26%,     163MB,   63840         Cozmo::VisionComponent::CompressAndSendImage
        19.3%, 117MB,   23940           cv::JpegEncoder
         9.0%, 274KB,    4788           cv::findEncoder
    16%,       100MB, 1478710       Cozmo::VisionComponent::UpdateAllResults
     6%,        40MB,  528276         CreateObjectsFromMarkers
     5%,        30MB,  489172         AddAndUpdateObjects
     2.5%,      15MB,  239400         UpdatePoseOfStackedObjcets
  13%,          99MB,  1508447  Cozmo::Robot::UpdateRobotConnection
```

#### Anim process
```
  90%,         3.2GB, 196785    ippMalloc
   4%,         156MB,  61374    Cozmo::AnimEngine::Update
```
