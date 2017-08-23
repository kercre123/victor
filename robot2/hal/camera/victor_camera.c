#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <ctype.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "kernel_includes.h"

#include "mm_qcamera_app.h"
#include "mm_qcamera_dbg.h"
#include "mm_qcamera_socket.h"

/*************************************/
#include "victor_camera.h"

typedef struct cameraobj_t
{
  mm_camera_lib_handle lib_handle;

} CameraObj;


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

static pthread_mutex_t app_mutex;
static int thread_status = 0;
static pthread_cond_t app_cond_v;

#define MM_QCAMERA_APP_NANOSEC_SCALE 1000000000



int mm_camera_app_wait()
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(FALSE == thread_status){
        pthread_cond_wait(&app_cond_v, &app_mutex);
    }
    thread_status = FALSE;
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

void mm_camera_app_done()
{
  pthread_mutex_lock(&app_mutex);
  thread_status = TRUE;
  pthread_cond_signal(&app_cond_v);
  pthread_mutex_unlock(&app_mutex);
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

    /* close jpeg client */
    if (test_obj->jpeg_hdl && test_obj->jpeg_ops.close) {
        rc = test_obj->jpeg_ops.close(test_obj->jpeg_hdl);
        test_obj->jpeg_hdl = 0;
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s: close jpeg failed, rc=%d", __func__, rc);
        }
    }

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

mm_camera_channel_t *mm_app_get_channel_by_type(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_type_t ch_type)
{
    return &test_obj->channels[ch_type];
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

#define DIM 2
#define X 640
#define Y 360
#define X6 214
static void downsample_frame(uint64_t in[Y*2][X6], int len, uint8_t out[Y][X]) {
     int x = 0, y = 0, i = 0;
     for (y = 0; y < Y; y++)
     {
          for (x = 0; x < X6; x++)
          {
               unsigned long long p = in[y*2][x], q = in[y*2+1][x];
               int g;
               g = (((p>>00) & 1023) + ((p>>10) & 1023) + ((q>>00) & 1023) + ((q>>10) & 1023)) >> DIM;
               out[y][x*3+0] = (g > 255) ? 255 : g;
               g = (((p>>20) & 1023) + ((p>>30) & 1023) + ((q>>20) & 1023) + ((q>>30) & 1023)) >> DIM;
               out[y][x*3+1] = (g > 255) ? 255 : g;
               g = (((p>>40) & 1023) + ((p>>50) & 1023) + ((q>>40) & 1023) + ((q>>50) & 1023)) >> DIM;
               out[y][x*3+2] = (g > 255) ? 255 : g;
          }
     }
}


#define IMAGE_RING_SIZE 3 //Need to add more buffers if you change this
static uint8_t raw_buffer[IMAGE_RING_SIZE][Y+1][X];
static uint8_t raw_buffer2[Y+1][X];
static uint8_t raw_buffer3[Y+1][X];
//static uint8_t* ring_buffer[IMAGE_RING_SIZE] = {raw_buffer1, raw_buffer2, raw_buffer3};

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

     CDBG("%s: BEGIN\n", __func__);

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

     /* find snapshot frame */
     printf("%d bufs in stream\n", bufs->num_bufs);

     if (user_frame_callback) { 
          for (i = 0; i < bufs->num_bufs; i++) {
               if (bufs->bufs[i]->stream_id == m_stream->s_id) {
                    
                    uint8_t* outbuf = (uint8_t*) raw_buffer[frameid % IMAGE_RING_SIZE];
                    m_frame = bufs->bufs[i];
                    
                    downsample_frame((uint64_t (*)[X6])m_frame->buffer,
                                     m_frame->planes[0].length,
                                     (uint8_t (*)[X])outbuf);
                    user_frame_callback(outbuf, X, Y);
                    frameid++;
               }
          }
     }
     if (NULL == m_frame) {
          CDBG_ERROR("%s: main frame is NULL", __func__);
          rc = -1;
          goto EXIT;
     }


EXIT:
     for (i=0; i<bufs->num_bufs; i++) {
          if (MM_CAMERA_OK != pme->cam->ops->qbuf(bufs->camera_handle,
                                                  bufs->ch_id,
                                                  bufs->bufs[i])) {
               CDBG_ERROR("%s: Failed in Qbuf\n", __func__);
          }
     }
     CDBG("%s: END\n", __func__);
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
    attr.max_unmatched_frames = 2;
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

    ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_CAPTURE);

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




int mm_camera_lib_send_command(mm_camera_lib_handle *handle,
                               mm_camera_lib_commands cmd,
                               void *in_data, void *out_data)
{
    uint32_t width, height;
    int rc = MM_CAMERA_OK;
    cam_capability_t *camera_cap = NULL;
    mm_camera_lib_snapshot_params *dim = NULL;

    if ( NULL == handle ) {
        CDBG_ERROR(" %s : Invalid handle", __func__);
        rc = MM_CAMERA_E_INVALID_INPUT;
        goto EXIT;
    }

    camera_cap = (cam_capability_t *) handle->test_obj.cap_buf.mem_info.data;

    switch(cmd) {
        case MM_CAMERA_LIB_RAW_CAPTURE:
            width = handle->test_obj.buffer_width;
            height = handle->test_obj.buffer_height;
            handle->test_obj.buffer_width =
                    (uint32_t)camera_cap->raw_dim.width;
            handle->test_obj.buffer_height =
                    (uint32_t)camera_cap->raw_dim.height;
            handle->test_obj.buffer_format = DEFAULT_RAW_FORMAT;
            CDBG_ERROR("%s: MM_CAMERA_LIB_RAW_CAPTURE %dx%d\n",
                       __func__,
                       camera_cap->raw_dim.width,
                       camera_cap->raw_dim.height);
	    
            rc = mm_app_start_capture_raw(&handle->test_obj, 1);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_start_capture() err=%d\n",
                           __func__, rc);
                goto EXIT;
            }
            break;

    case MM_CAMERA_LIB_NO_ACTION:
         break;
    default:
         printf("YOu missed commmand %d\n",cmd);
         exit(-5);
         break;
    };

EXIT:

    return rc;
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
CameraHandle camera_alloc(void)
{
   return malloc(sizeof(CameraObj));
}

int camera_init(CameraObj* camera)
{
    int rc = 0;
    uint8_t previewing = 0;
    int isZSL = 0;
    uint8_t wnr_enabled = 0;
    mm_camera_lib_handle lib_handle;
    int num_cameras;

    
    cam_scene_mode_type default_scene= CAM_SCENE_MODE_OFF;
    int set_tintless= 0;

    mm_camera_test_obj_t test_obj;
    memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));

    //open camera lib 
    rc = mm_camera_lib_open(&lib_handle, 0);
    if (rc != MM_CAMERA_OK) {
        CDBG_ERROR("%s:mm_camera_lib_open() err=%d\n", __func__, rc);
        return -1;
    }

    //get number cameras
    num_cameras =  lib_handle.app_ctx.num_cameras;
    if ( num_cameras <= 0 ) {
        CDBG_ERROR("%s: No camera sensors reported!", __func__);
        rc = -1;
	return rc;
    } //else we are goint to use the first one.

    camera->lib_handle = lib_handle;
    return 0;
}

extern void camera_install_callback(camera_cb cb);

int camera_start(CameraObj* camera, camera_cb cb)
{
  int rc = MM_CAMERA_OK;
  camera_install_callback(cb);

  rc = mm_camera_lib_send_command(&(camera->lib_handle),
				  MM_CAMERA_LIB_RAW_CAPTURE,
				  NULL,
				  NULL);
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s:mm_camera_lib_send_command() err=%d\n", __func__, rc);
    return rc;
  }
  return rc;
}

int camera_stop(CameraObj* camera)
{
  int rc;
  mm_camera_app_done();

//     printf("Wait\n");
  mm_camera_app_wait();

//  printf("Stop capture Raw\n");
  rc = mm_app_stop_capture_raw(&(camera->lib_handle.test_obj));
  if (rc != MM_CAMERA_OK) {
    CDBG_ERROR("%s: mm_app_stop_capture() err=%d\n",
	       __func__, rc);
  }
    
  return rc;
}


int camera_cleanup(CameraObj* camera)
{
  mm_camera_lib_close(&camera->lib_handle);
  return 0;
}


#define AS_APPLICATION
#ifdef AS_APPLICATION

int my_camera_callback(const uint8_t *frame, int width, int height)
{
  char file_name[64];
  static int frame_idx = 0;
  const char* name = "main";
  const char* ext = "gray";
  int file_fd;

  snprintf(file_name, sizeof(file_name), "/data/misc/camera/test/%s_%04d.%s", name, frame_idx++, ext);
  //      seq--;                                                                                                                    
  file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
  if (file_fd < 0) {
    CDBG_ERROR("%s: cannot open file %s \n", __func__, file_name);
  } else {
    write(file_fd,
          frame,
          width * height);
  }

  close(file_fd);
  CDBG("dump %s", file_name);
  //save file                                                                                                                       
  return 0;
}




int main()
{
    int rc = 0;

    CameraHandle camera  = camera_alloc();
    printf("Camera Capturer\n");

    camera_init(camera);

    printf("Starting\n");
    camera_start(camera, my_camera_callback);

    printf("Sleeping\n");
    sleep(2.0);
    printf("Done Sleeping\n");

    camera_stop(camera);

    camera_cleanup(camera);

    free(camera);
    
    printf("Exiting application\n");

    return rc;
}

#endif


///replace all the headers with #include "camera_dependencies.h"
/// generate that by preprocessing the current headers.
/// But what about the #defines
