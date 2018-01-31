#include <dlfcn.h>
#include <pthread.h>
#include <sys/mman.h>

#include "kernel_includes.h"

#include "mm_qcamera_app.h"
#include "mm_qcamera_dbg.h"

/*************************************/
#include "victor_camera.h"

#define DEFAULT_RAW_RDI_FORMAT        CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR

typedef struct cameraobj_t
{
  mm_camera_lib_handle lib_handle;
  
} CameraObj;

mm_camera_stream_t * mm_app_add_rdi_stream(mm_camera_test_obj_t *test_obj,
                                           mm_camera_channel_t *channel,
                                           mm_camera_buf_notify_t stream_cb,
                                           void *userdata,
                                           uint8_t num_bufs,
                                           uint8_t num_burst)
{
  int rc = MM_CAMERA_OK;
  size_t i;
  mm_camera_stream_t *stream = NULL;
  cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);
  cam_format_t fmt = CAM_FORMAT_MAX;
  cam_stream_buf_plane_info_t *buf_planes;

  stream = mm_app_add_stream(test_obj, channel);
  if (NULL == stream) {
      CDBG_ERROR("%s: add stream failed\n", __func__);
      return NULL;
  }

  // BRC: Supported raw formats based on capabilities
  // CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR
  // CAM_FORMAT_BAYER_IDEAL_RAW_QCOM_10BPP_BGGR
  // CAM_FORMAT_BAYER_QCOM_RAW_10BPP_BGGR
  // CAM_FORMAT_YUV_422_NV16
  // CAM_FORMAT_YUV_422_NV61
  // Only CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR seems to actually bypass ISP

  CDBG("%s: raw_dim w:%d height:%d\n", __func__,
                                      cam_cap->raw_dim.width,
                                      cam_cap->raw_dim.height);
  for (i = 0;i < cam_cap->supported_raw_fmt_cnt;i++) {
    cam_format_t cur_fmt = cam_cap->supported_raw_fmts[i];
    CDBG("%s: supported_raw_fmts[%zu]=%d\n", __func__, i, cur_fmt);
    if (DEFAULT_RAW_RDI_FORMAT == cur_fmt) {
      fmt = cur_fmt;
      break;
    }
  }

  if (CAM_FORMAT_MAX == fmt) {
      CDBG_ERROR("%s: rdi format not supported\n", __func__);
      return NULL;
  }

  // BRC: Leaving this commented out as documentation about what formats
  // actually work as expected with RDI (from mm_qcamera_rdi.c)
  // if (!(CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG <= fmt &&
  //       CAM_FORMAT_BAYER_MIPI_RAW_12BPP_BGGR >= fmt)) {
  //     CDBG_ERROR("%s: rdi does not support DEFAULT_RAW_FORMAT\n", __func__);
  //     return NULL;
  // }

  stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf;
  stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf;
  stream->s_config.mem_vtbl.clean_invalidate_buf = mm_app_stream_clean_invalidate_buf;
  stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf;
  stream->s_config.mem_vtbl.user_data = (void *)stream;
  stream->s_config.stream_cb = stream_cb;
  stream->s_config.userdata = userdata;
  stream->num_of_bufs = num_bufs;

  stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
  memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
  stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_RAW;
  if (num_burst == 0) {
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
  } else {
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_BURST;
    stream->s_config.stream_info->num_of_burst = num_burst;
  }
  stream->s_config.stream_info->fmt = fmt;
  stream->s_config.stream_info->dim.width = cam_cap->raw_dim.width;
  stream->s_config.stream_info->dim.height = cam_cap->raw_dim.height;
  stream->s_config.padding_info = cam_cap->padding_info;

  rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:config rdi stream err=%d\n", __func__, rc);
    return NULL;
  }

  buf_planes = &stream->s_config.stream_info->buf_planes;
  CDBG("%s: plane_info %dx%d len:%d frame_len:%d\n", __func__,
      buf_planes->plane_info.mp[0].stride, buf_planes->plane_info.mp[0].scanline,
      buf_planes->plane_info.mp[0].len, buf_planes->plane_info.frame_len);

  return stream;
}

mm_camera_channel_t * mm_app_add_rdi_channel(mm_camera_test_obj_t *test_obj,
                                             uint8_t num_burst,
                                             mm_camera_buf_notify_t stream_cb)
{
  mm_camera_channel_t *channel = NULL;
  mm_camera_stream_t *stream = NULL;

  channel = mm_app_add_channel(test_obj,
                                MM_CHANNEL_TYPE_RDI,
                                NULL,
                                NULL,
                                NULL);
  if (NULL == channel) {
    CDBG_ERROR("%s: add channel failed", __func__);
    return NULL;
  }

  stream = mm_app_add_rdi_stream(test_obj,
                                  channel,
                                  stream_cb,
                                  (void *)test_obj,
                                  RDI_BUF_NUM,
                                  num_burst);
  if (NULL == stream) {
    CDBG_ERROR("%s: add stream failed\n", __func__);
    mm_app_del_channel(test_obj, channel);
    return NULL;
  }

  CDBG("%s: channel=%d stream=%d\n", __func__, channel->ch_id, stream->s_id);
  return channel;
}

int mm_app_stop_and_del_rdi_channel(mm_camera_test_obj_t *test_obj,
                                    mm_camera_channel_t *channel)
{
  int rc = MM_CAMERA_OK;
  mm_camera_stream_t *stream = NULL;
  uint8_t i;

  rc = mm_app_stop_channel(test_obj, channel);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:Stop RDI failed rc=%d\n", __func__, rc);
  }

  for (i = 0; i < channel->num_streams; i++) {
    stream = &channel->streams[i];
    rc = mm_app_del_stream(test_obj, channel, stream);
    if (MM_CAMERA_OK != rc) {
      CDBG_ERROR("%s:del stream(%d) failed rc=%d\n", __func__, i, rc);
    }
  }

  rc = mm_app_del_channel(test_obj, channel);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:delete channel failed rc=%d\n", __func__, rc);
  }

  return rc;
}

int victor_start_rdi(mm_camera_test_obj_t *test_obj,
                     uint8_t num_burst,
                     mm_camera_buf_notify_t stream_cb)
{
  int rc = MM_CAMERA_OK;
  mm_camera_channel_t *channel = NULL;

  channel = mm_app_add_rdi_channel(test_obj, num_burst, stream_cb);
  if (NULL == channel) {
    CDBG_ERROR("%s: add channel failed", __func__);
    return -MM_CAMERA_E_GENERAL;
  }

  rc = mm_app_start_channel(test_obj, channel);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:start rdi failed rc=%d\n", __func__, rc);
    mm_app_del_channel(test_obj, channel);
    return rc;
  }

  return rc;
}

int victor_stop_rdi(mm_camera_test_obj_t *test_obj)
{
  int rc = MM_CAMERA_OK;

  mm_camera_channel_t *channel =
      &test_obj->channels[MM_CHANNEL_TYPE_RDI];

  rc = mm_app_stop_and_del_rdi_channel(test_obj, channel);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:Stop RDI failed rc=%d\n", __func__, rc);
  }

  return rc;
}

mm_camera_stream_t * mm_app_add_raw_stream(mm_camera_test_obj_t *test_obj,
                                           mm_camera_channel_t *channel,
                                           mm_camera_buf_notify_t stream_cb,
                                           void *userdata,
                                           uint8_t num_bufs,
                                           uint8_t num_burst)
{
  int rc = MM_CAMERA_OK;
  mm_camera_stream_t *stream = NULL;
  cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);
  
  stream = mm_app_add_stream(test_obj, channel);
  if (NULL == stream) {
    CDBG_ERROR("%s: add stream failed\n", __func__);
    return NULL;
  }
  
  stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf;
  stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf;
  stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf;
  stream->s_config.mem_vtbl.clean_invalidate_buf = mm_app_stream_clean_invalidate_buf;
  stream->s_config.mem_vtbl.user_data = (void *)stream;
  stream->s_config.stream_cb = stream_cb;
  stream->s_config.userdata = userdata;
  stream->num_of_bufs = num_bufs;
  
  stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
  memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
  stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_RAW;
  if (num_burst == 0) {
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
  } else {
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_BURST;
    stream->s_config.stream_info->num_of_burst = num_burst;
  }
  stream->s_config.stream_info->fmt = test_obj->buffer_format;
  if ( test_obj->buffer_width == 0 || test_obj->buffer_height == 0 ) {
    stream->s_config.stream_info->dim.width = DEFAULT_SNAPSHOT_WIDTH;
    stream->s_config.stream_info->dim.height = DEFAULT_SNAPSHOT_HEIGHT;
  } else {
    stream->s_config.stream_info->dim.width = (int32_t)test_obj->buffer_width;
    stream->s_config.stream_info->dim.height = (int32_t)test_obj->buffer_height;
  }
  stream->s_config.padding_info = cam_cap->padding_info;
  
  rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:config preview stream err=%d\n", __func__, rc);
    return NULL;
  }
  
  return stream;
}


int mm_app_load_hal(mm_camera_app_t *my_cam_app)
{
  memset(&my_cam_app->hal_lib, 0, sizeof(hal_interface_lib_t));
  my_cam_app->hal_lib.ptr = dlopen("libmmcamera_interface.so", RTLD_NOW);
  my_cam_app->hal_lib.ptr_jpeg = dlopen("libmmjpeg_interface.so", RTLD_NOW);
  if (!my_cam_app->hal_lib.ptr || !my_cam_app->hal_lib.ptr_jpeg) {
    CDBG_ERROR("%s Error opening HAL library %s\n", __func__, dlerror());
    return -MM_CAMERA_E_GENERAL;
  }
  *(void **)&(my_cam_app->hal_lib.get_num_of_cameras) =
  dlsym(my_cam_app->hal_lib.ptr, "get_num_of_cameras");
  *(void **)&(my_cam_app->hal_lib.mm_camera_open) =
  dlsym(my_cam_app->hal_lib.ptr, "camera_open");
  *(void **)&(my_cam_app->hal_lib.jpeg_open) =
  dlsym(my_cam_app->hal_lib.ptr_jpeg, "jpeg_open");
  
  if (my_cam_app->hal_lib.get_num_of_cameras == NULL ||
      my_cam_app->hal_lib.mm_camera_open == NULL ||
      my_cam_app->hal_lib.jpeg_open == NULL) {
    CDBG_ERROR("%s Error loading HAL sym %s\n", __func__, dlerror());
    return -MM_CAMERA_E_GENERAL;
  }
  
  my_cam_app->num_cameras = my_cam_app->hal_lib.get_num_of_cameras();
  CDBG("%s: num_cameras = %d\n", __func__, my_cam_app->num_cameras);
  
  return MM_CAMERA_OK;
}

int mm_app_allocate_ion_memory(mm_camera_app_buf_t *buf, unsigned int ion_type)
{
  int rc = MM_CAMERA_OK;
  struct ion_handle_data handle_data;
  struct ion_allocation_data alloc;
  struct ion_fd_data ion_info_fd;
  int main_ion_fd = 0;
  void *data = NULL;
  
  main_ion_fd = open("/dev/ion", O_RDONLY);
  if (main_ion_fd <= 0) {
    CDBG_ERROR("Ion dev open failed %s\n", strerror(errno));
    goto ION_OPEN_FAILED;
  }
  
  memset(&alloc, 0, sizeof(alloc));
  alloc.len = buf->mem_info.size;
  /* to make it page size aligned */
  alloc.len = (alloc.len + 4095U) & (~4095U);
  alloc.align = 4096;
  alloc.flags = ION_FLAG_CACHED;
  alloc.heap_id_mask = ion_type;
  rc = ioctl(main_ion_fd, ION_IOC_ALLOC, &alloc);
  if (rc < 0) {
    CDBG_ERROR("ION allocation failed\n");
    goto ION_ALLOC_FAILED;
  }
  
  memset(&ion_info_fd, 0, sizeof(ion_info_fd));
  ion_info_fd.handle = alloc.handle;
  rc = ioctl(main_ion_fd, ION_IOC_SHARE, &ion_info_fd);
  if (rc < 0) {
    CDBG_ERROR("ION map failed %s\n", strerror(errno));
    goto ION_MAP_FAILED;
  }
  
  data = mmap(NULL,
              alloc.len,
              PROT_READ  | PROT_WRITE,
              MAP_SHARED,
              ion_info_fd.fd,
              0);
  
  if (data == MAP_FAILED) {
    CDBG_ERROR("ION_MMAP_FAILED: %s (%d)\n", strerror(errno), errno);
    goto ION_MAP_FAILED;
  }
  buf->mem_info.main_ion_fd = main_ion_fd;
  buf->mem_info.fd = ion_info_fd.fd;
  buf->mem_info.handle = ion_info_fd.handle;
  buf->mem_info.size = alloc.len;
  buf->mem_info.data = data;
  return MM_CAMERA_OK;
  
ION_MAP_FAILED:
  memset(&handle_data, 0, sizeof(handle_data));
  handle_data.handle = ion_info_fd.handle;
  ioctl(main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
  close(main_ion_fd);
ION_OPEN_FAILED:
  return -MM_CAMERA_E_GENERAL;
}

int mm_app_deallocate_ion_memory(mm_camera_app_buf_t *buf)
{
  struct ion_handle_data handle_data;
  int rc = 0;
  
  rc = munmap(buf->mem_info.data, buf->mem_info.size);
  
  if (buf->mem_info.fd > 0) {
    close(buf->mem_info.fd);
    buf->mem_info.fd = 0;
  }
  
  if (buf->mem_info.main_ion_fd > 0) {
    memset(&handle_data, 0, sizeof(handle_data));
    handle_data.handle = buf->mem_info.handle;
    ioctl(buf->mem_info.main_ion_fd, ION_IOC_FREE, &handle_data);
    close(buf->mem_info.main_ion_fd);
    buf->mem_info.main_ion_fd = 0;
  }
  return rc;
}

/* cmd = ION_IOC_CLEAN_CACHES, ION_IOC_INV_CACHES, ION_IOC_CLEAN_INV_CACHES */
int mm_app_cache_ops(mm_camera_app_meminfo_t *mem_info,
                     int cmd)
{
  struct ion_flush_data cache_inv_data;
  struct ion_custom_data custom_data;
  int ret = MM_CAMERA_OK;
  
#ifdef USE_ION
  if (NULL == mem_info) {
    CDBG_ERROR("%s: mem_info is NULL, return here", __func__);
    return -MM_CAMERA_E_GENERAL;
  }
  
  memset(&cache_inv_data, 0, sizeof(cache_inv_data));
  memset(&custom_data, 0, sizeof(custom_data));
  cache_inv_data.vaddr = mem_info->data;
  cache_inv_data.fd = mem_info->fd;
  cache_inv_data.handle = mem_info->handle;
  cache_inv_data.length = (unsigned int)mem_info->size;
  custom_data.cmd = (unsigned int)cmd;
  custom_data.arg = (unsigned long)&cache_inv_data;
  
  CDBG("addr = %p, fd = %d, handle = %lx length = %d, ION Fd = %d",
       cache_inv_data.vaddr, cache_inv_data.fd,
       (unsigned long)cache_inv_data.handle, cache_inv_data.length,
       mem_info->main_ion_fd);
  if(mem_info->main_ion_fd > 0) {
    if(ioctl(mem_info->main_ion_fd, ION_IOC_CUSTOM, &custom_data) < 0) {
      ALOGE("%s: Cache Invalidate failed\n", __func__);
      ret = -MM_CAMERA_E_GENERAL;
    }
  }
#endif
  
  return ret;
}



int mm_app_alloc_bufs(mm_camera_app_buf_t* app_bufs,
                      cam_frame_len_offset_t *frame_offset_info,
                      uint8_t num_bufs,
                      uint8_t is_streambuf,
                      size_t multipleOf)
{
  uint32_t i, j;
  unsigned int ion_type = 0x1 << CAMERA_ION_FALLBACK_HEAP_ID;
  
  if (is_streambuf) {
    ion_type |= 0x1 << CAMERA_ION_HEAP_ID;
  }
  
  for (i = 0; i < num_bufs ; i++) {
    if ( 0 < multipleOf ) {
      size_t m = frame_offset_info->frame_len / multipleOf;
      if ( ( frame_offset_info->frame_len % multipleOf ) != 0 ) {
        m++;
      }
      app_bufs[i].mem_info.size = m * multipleOf;
    } else {
      app_bufs[i].mem_info.size = frame_offset_info->frame_len;
    }
    mm_app_allocate_ion_memory(&app_bufs[i], ion_type);
    
    app_bufs[i].buf.buf_idx = i;
    app_bufs[i].buf.num_planes = (int8_t)frame_offset_info->num_planes;
    app_bufs[i].buf.fd = app_bufs[i].mem_info.fd;
    app_bufs[i].buf.frame_len = app_bufs[i].mem_info.size;
    app_bufs[i].buf.buffer = app_bufs[i].mem_info.data;
    app_bufs[i].buf.mem_info = (void *)&app_bufs[i].mem_info;
    
    /* Plane 0 needs to be set seperately. Set other planes
     * in a loop. */
    app_bufs[i].buf.planes[0].length = frame_offset_info->mp[0].len;
    app_bufs[i].buf.planes[0].m.userptr =
    (long unsigned int)app_bufs[i].buf.fd;
    app_bufs[i].buf.planes[0].data_offset = frame_offset_info->mp[0].offset;
    app_bufs[i].buf.planes[0].reserved[0] = 0;
    for (j = 1; j < (uint8_t)frame_offset_info->num_planes; j++) {
      app_bufs[i].buf.planes[j].length = frame_offset_info->mp[j].len;
      app_bufs[i].buf.planes[j].m.userptr =
      (long unsigned int)app_bufs[i].buf.fd;
      app_bufs[i].buf.planes[j].data_offset = frame_offset_info->mp[j].offset;
      app_bufs[i].buf.planes[j].reserved[0] =
      app_bufs[i].buf.planes[j-1].reserved[0] +
      app_bufs[i].buf.planes[j-1].length;
    }
  }
  CDBG("%s: X", __func__);
  return MM_CAMERA_OK;
}

int mm_app_release_bufs(uint8_t num_bufs,
                        mm_camera_app_buf_t* app_bufs)
{
  int i, rc = MM_CAMERA_OK;
  
  CDBG("%s: E", __func__);
  
  for (i = 0; i < num_bufs; i++) {
    rc = mm_app_deallocate_ion_memory(&app_bufs[i]);
  }
  memset(app_bufs, 0, num_bufs * sizeof(mm_camera_app_buf_t));
  CDBG("%s: X", __func__);
  return rc;
}

int mm_app_stream_initbuf(cam_frame_len_offset_t *frame_offset_info,
                          uint8_t *num_bufs,
                          uint8_t **initial_reg_flag,
                          mm_camera_buf_def_t **bufs,
                          mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                          void *user_data)
{
  mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
  mm_camera_buf_def_t *pBufs = NULL;
  uint8_t *reg_flags = NULL;
  int i, rc;
  
  stream->offset = *frame_offset_info;
  
  CDBG("%s: alloc buf for stream_id %d, len=%d, num planes: %d, offset: %d",
       __func__,
       stream->s_id,
       frame_offset_info->frame_len,
       frame_offset_info->num_planes,
       frame_offset_info->mp[1].offset);
  
  pBufs = (mm_camera_buf_def_t *)malloc(sizeof(mm_camera_buf_def_t) * stream->num_of_bufs);
  reg_flags = (uint8_t *)malloc(sizeof(uint8_t) * stream->num_of_bufs);
  if (pBufs == NULL || reg_flags == NULL) {
    CDBG_ERROR("%s: No mem for bufs", __func__);
    if (pBufs != NULL) {
      free(pBufs);
    }
    if (reg_flags != NULL) {
      free(reg_flags);
    }
    return -1;
  }
  if (stream->num_of_bufs > MM_CAMERA_MAX_NUM_FRAMES) {
    CDBG_ERROR("%s: more stream buffers per stream than allowed", __func__);
    return -1;
  }
  
  rc = mm_app_alloc_bufs(&stream->s_bufs[0],
                         frame_offset_info,
                         stream->num_of_bufs,
                         1,
                         stream->multipleOf);
  
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_stream_alloc_bufs err = %d", __func__, rc);
    free(pBufs);
    free(reg_flags);
    return rc;
  }
  
  for (i = 0; i < stream->num_of_bufs; i++) {
    /* mapping stream bufs first */
    pBufs[i] = stream->s_bufs[i].buf;
    reg_flags[i] = 1;
    rc = ops_tbl->map_ops(pBufs[i].buf_idx,
                          -1,
                          pBufs[i].fd,
                          (uint32_t)pBufs[i].frame_len,
                          ops_tbl->userdata);
    if (rc != MM_CAMERA_OK) {
      CDBG_ERROR("%s: mapping buf[%d] err = %d", __func__, i, rc);
      break;
    }
  }
  
  if (rc != MM_CAMERA_OK) {
    int j;
    for (j=0; j>i; j++) {
      ops_tbl->unmap_ops(pBufs[j].buf_idx, -1, ops_tbl->userdata);
    }
    mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);
    free(pBufs);
    free(reg_flags);
    return rc;
  }
  
  *num_bufs = stream->num_of_bufs;
  *bufs = pBufs;
  *initial_reg_flag = reg_flags;
  
  CDBG("%s: X",__func__);
  return rc;
}

int32_t mm_app_stream_deinitbuf(mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                void *user_data)
{
  mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
  int i;
  
  for (i = 0; i < stream->num_of_bufs ; i++) {
    /* mapping stream bufs first */
    ops_tbl->unmap_ops(stream->s_bufs[i].buf.buf_idx, -1, ops_tbl->userdata);
  }
  
  mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);
  
  CDBG("%s: X",__func__);
  return 0;
}

int32_t mm_app_stream_clean_invalidate_buf(uint32_t index, void *user_data)
{
  mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
  return mm_app_cache_ops(&stream->s_bufs[index].mem_info,
                          ION_IOC_CLEAN_INV_CACHES);
}

int32_t mm_app_stream_invalidate_buf(uint32_t index, void *user_data)
{
  mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
  return mm_app_cache_ops(&stream->s_bufs[index].mem_info, ION_IOC_INV_CACHES);
}


int mm_app_open(mm_camera_app_t *cam_app,
                int cam_id,
                mm_camera_test_obj_t *test_obj)
{
  int32_t rc;
  cam_frame_len_offset_t offset_info;
  
  CDBG("%s:BEGIN\n", __func__);
  
  test_obj->cam = cam_app->hal_lib.mm_camera_open((uint8_t)cam_id);
  if(test_obj->cam == NULL) {
    CDBG_ERROR("%s:dev open error\n", __func__);
    return -MM_CAMERA_E_GENERAL;
  }
  
  CDBG("Open Camera id = %d handle = %d", cam_id, test_obj->cam->camera_handle);
  
  /* alloc ion mem for capability buf */
  memset(&offset_info, 0, sizeof(offset_info));
  offset_info.frame_len = sizeof(cam_capability_t);
  
  rc = mm_app_alloc_bufs(&test_obj->cap_buf,
                         &offset_info,
                         1,
                         0,
                         0);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:alloc buf for capability error\n", __func__);
    goto error_after_cam_open;
  }
  
  /* mapping capability buf */
  rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                   CAM_MAPPING_BUF_TYPE_CAPABILITY,
                                   test_obj->cap_buf.mem_info.fd,
                                   test_obj->cap_buf.mem_info.size);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:map for capability error\n", __func__);
    goto error_after_cap_buf_alloc;
  }
  
  /* alloc ion mem for getparm buf */
  memset(&offset_info, 0, sizeof(offset_info));
  offset_info.frame_len = ONE_MB_OF_PARAMS;
  rc = mm_app_alloc_bufs(&test_obj->parm_buf,
                         &offset_info,
                         1,
                         0,
                         0);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:alloc buf for getparm_buf error\n", __func__);
    goto error_after_cap_buf_map;
  }
  
  /* mapping getparm buf */
  rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                   CAM_MAPPING_BUF_TYPE_PARM_BUF,
                                   test_obj->parm_buf.mem_info.fd,
                                   test_obj->parm_buf.mem_info.size);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:map getparm_buf error\n", __func__);
    goto error_after_getparm_buf_alloc;
  }
  test_obj->params_buffer = (parm_buffer_new_t*) test_obj->parm_buf.mem_info.data;
  CDBG_HIGH("\n%s params_buffer=%p\n",__func__,test_obj->params_buffer);
  
  rc = test_obj->cam->ops->query_capability(test_obj->cam->camera_handle);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: failed query_capability", __func__);
    rc = -MM_CAMERA_E_GENERAL;
    goto error_after_getparm_buf_map;
  }
  memset(&test_obj->jpeg_ops, 0, sizeof(mm_jpeg_ops_t));
  test_obj->jpeg_hdl = 0;
  
  return rc;
  
error_after_getparm_buf_map:
  test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                CAM_MAPPING_BUF_TYPE_PARM_BUF);
error_after_getparm_buf_alloc:
  mm_app_release_bufs(1, &test_obj->parm_buf);
error_after_cap_buf_map:
  test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                CAM_MAPPING_BUF_TYPE_CAPABILITY);
error_after_cap_buf_alloc:
  mm_app_release_bufs(1, &test_obj->cap_buf);
error_after_cam_open:
  test_obj->cam->ops->close_camera(test_obj->cam->camera_handle);
  test_obj->cam = NULL;
  return rc;
}

int mm_app_close(mm_camera_test_obj_t *test_obj)
{
  int32_t rc = MM_CAMERA_OK;
  
  if (test_obj == NULL || test_obj->cam ==NULL) {
    CDBG_ERROR("%s: cam not opened", __func__);
    return -MM_CAMERA_E_GENERAL;
  }
  
  /* unmap capability buf */
  rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_CAPABILITY);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: unmap capability buf failed, rc=%d", __func__, rc);
  }
  
  /* unmap parm buf */
  rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_PARM_BUF);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: unmap setparm buf failed, rc=%d", __func__, rc);
  }
  
  rc = test_obj->cam->ops->close_camera(test_obj->cam->camera_handle);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: close camera failed, rc=%d", __func__, rc);
  }
  test_obj->cam = NULL;
  
  
  /* dealloc capability buf */
  rc = mm_app_release_bufs(1, &test_obj->cap_buf);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: release capability buf failed, rc=%d", __func__, rc);
  }
  
  /* dealloc parm buf */
  rc = mm_app_release_bufs(1, &test_obj->parm_buf);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: release setparm buf failed, rc=%d", __func__, rc);
  }
  
  return MM_CAMERA_OK;
}

mm_camera_channel_t * mm_app_add_channel(mm_camera_test_obj_t *test_obj,
                                         mm_camera_channel_type_t ch_type,
                                         mm_camera_channel_attr_t *attr,
                                         mm_camera_buf_notify_t channel_cb,
                                         void *userdata)
{
  uint32_t ch_id = 0;
  mm_camera_channel_t *channel = NULL;
  
  ch_id = test_obj->cam->ops->add_channel(test_obj->cam->camera_handle,
                                          attr,
                                          channel_cb,
                                          userdata);
  if (ch_id == 0) {
    CDBG_ERROR("%s: add channel failed", __func__);
    return NULL;
  }
  channel = &test_obj->channels[ch_type];
  channel->ch_id = ch_id;
  return channel;
}

int mm_app_del_channel(mm_camera_test_obj_t *test_obj,
                       mm_camera_channel_t *channel)
{
  test_obj->cam->ops->delete_channel(test_obj->cam->camera_handle,
                                     channel->ch_id);
  memset(channel, 0, sizeof(mm_camera_channel_t));
  return MM_CAMERA_OK;
}

mm_camera_stream_t * mm_app_add_stream(mm_camera_test_obj_t *test_obj,
                                       mm_camera_channel_t *channel)
{
  mm_camera_stream_t *stream = NULL;
  int rc = MM_CAMERA_OK;
  cam_frame_len_offset_t offset_info;
  
  stream = &(channel->streams[channel->num_streams++]);
  stream->s_id = test_obj->cam->ops->add_stream(test_obj->cam->camera_handle,
                                                channel->ch_id);
  if (stream->s_id == 0) {
    CDBG_ERROR("%s: add stream failed", __func__);
    return NULL;
  }
  
  stream->multipleOf = test_obj->slice_size;
  
  /* alloc ion mem for stream_info buf */
  memset(&offset_info, 0, sizeof(offset_info));
  offset_info.frame_len = sizeof(cam_stream_info_t);
  
  rc = mm_app_alloc_bufs(&stream->s_info_buf,
                         &offset_info,
                         1,
                         0,
                         0);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:alloc buf for stream_info error\n", __func__);
    test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                      channel->ch_id,
                                      stream->s_id);
    stream->s_id = 0;
    return NULL;
  }
  
  /* mapping streaminfo buf */
  rc = test_obj->cam->ops->map_stream_buf(test_obj->cam->camera_handle,
                                          channel->ch_id,
                                          stream->s_id,
                                          CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                          0,
                                          -1,
                                          stream->s_info_buf.mem_info.fd,
                                          (uint32_t)stream->s_info_buf.mem_info.size);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:map setparm_buf error\n", __func__);
    mm_app_deallocate_ion_memory(&stream->s_info_buf);
    test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                      channel->ch_id,
                                      stream->s_id);
    stream->s_id = 0;
    return NULL;
  }
  
  return stream;
}

int mm_app_del_stream(mm_camera_test_obj_t *test_obj,
                      mm_camera_channel_t *channel,
                      mm_camera_stream_t *stream)
{
  test_obj->cam->ops->unmap_stream_buf(test_obj->cam->camera_handle,
                                       channel->ch_id,
                                       stream->s_id,
                                       CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                       0,
                                       -1);
  mm_app_deallocate_ion_memory(&stream->s_info_buf);
  test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                    channel->ch_id,
                                    stream->s_id);
  memset(stream, 0, sizeof(mm_camera_stream_t));
  return MM_CAMERA_OK;
}


int mm_app_config_stream(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel,
                         mm_camera_stream_t *stream,
                         mm_camera_stream_config_t *config)
{
  return test_obj->cam->ops->config_stream(test_obj->cam->camera_handle,
                                           channel->ch_id,
                                           stream->s_id,
                                           config);
}

int mm_app_start_channel(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel)
{
  return test_obj->cam->ops->start_channel(test_obj->cam->camera_handle,
                                           channel->ch_id);
}

int mm_app_stop_channel(mm_camera_test_obj_t *test_obj,
                        mm_camera_channel_t *channel)
{
  return test_obj->cam->ops->stop_channel(test_obj->cam->camera_handle,
                                          channel->ch_id);
}




camera_cb  user_frame_callback = NULL;
void camera_install_callback(camera_cb cb)
{
  user_frame_callback = cb;
}

#define CLIP(in__, out__) out__ = ( ((in__) < 0) ? 0 : ((in__) > 255 ? 255 : (in__)) )

#define X 640
#define Y 360

//
// raw RDI pixel format is CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR
// This appears to be the same as:
// https://www.linuxtv.org/downloads/v4l-dvb-apis-old/pixfmt-srggb10p.html
//
// 4 pixels stored in 5 bytes. Each of the first 4 bytes contain the 8 high order bits
// of the pixel. The 5th byte contains the two least significant bits of each pixel in the
// same order.
//
#define USE_NEON_DOWNSAMPLE 1
#if USE_NEON_DOWNSAMPLE
static void downsample_frame(uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  // Each iteration of the inline assembly processes 20 bytes at a time
  const uint8_t  kNumBytesProcessedPerLoop = 20;
  assert((bayer_sx % kNumBytesProcessedPerLoop) == 0);
  
  const uint32_t kNumInnerLoops = bayer_sx / kNumBytesProcessedPerLoop;
  const uint32_t kNumOuterLoops = bayer_sy >> 1;

  uint8_t* bayer2 = bayer + bayer_sx;

  uint32_t i, j;
  for(i = 0; i < kNumOuterLoops; ++i)
  {
    for(j = 0; j < kNumInnerLoops; ++j)
    {
      __asm__ volatile
      (
        "VLD1.8 {d0}, [%[ptr]] \n\t"  // Load 8 bytes from raw bayer image
        "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer by 5 (5 byte packing)
        "VLD1.8 {d1}, [%[ptr]] \n\t"  // Load 8 more bytes
        "VSHL.I64 d0, d0, #32 \n\t"   // Shift out the partial packed bytes from d0
        "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer again, after ld and shl so they can be dual issued
        "VEXT.8 d0, d0, d1, #4 \n\t"  // Extract first 4 bytes from d0 and following 4 bytes from d1
        // d0 is now alternating red and green bytes

        // Repeat above steps for next 8 bytes
        "VLD1.8 {d1}, [%[ptr]] \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VLD1.8 {d2}, [%[ptr]] \n\t"
        "VSHL.I64 d1, d1, #32 \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VEXT.8 d1, d1, d2, #4 \n\t" // d1 is alternating red and green

        "VUZP.8 d0, d1 \n\t" // Unzip alternating bytes so d0 is all red bytes and d1 is all green bytes

        // Repeat above for the second row containing green and blue bytes
        "VLD1.8 {d2}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "VSHL.I64 d2, d2, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VEXT.8 d2, d2, d3, #4 \n\t" // d2 is alternating green and blue

        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d4}, [%[ptr2]] \n\t"
        "VSHL.I64 d3, d3, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VEXT.8 d3, d3, d4, #4 \n\t" // d3 is alternating green and blue

        "VUZP.8 d2, d3 \n\t" // d2 is green, d3 is blue
        "VSWP.8 d2, d3 \n\t" // Due to required register stride on saving need to have blue in d2 so swap

        // Average the green data with a vector halving add
        // Could save an instruction by just ignoring the second set of green data (d3)
        "VHADD.U8 d1, d1, d3 \n\t"

        // Perform a saturating left shift on all elements since each color byte is the 8 high bits
        // from the 10 bit data. This is like adding 00 as the two low bits
        "VQSHL.U8 d0, d0, #2 \n\t"
        "VQSHL.U8 d1, d1, #2 \n\t"
        "VQSHL.U8 d2, d2, #2 \n\t"

        // Interleaving store of red, green, and blue bytes into output rgb image
        "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

        : [ptr] "+r" (bayer),  // Output list because we want the output of the pointer adds, + since we are reading
          [out] "+r" (rgb),    //   and writing from these registers
          [ptr2] "+r" (bayer2)
        :
        : "d0","d1","d2","d3","d4","memory" // Clobber list of registers used and memory since it is being written to
      );
    }

    // Skip a row
    bayer += bayer_sx;
    bayer2 += bayer_sx;
  }
}

#else

static void downsample_frame(uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  uint8_t *outR, *outG, *outB;
  register int i, j;
  int tmp;

  outB = &rgb[0];
  outG = &rgb[1];
  outR = &rgb[2];

  // Raw image are reported as 1280x720, 10bpp BGGR MIPI Bayer format
  // Based on frame metadata, the raw image dimensions are actually 1600x720 10bpp pixels.
  // Simple conversion + downsample to RGB yields: 640x360 images
  const int dim = (bayer_sx*bayer_sy);

  // output rows have 8-bit pixels
  const int out_sx = bayer_sx * 8/bpp;

  const int dim_step = (bayer_sx << 1);
  const int out_dim_step = (out_sx << 1);

  int out_i, out_j;
  
  for (i = 0, out_i = 0; i < dim; i += dim_step, out_i += out_dim_step) {
    // process 2 rows at a time
    for (j = 0, out_j = 0; j < bayer_sx; j += 5, out_j += 4) {
      // process 4 col at a time
      // A B A_ B_ -> B G B G
      // C D C_ D_    G R G R

      // Read 4 10-bit px from row0
      const int r0_idx = i + j;
      uint16_t px_A  = (bayer[r0_idx+0] << 2) | ((bayer[r0_idx+4] & 0xc0) >> 6);
      uint16_t px_B  = (bayer[r0_idx+1] << 2) | ((bayer[r0_idx+4] & 0x30) >> 4);
      uint16_t px_A_ = (bayer[r0_idx+2] << 2) | ((bayer[r0_idx+4] & 0x0c) >> 2);
      uint16_t px_B_ = (bayer[r0_idx+3] << 2) |  (bayer[r0_idx+4] & 0x03);

      // Read 4 10-bit px from row1
      const int r1_idx = (i + bayer_sx) + j;
      uint16_t px_C  = (bayer[r1_idx+0] << 2) | ((bayer[r1_idx+4] & 0xc0) >> 6);
      uint16_t px_D  = (bayer[r1_idx+1] << 2) | ((bayer[r1_idx+4] & 0x30) >> 4);
      uint16_t px_C_ = (bayer[r1_idx+2] << 2) | ((bayer[r1_idx+4] & 0x0c) >> 2);
      uint16_t px_D_ = (bayer[r1_idx+3] << 2) |  (bayer[r1_idx+4] & 0x03);

      // output index:
      // out_i represents 2 rows -> divide by 4
      // out_j represents 1 col  -> divide by 2
      const int rgb_0_idx = ((out_i >> 2) + (out_j >> 1)) * 3;
      tmp = (px_C + px_B) >> 1;
      CLIP(tmp, outG[rgb_0_idx]);
      tmp = px_D;
      CLIP(tmp, outR[rgb_0_idx]);
      tmp = px_A;
      CLIP(tmp, outB[rgb_0_idx]);

      const int rgb_1_idx = ((out_i >> 2) + ((out_j+2) >> 1)) * 3;
      tmp = (px_C_ + px_B_) >> 1;
      CLIP(tmp, outG[rgb_1_idx]);
      tmp = px_D_;
      CLIP(tmp, outR[rgb_1_idx]);
      tmp = px_A_;
      CLIP(tmp, outB[rgb_1_idx]);
    }
  }
}

#endif

#define BUFFER_SIZE 3

// Should only need a buffer of three images, one for the image that is currently being
// used by outside sources (engine) and then we alternate writing new images to the other two
// This way there is always a complete image available that can be switched to after the current image
// is done being processed
static uint8_t raw_buffer[BUFFER_SIZE][Y][X][3] __attribute__((aligned(64)));
static pthread_mutex_t lock;

// Index that is currently being processed by the outside and we should not touch
static uint8_t processing_idx = 2;

// The next index to switch to should a new image to process be needed
static uint8_t potential_processing_idx = 1;

// The next index to write new images to
static uint8_t next_idx = 0;

int dump_image_data(uint8_t *img, int width, int height)
{
  char file_name[64];
  static int frame_idx = 0;
  const char* name = "vic_cam";
  const char* ext = "bayer";
  int file_fd;

  snprintf(file_name, sizeof(file_name), "/data/misc/camera/test/%s_%04d.%s", name, frame_idx++, ext);
  file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
  if (file_fd < 0) {
     printf("%s: cannot open file %s \n", __func__, file_name);
  } else {
    write(file_fd,
          img,
          width * height);
  }

  close(file_fd);
  printf("dump %s", file_name);
  return 0;
}

static uint8_t frame_requested_ = FALSE;
static void mm_app_snapshot_notify_cb_raw(mm_camera_super_buf_t *bufs,
                                          void *user_data)
{
  static int frameid = 0;
  int rc;
  uint32_t i = 0;
  mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;
  mm_camera_channel_t *channel = NULL;
  mm_camera_stream_t *m_stream = NULL;
  mm_camera_buf_def_t *m_frame = NULL;
  
  /* find channel */
  for (i = 0; i < MM_CHANNEL_TYPE_MAX; i++) {
    if (pme->channels[i].ch_id == bufs->ch_id) {
      channel = &pme->channels[i];
      break;
    }
  }
  if (NULL == channel) {
    CDBG_ERROR("%s: Wrong channel id (%d)", __func__, bufs->ch_id);
    rc = -1;
    goto EXIT;
  }
  
  /* find snapshot stream */
  for (i = 0; i < channel->num_streams; i++) {
    if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_RAW) {
      m_stream = &channel->streams[i];
      break;
    }
  }
  if (NULL == m_stream) {
    CDBG_ERROR("%s: cannot find snapshot stream", __func__);
    rc = -1;
    goto EXIT;
  }

  // Get raw frame info from stream
  cam_stream_buf_plane_info_t *buf_planes = &m_stream->s_config.stream_info->buf_planes;
  const int raw_frame_width = buf_planes->plane_info.mp[0].stride;
  const int raw_frame_height = buf_planes->plane_info.mp[0].scanline;

  static const uint8_t kProcessEveryNthFrame = 4;
  static uint8_t count = 0;
  if(frame_requested_ && ++count > kProcessEveryNthFrame)
  {
    count = 0;

    // find snapshot frame
    if (user_frame_callback) {
      for (i = 0; i < bufs->num_bufs; i++) {
        if (bufs->bufs[i]->stream_id == m_stream->s_id) {
          
          uint8_t* outbuf = (uint8_t*)&raw_buffer[next_idx];
          m_frame = bufs->bufs[i];
          uint8_t* inbuf = (uint8_t *)m_frame->buffer + m_frame->planes[i].data_offset;

          downsample_frame(inbuf, outbuf, raw_frame_width, raw_frame_height, 10 /* bpp */);
          
          pthread_mutex_lock(&lock);
            
          // image has been taken from the buffer and downsampled, it is now safe for
          // it to be potentially processed by engine
          // any currently partially-locked buffer (not yet locked for processing) is
          // safe to reuse, so we unlock it
            
          uint8_t temp = potential_processing_idx;
          potential_processing_idx = next_idx;
          next_idx = temp;
            
          pthread_mutex_unlock(&lock);
          
          frame_requested_ = FALSE;

          user_frame_callback(outbuf, raw_frame_width, raw_frame_height);
          frameid++;
        }
      }
    }
    if (NULL == m_frame) {
      CDBG_ERROR("%s: main frame is NULL", __func__);
      rc = -1;
      goto EXIT;
    }
  }
  
EXIT:
  for (i=0; i<bufs->num_bufs; i++) {
    if (MM_CAMERA_OK != pme->cam->ops->qbuf(bufs->camera_handle,
                                            bufs->ch_id,
                                            bufs->bufs[i])) {
      CDBG_ERROR("%s: Failed in Qbuf\n", __func__);
    }
  }
}

void camera_swap_locks()
{
  pthread_mutex_lock(&lock);

  // NOTE: we do not modify next index within this mutex!

  // Swap potential and processing 
  uint8_t temp = processing_idx;
  processing_idx = potential_processing_idx;
  potential_processing_idx = temp;

  pthread_mutex_unlock(&lock);
}

int mm_app_start_capture_raw(mm_camera_test_obj_t *test_obj, uint8_t num_snapshots)
{
  int32_t rc = MM_CAMERA_OK;
  mm_camera_channel_t *channel = NULL;
  mm_camera_stream_t *s_main = NULL;
  mm_camera_channel_attr_t attr;
  
  memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
  attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
  //attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS;
  attr.max_unmatched_frames = 1; //I had tried 2
  
  //attr.post_frame_skip = 4; //4x decimation?
  
  channel = mm_app_add_channel(test_obj,
                               MM_CHANNEL_TYPE_CAPTURE,
                               &attr,
                               mm_app_snapshot_notify_cb_raw,
                               test_obj);
  if (NULL == channel) {
    CDBG_ERROR("%s: add channel failed", __func__);
    return -MM_CAMERA_E_GENERAL;
  }
  
  test_obj->buffer_format = DEFAULT_RAW_FORMAT;
  s_main = mm_app_add_raw_stream(test_obj,
                                 channel,
                                 mm_app_snapshot_notify_cb_raw,
                                 test_obj,
                                 num_snapshots,
                                 num_snapshots);
  if (NULL == s_main) {
    CDBG_ERROR("%s: add main snapshot stream failed\n", __func__);
    mm_app_del_channel(test_obj, channel);
    return rc;
  }
  
  rc = mm_app_start_channel(test_obj, channel);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:start zsl failed rc=%d\n", __func__, rc);
    mm_app_del_stream(test_obj, channel, s_main);
    mm_app_del_channel(test_obj, channel);
    return rc;
  }
  
  return rc;
}

int mm_app_stop_capture_raw(mm_camera_test_obj_t *test_obj)
{
  int rc = MM_CAMERA_OK;
  mm_camera_channel_t *ch = NULL;
  int i;
  
  ch = &test_obj->channels[MM_CHANNEL_TYPE_CAPTURE];
  
  
  rc = mm_app_stop_channel(test_obj, ch);
  if (MM_CAMERA_OK != rc) {
    CDBG_ERROR("%s:stop recording failed rc=%d\n", __func__, rc);
  }
  
  for ( i = 0 ; i < ch->num_streams ; i++ ) {
    mm_app_del_stream(test_obj, ch, &ch->streams[i]);
  }
  
  mm_app_del_channel(test_obj, ch);
  
  return rc;
}


int mm_camera_lib_open(mm_camera_lib_handle *handle, int cam_id)
{
  int rc = MM_CAMERA_OK;
  
  if ( NULL == handle ) {
    CDBG_ERROR(" %s : Invalid handle", __func__);
    rc = MM_CAMERA_E_INVALID_INPUT;
    goto EXIT;
  }
  
  memset(handle, 0, sizeof(mm_camera_lib_handle));
  rc = mm_app_load_hal(&handle->app_ctx);
  if( MM_CAMERA_OK != rc ) {
    CDBG_ERROR("%s:mm_app_init err\n", __func__);
    goto EXIT;
  }
  
  handle->test_obj.buffer_width = DEFAULT_PREVIEW_WIDTH;
  handle->test_obj.buffer_height = DEFAULT_PREVIEW_HEIGHT;
  handle->test_obj.buffer_format = DEFAULT_SNAPSHOT_FORMAT;
  handle->current_params.stream_width = DEFAULT_SNAPSHOT_WIDTH;
  handle->current_params.stream_height = DEFAULT_SNAPSHOT_HEIGHT;
  handle->current_params.af_mode = CAM_FOCUS_MODE_AUTO; // Default to auto focus mode
  rc = mm_app_open(&handle->app_ctx, (uint8_t)cam_id, &handle->test_obj);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
               __func__, cam_id, rc);
    goto EXIT;
  }
  
  //rc = mm_app_initialize_fb(&handle->test_obj);
  rc = MM_CAMERA_OK;
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_app_initialize_fb() cam_idx=%d, err=%d\n",
               __func__, cam_id, rc);
    goto EXIT;
  }
  
EXIT:
  
  return rc;
}


int mm_camera_start_rdi_capture(mm_camera_lib_handle *handle) {
  int rc = victor_start_rdi(&handle->test_obj, 0, mm_app_snapshot_notify_cb_raw);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_app_start_rdi() err=%d\n",
               __func__, rc);
    return rc;
  }
  return 0;
}


int mm_camera_start_raw_capture(mm_camera_lib_handle *handle) {
  
  cam_capability_t *camera_cap = (cam_capability_t *) handle->test_obj.cap_buf.mem_info.data;
  
  handle->test_obj.buffer_width =
  (uint32_t)camera_cap->raw_dim.width;
  handle->test_obj.buffer_height =
  (uint32_t)camera_cap->raw_dim.height;
  handle->test_obj.buffer_format = DEFAULT_RAW_FORMAT;
  CDBG_ERROR("%s: MM_CAMERA_LIB_RAW_CAPTURE %dx%d\n",
             __func__,
             camera_cap->raw_dim.width,
             camera_cap->raw_dim.height);
  
  int rc = mm_app_start_capture_raw(&handle->test_obj, 1);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_app_start_capture() err=%d\n",
               __func__, rc);
    return rc;
  }
  return 0;
}


int mm_camera_lib_close(mm_camera_lib_handle *handle)
{
  int rc = MM_CAMERA_OK;
  
  if ( NULL == handle ) {
    CDBG_ERROR(" %s : Invalid handle", __func__);
    rc = MM_CAMERA_E_INVALID_INPUT;
    goto EXIT;
  }
  
  //rc = mm_app_close_fb(&handle->test_obj);
  rc = MM_CAMERA_OK;
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_app_close_fb() err=%d\n",
               __func__, rc);
    goto EXIT;
  }
  
  rc = mm_app_close(&handle->test_obj);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_app_close() err=%d\n",
               __func__, rc);
    goto EXIT;
  }
  
EXIT:
  return rc;
}




/**************************************************************/

static CameraObj gTheCamera;

int camera_init()
{
  int rc = 0;
  /* mm_camera_test_obj_t test_obj; */
  /* memset(&test_obj, 0, sizeof(mm_camera_test_obj_t)); */
  
  //open camera lib
  rc = mm_camera_lib_open(&gTheCamera.lib_handle, 0);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_camera_lib_open() err=%d\n", __func__, rc);
    return -1;
  }
  
  //get number cameras
  int num_cameras =  gTheCamera.lib_handle.app_ctx.num_cameras;
  if ( num_cameras <= 0 ) {
    CDBG_ERROR("%s: No camera sensors reported!", __func__);
    rc = -1;
  } //else we are goint to use the first one.
  
  if (pthread_mutex_init(&lock, NULL) != 0)
  {
    CDBG_ERROR("%s: Mutex init failed", __func__);
    rc = -1;
  }

  return rc;
}

int camera_start(camera_cb cb)
{
  int rc = MM_CAMERA_OK;
  camera_install_callback(cb);
  
  rc = mm_camera_start_rdi_capture(&gTheCamera.lib_handle);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_camera_lib_send_command() err=%d\n", __func__, rc);
    return rc;
  }
  return rc;
}


int camera_stop()
{
  int rc;
  CameraObj* camera = &gTheCamera;
  
  rc = victor_stop_rdi(&(camera->lib_handle.test_obj));
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_app_stop_capture() err=%d\n",
               __func__, rc);
  }
  
  return rc;
}


int camera_cleanup()
{
  mm_camera_lib_close(&gTheCamera.lib_handle);
  return 0;
}

void camera_request_frame()
{
  frame_requested_ = TRUE;
}

