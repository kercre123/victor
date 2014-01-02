// numtest constants
#define NUM 3
#define NAN 2
#define INF 1 

// __fpclassify constants
#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4 


#ifndef __ASSEMBLER__
extern const float fp32consts[];
#endif

#define FP32IDX_ZERO     (0+0)
#define FP32IDX_ZERO_N   (0+1)
#define FP32IDX_INF      (0+2)
#define FP32IDX_INF_N    (0+3)
#define FP32IDX_SNAN     (0+4)
#define FP32IDX_QNAN     (0+5)
#define FP32IDX_MINFLOAT (0+6)
#define FP32IDX_MAXFLOAT (0+7)

#define FP32IDX_0_5       (8+0)
#define FP32IDX_1         (8+1)
#define FP32IDX_2         (8+2)
#define FP32IDX_ROOTEPS   (8+3)
#define FP32IDX_ROOTEPS_F (8+4)
#define FP32IDX_BIGX      (8+5)
#define FP32IDX_SMALLX    (8+6)

#define FP32IDX_E         (15+ 0)
#define FP32IDX_LN_2      (15+ 1)
#define FP32IDX_LN_3      (15+ 2)
#define FP32IDX_LN_PI     (15+ 3)
#define FP32IDX_LN_10     (15+ 4)
#define FP32IDX_LOG_2     (15+ 5)
#define FP32IDX_LOG_E     (15+ 6)
#define FP32IDX_LOG_PI    (15+ 7)
#define FP32IDX_INV_E     (15+ 8)
#define FP32IDX_INV_LN_2  (15+ 9)
#define FP32IDX_INV_LOG_2 (15+10)
#define FP32IDX_HALF_LN_3 (15+11)
#define FP32IDX_LN2LO     (15+12)
#define FP32IDX_LN2HI     (15+13)

#define FP32IDX_PI                (29+0)
#define FP32IDX_2_PI              (29+1)
#define FP32IDX_HALF_PI           (29+2)
#define FP32IDX_QUARTER_PI        (29+3)
#define FP32IDX_THREE_QUARTERS_PI (29+4)
#define FP32IDX_INV_PI            (29+5)
#define FP32IDX_2_INV_PI          (29+6)
#define FP32IDX_SQRT_PI           (29+7)
#define FP32IDX_2_SQRT_PI         (29+8)

#define FP32IDX_SQRT_0_5          (38+0)
#define FP32IDX_SQRT_2            (38+1)
#define FP32IDX_SQRT_3            (38+2)

#define FP32IDX_TABLE_SINE        (41)
#define FP32IDX_TABLE_SINE_N      6

#define FP32IDX_TABLE_LOGARITHM   (47)
#define FP32IDX_TABLE_LOGARITHM_N 7

#define FP32IDX_TABLE_EXP         (54)
#define FP32IDX_TABLE_EXP_N       7

#define FP32IDX_TABLE_ASINE      (61)
#define FP32IDX_TABLE_ASINE_N    8

#define FP32IDX_TABLE_TAN        (69)
#define FP32IDX_TABLE_TAN_N      3
