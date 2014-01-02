.version 00.70.00
.include svuCommonDefinitions.incl
.include svuCommonMacros.incl

;.set no_kernel  ; if set dezactivate the kernel

.ifdef no_kernel
.code .text.RunHaarClassifierCascade_asm
;int RunHaarClassifierCascade(const HaarClassifierCascade* _cascade(i18), int p_offset(i17), float *p_stage_sum(i16), int start_stage(i15))
RunHaarClassifierCascade_asm:

LSU0.LDIL i18, 0
IAU.SUB i18, i18, 1
BRU.JMP i30
NOP 5
.else

.data .data.RunHaarClassifierCascade_asm

;typedef struct
;{
;    sumtype *p[MVCV_HAAR_FEATURE_MAX][4];  // offset  0
;    float   weight[MVCV_HAAR_FEATURE_MAX]; // offset 48
;    float   threshold;                     // offset 60
;    float   left;                          // offset 64
;    float   right;                         // offset 68
;} HidHaarTreeNode;
.set NODE_STRUCT_SIZE         72
.set NODE_ADD_OFFSET          24

;typedef struct HidHaarClassifier
;{
;    HidHaarTreeNode node[2];               // offset 0
;}
;HidHaarClassifier;
.set CLASSIFIER_STRUCT_SIZE   144; 2*66 + 3*4
.set CLASSIFIER_STRUCT_SIZE_8 152

;typedef struct HidHaarStageClassifier
;{
;    HidHaarClassifier *classifier;         // offset 0
;    float             threshold;           // offset 4
;    u8                count;               // offset 8
;}
;HidHaarStageClassifier;
.set STAGE_STRUCT_SIZE        12
.set STAGE_THRESHOLD_OFFSET    4
.set STAGE_COUNT_OFFSET        8

;typedef struct HidHaarClassifierCascade
;{
;    HidHaarStageClassifier stage_classifier[MVCV_MAX_STAGES]; // offset  0
;    float                  inv_window_area;   // offset  0 + 12*20
;    {
;        s32 type;                             // offset  4 + 12*20
;        s32 rows;                             // offset  8 + 12*20
;        s32 cols;                             // offset 12 + 12*20
;        s32 flags;                            // offset 16 + 12*20
;        s32 size[2];                          // offset 20 + 12*20
;        s32 step0[2];                         // offset 28 + 12*20
;        u8* data;                             // offset 36 + 12*20
;        u8 allocated;                         // offset 40 + 12*20
;        size_t step;                          // offset 44 + 12*20
;    } Mat                  sum;
;    {
;        s32 type;                             // offset 48 + 12*20
;        s32 rows;                             // offset 52 + 12*20
;        s32 cols;                             // offset 56 + 12*20
;        s32 flags;                            // offset 60 + 12*20
;        s32 size[2];                          // offset 64 + 12*20
;        s32 step0[2];                         // offset 72 + 12*20
;        u8* data;                             // offset 80 + 12*20
;        u8 allocated;                         // offset 84 + 12*20
;        size_t step;                          // offset 88 + 12*20
;    } Mat                  sqsum;
;    sqsumtype              *pq0;              // offset 92 + 12*20
;    sqsumtype              *pq1;              // offset  96 + 12*20
;    sqsumtype              *pq2;              // offset 100 + 12*20
;    sqsumtype              *pq3;              // offset 104 + 12*20
;    sumtype                *p0;               // offset 108 + 12*20
;    sumtype                *p1;               // offset 112 + 12*20
;    sumtype                *p2;               // offset 116 + 12*20
;    sumtype                *p3;               // offset 120 + 12*20
;    u8                     count;             // offset 124 + 12*20
;} HidHaarClassifierCascade;
.set INV_WINDOW_OFFSET 240
.set PQ0_OFFSET        332
.set PQ1_OFFSET        336
.set PQ2_OFFSET        340
.set PQ3_OFFSET        344
.set P0_OFFSET         348
.set P1_OFFSET         352
.set P2_OFFSET         356
.set P3_OFFSET         360
.set COUNT_OFFSET      364

;NOTE!!!:
; 1) To check all the structure sizes and offsets
; 2) opt by removing p[2][0]==0
; 3) opt by adding one classifier to each stage

.code .text.RunHaarClassifierCascade_asm
;int RunHaarClassifierCascade(const HidHaarClassifierCascade *cascade(i18), int p_offset(i17), float *p_stage_sum(i16), int start_stage(i15), int p_offset(i14))
RunHaarClassifierCascade_asm:
;----------------- PART I -----------------

; ------------- IRF -------------
; common part I-II
.set iNull                  i0
.set stage                  i1
.set max_stages             i2
.set start_stage            i15
.set p_stage_sum            i16
.set p_offset               i17
.set cascade                i18
.set result                 i18

; part I
.set iTMP0                  i9
.set iTMP1                  i10
.set iTMP2                  i11
.set iTMP3                  i12

; part II
.set node0                  i3
.set node1                  i4
.set node2                  i5
.set max_classifiers        i6
.set minC                   i7
.set maxC                   i8

.set p00                  i9
.set p01                  i10
.set p02                  i9
.set p03                  i10
.set p10                  i11
.set p11                  i12
.set p12                  i11
.set p13                  i12
.set p20                  i13
.set p21                  i14
.set p22                  i13
.set p23                  i14

.set _node2               i7
.set classifier_size      i17

; ------------- SRF -------------
; common part I-II
.set sNull                  s0
.set var_norm_factor        s1
; part I
.set inv_window_area        s2
.set mean                   s3
.set sTMP0                  s4
.set sTMP1                  s5
.set sTMP2                  s6
.set sTMP3                  s7
; part II
.set stage_threshold        s2
.set stage_sum              s3
.set sum0                   s4
.set sum1                   s5
.set sum2                   s6
.set t0                     s7
.set t1                     s8
.set t2                     s9
.set left0                  s10
.set right0                 s11
.set alpha0                 s11 ; alpha <=> right
.set left1                  s12
.set right1                 s13
.set alpha1                 s13 ; alpha <=> right
.set left2                  s14
.set right2                 s15
.set alpha2                 s15 ; alpha <=> right
.set alpha                  s16

; ------------- VRF -------------
; common part I-II
.set voffset                v0
; part I
.set real_window_size       v1

; part II
.set weight0                v1 ; explicit value
.set weight1                v2 ; explicit value
.set weight2                v3 ; explicit value


.set vP00                   v4
.set vP01                   v5
.set vP02                   v6
.set vP03                   v7

.set vT                     v8
.set vVarNormFactor         v9


.set vTMP0                  v10
.set vTMP1                  v11
.set vTMP2                  v12
.set vTMP3                  v13
.set vTMP4                  v14
.set vTMP5                  v15
.set vTMP6                  v1
.set vTMP7                  v2
.set vTMP8                  v3

.set vP10                   v16
.set vP11                   v17
.set vP12                   v18
.set vP13                   v19
.set vP20                   v20
.set vP21                   v21
.set vP22                   v22
.set vP23                   v23

LSU0.LDO64.l vTMP0, cascade, PQ0_OFFSET                || LSU1.LDO64.h vTMP0, cascade, PQ2_OFFSET ; *pq3 *pq2 *pq1 *pq0
LSU0.LDO64.l vTMP1, cascade, P0_OFFSET                 || LSU1.LDO64.h vTMP1, cascade, P2_OFFSET ; *p3 *p2 *p1 *p0
LSU0.LDO32 inv_window_area, cascade, INV_WINDOW_OFFSET || LSU1.LDO8 max_stages, cascade, COUNT_OFFSET ; inv_window_area stage
IAU.XOR iNull, iNull, iNull
CMU.CPIS.i32.f32 sNull, iNull                          || IAU.SHL p_offset, p_offset, 2
IAU.XOR max_stages, max_stages, max_stages             || CMU.CPIVR.x32 voffset, p_offset
VAU.ADD.u32s vTMP0, vTMP0, voffset
VAU.ADD.u32s vTMP1, vTMP1, voffset
SAU.SUM.u32 iTMP0, vTMP0, 0x1 || CMU.CPVI.x32 iTMP1, v10.1
SAU.SUM.u32 iTMP2, vTMP0, 0x4 || CMU.CPVI.x32 iTMP3, v10.3
SAU.SUM.u32 iTMP0, vTMP1, 0x1 || CMU.CPVI.x32 iTMP1, v11.1 || LSU0.LD32 sTMP0, iTMP0 || LSU1.LD32 sTMP1, iTMP1
SAU.SUM.u32 iTMP2, vTMP1, 0x4 || CMU.CPVI.x32 iTMP3, v11.3 || LSU0.LD32 sTMP2, iTMP2 || LSU1.LD32 sTMP3, iTMP3
                                                              LSU0.LD32 iTMP0, iTMP0 || LSU1.LD32 iTMP1, iTMP1
                                                              LSU0.LD32 iTMP2, iTMP2 || LSU1.LD32 iTMP3, iTMP3
NOP
NOP
SAU.SUB.f32 sTMP0, sTMP0, sTMP1
SAU.SUB.f32 sTMP2, sTMP3, sTMP2
                                   IAU.SUB iTMP0, iTMP0, iTMP1
                                   IAU.SUB iTMP0, iTMP0, iTMP2
SAU.ADD.f32 sTMP0, sTMP0, sTMP2 || IAU.ADD iTMP0, iTMP0, iTMP3
                                   CMU.CPIS.i32.f32 mean, iTMP0
IAU.XOR max_classifiers, max_classifiers, max_classifiers
SAU.MUL.f32 mean, mean, inv_window_area
SAU.MUL.f32 sTMP0, sTMP0, inv_window_area
CMU.CPII stage, cascade
SAU.MUL.f32 mean, mean, mean    || LSU1.LDO8 max_classifiers, stage, STAGE_COUNT_OFFSET
LSU0.LDIL result, 1
CMU.CPIS.i32.f32 var_norm_factor, result
CMU.CMSS.f32 sTMP0, mean || SAU.SUB.f32 sTMP0, sTMP0, mean
LSU0.LD32 node0, stage
LSU0.LDO32 stage_threshold, stage, STAGE_THRESHOLD_OFFSET
CMU.CPSS.f32.f16 sTMP0, sTMP0
NOP
SAU.SQT.f16.l sTMP0, sTMP0
IAU.MUL.u32s p_offset, start_stage, STAGE_STRUCT_SIZE
CMU.CPSS stage_sum, sNull
IAU.ADD.u32s stage, stage, p_offset
IAU.ADD.u32s classifier_size, iNull, CLASSIFIER_STRUCT_SIZE
NOP 2
PEU.PC1C GTE || CMU.CPSS.f16l.f32 var_norm_factor, sTMP0
NOP
CMU.VSIGN.f32 var_norm_factor, var_norm_factor [n]
CMU.CPSVR.x32 vVarNormFactor, var_norm_factor || SAU.MUL.u32s max_classifiers, max_classifiers, classifier_size
; pre-loop of __stage_loop
IAU.ADD.u32s node1, node0, CLASSIFIER_STRUCT_SIZE    || LSU0.LD64.l vP00,  node0     || LSU1.LDO64.h vP00, node0,  8  ; p03 p02 p01 p00 ; v4
IAU.ADD.u32s node2, node1, CLASSIFIER_STRUCT_SIZE    || LSU0.LDO64.l vP01, node0, 16 || LSU1.LDO64.h vP01, node0, 24  ; p13 p12 p11 p10 ; v5
IAU.ADD.u32s maxC, node2, iNull                      || LSU0.LDO64.l vP02, node0, 32 || LSU1.LDO64.h vP02, node0, 40  ; p23 p22 p21 p20 ; v6
IAU.ADD.u32s max_classifiers, max_classifiers, node0 || LSU0.LD64.l vP10,  node1     || LSU1.LDO64.h vP10, node1,  8  ; p03 p02 p01 p00 ; v16
LSU0.LDO64.l vP11, node1, 16 || LSU1.LDO64.h vP11, node1, 24  ; p13 p12 p11 p10 ; v17
LSU0.LDO64.l vP12, node1, 32 || LSU1.LDO64.h vP12, node1, 40  ; p23 p22 p21 p20 ; v18
                                                        VAU.ADD.u32s vP00, vP00, voffset || LSU0.LD64.l vP20,  node2     || LSU1.LDO64.h vP20, node2,  8  ; p03 p02 p01 p00 ; v20
                                                        VAU.ADD.u32s vP01, vP01, voffset || LSU0.LDO64.l vP21, node2, 16 || LSU1.LDO64.h vP21, node2, 24  ; p13 p12 p11 p10 ; v21
SAU.SUM.u32 p00, vP00, 0x1 || CMU.CPVI.x32 p01, v4.1 || VAU.ADD.u32s vP02, vP02, voffset || LSU0.LDO64.l vP22, node2, 32 || LSU1.LDO64.h vP22, node2, 40  ; p23 p22 p21 p20 ; v22
SAU.SUM.u32 p10, vP01, 0x1 || CMU.CPVI.x32 p11, v5.1 || VAU.ADD.u32s vP10, vP10, voffset || LSU1.LD32r vTMP8, p01
SAU.SUM.u32 p20, vP02, 0x1 || CMU.CPVI.x32 p21, v6.1 || VAU.ADD.u32s vP11, vP11, voffset || LSU1.LD32r vP03, p00
;----------------- PART II -----------------
.lalign
__stage_classifier_nood_loop:
;.128
; =================================================== node0 ========================================== || =================================================== node1 ============================================= || =================================================== node2 =====================================================
;NOTE: 54 cycles
SAU.SUM.u32 p02, vP00, 0x4 || CMU.CPVI.x32 p03, v4.3  || LSU0.LD32r vP02, p10  || LSU1.LD32r vTMP7, p11 || VAU.ADD.u32s vP12, vP12, voffset
SAU.SUM.u32 p12, vP01, 0x4 || CMU.CPVI.x32 p13, v5.3  || LSU0.LD32r vP01, p20  || LSU1.LD32r vTMP6, p21 || VAU.ADD.u32s vP20, vP20, voffset
SAU.SUM.u32 p22, vP02, 0x4 || CMU.CPVI.x32 p23, v6.3  || LSU0.LD32r vTMP5, p02 || LSU1.LD32r vTMP2, p03 || VAU.ADD.u32s vP21, vP21, voffset
SAU.SUM.u32 p00, vP10, 0x1 || CMU.CPVI.x32 p01, v16.1 || LSU0.LD32r vTMP4, p12 || LSU1.LD32r vTMP1, p13 || VAU.ADD.u32s vP22, vP22, voffset
; LSU0.LD32r vTMP4, p12 || LSU1.LD32r vTMP1, p13 || SAU.SUM.u32 p00, vP10, 0x1 || CMU.CPVI.x32 p01, v16.1 || VAU.ADD.u32s vP22, vP22, voffset
; => always "Stall due to empty instruction fifo"
; and implicitly
; => INFO: SHAVE0: LSU 0 - Stall due to late read return
; => INFO: SHAVE0: LSU 1 - Stall due to late read return

SAU.SUM.u32 p10, vP11, 0x1 || CMU.CPVI.x32 p11, v17.1 || LSU0.LD32r vTMP3, p22 || LSU1.LD32r vTMP0, p23 || IAU.ADD.u32s node0, node0, 48
SAU.SUM.u32 p20, vP12, 0x1 || CMU.CPVI.x32 p21, v18.1 || LSU0.LD32r vP13, p00  || LSU1.LD32r vTMP8, p01 || IAU.ADD.u32s node1, node1, 48
SAU.SUM.u32 p02, vP10, 0x4 || CMU.CPVI.x32 p03, v16.3 || LSU0.LD32r vP12, p10  || LSU1.LD32r vTMP7, p11 || IAU.ADD.u32s node2, node2, 48
SAU.SUM.u32 p12, vP11, 0x4 || CMU.CPVI.x32 p13, v17.3 || LSU0.LD32r vP11, p20  || LSU1.LD32r vTMP6, p21 || VAU.ALIGNVEC vP00, vP03, vP02, 4
SAU.SUM.u32 p22, vP12, 0x4 || CMU.CPVI.x32 p23, v18.3 || LSU0.LD32r vTMP5, p02 || LSU1.LD32r vTMP2, p03 || VAU.ALIGNVEC vP00, vP00, vP01,   8
SAU.SUM.u32 p00, vP20, 0x1 || CMU.CPVI.x32 p01, v20.1 || LSU1.LD32r vTMP4, p12 || LSU0.LD32r vTMP1, p13 || VAU.ALIGNVEC vP01, vTMP8, vTMP7, 4
SAU.SUM.u32 p10, vP21, 0x1 || CMU.CPVI.x32 p11, v21.1 || LSU0.LD32r vTMP3, p22 || LSU1.LD32r vTMP0, p23 || VAU.ALIGNVEC vP01, vP01, vTMP6,  8
SAU.SUM.u32 p20, vP22, 0x1 || CMU.CPVI.x32 p21, v22.1 || LSU0.LD32r vP23, p00  || LSU1.LD32r vTMP8, p01 || VAU.ALIGNVEC vP02, vTMP5, vTMP4, 4
VAU.ALIGNVEC vP02, vP02, vTMP3, 8  || CMU.CPVI.x32 p03, v20.3     || LSU0.LD32r vP22, p10  || LSU1.LD32r vTMP7, p11 || SAU.SUM.u32 p02, vP20, 0x4
VAU.ALIGNVEC vP03, vTMP2, vTMP1, 4 || CMU.CPVI.x32 p13, v21.3     || LSU0.LD32r vP21, p20  || LSU1.LD32r vTMP6, p21 || SAU.SUM.u32 p12, vP21, 0x4
VAU.ALIGNVEC vP03, vP03, vTMP0, 8  || CMU.CPVI.x32 p23, v22.3     || LSU0.LD32r vTMP5, p02 || LSU1.LD32r vTMP2, p03 || SAU.SUM.u32 p22, vP22, 0x4
VAU.ACCPZ.i32 vP00                 || CMU.CPVCR.x32 vP10, v16.0   || LSU1.LD32r vTMP4, p12 || LSU0.LD32r vTMP1, p13
VAU.ACCN.i32  vP01                 || CMU.CPVCR.x32 vP11, v0.0    || LSU1.LD32r vTMP3, p22
VAU.ACCN.i32  vP02                 || CMU.CPVCR.x32 vP12, v12.0   || LSU1.LD32r vTMP0, p23
VAU.ACCPW.i32 vP00, vP03           || CMU.CPVCR.x32 vP13, v9.0    || LSU0.LD64.l weight0, node0 || LSU1.LDO64.h weight0, node0, 8 ; LSU1.LDXV weight0, node0; t0 weight[2] weight[1] weight[0]
VAU.ACCPZ.i32 vP10                 || CMU.CPVCR.x32 vP20, v20.0   || LSU0.LDXV weight1, node1 ; LSU0.LD64.l weight0, node0 || LSU1.LDO64.h weight0, node0, 8 ; t1 weight[2] weight[1] weight[0]
VAU.ACCN.i32  vP11                 || CMU.CPVCR.x32 vP21, v0.0    || LSU1.LDXV weight2, node2 ; t2 weight[2] weight[1] weight[0]
VAU.ACCN.i32  vP12                                                || LSU0.LDO32 left0, node0, 16
VAU.ACCPW.i32 vP10, vP13           || CMU.CPVCR.x32 vP22, v12.0   || LSU1.LDO32 right0, node0, 20
VAU.ACCPZ.i32 vP20                 || CMU.CPVCR.x32 vP23, v9.0    || LSU0.LDO32 left1, node1, 16
VAU.ACCN.i32  vP21                 || CMU.CPVV.i32.f32 vP00, vP00 || LSU1.LDO32 right1, node1, 20
VAU.ACCN.i32  vP22                 || CMU.CPVV.i32.f32 vP10, vP10 || LSU0.LDO32 left2, node2, 16
VAU.ACCPW.i32 vP20, vP23                                          || LSU1.LDO32 right2, node2, 20
CMU.CPVCR.x32 vT, v0.3
VAU.MUL.f32 vT, vT, vVarNormFactor
VAU.MUL.f32 vP00, vP00, weight0    || CMU.CPVV.i32.f32 vP20, vP20
VAU.MUL.f32 vP10, vP10, weight1
VAU.MUL.f32 vP20, vP20, weight2    || IAU.ADD.u32s node0, node0, NODE_ADD_OFFSET
CMU.CPVRC.x32 v3.3, vT
SAU.SUM.f32 sum0, vP00, 0xF        || CMU.CPVRC.x32 v14.3, vT
SAU.SUM.f32 sum1, vP10, 0xF        || CMU.CPVRC.x32 v17.3, vT || LSU1.LD64.l vP00,  node0
SAU.SUM.f32 sum2, vP20, 0xF        || LSU1.LDO64.h vP00, node0,  8       || IAU.ADD.u32s node1, node1, NODE_ADD_OFFSET
LSU1.LD64.l vP10,  node1
LSU1.LDO64.h vP10, node1,  8       || IAU.ADD.u32s node2, node2, NODE_ADD_OFFSET
PEU.PC1S LT || CMU.CPSS alpha0, left0
PEU.PC1S LT || CMU.CPSS alpha1, left1
PEU.PC1S LT || CMU.CPSS alpha2, left2
CMU.CMZ.f32 alpha0
PEU.PCCX.NEQ 0x33 || IAU.ADD.u32s maxC, maxC, CLASSIFIER_STRUCT_SIZE || SAU.ADD.u32s node0, maxC, classifier_size || CMU.CMZ.f32 alpha1 || LSU0.LDO64.l vP00, maxC, CLASSIFIER_STRUCT_SIZE || LSU1.LDO64.h vP00, maxC, CLASSIFIER_STRUCT_SIZE_8
PEU.PCCX.NEQ 0x33 || IAU.ADD.u32s maxC, maxC, CLASSIFIER_STRUCT_SIZE || SAU.ADD.u32s node1, maxC, classifier_size || CMU.CMZ.f32 alpha2 || LSU0.LDO64.l vP10, maxC, CLASSIFIER_STRUCT_SIZE || LSU1.LDO64.h vP10, maxC, CLASSIFIER_STRUCT_SIZE_8
PEU.PCCX.NEQ 0x3  || IAU.ADD.u32s maxC, maxC, CLASSIFIER_STRUCT_SIZE || SAU.ADD.u32s node2, maxC, classifier_size                       || LSU0.LDO64.l vP01, node0, 16 || LSU1.LDO64.h vP01, node0, 24  ; p13 p12 p11 p10 ; v5
; PEU.PCCX.NEQ 0x3  || IAU.ADD.u32s maxC, maxC, CLASSIFIER_STRUCT_SIZE || SAU.ADD.u32s node2, maxC, classifier_size || LSU0.LDO64.l vP01, node0, 16 || LSU1.LDO64.h vP01, node0, 24  ; p13 p12 p11 p10 ; v5
; => always "Stall due to empty instruction fifo"
IAU.MIN.u32 minC, node0, node1 || SAU.ADD.f32 alpha0, alpha0, alpha1                                                                    || LSU0.LDO64.l vP02, node0, 32 || LSU1.LDO64.h vP02, node0, 40  ; p23 p22 p21 p20 ; v6
IAU.MIN.u32 minC, minC, node2  || SAU.ADD.f32 stage_sum, stage_sum, alpha2                                                              || LSU0.LDO64.l vP11, node1, 16 || LSU1.LDO64.h vP11, node1, 24  ; p13 p12 p11 p10 ; v17
IAU.SUB.u32s minC, max_classifiers, minC                                                                                                || LSU0.LDO64.l vP12, node1, 32 || LSU1.LDO64.h vP12, node1, 40  ; p23 p22 p21 p20 ; v18
PEU.PC1I GT || BRU.BRA __stage_classifier_nood_loop                                                                                     || LSU0.LD64.l vP20,  node2     || LSU1.LDO64.h vP20, node2,  8
; PEU.PC1I GT || BRU.BRA __classifier_nood_loop || LSU0.LD64.l vP20,  node2     || LSU1.LDO64.h vP20, node2,  8
; => sometime "Stall due to BRU miss"
SAU.ADD.f32 stage_sum, stage_sum, alpha0                                                 || VAU.ADD.u32s vP00, vP00, voffset            || LSU0.LDO64.l vP21, node2, 16 || LSU1.LDO64.h vP21, node2, 24  ; p13 p12 p11 p10 ; v21
                                                                                            VAU.ADD.u32s vP01, vP01, voffset
SAU.SUM.u32 p00, vP00, 0x1 || CMU.CPVI.x32 p01, v4.1                                     || VAU.ADD.u32s vP02, vP02, voffset            || LSU0.LDO64.l vP22, node2, 32 || LSU1.LDO64.h vP22, node2, 40  ; p23 p22 p21 p20 ; v22
SAU.SUM.u32 p10, vP01, 0x1 || CMU.CPVI.x32 p11, v5.1 || LSU1.LD32r vTMP8, p01            || VAU.ADD.u32s vP10, vP10, voffset
SAU.SUM.u32 p20, vP02, 0x1 || CMU.CPVI.x32 p21, v6.1 || LSU1.LD32r vP03, p00 || VAU.ADD.u32s vP11, vP11, voffset
;.endb
;    }
;    while(minC < stage->count);
;-------------------------------------------------------------
CMU.CMSS.f32 stage_sum, stage_threshold || IAU.ADD.u32s stage, stage, STAGE_STRUCT_SIZE
; Load the address of node0 to iTMP0 to avoid later a load from unavailable address
LSU1.LDO8 max_classifiers, stage, STAGE_COUNT_OFFSET || LSU0.LD32 iTMP0, stage
PEU.PC1C LT || IAU.SUB result, iNull, start_stage || CMU.CPII start_stage, max_stages
IAU.INCS start_stage, 1 || LSU0.LDO32 stage_threshold, stage, STAGE_THRESHOLD_OFFSET
CMU.CMII.i32 start_stage, max_stages || IAU.XOR max_classifiers, max_classifiers, max_classifiers
PEU.PC1C GTE || BRU.JMP i30 || LSU0.ST32 stage_sum, p_stage_sum
CMU.CPSS stage_sum, sNull
PEU.PC1C LT || CMU.CPII node0, iTMP0

; post-loop of __stage_loop
SAU.MUL.u32s max_classifiers, max_classifiers, classifier_size
IAU.ADD.u32s node1, node0, CLASSIFIER_STRUCT_SIZE                                        || LSU0.LD64.l vP00,  node0 || LSU1.LDO64.h vP00, node0,  8  ; p03 p02 p01 p00 ; v4
IAU.ADD.u32s node2, node1, CLASSIFIER_STRUCT_SIZE                                        || LSU0.LDO64.l vP01, node0, 16 || LSU1.LDO64.h vP01, node0, 24  ; p13 p12 p11 p10 ; v5
IAU.ADD.u32s maxC, node2, iNull                                                          || LSU0.LDO64.l vP02, node0, 32 || LSU1.LDO64.h vP02, node0, 40  ; p23 p22 p21 p20 ; v6
IAU.ADD.u32s max_classifiers, max_classifiers, node0                                     || LSU0.LD64.l vP10,  node1     || LSU1.LDO64.h vP10, node1,  8  ; p03 p02 p01 p00 ; v16
                                                                                            LSU0.LDO64.l vP11, node1, 16 || LSU1.LDO64.h vP11, node1, 24  ; p13 p12 p11 p10 ; v17
BRU.BRA __stage_classifier_nood_loop                                                     || LSU0.LDO64.l vP12, node1, 32 || LSU1.LDO64.h vP12, node1, 40  ; p23 p22 p21 p20 ; v18
                                                        VAU.ADD.u32s vP00, vP00, voffset || LSU0.LD64.l vP20,  node2     || LSU1.LDO64.h vP20, node2,  8  ; p03 p02 p01 p00 ; v20
                                                        VAU.ADD.u32s vP01, vP01, voffset || LSU0.LDO64.l vP21, node2, 16 || LSU1.LDO64.h vP21, node2, 24  ; p13 p12 p11 p10 ; v21
SAU.SUM.u32 p00, vP00, 0x1 || CMU.CPVI.x32 p01, v4.1 || VAU.ADD.u32s vP02, vP02, voffset || LSU0.LDO64.l vP22, node2, 32 || LSU1.LDO64.h vP22, node2, 40  ; p23 p22 p21 p20 ; v22
SAU.SUM.u32 p10, vP01, 0x1 || CMU.CPVI.x32 p11, v5.1 || VAU.ADD.u32s vP10, vP10, voffset || LSU1.LD32r vTMP8, p01
SAU.SUM.u32 p20, vP02, 0x1 || CMU.CPVI.x32 p21, v6.1 || VAU.ADD.u32s vP11, vP11, voffset || LSU1.LD32r vP03, p00
;-------------------------------------------------------------
;}
;while(start_stage < max_stages);
.end
.endif

