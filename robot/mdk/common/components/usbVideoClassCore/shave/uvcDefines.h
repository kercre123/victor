///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Video stream structure defines
///

#ifndef UVC_TYPES_H_
#define UVC_TYPES_H_

#include "usbDefines.h"

typedef struct __attribute__((packed))
{
    // interface raw yuv IN streaming
    interface_desc_t                     vs_interface_if0;
    vs_interface_input_header_desc_t     vs_interface_input_header_if0;
    vs_uncompressed_video_format_desc_t  vs_uncompressed_video_format_if0;
    vs_uncompressed_video_frame_desc_t   vs_uncompressed_video_frame_if0;
    vs_color_matching_desc_t             vs_color_matching_if0;
    interface_desc_t                     vs_interface_if1;
    endpoint_desc_t                      vs_endpoint_if1;
} uvc_vs_yuv_t;

typedef struct __attribute__((packed))
{
    // interface mpeg2ts IN streaming
    interface_desc_t                     vs_interface_if0;
    vs_interface_input_header_desc_t     vs_interface_input_header_if0;
    vs_mpeg2ts_video_format_desc_t       vs_mpeg2ts_video_format_if0;
    vs_color_matching_desc_t             vs_color_matching_if0;
    interface_desc_t                     vs_interface_if1;
    endpoint_desc_t                      vs_endpoint_if1;
} uvc_vs_mpeg_t;

// data structures following configuration descriptor are read together as part of GET_DESCRIPTOR, CONFIGURATION command
typedef struct __attribute__((packed))
{
    config_desc_t                        config;
    iad_desc_t                           iad;
    // interface 0 is always video control
    interface_desc_t                     vc_interface_if00;
    vc_interface_header_desc_t           vc_interface_header_if00;
    vc_camera_terminal_desc_t            vc_camera_terminal_cam;
    vc_output_terminal_desc_t            vc_output_terminal_usb;
} uvc_config_t;

typedef struct
{
    unsigned int interface_index;

    enum {YUV, MPEG2TS} type;
    union
    {
        uvc_vs_yuv_t  *vs_yuv;
        uvc_vs_mpeg_t *vs_mpeg;
    };

    video_probe_and_commit_control_t __attribute__((aligned(4))) *vs_commit_control_if;
    video_probe_and_commit_control_t __attribute__((aligned(4))) *vs_probe_control_if;

    void          (*probe)(unsigned int);
    unsigned int  (*mux)(unsigned int);
    unsigned int  (*header)(unsigned int);
    void          (*disable)(void);
    void          (*enable)(void);

}video_interface_t;

#define  uvc_vs_interface_header_uncompressed_sizeof (sizeof(vs_interface_input_header_desc_t) + sizeof(vs_uncompressed_video_format_desc_t) + sizeof(vs_uncompressed_video_frame_desc_t) + sizeof(vs_color_matching_desc_t))
#define  uvc_vs_interface_header_compressed_sizeof   (sizeof(vs_interface_input_header_desc_t) + sizeof(vs_mpeg2ts_video_format_desc_t) + sizeof(vs_color_matching_desc_t))

#endif /* UVC_TYPES_H_ */
