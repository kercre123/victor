///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief Haar features calculation
///

#include "HaarDetector.h"
#include "HaarDetectorShared.h"
#include <mvcv.h>
#include <mvstl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

using namespace mvcv;
using namespace mvstl;

#define ALIGN_TO_LEFT(_addr)  (((u32)(_addr)) & 0xFFFFFFF8)
#define ALIGN_TO_RIGHT(_addr) ((((u32)(_addr)) + 4) & 0xFFFFFFF8)
unsigned int function_call = 0, stage_count = 0, classifier_count = 0, node_count = 0;

u32 frames = 0;

//u32 precomputedHidsCreated[MAX_HID_CASCADES];
//u32 precomputedHidsDMAin[MAX_HID_CASCADES];
//u32 precomputedHidsDDR[MAX_HID_CASCADES];
u32 liveHids[MAX_HID_CASCADES];

//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//
//--------------------------------- Specific Defines ----------------------------------//
//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//

#define MVCV_HAAR_DO_CANNY_PRUNING    1
#define MVCV_HAAR_SCALE_IMAGE         2
#define MVCV_MAGIC_MASK               0xFFFF0000
#define MVCV_HAAR_MAGIC_VAL           0x42500000
#define FLT_MIN          1.175494350822287507969e-38f

#define calc_sum( rect, offset ) \
    ((rect).p0[offset] - (rect).p1[offset] - (rect).p2[offset] + (rect).p3[offset])

#define calc_sum_p( sum0, sum1, sum2, sum3, offset ) \
    (sum0[offset] - sum1[offset] - sum2[offset] + sum3[offset])

#define MVCV_MAT_ELEM_PTR_FAST( mat, row, col, pix_size ) \
    ((mat).data + (size_t)(mat).step*(row) + (pix_size)*(col))

#define mvcv_get_ptr( sum, row, col ) \
    (((sumtype*)MVCV_MAT_ELEM_PTR_FAST((sum),(row),(col),sizeof(sumtype))))

#define mvcv_get_ptr_sq( sqsum, row, col ) \
    (((sqsumtype*)MVCV_MAT_ELEM_PTR_FAST((sqsum),(row),(col),sizeof(sqsumtype))))

#define MVCV_IS_HAAR_CLASSIFIER( haar )                                                  \
    ((haar) != NULL &&                                                                   \
    (((const HaarClassifierCascade*)(haar))->flags & MVCV_MAGIC_MASK)==MVCV_HAAR_MAGIC_VAL)

#ifdef __MOVICOMPILE__
fp32 fabs(fp32 x)
{
	if (x < 0)
		return -x;
	return x;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Create more efficient internal representation of haar classifier cascade
///////////////////////////////////////////////////////////////////////////////
HidHaarClassifierCascade* CreateHidHaarClassifierCascade(HaarClassifierCascade* cascade, u8* hiddenCascadeBuff)
{
    const float icv_stage_threshold_bias = 0.0001f;

    HidHaarClassifier *haar_classifier_ptr;
    HidHaarTreeNode   *haar_node_ptr;
    int               i, j, k, l;
    int               total_classifiers = 0;
    int               total_nodes = 0;
    Size_t            orig_window_size;
    int               max_count = 0;

//#ifndef NDEBUG
	// Debug: Clear the buffer so we make sure we're not reading previous values
	for (u32 i = 0; i < HID_CASCADE_SIZE; i++)
		hiddenCascadeBuff[i] = 0xCA;
//#endif

	assert(MVCV_IS_HAAR_CLASSIFIER(cascade) && "Invalid classifier pointer\n");
	assert(cascade->hid_cascade == null && "hid_cascade has been already created\n");
	assert((cascade->stage_classifier != null) && "No stage Classifier\n");
	assert((cascade->count > 0) && "Negative number of cascade stages");

	//profile& pHid = profileManager::getProfile(__FUNCTION__);
	//pHid.start();

    orig_window_size = cascade->orig_window_size;

    /* check input structure correctness and calculate total memory size needed for
       internal representation of the classifier cascade */
    for(i = 0; i < cascade->count; i++)
    {
        HaarStageClassifier* stage_classifier = cascade->stage_classifier + i;

        if( !stage_classifier->classifier || stage_classifier->count <= 0 )
        {
            DEBUG_PRINT1("header of the stage classifier #%d is invalid "
                     "(has null pointers or non-positive classfier count)", i );
        }

        max_count = MAX( max_count, stage_classifier->count ); // MAX is also defined in mvcv
        total_classifiers += stage_classifier->count;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            HaarClassifier* classifier = stage_classifier->classifier + j;

            total_nodes += classifier->count;
            for( l = 0; l < classifier->count; l++ )
            {
                for( k = 0; k < MVCV_HAAR_FEATURE_MAX; k++ )
                {
                    if( classifier->haar_feature[l].rect[k].r.width )
                    {
                        Rect_t r = classifier->haar_feature[l].rect[k].r;
                        if( r.width < 0 || r.height < 0 || r.y < 0 ||
                            r.x + r.width > orig_window_size.width ||
                            ((r.x < 0 || r.y + r.height > orig_window_size.height)))
                        {
                            DEBUG_PRINT3("rectangle #%d of the classifier #%d of "
                                     "the stage classifier #%d is not inside "
                                     "the reference (original) cascade window", k, j, i );
                        }
                    }
                }
            }
        }
    }

    HidHaarClassifierCascade* out = (HidHaarClassifierCascade*)hiddenCascadeBuff;

    /* init header */
    out->count = cascade->count;
	haar_classifier_ptr = (HidHaarClassifier*)(hiddenCascadeBuff + sizeof(HidHaarClassifierCascade) + 8);
	// Align to 64bits
	haar_classifier_ptr = (HidHaarClassifier*)( (int)haar_classifier_ptr & 0xfffffff8 );

    /* initialize internal representation */
    for( i = 0; i < cascade->count; i++ )
    {
        HaarStageClassifier    *stage_classifier = cascade->stage_classifier + i;
        HidHaarStageClassifier *hid_stage_classifier = out->stage_classifier + i;

        hid_stage_classifier->count = stage_classifier->count;
        hid_stage_classifier->threshold = stage_classifier->threshold - icv_stage_threshold_bias;
        hid_stage_classifier->classifier = haar_classifier_ptr;
#ifdef _ASM_KERNEL_OPT
        haar_classifier_ptr += (stage_classifier->count + 2);
#else /*_ASM_KERNEL_OPT*/
        haar_classifier_ptr += stage_classifier->count;
#endif /*_ASM_KERNEL_OPT*/

        for( j = 0; j < stage_classifier->count; j++ )
        {
            HaarClassifier    *classifier = stage_classifier->classifier + j;
            HidHaarClassifier *hid_classifier = hid_stage_classifier->classifier + j;
            int                node_count = classifier->count;

            for( l = 0; l < node_count; l++ )
            {
                HidHaarTreeNode* node = hid_classifier->node + l;
                HaarFeature* feature = classifier->haar_feature + l;
                memset(node, -1, sizeof(*node) );
                node->threshold = classifier->threshold[l];
                int idx = -classifier->left[l];
                node->left = (idx >= 0) ? classifier->alpha[idx] : 0.0;
                idx = -classifier->right[l];
                node->right = (idx >= 0) ? classifier->alpha[idx] : 0.0;
                if( fabs(feature->rect[2].weight) < DBL_EPSILON ||
                    feature->rect[2].r.width == 0               ||
                    feature->rect[2].r.height == 0)
                {
                    node->p[2][0] = node->p[2][1] = node->p[2][2] = node->p[2][3] = 0;
                }
            }
        }
    }

    cascade->hid_cascade = out;

	//pHid.stop();

    return out;
}

int cvRoundLocal( float value )
{
   int a;
   a =   (int)(value + (value >= 0 ? 0.5 : -0.5));
   return a;
}

#if 1
 #define cvRoundLocalP(_value) ((int)((_value) + 0.5))
#else
 #define cvRoundLocalP(_value) cvRoundLocal(_value)
#endif

#ifndef NDEBUG

u32 checksum(u8* buffer, u32 szBytes)
{
	u32 chksum = 0;
	u32* elements = (u32*)buffer;
	u32 step = sizeof(elements[0]);

	for (u32 i = 0; i < szBytes / step; i++)
		chksum ^= elements[i];

	return chksum;
}

#else

// Dummy define to remove calls
#define checksum(a, b) 0xCACACACA

#endif

///////////////////////////////////////////////////////////////////////////////
// Add integral images and other important pointers to HID structure
///////////////////////////////////////////////////////////////////////////////
void SetImagesForHaarClassifierCascade(HaarClassifierCascade* _cascade,
	u8* hiddenCascadeBuff,
	Mat* sum,
	Mat* sqsum,
	float scale,
	u32* outChecksum)
{
    HidHaarClassifierCascade *cascade;
    int                      i, j, k, l;
    Rect                     equRect;
    float                    weight_scale, roundValue;

	assert(MVCV_IS_HAAR_CLASSIFIER(_cascade) && "Invalid classifier pointer\n");
	assert(scale > 0 && "Scale must be positive\n");
	assert(IS_DDR(cascade) || IS_CMX(cascade));

	//profile& pSetImg = profileManager::getProfile(__FUNCTION__);
	//pSetImg.start();

    cascade = (HidHaarClassifierCascade*)hiddenCascadeBuff;

    equRect.x = equRect.y = cvRoundLocalP(scale);
    equRect.width = cvRoundLocal((_cascade->orig_window_size.width-2)*scale);
    equRect.height = cvRoundLocal((_cascade->orig_window_size.height-2)*scale);
    weight_scale = 1./(equRect.width*equRect.height);
    cascade->inv_window_area = weight_scale;

    cascade->p0 = mvcv_get_ptr(*sum, equRect.y, equRect.x);
    cascade->p1 = cascade->p0 + equRect.width;
    cascade->p2 = cascade->p0 + sum->step * equRect.height/sizeof(sum->step);
    cascade->p3 = cascade->p2 + equRect.width;

    cascade->pq0 = mvcv_get_ptr_sq(*sqsum, equRect.y, equRect.x);
    cascade->pq1 = cascade->pq0 + equRect.width;
    cascade->pq2 = cascade->pq0 + sum->step * equRect.height/sizeof(sum->step);
    cascade->pq3 = cascade->pq2 + equRect.width;

    /* init pointers in haar features according to real window size and given image pointers */
    for( i = 0; i < _cascade->count; i++ )
    {
        HaarStageClassifier* stage_classifier = &_cascade->stage_classifier[i];
        HidHaarStageClassifier* hid_stage_classifier = &cascade->stage_classifier[i];

		assert(IS_DDR(stage_classifier) || IS_CMX(stage_classifier));

        for(j = 0; j < stage_classifier->count; j++)
        {
            HaarClassifier* haar_classifier = &stage_classifier->classifier[j];
            HidHaarClassifier* hid_haar_classifier = &hid_stage_classifier->classifier[j];

			assert(IS_DDR(haar_classifier) || IS_CMX(haar_classifier));
			assert(IS_DDR(hid_haar_classifier) || IS_CMX(hid_haar_classifier));

            for (l = 0; l < MVCV_NODES_FRONTALFACE_MAX; l++)
            {
                HaarFeature *feature = &haar_classifier->haar_feature[l];
                HidHaarTreeNode *hidfeature = &hid_haar_classifier->node[l];
                float           sum0 = 0, area0 = 0;
                Rect_t  r[MVCV_HAAR_FEATURE_MAX];

                int base_w = -1, base_h = -1;
                int new_base_w = 0, new_base_h = 0;
                int kx, ky;
                int flagx = 0, flagy = 0;
                int x0 = 0, y0 = 0;
                int nr;

                hidfeature->weight[0] = hidfeature->weight[1] = hidfeature->weight[2] = 0.0;

                int x1, x2, x3, y1, y2, y3;
                int width1, width2, width3, height1, height2, height3;

                /* align blocks */
                r[0] = feature->rect[0].r;
                r[1] = feature->rect[1].r;
                
                x1 = r[0].x;
                x2 = r[1].x;

                y1 = r[0].y;
                y2 = r[1].y;

                width1 = r[0].width;
                width2 = r[1].width;

                height1 = r[0].height;
                height2 = r[1].height;

                base_w = (int)MIN(width1, width2) - 1;
                base_w = (int)MIN((unsigned)base_w, (unsigned)(x2 - x1 - 1));
                base_h = (int)MIN(height1, height2) - 1;
                base_h = (int)MIN((unsigned)base_h, (unsigned)(y2 - y1 - 1));
                nr = MVCV_HAAR_FEATURE_MAX - 1;
                if(hidfeature->p[2][0])
                {
                    r[2] = feature->rect[2].r;
                    x3 = r[2].x;
                    y3 = r[2].y;
                    width3 = r[2].width;
                    height3 = r[2].height;

                    base_w = (int)MIN((unsigned)base_w, (unsigned)(width3      - 1));
                    base_w = (int)MIN((unsigned)base_w, (unsigned)(x3 - x1 - 1));
                    base_h = (int)MIN((unsigned)base_h, (unsigned)(height3     - 1));
                    base_h = (int)MIN((unsigned)base_h, (unsigned)(y3 - y1 - 1));
                    nr = MVCV_HAAR_FEATURE_MAX;
                }

                base_w += 1;
                base_h += 1;
                kx = width1 / base_w;
                ky = height1 / base_h;

                if( kx <= 0 )
                {
                    flagx = 1;
                    roundValue = width1 * scale;
                    new_base_w = cvRoundLocalP(roundValue) / kx;
                    roundValue = x1 * scale;
                    x0 = cvRoundLocalP(roundValue);
                }

                if( ky <= 0 )
                {
                    flagy = 1;
                    roundValue = height1 * scale;
                    new_base_h = cvRoundLocalP(roundValue) / ky;
                    roundValue = y1 * scale;
                    y0 = cvRoundLocalP(roundValue);
                }

                for( k = 0; k < nr; k++ )
                {
                    Rect tr;
                    int x_tr, y_tr, width_tr, height_tr;
                    x_tr = tr.x;
                    y_tr = tr.y;
                    width_tr = tr.width;
                    height_tr = tr.height;

                    float correction_ratio;
                    int x_k, y_k, width_k, height_k;

                    x_k = r[k].x;
                    y_k = r[k].y;
                    width_k = r[k].width;
                    height_k = r[k].height;

                    if( flagx )
                    {
                        x_tr = (x_k - x1) * new_base_w / base_w + x0;
                        width_tr = width_k * new_base_w / base_w;
                    }
                    else
                    {
                        roundValue = x_k * scale;
                        x_tr = cvRoundLocalP(roundValue);
                        roundValue = width_k * scale;
                        width_tr = cvRoundLocalP(roundValue);
                    }

                    if( flagy )
                    {
                        y_tr = (y_k - y1) * new_base_h / base_h + y0;
                        height_tr = height_k * new_base_h / base_h;
                    }
                    else
                    {
                        roundValue = y_k * scale;
                        y_tr = cvRoundLocalP(roundValue);
                        roundValue = height_k * scale;
                        height_tr = cvRoundLocalP(roundValue);
                    }

                    correction_ratio = weight_scale;
                    hidfeature->p[k][0] = mvcv_get_ptr(*sum, y_tr, x_tr);
                    hidfeature->p[k][1] = hidfeature->p[k][0] +  width_tr;
                    hidfeature->p[k][2] = hidfeature->p[k][0] + sum->step * height_tr/4;
                    hidfeature->p[k][3] = hidfeature->p[k][2] + width_tr;

                    hidfeature->weight[k] = (float)(feature->rect[k].weight * correction_ratio);

                    if( k == 0 )
                        area0 = width_tr * height_tr;
                    else
                        sum0 += hidfeature->weight[k] * width_tr * height_tr;
                }

                hidfeature->weight[0] = (float)(-sum0/area0);
                if(hidfeature->p[2][0] == 0)
                {
                    hidfeature->p[2][0] = (sumtype*)ALIGN_TO_LEFT(hidfeature->p[0][0]);
                    hidfeature->p[2][1] = (sumtype*)ALIGN_TO_RIGHT(hidfeature->p[0][1]);
                    hidfeature->p[2][2] = (sumtype*)ALIGN_TO_LEFT(hidfeature->p[0][2]);
                    hidfeature->p[2][3] = (sumtype*)ALIGN_TO_RIGHT(hidfeature->p[0][3]);
                }
           } /* l */
        } /* j */
#ifdef _ASM_KERNEL_OPT
        for(j = hid_stage_classifier->count; j < hid_stage_classifier->count + 2; j++)
        {
            HidHaarClassifier *hid_classifier = &hid_stage_classifier->classifier[j];
            for(l = 0; l < 2; l++)
            {
                HidHaarTreeNode* node = hid_classifier->node + l;

                node->threshold = 0.0;
                node->left = 0.0;
                node->right = 0.0;
                for(k = 0; k < 3; k++ )
                {
                    node->p[k][0] = mvcv_get_ptr(*sum, 0, 0);
                    node->p[k][1] = mvcv_get_ptr(*sum, 0, 1);
                    node->p[k][2] = mvcv_get_ptr(*sum, 0, 2);
                    node->p[k][3] = mvcv_get_ptr(*sum, 0, 3);
                    node->weight[k] = 0.0;
                }
            }
        }
#endif /*_ASM_KERNEL_OPT*/
    }
	//pSetImg.stop();

	// Compute checksum
	if (outChecksum != null)
		*outChecksum = checksum(hiddenCascadeBuff, KB(160));
}


// Group together all multiple detection of the same face
// check if rectangles overlap
inline bool PredicateMVCV( const Rect r1,
                           const Rect r2,
                           float eps)
{
    float delta = eps*(min(r1.width, r2.width) + min(r1.height, r2.height))*0.5;
    return abs(r1.x - r2.x) <= delta &&
    abs(r1.y - r2.y) <= delta &&
    abs(r1.x + r1.width - r2.x - r2.width) <= delta &&
    abs(r1.y + r1.height - r2.y - r2.height) <= delta;
}

// detect the number of faces in the iamge
int PartitionMVCV( vector<Rect>& _vec,
                   vector<int>& labels,
                   float eps)
{
    int i, j, N = (int)_vec.size();
    const Rect* vec = &_vec[0];
    const int PARENT=0;
    const int RANK=1;

    vector<int> _nodes(N*2);
    int (*nodes)[2] = (int(*)[2])_nodes.data();

    // The first O(N) pass: create N single-vertex trees
    for(i = 0; i < N; i++)
    {
        nodes[i][PARENT]=-1;
        nodes[i][RANK] = 0;
    }

    // The main O(N^2) pass: merge connected components
    for( i = 0; i < N; i++ )
    {
        int root = i;

        // find root
        while( nodes[root][PARENT] >= 0 )
            root = nodes[root][PARENT];

        for( j = 0; j < N; j++ )
        {
            if( i == j || !PredicateMVCV(vec[i], vec[j], eps))
                continue;
            int root2 = j;

            while( nodes[root2][PARENT] >= 0 )
                root2 = nodes[root2][PARENT];

            if( root2 != root )
            {
                // unite both trees
                int rank = nodes[root][RANK], rank2 = nodes[root2][RANK];
                if( rank > rank2 )
                    nodes[root2][PARENT] = root;
                else
                {
                    nodes[root][PARENT] = root2;
                    nodes[root2][RANK] += rank == rank2;
                    root = root2;
                }

                int k = j, parent;

                // compress the path from node2 to root
                while( (parent = nodes[k][PARENT]) >= 0 )
                {
                    nodes[k][PARENT] = root;
                    k = parent;
                }

                // compress the path from node to root
                k = i;
                while( (parent = nodes[k][PARENT]) >= 0 )
                {
                    nodes[k][PARENT] = root;
                    k = parent;
                }
            }
        }
    }
    // Final O(N) pass: enumerate classes
    labels.resize(N);
    int nclasses = 0;

    for( i = 0; i < N; i++ )
    {
        int root = i;
        while( nodes[root][PARENT] >= 0 )
            root = nodes[root][PARENT];
        // re-use the rank as the class label
        if( nodes[root][RANK] >= 0 )
            nodes[root][RANK] = ~nclasses++;
        labels.push_back(~nodes[root][RANK]);
    }

    return nclasses;
}
// group toughether and retain only detection with min group threshold number of neighbords
void GroupRectanglesMVCV( vector<Rect>& rectList,
                          int groupThreshold,
                          float eps,
                          vector<int>* weights,
                          vector<float>* levelWeights)
{
    //profile& pGroupRect = profileManager::getProfile(__FUNCTION__);
	//pGroupRect.start();

    if( groupThreshold <= 0 || rectList.empty() )
    {
        if( weights )
        {
            size_t i, sz = rectList.size();
            weights->resize(sz);
            for( i = 0; i < sz; i++ )
                (*weights)[i] = 1;
        }
        return;
    }
    vector<int> labels(rectList.size());
    int nclasses = PartitionMVCV(rectList, labels, eps);

    vector<Rect> rrects(nclasses); 

    Rect r(0,0,0,0);
    for (int i = 0; i < nclasses; i++)
    {
        rrects.push_back(r);
        
    }
    vector<int> rweights(nclasses, 0);
    vector<int> rejectLevels(nclasses, 0);
    vector<float> rejectWeights(nclasses, FLT_MIN);
    int i, j, nlabels = (int)labels.size();

    for( i = 0; i < nlabels; i++ )
    {
        int cls = labels[i];
        rrects[cls].x += rectList[i].x;
        rrects[cls].y += rectList[i].y;
        rrects[cls].width += rectList[i].width;
        rrects[cls].height += rectList[i].height;
        rweights[cls]++;
    }
    if ( levelWeights && weights && !weights->empty() && !levelWeights->empty() )
    {
        for( i = 0; i < nlabels; i++ )
        {
            int cls = labels[i];
            if( (*weights)[i] > rejectLevels[cls] )
            {
                rejectLevels[cls] = (*weights)[i];
                rejectWeights[cls] = (*levelWeights)[i];
            }
            else if( ( (*weights)[i] == rejectLevels[cls] ) && ( (*levelWeights)[i] > rejectWeights[cls] ) )
                rejectWeights[cls] = (*levelWeights)[i];
        }
    }
    
    for( i = 0; i < nclasses; i++ )
    {
        Rect r = rrects[i];
        float s = 1.f/rweights[i];
        rrects[i] = Rect( cvRound(r.x*s),
                          cvRound(r.y*s),
                          cvRound(r.width*s),
                          cvRound(r.height*s));
    }
    
    rectList.clear();
    if( weights )
        weights->clear();
    if( levelWeights )
        levelWeights->clear();
    
    for( i = 0; i < nclasses; i++ )
    {
        Rect r1 = rrects[i];
        int n1 = levelWeights ? rejectLevels[i] : rweights[i];
        float w1 = rejectWeights[i];
        if( n1 <= groupThreshold )
            continue;
        // filter out small face rectangles inside large rectangles
        for( j = 0; j < nclasses; j++ )
        {
            int n2 = rweights[j];
            
            if( j == i || n2 <= groupThreshold )
                continue;
            Rect r2 = rrects[j];
            
            int dx = cvRound( r2.width * eps );
            int dy = cvRound( r2.height * eps );
            
            if( i != j &&
                r1.x >= r2.x - dx &&
                r1.y >= r2.y - dy &&
                r1.x + r1.width <= r2.x + r2.width + dx &&
                r1.y + r1.height <= r2.y + r2.height + dy &&
                (n2 > max(3, n1) || n1 < 3) )
                break;
        }
        
        if( j == nclasses )
        {
            rectList.push_back(r1);
            if( weights )
                weights->push_back(n1);
            if( levelWeights )
                levelWeights->push_back(w1);
        }
    }
    //pGroupRect.stop();
}

//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//
//-----------------------------------Main Functions -----------------------------------//
//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//
///////////////////////////////////////////////////////////////////////////////
// Evalaute the classifier
///////////////////////////////////////////////////////////////////////////////
float EvalHidHaarClassifier(HidHaarClassifier *classifier,
                                    float             variance_norm_factor,
                                    unsigned int      p_offset)
{
    int   idx = 0;
    float alpha = 0.0;
    do
    {
        HidHaarTreeNode *node = &classifier->node[idx];
        float           t;
        float           sum;
        node_count++;
        t = node->threshold * variance_norm_factor;
        sum  = calc_sum_p(node->p[0][0], node->p[0][1], node->p[0][2], node->p[0][3], p_offset) * node->weight[0];
        sum += calc_sum_p(node->p[1][0], node->p[1][1], node->p[1][2], node->p[1][3], p_offset) * node->weight[1];
        /* In case of no using the third rect => node->weight[2] == 0 and p[2][x] == p[0][x] */
        sum += calc_sum_p(node->p[2][0], node->p[2][1], node->p[2][2], node->p[2][3], p_offset) * node->weight[2];
        alpha = sum < t ? node->left : node->right;
        idx ++;
    }
    while(alpha == 0.0);

    return alpha;
}

///////////////////////////////////////////////////////////////////////////////
// Run cascade classifier over the image
///////////////////////////////////////////////////////////////////////////////
int RunHaarClassifierCascade(const HidHaarClassifierCascade *cascade,
                             int                            p_offset,
                             float                          *p_stage_sum,
                             int                            start_stage)
{
    int                      result = -1;
    int                      i, j;
    float                    mean, variance_norm_factor;

    if(!cascade)
    {
        DEBUG_PRINT("Hidden cascade has not been created \n");
        return -1;
    }

    mean = calc_sum(*cascade, p_offset) * cascade->inv_window_area;
    variance_norm_factor = cascade->pq0[p_offset] - cascade->pq1[p_offset] - cascade->pq2[p_offset] + cascade->pq3[p_offset];

    variance_norm_factor = variance_norm_factor * cascade->inv_window_area - mean * mean;
    if(variance_norm_factor >= 0.)
        variance_norm_factor = sqrt(variance_norm_factor);
    else
        variance_norm_factor = 1.;
	function_call++;

    for(i = start_stage; i < cascade->count; i++)
    {
		stage_count++;
        *p_stage_sum = 0.0;
        for(j = 0; j < cascade->stage_classifier[i].count; j++)
        {
			classifier_count++;
            *p_stage_sum += EvalHidHaarClassifier(cascade->stage_classifier[i].classifier + j,
                                                    variance_norm_factor,
                                                    p_offset);
        }
        if(*p_stage_sum < cascade->stage_classifier[i].threshold)
            return -i;
    }
    return 1;
}

extern "C" int RunHaarClassifierCascade_asm(const HidHaarClassifierCascade *cascade, int p_offset, float *p_stage_sum, int start_stage);
///////////////////////////////////////////////////////////////////////////////
// Call detect object cascade classifier withe scaling model
///////////////////////////////////////////////////////////////////////////////
void
HaarDetectObjects_ScaleCascade_Invoker( const Range range,
                                        const HaarClassifierCascade* cascade,
                                        CvSize winsize,
                                        const Range xrange,
                                        float ystep,
                                        size_t sumstep,
                                        const int** pq,
                                        vector<Rect>& _vec)
{
    vector<Rect>* vec = &_vec;
    int iy, startY = range.start, endY = range.end;
    const int *pq0 = pq[0], *pq1 = pq[1], *pq2 = pq[2], *pq3 = pq[3];
    int sstep = (int)(sumstep/sizeof(pq0[0]));
    int result;

    //profile& pHaar = profileManager::getProfile(__FUNCTION__);
    //pHaar.start();

    for( iy = startY; iy < endY; iy++ )
    {
        int ix, y = cvRound(iy*ystep), ixstep = 1;
        for( ix = xrange.start; ix < xrange.end; ix += ixstep )
        {
            int x = cvRound(ix*ystep); 
               
            int offset = y*sstep + x;
            int sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
            if( sq < 20 )
            {
                ixstep = 2;
                continue;
            }
            float stage_sum = 0.0;
#ifndef __MOVICOMPILE__
            result = RunHaarClassifierCascade(cascade->hid_cascade, offset, &stage_sum, 0);
#else /*__MOVICOMPILE__*/
            result = RunHaarClassifierCascade_asm(cascade->hid_cascade, offset, &stage_sum, 0);
#endif /*__MOVICOMPILE__*/
            if( result > 0 )
                vec->push_back(Rect(x, y, winsize.width, winsize.height));

            ixstep = result != 0 ? 1 : 2;
        }
    }
    
	//pHaar.stop();
}
///////////////////////////////////////////////////////////////////////////////
// Prepare HID cascade classifier and call specific invoker
///////////////////////////////////////////////////////////////////////////////
vector<Rect> HaarDetectObjectsForROC(Mat& img, Mat& sum, Mat& sqsum, 
	u8* cascadeBuffer, u8* hidCascadeLocalBuffer, u8* hidCascadesAllBuffers[], 
	fp32 scaleFactor, u32 minNeighbors,	CvSize maxWinSize, CvSize minWinSize, 
	u32 winSizes[], u32 flags, bool usePrecomputedHid)
{
    HaarClassifierCascade* cascade  = (HaarClassifierCascade*)cascadeBuffer;
    u8* hiddenCascadeBuff           = hidCascadeLocalBuffer;
    const float GROUP_EPS           = 0.2;
    float ystep                     = 0.0;
	float factor;
	int result;

	assert(MVCV_IS_HAAR_CLASSIFIER(cascade) && "\nInvalid classifier cascade");
    assert(scaleFactor > 1 && "\nScale factor must be > 1");
    assert(img.type == CV_8UC1 && "\nOnly 8-bit images are supported");

	//profile& pDma = profileManager::getProfile("DMA");

	//profile& pHaarDetect = profileManager::getProfile(__FUNCTION__);
	//pHaarDetect.start();

    // max window size by default is the size of the frame
	if (maxWinSize.height == 0 || maxWinSize.width == 0)
    {
		maxWinSize.height = img.rows;
		maxWinSize.width  = img.cols;
    }

    vector<Rect> allCandidates;
    vector<Rect> rectList;
    vector<int> rweights;

    int total_size = 0;

    if( !(flags & MVCV_HAAR_SCALE_IMAGE) )
    {
        factor = 1;
        int scale = 0;

    // Read integral images thourgh L2 cache
    sqsum.data = L2CACHE(sqsum.data);

    HidHaarClassifierCascade* _cascade = (HidHaarClassifierCascade*)hiddenCascadeBuff;
    _cascade = cascade->hid_cascade;
    _cascade->sum = sum;
    _cascade->sqsum = sqsum;

    for (int i = 0; i < MAX_SCALES; i++)
    {
        int current_window_size = winSizes[i];
        CvSize winSize = CvSize(current_window_size, current_window_size);

        if (FIXED_WINDOW_SIZES == 1)
        {
            factor = current_window_size/(20.0);
            ystep = factor;
        }
        else 
        {
            if (i > 0) // factor of first scale must be 1
                factor *= scaleFactor;
            ystep = 2. > factor ? 2. : factor;
        }

        int equX, equY, equWidth, equHeight;
        int *pq[4] = {0,0,0,0};
        int endX = cvRoundLocal((img.cols - winSize.width) / ystep);
        int endY = cvRoundLocal((img.rows - winSize.height) / ystep);

        if (winSize.width < minWinSize.width || winSize.height < minWinSize.height)
            continue;

        cascade->scale = factor;
        cascade->real_window_size.width = winSize.width;
        cascade->real_window_size.height = winSize.height;

        if (usePrecomputedHid)
        {
            //precomputedHidsDDR[scale] = checksum(cfg->all_cascade_hid_ptr[scale], KB(160));
            //pDma.start();
			scDmaSetup(DMA_TASK_0, (u8*)L2CACHE(hidCascadesAllBuffers[scale]), hidCascadeLocalBuffer, HID_CASCADE_SIZE);
            scDmaStart(START_DMA0);
            scDmaWaitFinished();
            //pDma.stop();

            //precomputedHidsDMAin[scale] = checksum(cfg->cascade_hid_ptr, KB(160));

            // Hidden cascades integrity check
            ///assert(precomputedHidsDDR[scale] == precomputedHidsDMAin[scale]);

            /*assert(precomputedHidsDMAin[scale] == liveHids[scale] && "Make sure the hidden cascades were \
                    computed on the fly and are correct, before the code reaches this line");*/

            cascade->hid_cascade = (HidHaarClassifierCascade *)hidCascadeLocalBuffer;
            cascade->hid_cascade->sum = sum;
            cascade->hid_cascade->sqsum = sqsum;
        }
        else
        {
            SetImagesForHaarClassifierCascade(cascade, hiddenCascadeBuff, &sum, &sqsum, factor, &liveHids[scale]);
        }

        scale++;

        equX      = cvRoundLocalP(winSize.width*0.15);
        equY      = cvRoundLocalP(winSize.height*0.15);
        equWidth  = cvRoundLocalP(winSize.width*0.7);
        equHeight = cvRoundLocalP(winSize.height*0.7);

        pq[0] = (int*)(sum.data + equY*sum.step) + equX;
        pq[1] = pq[0] + equWidth;
        pq[2] = pq[0] + equHeight*sum.step/sizeof(sum.step);
        pq[3] = pq[2] + equWidth;

        int startX = 0, startY = 0;

        HaarDetectObjects_ScaleCascade_Invoker( Range(startY, endY), cascade, winSize, Range(startX, endX),
                                                    ystep, sum.step, (const int**)pq, allCandidates );
        }
    }

    if( minNeighbors != 0 )
    {
        GroupRectanglesMVCV(allCandidates, minNeighbors, GROUP_EPS, &rweights, 0);
    }
	frames++;

	// Flip hidden cascades creation strategy
	//usePrecomputedHid = !usePrecomputedHid;

	//pHaarDetect.stop();

    return allCandidates;
}

/* End of file. */
