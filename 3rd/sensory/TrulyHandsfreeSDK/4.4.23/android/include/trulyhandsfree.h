/*
 * Sensory Confidential
 *
 * Copyright (C)2000-2017 Sensory Inc.
 * http://www.sensory.com/
 *
 *---------------------------------------------------------------------------
 * TrulyHandsfree SDK
 *---------------------------------------------------------------------------
 */
#ifndef TRULYHANDSFREE_H
#define TRULYHANDSFREE_H
#if defined(__cplusplus)
extern "C" {
#endif

#include <sensorytypes.h>
#include <time.h>

#define THF_NAME "TrulyHandsfree"
#define THF_VERSION "4.4.23"
#define THF_VERSION_MAJOR 4
#define THF_VERSION_MINOR 4
#define THF_VERSION_PATCH 23
#define THF_VERSION_PRE ""
#define THF_VERSION_ID 123

#define _QUOTE(s) #s
#define QUOTE(s) _QUOTE(s)

/* thfRecogPipe() status definitions */
enum {
  RECOG_QUIET,
  RECOG_SILENCE,
  RECOG_SOUND,
  RECOG_DONE,
  RECOG_MAXREC,
  RECOG_IGNORE,
  RECOG_NODATA
};

/* thfRecogPipe() mode definitions */
enum { SDET_ONLY = 0x01, RECOG_ONLY, SDET_RECOG };

/* thfRecogCreateFromFile() mode definitions */
enum { NO_SDET, SDET };

/* thfSearchCreate... definitions */
enum { NO_PHRASESPOTTING, PHRASESPOTTING };

/* thfRecogInit() keep definitions */
enum {
  RECOG_KEEP_NONE,
  RECOG_KEEP_WAVE,
  RECOG_KEEP_WAVE_WORD,
  RECOG_KEEP_WAVE_WORD_PHONEME,
  RECOG_KEEP_WORD_PHONEME
};

/* thfTuneCreate() mode definitions */
enum { NO_SILENCE, REQUIRE_SILENCE };

/* thfRecogConfigSet(), thfRecogConfigGet() definitions */
typedef enum {
  REC_LSILENCE,
  REC_TSILENCE,
  REC_MAXREC,
  REC_MINDUR,
  REC_ESILENCET,
  REC_ESPEECHT,
  REC_SHORTTERMMS,
  REC_LONGTERMMS,
  REC_BACKOFF,
  REC_THI,
  REC_SDET_EDIFF,
  REC_SDET_INITIAL_DELAY,
  REC_SDET_STEM,
  REC_SDET_LTEM,
  REC_SDET_MODE,
  REC_KEEP_SDET_HISTORY,
  REC_KEEP_FEATURE_HISTORY,
  REC_KEEP_FEATURES,
  REC_EPQ_ENABLE,
  REC_EPQ_MIN,
  REC_EPQ_SCORE,
  REC_EPQ_NOISE,
  REC_SV_ADJUST_METHOD,
  REC_SV_ADJUST1,
  REC_SV_ADJUST2,
  REC_SV_ADJUST3,
  REC_SV_ADJUST4,
  REC_PARAMA_ADJUST_METHOD,
  REC_PARAMA_ADJUST1,
  REC_PARAMA_ADJUST2,
  REC_PARAMA_ADJUST3,
  REC_PARAMA_ADJUST4,
  REC_PHONEMEREC_SEARCH_TYPE,
  REC_USE_FEAT,
  REC_CHECK_ENERGY_MIN,
  REC_CHECK_ENERGY_STD_DEV,
  REC_CHECK_SIL_BEG_MSEC,
  REC_CHECK_SIL_END_MSEC,
  REC_CHECK_SNR,
  REC_CHECK_RECORDING_VARIANCE,
  REC_CHECK_CLIPPING_PERCENT,
  REC_CHECK_CLIPPING_MSEC,
  REC_CHECK_POOR_RECORDINGS,
  REC_CHECK_SPOT,
  REC_CHECK_VOWEL_DUR,
  REC_EARLYSTOP_NBEST,
  REC_FEATURE_AGC
} thfRecogConfig_t;

typedef enum {
  REC_SDET_MODE_TRIGGER_COMMAND,
  REC_SDET_MODE_COMMAND,
  REC_SDET_MODE_ENROLL
} thfRecogSdetModeConfig_t;

#define REC_EPQ_MINSNR REC_EPQ_MIN

/* thfRecogConfigGet() EPQ defaults */
#define EPQ_DEFAULT_MIN -5.0

/* thfSearchConfigSet(), thfSearchConfigGet() definitions */
enum thfSearchConfigEnum {
  SCH_PRUNING,
  SCH_BEAM,
  SCH_GARBAGE,
  SCH_ANY,
  SCH_NOTA,
  SCH_LONGPEN
};

/* thfPhrasespotConfigSet(), thfPhrasespotConfigGet() definitions */
typedef enum {
  PS_PARAMA_OFFSET,
  PS_BEAM,
  PS_ABSBEAM,
  PS_DELAY,
  PS_LONGPEN,
  PS_SEQ_BUFFER,
  PS_SEQ_SPOT_OPTIONAL,
  PS_SEQ_SPOT_INCLUDED,
  PS_SEQ_SAMPLES
} thfPhrasespotConfig_t;

/* thfPhrasespotConfigSet() special DELAY flags */
#define PHRASESPOT_DELAY_ASAP 0xFFFE /* as soon as probable */

/* thfPhrasespotCreateFromList definitions */
#define PHRASESPOT_LISTEN_SHORT 10 /* Listening window in seconds */
#define PHRASESPOT_LISTEN_MEDIUM 30
#define PHRASESPOT_LISTEN_LONG 60
#define PHRASESPOT_LISTEN_CONTINUOUS 3600

/* thfSpeaker definitions */
#define SPEAKER_IGNORE_PAUSE "IGNORE_PAUSE"
#define SPEAKER_IGNORE_SV_DEFAULT "IGNORE_SV_DEFAULT"
#define SPEAKER_IGNORE_RTA_DEFAULT "IGNORE_RTA_DEFAULT"

#define SPEAKER_SET_WANT_NUM_RECORDINGS (1 << 1)

typedef enum {
  SPEAKER_WANT_NUM_RECORDINGS,
  SPEAKER_HAVE_NUM_RECORDINGS,
  SPEAKER_DONE_ADAPTATION,
  SPEAKER_RECOG_WITH_ADAPTED,
  SPEAKER_SPEED_ACCURACY,
  SPEAKER_SAMP_PER_CAT,
  SPEAKER_TRIGGER_SAMP_PER_CAT,
  SPEAKER_SAMP_PER_CAT_WITHIN,
  SPEAKER_TARGET_SNR,
  SPEAKER_DUR_MIN_FACTOR,
  SPEAKER_DUR_MAX_FACTOR,
  SPEAKER_USE_FEAT,
  SPEAKER_TRAIN_ITERATIONS,
  SPEAKER_TRAIN_ITERATIONS_WITHIN,
  SPEAKER_LEARN_RATE,
  SPEAKER_LEARN_RATE_WITHIN,
  SPEAKER_DROPOUT_WITHIN,
  SPEAKER_ADAPT_TYPE,
  SPEAKER_DSP_SV_THRESH
} thfSpeakerConfig_t;

/* Changed SPEAKER_DEFAULT_TRIGGER_SAMP_PER_CAT from 60 to 256 on 11-Dec-2013;
 * increase is primarily due to the introduction of SPEAKER_TARGET_SNR
 */
#define SPEAKER_DEFAULT_TRIGGER_SAMP_PER_CAT (256)
#define SPEAKER_DEFAULT_SAMP_PER_CAT_WITHIN (0)
#define SPEAKER_DEFAULT_TARGET_SNR (20.0f)
#define SPEAKER_DEFAULT_DUR_MIN_FACTOR (0.5f)
#define SPEAKER_DEFAULT_DUR_MAX_FACTOR (1.25f)
#define SPEAKER_DEFAULT_USE_FEAT (0)
#define SPEAKER_DEFAULT_TRAIN_ITERATIONS (1)
#define SPEAKER_DEFAULT_TRAIN_ITERATIONS_WITHIN (1)
#define SPEAKER_DEFAULT_LEARN_RATE (0.1640625f)
#define SPEAKER_DEFAULT_LEARN_RATE_WITHIN (0.1640625f)
#define SPEAKER_DEFAULT_DROPOUT_WITHIN (0.0f)
#define SPEAKER_DEFAULT_ADAPT_TYPE (0)

#define SPEAKER_SAVE_MODEL (1 << 1)
#define SPEAKER_SAVE_DATA (1 << 2)
#define SPEAKER_SAVE_UDT (1 << 3)
#define SPEAKER_SAVE_BOTH (SPEAKER_SAVE_MODEL | SPEAKER_SAVE_DATA)
#define SPEAKER_SAVE_UDTDATA (SPEAKER_SAVE_DATA | SPEAKER_SAVE_UDT)
#define SPEAKER_SAVE_UDTMODEL (SPEAKER_SAVE_MODEL | SPEAKER_SAVE_UDT)
#define SPEAKER_SAVE_UDTBOTH \
  (SPEAKER_SAVE_MODEL | SPEAKER_SAVE_DATA | SPEAKER_SAVE_UDT)

#define SPEAKER_READ_MODEL (1 << 1)
#define SPEAKER_READ_DATA (1 << 2)
#define SPEAKER_READ_UDT (1 << 3)
#define SPEAKER_READ_BOTH (SPEAKER_READ_MODEL | SPEAKER_READ_DATA)
#define SPEAKER_READ_UDTDATA (SPEAKER_READ_DATA | SPEAKER_READ_UDT)
#define SPEAKER_READ_UDTMODEL (SPEAKER_READ_MODEL | SPEAKER_READ_UDT)
#define SPEAKER_READ_UDTBOTH \
  (SPEAKER_READ_MODEL | SPEAKER_READ_DATA | SPEAKER_READ_UDT)

/* thfUdtConfigSet(), thfUdtConfigGet() definitions */
typedef enum {
  UDT_SAMP_PER_CAT,
  UDT_PARAM_A_START,
  UDT_PARAM_A_NUM,
  UDT_PARAM_A_STEP,
  UDT_PARAM_B_START,
  UDT_PARAM_B_NUM,
  UDT_PARAM_B_STEP,
  UDT_MAX_PARAM_D,
  UDT_NUM_CHUNKS,
  UDT_MAX_MEMORY,
  UDT_ABSBEAM,
  UDT_REQUIRE_SILENCE,
  UDT_SIMILARITY_THRESH,
  UDT_PARAM_E,
  UDT_PARAM_F,
  UDT_PARAM_G,
  UDT_SAMP_PER_CAT_WITHIN,
  UDT_IGNORE_TYPE,
  UDT_SIL_PERCENT,
  UDT_NOISE_PERCENT,
  UDT_PHONEMEREC_SEARCH_TYPE,
  UDT_SANITIZE,
  UDT_DUR_MIN_FACTOR,
  UDT_DUR_MAX_FACTOR,
  UDT_POP_THRESH,
  UDT_DBA_FILTER,
  UDT_TARGET_SNR,
  UDT_USE_FEAT,

  UDT_CHECK_ENERGY_MIN,
  UDT_CHECK_ENERGY_STD_DEV,
  UDT_CHECK_SIL_BEG_MSEC,
  UDT_CHECK_SIL_END_MSEC,
  UDT_CHECK_SNR,
  UDT_CHECK_RECORDING_VARIANCE,
  UDT_CHECK_CLIPPING_PERCENT,
  UDT_CHECK_CLIPPING_MSEC,
  UDT_CHECK_POOR_RECORDINGS,
  UDT_CHECK_VOWEL_DUR,
  UDT_CHECK_REPETITION,

  UDT_TRAIN_ITERATIONS,
  UDT_TRAIN_ITERATIONS_WITHIN,
  UDT_LEARN_RATE,
  UDT_LEARN_RATE_WITHIN,
  UDT_DROPOUT_WITHIN,
  UDT_ADAPT_TYPE,

  UDT_DEBUG_LEVEL
} thfUdtConfig_t;

#define UDT_DEFAULT_SAMP_PER_CAT (512)
#define UDT_DEFAULT_PARAM_A_START (-1100)
#define UDT_DEFAULT_PARAM_A_NUM (1)
#define UDT_DEFAULT_PARAM_A_STEP (20)
#define UDT_DEFAULT_PARAM_B_START (850)
#define UDT_DEFAULT_PARAM_B_NUM (1)
#define UDT_DEFAULT_PARAM_B_STEP (100)
#define UDT_DEFAULT_MAX_PARAM_D (200)
#define UDT_DEFAULT_NUM_CHUNKS (20)
#define UDT_DEFAULT_MAX_MEMORY (20000000)
#define UDT_DEFAULT_ABSBEAM (1000.0f)
#define UDT_DEFAULT_REQUIRE_SILENCE (0)
#define UDT_DEFAULT_SIMILARITY_THRESH (350.0f)
#define UDT_DEFAULT_PARAM_E (800.0f)
#define UDT_DEFAULT_PARAM_F (500.0f)
#define UDT_DEFAULT_PARAM_G (0.05f)
#define UDT_DEFAULT_SAMP_PER_CAT_WITHIN (16)
#define UDT_DEFAULT_IGNORE_TYPE (1)
#define UDT_DEFAULT_SIL_PERCENT (10)
#define UDT_DEFAULT_NOISE_PERCENT (40)
#define UDT_DEFAULT_PHONEMEREC_SEARCH_TYPE (1)
#define UDT_DEFAULT_SANITIZE (5)
/* To turn off duration modification, set min > max */
#define UDT_DEFAULT_DUR_MIN_FACTOR (0.50f)
#define UDT_DEFAULT_DUR_MAX_FACTOR (1.25f)
#define UDT_DEFAULT_POP_THRESH (10.0f)
#define UDT_DEFAULT_DBA_FILTER (1)
#define UDT_DEFAULT_TARGET_SNR (20.0f)
#define UDT_DEFAULT_TRAIN_ITERATIONS (1)
#define UDT_DEFAULT_TRAIN_ITERATIONS_WITHIN (1)
#define UDT_DEFAULT_LEARN_RATE (0.1640625f)
#define UDT_DEFAULT_LEARN_RATE_WITHIN (0.1640625f)
#define UDT_DEFAULT_DROPOUT_WITHIN (0.0f)
#define UDT_DEFAULT_ADAPT_TYPE (0)

/* Bit fields that indicate a particular problem in a recording */
#define CHECKRECORDING_BITFIELD_ENERGY_MIN (1 << 0)
#define CHECKRECORDING_BITFIELD_ENERGY_STD_DEV (1 << 1)
#define CHECKRECORDING_BITFIELD_SIL_BEG_MSEC (1 << 2)
#define CHECKRECORDING_BITFIELD_SIL_END_MSEC (1 << 3)
#define CHECKRECORDING_BITFIELD_SNR (1 << 4)
#define CHECKRECORDING_BITFIELD_RECORDING_VARIANCE (1 << 5)
#define CHECKRECORDING_BITFIELD_CLIPPING_PERCENT (1 << 6)
#define CHECKRECORDING_BITFIELD_CLIPPING_MSEC (1 << 7)
#define CHECKRECORDING_BITFIELD_POOR_RECORDINGS (1 << 8)
#define CHECKRECORDING_BITFIELD_SPOT (1 << 9)
#define CHECKRECORDING_BITFIELD_VOWEL_DUR (1 << 10)
#define CHECKRECORDING_BITFIELD_REPETITION (1 << 11)

/* thfTtsConfigSet(), thfTtsConfigGet() definitions */
typedef enum {
  PROSODY_DOMAIN,
  PROSODY_PITCHRANGE,
  PROSODY_PHRASEBREAK_RESTART,
  TTS_SAMPLERATE,
  TTS_VOCODER
} thfTtsConfig_t;

/* PROSODY_DOMAIN  values*/
enum { GENERIC, NAMES };

/* PROSODY_PITCHRANGE values */
enum { FULL, HALF };

/* PROSODY_PHRASEBREAK_RESTART values */
#if !defined(NO) && !defined(YES)
enum { NO, YES };
#endif

/* TTS_SAMPLERATE values */
enum { SR_16K, SR_8K };

/* TTS_VOCODER values */
enum { VOC_SENSORY, VOC_MELP };

#define VOCODER_DEFAULT (0)
#define VOCODER_SENSORY (1)
#define VOCODER_MARY (1 << 1)

/* Internal data structures hidden from the user */
#ifndef COMPILINGFLUENTSDK
typedef struct thf_s thf_t;
typedef struct searchs_t_ searchs_t;
typedef struct recog_t_ recog_t;
typedef struct pronuns_t_ pronuns_t;
typedef struct tts_t_ tts_t;
typedef struct udt_t_ udt_t;
typedef enum thfSearchConfigEnum thfSearchConfig_t;
typedef struct phsearch_t_ phsearch_t;
typedef struct adaptObj_t_ adaptObj_t;
#endif

/* static wave structure [output] */
typedef struct swave_s {
  unsigned short len; /* wave buffer length */
  short *buf;         /* wave buffer */
} swave_t;

/* static data structure [input] */
typedef struct sdata_s {
  unsigned short I; /* Number of data objects */
  const void **d;   /* Array of data objects */
} sdata_t;

/* UDT callback data and procedure definitions */
typedef struct UDT_CBdata_s UDT_CBdata_t;
typedef short int(UDT_CBfunction_t)(struct UDT_CBdata_s *cd);
struct UDT_CBdata_s {
  UDT_CBfunction_t *cbFunction;
  void *cbData;
  unsigned long where;
  unsigned long whereMax;
  recog_t *tempRecog;
  searchs_t *tempSearch;
};

/* structure for one enrollment recording for UDT or speaker verification */
typedef struct thfEnrollment_s {
  short *audio;
  unsigned long audioLen;
  unsigned long audioSignature;
  char *filename;
  char *reserved1; /* set to NULL */
  char *pronun;    /* set to NULL or optional pipelined UDT pronunciation */
} thfEnrollment_t;
typedef thfEnrollment_t udtEnrollment_t;

/* structure for one user for UDT or speaker verification, containing a name and
 * one or more enrollments */
typedef struct thfUser_s {
  char *userName;
  char *tag;
  unsigned short numEnroll;
  thfEnrollment_t *enroll;
  char *reserved1; /* set to NULL */
  char *reserved2; /* set to NULL */
} thfUser_t;

/*-- Adapt structures ---*/

#define ADAPT_UNSPECIFIED_ID -32768

/* structure for adapt task information */

typedef struct thfAdaptTaskInfo_s {
  char *taskID;
  char *taskLanguage;
  char *taskVersion;
  int userDefinedTrigger;
  int featureType;
  int sampleRate;
  int frameSize;
  int saveEnrollmentAudio;
  int numEnrollments;
  int numEnrollmentsWithContext;
  int delay1;
  int delayM;
  float epqMin;
  float svThresh;
  int paramAoffset;
  int doSpeechDetect;
  int contextPostprocess;
  float checkPhraseQuality;
  float checkEnergyMin;
  float checkEnergyStdDev;
  float checkSilBegMsec;
  float checkSilEndMsec;
  float checkSNR;
  float checkRecordingVariance;
  float checkPoorRecordingsLimit;
  float checkClippingFrames;
  float checkVowelDur;
  float checkRepetition;
  float checkSilInPhrase;
  float checkSpot;
} thfAdaptTaskInfo_t;

/* structures for adapt user and enrollment information */

typedef struct thfAdaptEnrollInfo_s {
  long ID;
  int used;
  long score;
  time_t time;
  long phraseBeginTime;
  long phraseEndTime;
  long audioLen;
  short *audio;
} thfAdaptEnrollInfo_t;

typedef struct thfAdaptUser_s {
  char *name;
  int numEnroll;
  thfAdaptEnrollInfo_t *enrollInfo;
} thfAdaptUser_t;

typedef struct thfAdaptUserInfo_s {
  int numUsers;
  thfAdaptUser_t *user;
} thfAdaptUserInfo_t;

/* structure for one enrollment recording with 'adapt' functions */

typedef struct thfTriggerEnrollment_s {
  long sampFreq;
  short *audio;
  unsigned long audioLen;
  long score;
  time_t time;
  long enrollmentID;
  char *reserved1;     /* set to NULL */
  char *reserved2;     /* set to NULL */
  char *transcription; /* set to NULL */
  char *context;       /* set to NULL unless have enrollment in context */
} thfTriggerEnrollment_t;

/* constants and structures for checking 'adapt' enrollments */

typedef enum {
  CHECK_TRIGGER_ENERGY_MIN,
  CHECK_TRIGGER_ENERGY_STD_DEV,
  CHECK_TRIGGER_SIL_BEG_MSEC,
  CHECK_TRIGGER_SIL_END_MSEC,
  CHECK_TRIGGER_SNR,
  CHECK_TRIGGER_RECORDING_VARIANCE,
  CHECK_TRIGGER_POOR_RECORDINGS_LIMIT,
  CHECK_TRIGGER_CLIPPING_FRAMES,
  CHECK_TRIGGER_VOWEL_DUR,
  CHECK_TRIGGER_REPETITION,
  CHECK_TRIGGER_SIL_IN_PHRASE,
  CHECK_TRIGGER_SPOT,
} thfTriggerEnrollmentCheckTypes_t;

typedef struct thfEnrollmentCheckOne_s {
  long enrollmentID;
  int numChecks;
  int *pass;
  float *score;
  float *thresh;
} thfEnrollmentCheckOne_t;

typedef struct thfEnrollmentCheck_s {
  int allPass;
  float quality;
  float qualityThresh;
  int numEnroll;
  thfEnrollmentCheckOne_t *enroll;
} thfEnrollmentCheck_t;

/* constants and structure for models created by adaptation */

typedef enum {
  TRIGGER_ENROLL_DELAY,
  TRIGGER_ENROLL_EPQ_MIN,
  TRIGGER_ENROLL_SV_THRESH,
} thfTriggerEnrollParams_t;

typedef struct thfModelTrigger_s {
  char *taskName;
  int numUsers;
  float *spotParams;
  int numSpotParams;
  recog_t *recog;
  searchs_t *search;
} thfModelTrigger_t;

/*-----*/

/* read-from-stream function type */
typedef enum {
  THF_STREAM_OK,
  THF_STREAM_EOF,
  THF_STREAM_ERROR
} thfReadFuncError_t;

typedef size_t (*thfReadFunc_t)(void *buffer, size_t size, size_t nitems,
                                void *privateData, thfReadFuncError_t *err);

#if defined(_WIN32) && defined(EXPRT) && defined(FLUENTSDK)
#define THF_PROTOTYPE(a, b, c) __declspec(dllexport) a b c
#elif defined(__SYMBIAN32__)
#define THF_PROTOTYPE(a, b, c) EXPORT_C a b c
#elif defined(_WIN32) || defined(_WIN32_WCE)
#define THF_PROTOTYPE(a, b, c) a __cdecl b c
#elif defined(__SH_3)
#define THF_PROTOTYPE(a, b, c) extern a b c
#else
#define THF_PROTOTYPE(a, b, c) a b c
#endif

/* Functions marked with THF_EXPERIMENTAL are in a state of flux.
 * Do not use these unless explicilty recommended by Sensory tech support.
 */
#define THF_EXPERIMENTAL(a, b, c) THF_PROTOTYPE(a, b, c)

/* function prototypes */
/* Core funcs */
THF_PROTOTYPE(thf_t *, thfSessionCreate, (void));
THF_PROTOTYPE(void, thfSessionDestroy, (thf_t * thf));
THF_PROTOTYPE(void, thfFree, (void *r));
THF_PROTOTYPE(void *, thfMalloc, (size_t size));
THF_PROTOTYPE(const char *, thfGetLastError, (thf_t * thf));
THF_PROTOTYPE(void, thfResetLastError, (thf_t * thf));
THF_PROTOTYPE(const char *, thfVersion, (void));
THF_PROTOTYPE(time_t, thfGetLicenseExpiration, (void));
THF_PROTOTYPE(const char *, thfGetLicenseExpirationString, (void));
THF_PROTOTYPE(const char *, thfGetLicenseFeatures, (void));
THF_EXPERIMENTAL(const char *, thfGetAccelerated, (void));

THF_EXPERIMENTAL(void, thfLibraryInit, (u32 * (*handler)(u32 * in)));

#ifdef _WIN32
/* Miscellaneous compatibility functions, for convenience only.
 * These functions are not documented in the SDK API and are not
 * supported in any way.
 */
THF_PROTOTYPE(int, getopt, (int argc, char *const argv[], const char *optstr));
#endif

/* Text normalization */
THF_PROTOTYPE(const char *, thfTextNormalize,
              (thf_t * thf, pronuns_t *p, const char *word,
               unsigned short conditionCode));

/* Pronunciation funcs */
THF_PROTOTYPE(const char *, thfPronunCompute,
              (thf_t * thf, pronuns_t *p, const char *word,
               unsigned short nbest, const char *suffix, const char *pos));
THF_PROTOTYPE(pronuns_t *, thfPronunCreateFromFile,
              (thf_t * thf, const char *pronunfile,
               unsigned short dialectIndex));
THF_PROTOTYPE(pronuns_t *, thfPronunCreateFromStatic,
              (thf_t * thf, sdata_t *global, unsigned short idx,
               unsigned short dialectIndex));
THF_PROTOTYPE(void, thfPronunDestroy, (pronuns_t * p));

/* Search funcs */
THF_PROTOTYPE(searchs_t *, thfSearchCreateFromStatic,
              (thf_t * thf, recog_t *r, sdata_t *global, unsigned short idx,
               unsigned short nbest));
THF_PROTOTYPE(void, thfSearchDestroy, (searchs_t * s));
THF_PROTOTYPE(int, thfSearchConfigGet,
              (thf_t * thf, recog_t *r, searchs_t *s, thfSearchConfig_t key,
               float *value));
THF_PROTOTYPE(int, thfSearchConfigSet,
              (thf_t * thf, recog_t *r, searchs_t *s, thfSearchConfig_t key,
               float value));
THF_PROTOTYPE(int, thfPhrasespotConfigGet,
              (thf_t * thf, recog_t *r, searchs_t *s, thfPhrasespotConfig_t key,
               float *value));
THF_PROTOTYPE(int, thfPhrasespotConfigSet,
              (thf_t * thf, recog_t *r, searchs_t *s, thfPhrasespotConfig_t key,
               float value));
THF_PROTOTYPE(searchs_t *, thfPhrasespotCreateFromList,
              (thf_t * thf, recog_t *r, pronuns_t *p, const char **phrases,
               unsigned short phraseCount, const char **word,
               const char **pronun, unsigned short wordCount,
               unsigned short listenWindow));

THF_PROTOTYPE(searchs_t *, thfSearchCreateFromFile,
              (thf_t * thf, recog_t *r, const char *searchfile,
               unsigned short nbest));
THF_PROTOTYPE(searchs_t *, thfSearchCreateFromFunc,
              (thf_t * thf, recog_t *r, thfReadFunc_t readFunc,
               void *privateData, unsigned short nbest));
THF_PROTOTYPE(searchs_t *, thfSearchCreateFromList,
              (thf_t * thf, recog_t *r, pronuns_t *p, const char **word,
               const char **pronun, unsigned short count,
               unsigned short nbest));
THF_PROTOTYPE(searchs_t *, thfSearchCreateIncremental,
              (thf_t * thf, recog_t *r, searchs_t *s, pronuns_t *p,
               const char **removeGrams, unsigned short numRemove,
               const char **addGrams, unsigned short numAdd, const char **word,
               const char **pronun, unsigned short numWords,
               unsigned short nbest));
THF_PROTOTYPE(searchs_t *, thfSearchCreateFromGrammar,
              (thf_t * thf, recog_t *r, pronuns_t *p, const char *grammar,
               const char **word, const char **pronun, unsigned short count,
               unsigned short nbest, char usePhrasespotting));
THF_PROTOTYPE(int, thfSearchPhrases,
              (thf_t * thf, searchs_t *s, char ***map, int *count));
THF_PROTOTYPE(int, thfNnsvIdxToWordID,
              (thf_t * thf, searchs_t *s, recog_t *r, int nnsvIdx));
THF_PROTOTYPE(int, thfSearchSaveToFile,
              (thf_t * thf, searchs_t *s, const char *filename));
THF_PROTOTYPE(int, thfSaveEmbedded,
              (thf_t * thf, recog_t *r, searchs_t *s, const char *target,
               char *rfilename, char *sfilename, int reserved, int trigger));
THF_PROTOTYPE(int, thfEmbeddedTargetsGet,
              (thf_t * thf, char ***targets, int *count));

/* Recognizer funcs */
THF_PROTOTYPE(recog_t *, thfRecogCreateStub,
              (thf_t * thf, unsigned long samplerate));
THF_PROTOTYPE(recog_t *, thfRecogCreateFromStatic,
              (thf_t * thf, sdata_t *global, unsigned short idx,
               unsigned long maxbuf, unsigned short backoff, char useSdet));
THF_PROTOTYPE(void, thfRecogDestroy, (recog_t * r));
THF_PROTOTYPE(int, thfRecogInit,
              (thf_t * thf, recog_t *r, searchs_t *s, unsigned char keep));
THF_PROTOTYPE(int, thfRecogReset, (thf_t * thf, recog_t *r));
THF_PROTOTYPE(int, thfRecogPipe,
              (thf_t * thf, recog_t *r, unsigned long ilen, short *ibuf,
               char mode, unsigned short *stateResult));
THF_PROTOTYPE(int, thfRecogPipeFlush,
              (thf_t * thf, recog_t *r, char mode,
               unsigned short *stateResult));
THF_PROTOTYPE(float, thfRecogSVscore, (thf_t * thf, recog_t *r));
THF_PROTOTYPE(int, thfRecogResult,
              (thf_t * thf, recog_t *r, float *score, const char **res,
               const char **wordAlign, const char **phoneAlign,
               const short **inSpeech, unsigned long *inSpeechSamples,
               const short **sdetSpeech, unsigned long *sdetSpeechSamples));
THF_PROTOTYPE(int, thfRecogSampleConvert,
              (thf_t * thf, recog_t *r, unsigned long inrate, short *inbuf,
               unsigned long inlen, short **outbuf, unsigned long *outlen));
THF_PROTOTYPE(int, thfRecogSearchSaveToFile,
              (thf_t * thf, searchs_t *search, recog_t *r, char *searchFilename,
               char *recogFilename));
THF_PROTOTYPE(int, thfRecogGetSpeechRange,
              (thf_t * thf, recog_t *r, float *from, float *to));
THF_PROTOTYPE(int, thfRecogGetClipping,
              (thf_t * thf, recog_t *r, float *clipped));
THF_PROTOTYPE(unsigned long, thfRecogGetSampleRate, (thf_t * thf, recog_t *r));
THF_PROTOTYPE(int, thfRecogCombine,
              (thf_t * thf, int prune, recog_t *r1, recog_t *r2, searchs_t *s1,
               searchs_t *s2, const char *prefix1, const char *prefix2,
               recog_t **outputRecog, searchs_t **outputSearch,
               float *compressionRatio));
THF_PROTOTYPE(int, thfRecogConfigGet,
              (thf_t * thf, recog_t *r, thfRecogConfig_t key, float *value));
THF_PROTOTYPE(int, thfRecogConfigSet,
              (thf_t * thf, recog_t *r, thfRecogConfig_t key, float value));
THF_PROTOTYPE(recog_t *, thfRecogCreateFromFile,
              (thf_t * thf, const char *netfile, unsigned long maxbuf,
               unsigned short backoff, char useSdet));
THF_PROTOTYPE(recog_t *, thfRecogCreateFromFunc,
              (thf_t * thf, thfReadFunc_t readFunc, void *privateData,
               unsigned long maxbuf, unsigned short backoff, char useSdet));
THF_PROTOTYPE(int, thfRecogCreateHybrid,
              (thf_t * thf, recog_t **r, pronuns_t **p,
               unsigned short numObjects, const char ***word,
               const char ***pronun, unsigned short *numWords,
               recog_t **hybridRecog, pronuns_t **hybridPronun));
THF_PROTOTYPE(int, thfRecogSdetForceDone, (thf_t * thf, recog_t *r));
THF_PROTOTYPE(int, thfRecogPrep,
              (thf_t * thf, recog_t *r, unsigned long ilen, short *ibuf));
THF_PROTOTYPE(int, thfRecogPrepSeq, (thf_t * thf, recog_t *dst, recog_t *src));
THF_PROTOTYPE(int, thfRecogDowngradeToCnn, (thf_t * thf, recog_t *r));

/* Speaker Verification functions */
THF_PROTOTYPE(int, thfSpeakerCreateFromFile,
              (thf_t * thf, recog_t *r, unsigned short numEnroll,
               char *filename));
THF_PROTOTYPE(int, thfSpeakerInit,
              (thf_t * thf, recog_t *r, unsigned short sID,
               unsigned short wantNumTrainFiles));
THF_PROTOTYPE(int, thfSpeakerAdapt,
              (thf_t * thf, recog_t *r, unsigned short sID,
               unsigned short trainMethod, char *ignorePhnSpec,
               unsigned short trainSampWant));
THF_PROTOTYPE(int, thfSpeakerCheckEnrollments,
              (thf_t * thf, recog_t *recog, thfUser_t *user,
               unsigned short whichUser, unsigned long *globalFeedback,
               unsigned long *feedbackArray, float *phraseQuality));
THF_PROTOTYPE(int, thfSpeakerCheckValueGet,
              (thf_t * thf, recog_t *r, thfRecogConfig_t param, float *val,
               unsigned short numEnroll));

THF_PROTOTYPE(int, thfSpeakerEnroll,
              (thf_t * thf, recog_t *r, char *phrase, char *ignorePhnSpec,
               unsigned short numUsers, thfUser_t *user,
               searchs_t **outputSearch, recog_t **outputRecog));

THF_PROTOTYPE(int, thfSpeakerSaveRecogToFile,
              (thf_t * thf, searchs_t *search, recog_t *recog, char *searchFile,
               char *recogFile));
THF_PROTOTYPE(int, thfSpeakerVerify,
              (thf_t * thf, recog_t *r, unsigned short sID, float *spkrRawScore,
               short *recordingOK));
THF_PROTOTYPE(int, thfSpeakerGender,
              (thf_t * thf, recog_t *r, float *genderProb));
THF_PROTOTYPE(int, thfSpeakerReadGenderModel,
              (thf_t * thf, recog_t *r, char *filename));
THF_PROTOTYPE(int, thfSpeakerReadAntiSpeakerData,
              (thf_t * thf, recog_t *r, char *filename));
THF_PROTOTYPE(int, thfSpeakerAddAntiData,
              (thf_t * thf, recog_t *r, unsigned short sID, short wantExamples,
               unsigned short useMonocat));
THF_PROTOTYPE(int, thfSpeakerStoreLastRecording,
              (thf_t * thf, recog_t *r, unsigned short sID, short score,
               short force));
THF_PROTOTYPE(int, thfSpeakerAdd,
              (thf_t * thf, recog_t *r, unsigned short sID));
THF_PROTOTYPE(int, thfSpeakerList,
              (thf_t * thf, recog_t *r, unsigned short sID,
               unsigned short **sIDlist, unsigned short *listLen));
THF_PROTOTYPE(int, thfSpeakerRemove,
              (thf_t * thf, recog_t *r, unsigned short sID));
THF_PROTOTYPE(int, thfSpeakerReset,
              (thf_t * thf, recog_t *r, unsigned short sID));
THF_PROTOTYPE(int, thfSpeakerConfigSet,
              (thf_t * thf, recog_t *r, unsigned short sID,
               thfSpeakerConfig_t key, float value));
THF_PROTOTYPE(int, thfSpeakerConfigGet,
              (thf_t * thf, recog_t *r, unsigned short sID,
               thfSpeakerConfig_t key, float *value));
THF_PROTOTYPE(int, thfSpeakerSaveToFile,
              (thf_t * thf, recog_t *r, unsigned short sID, char *filename,
               unsigned short set));
THF_PROTOTYPE(int, thfSpeakerReadFromFile,
              (thf_t * thf, recog_t *r, unsigned short SID, char *filename,
               unsigned short flag, unsigned short force));
THF_PROTOTYPE(void, thfFreeFeats, (short **feats, unsigned long numFrames));
THF_PROTOTYPE(int, thfSpeakerCombine,
              (thf_t * thf, recog_t *r, searchs_t *s1, searchs_t *s2,
               unsigned short sid, searchs_t **outSearch, recog_t **outRecog));
THF_PROTOTYPE(searchs_t *, thfSpeakerCombineAndSaveToFile,
              (thf_t * thf, recog_t *r, searchs_t *s1, searchs_t *s2,
               unsigned short sID, char *searchFile, char *recogFile));
THF_PROTOTYPE(int, thfCheckRecording,
              (thf_t * thf, recog_t *r, short *wave, unsigned long waveLen,
               unsigned long *feedback));
THF_PROTOTYPE(int, thfSpeakerStoreFeatsFromWaveform,
              (thf_t * thf, recog_t *r, searchs_t *search, unsigned short sID,
               short searchType, short *extras, short *wav,
               unsigned long wavLen, short force));

/* Tuning and UDT functions */
THF_PROTOTYPE(udt_t *, thfUdtCreate,
              (thf_t * thf, char *recogFile, char *phnSearchFile,
               char *udtDataFile, unsigned short maxNumEnroll,
               unsigned long samplerate));
THF_PROTOTYPE(int, thfUdtConfigSet,
              (thf_t * thf, udt_t *udt, thfUdtConfig_t key, float value));
THF_PROTOTYPE(int, thfUdtConfigGet,
              (thf_t * thf, udt_t *udt, thfUdtConfig_t key, float *value));
THF_PROTOTYPE(int, thfUdtCheckEnrollments,
              (thf_t * thf, udt_t *udt, thfUser_t *user,
               unsigned short whichUser, unsigned long *globalFeedback,
               unsigned long *feedbackArray, float *phraseQuality));
THF_PROTOTYPE(int, thfUdtCheckValueGet,
              (thf_t * thf, udt_t *udt, thfUdtConfig_t param, float *val,
               unsigned short numEnroll));
THF_PROTOTYPE(int, thfUdtEnroll,
              (thf_t * thf, udt_t *udt, unsigned short numUsers,
               thfUser_t *user, searchs_t **outputSearch, recog_t **outputRecog,
               void *cbData, UDT_CBfunction_t *cbProc));
THF_PROTOTYPE(int, thfUdtSaveToFile,
              (thf_t * thf, searchs_t *search, recog_t *r, char *searchFilename,
               char *recogFilename));
THF_PROTOTYPE(int, thfUdtCacheSaveToFile,
              (thf_t * thf, udt_t *udt, const char *cacheFilename));
THF_PROTOTYPE(int, thfUdtCacheLoadFromFile,
              (thf_t * thf, thfUser_t *user, unsigned short numUsers,
               udt_t *udt, const char *cacheFilename));
THF_PROTOTYPE(void, thfUdtDestroy, (udt_t * udt));

/* adaptation (UDT and EFT trigger) functions */
THF_PROTOTYPE(adaptObj_t *, thfAdaptCreate, (thf_t * thf));
THF_PROTOTYPE(int, thfAdaptDestroy, (thf_t * thf, adaptObj_t *aObj));
THF_PROTOTYPE(int, thfAdaptTaskCreate,
              (thf_t * thf, adaptObj_t *aObj, char *taskTypeName,
               char *taskDataFileName, char *taskName));
THF_PROTOTYPE(int, thfAdaptTaskGet,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *paramName,
               float *value));
THF_PROTOTYPE(int, thfAdaptTaskSet,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *paramName,
               float value));
THF_PROTOTYPE(int, thfAdaptTaskInfo,
              (thf_t * thf, adaptObj_t *aObj, char *taskName,
               thfAdaptTaskInfo_t **taskInfoOut));
THF_PROTOTYPE(int, thfAdaptTaskInfoDestroy,
              (thf_t * thf, adaptObj_t *aObj, char *taskName,
               thfAdaptTaskInfo_t *taskInfo));
THF_PROTOTYPE(int, thfAdaptTaskParamsSaveToFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *fileName));
THF_PROTOTYPE(int, thfAdaptTaskParamsReadFromFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *fileName));
THF_PROTOTYPE(int, thfAdaptTaskDestroy,
              (thf_t * thf, adaptObj_t *aObj, char *taskName));
THF_PROTOTYPE(int, thfAdaptUserAdd,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName));
THF_PROTOTYPE(int, thfAdaptUserSaveToFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               char *fileName));
THF_PROTOTYPE(int, thfAdaptUserReadFromFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *fileName));
THF_PROTOTYPE(int, thfAdaptUserDestroy,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName));
THF_PROTOTYPE(int, thfAdaptUserForget,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName));
THF_PROTOTYPE(int, thfAdaptUserRemember,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName));
THF_PROTOTYPE(int, thfAdaptUserInfo,
              (thf_t * thf, adaptObj_t *aObj, char *taskName,
               thfAdaptUserInfo_t **userInfoOut));
THF_PROTOTYPE(int, thfAdaptUserInfoDestroy,
              (thf_t * thf, thfAdaptUserInfo_t *userInfo));
THF_PROTOTYPE(int, thfAdaptEnrollmentAddTrigger,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               thfTriggerEnrollment_t *enroll));
THF_PROTOTYPE(int, thfAdaptEnrollmentReplaceTrigger,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               long enrollmentID, thfTriggerEnrollment_t *enroll));
THF_PROTOTYPE(int, thfAdaptEnrollmentSaveToFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               long enrollmentID, char *fileName));
THF_PROTOTYPE(int, thfAdaptEnrollmentReadFromFile,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               char *fileName));
THF_PROTOTYPE(int, thfAdaptEnrollmentRemove,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               long enrollmentID));
THF_PROTOTYPE(int, thfAdaptEnrollmentCheck,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               thfEnrollmentCheck_t **enrollmentCheckResult));
THF_PROTOTYPE(int, thfAdaptEnrollmentQuickCheck,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, short *wav,
               unsigned long wavLen, unsigned short sampFreq,
               thfEnrollmentCheck_t **enrollmentCheckResult));
THF_PROTOTYPE(int, thfAdaptEnrollmentCheckDestroy,
              (thf_t * thf, thfEnrollmentCheck_t *enrollmentCheckResult));
THF_PROTOTYPE(int, thfAdaptModelAdapt,
              (thf_t * thf, adaptObj_t *aObj, char *taskName, char *userName,
               float accuracy));
THF_PROTOTYPE(int, thfAdaptModelGetTrigger,
              (thf_t * thf, adaptObj_t *aObj, char *taskAndUserListIn,
               thfModelTrigger_t **modelTriggerOut));
THF_PROTOTYPE(int, thfAdaptModelSaveTriggerToFile,
              (thf_t * thf, adaptObj_t *aObj, thfModelTrigger_t *modelTrigger,
               char *searchFile, char *recogFile));
THF_PROTOTYPE(int, thfAdaptModelDestroyTrigger,
              (thf_t * thf, thfModelTrigger_t *modelTrigger, int keepRecog,
               int keepSearch));

/* phoneme recognition functions */
THF_PROTOTYPE(phsearch_t *, thfPhonemeSearchCreateFromFile,
              (thf_t * thf, recog_t *r, const char *filename,
               unsigned short nbest));
THF_PROTOTYPE(int, thfPhonemeRecogInit,
              (thf_t * thf, recog_t *r, phsearch_t *s, char keep));
THF_PROTOTYPE(void, thfPhonemeSearchDestroy, (phsearch_t * phsearch));

/* Miscellaneous speech functions */
THF_PROTOTYPE(int, thfWaveFromFile,
              (thf_t * thf, const char *filename, short **speech,
               unsigned long *samples, unsigned long *sampleRate));
THF_PROTOTYPE(int, thfWaveSaveToFile,
              (thf_t * thf, const char *filename, short *speech,
               unsigned long samples, unsigned long sampleRate));

/* Tts functions */
THF_PROTOTYPE(tts_t *, thfTtsCreateFromFile,
              (thf_t * thf, const char *voicefile));
THF_PROTOTYPE(short *, thfTtsSynthesize,
              (thf_t * thf, tts_t *tts, const char *text, float pitch,
               float duration, float speechrate, unsigned long *speechLen));
THF_PROTOTYPE(unsigned long, thfTtsPrep,
              (thf_t * thf, tts_t *tts, const char *text, float pitch,
               float duration, float speechrate, unsigned long *frameSz));
THF_PROTOTYPE(short *, thfTtsGetFrame,
              (thf_t * thf, tts_t *tts, unsigned long *speechLen));
THF_PROTOTYPE(void, thfTtsReset, (tts_t * tts));
THF_PROTOTYPE(void, thfTtsDestroy, (tts_t * tts));
THF_PROTOTYPE(tts_t *, thfTtsCreateFromStatic,
              (thf_t * thf, sdata_t *global, unsigned short idx));
THF_PROTOTYPE(int, thfTtsConfigGet,
              (thf_t * thf, tts_t *tts, thfTtsConfig_t key, int *value));
THF_PROTOTYPE(int, thfTtsConfigSet,
              (thf_t * thf, tts_t *tts, thfTtsConfig_t key, int value));
THF_PROTOTYPE(int, thfCheckAudioQuality,
              (thf_t * thf, recog_t *r, short *wav, unsigned long wavLen,
               unsigned short sampleRate));

#if defined(__cplusplus)
}
#endif
#endif /* TRULYHANDSFREE_H */
