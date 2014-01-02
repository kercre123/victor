#ifndef __HAAR_CASCADE_H__
#define __HAAR_CASCADE_H__

#ifndef __LEON__
#include <mvcv_types.h>
#endif

#define MVCV_HAAR_FEATURE_MAX 3

typedef struct
{
    s32 x, y, width, height;
} Rect_t;

typedef struct
{
    s32 width;
    s32 height;
} Size_t;

typedef struct HaarFeature
{
    int tilted;
    struct
    {
        Rect_t r;
        float  weight;
    } rect[MVCV_HAAR_FEATURE_MAX];
} HaarFeature;

typedef struct HaarClassifier
{
    int				count;
    HaarFeature*	haar_feature;
    float*			threshold;
    int*			left;
    int*			right;
    float*			alpha;
} HaarClassifier;

typedef struct HaarStageClassifier
{
    int				count;
    float			threshold;
    HaarClassifier*	classifier;
    int				next;
    int				child;
    int				parent;
} HaarStageClassifier;

typedef struct HidHaarClassifierCascade HidHaarClassifierCascade;

typedef struct HaarClassifierCascade
{
    int							flags;
    int							count;
    Size_t						orig_window_size;
    Size_t						real_window_size;
    float						scale;
    HaarStageClassifier*		stage_classifier;
    HidHaarClassifierCascade*	hid_cascade;
} HaarClassifierCascade;

#endif