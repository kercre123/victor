#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "sndfile.h"
#include "se_types.h"
#include "dumb_fir.h"


#define MAX_SIN_FILES 50
#define MAX_CMDLINE_DIAGS 50

typedef struct
{
    char      *filename;
    float     *data;        // this is a 2-d array of impulse response data.  The data order interleved, so when operated on, must use a stride of 'channels'
    sf_count_t frames;      // the number of frames in the data struct.  frames*channels = total data length of data
    int        channels;    // the number of channels in the data struct
    int        is_valid;
} ir_file_t;

int ir_file_get_channels(ir_file_t *ir);
int ir_file_get_ntaps   (ir_file_t *ir);
float *ir_file_get_taps (ir_file_t *ir, int channel);

typedef struct 
{
    char     *filename;
    SNDFILE  *sf;
    SF_INFO   sf_info;
    float     gaindB;
    float     gain;
    int       start_channel;
} wav_file_t;

typedef struct
{
    int ndiags;
    char *diag_setting[MAX_CMDLINE_DIAGS];
} cmd_line_diag_settings_t;

typedef struct 
{
    wav_file_t sin     [MAX_SIN_FILES]; // the sin input files.  each one is a multi-channel .wav file.
    ir_file_t  sin_ir  [MAX_SIN_FILES]; // the impulse responses.  One for each sin must be defined.
    fir_bank_t sin_firs[MAX_SIN_FILES]; // each sin gets an entire bank of fir filters.
    ir_file_t  spk_ir;                  // loudpseker -> microphone impulse response.
    fir_bank_t spk_fir;                 // fir filter bank for spk -> microphones path.
    int        sin_num;                 // the number of sin files specified.
    wav_file_t sout;
    wav_file_t rin;
    wav_file_t refin;
    wav_file_t rout;
    sf_count_t inputframes;
    int        beam;
    int        forceAdaptation;
    int        profileOnly;
    int        showHighwaterMark;
    int        narrowbandNoisegenChannel;
    int        dumpAllSeDiagNames;
    char *     dumpSingleSeDiag;
    int        forcedBeam;
    char *     diagSettingsFile;
    float      ref_delay_sec;
    int        numSkipSamples;
    int        copyRoutToRefin;
    int        printDocs;
    int        zeroMics; // flag set to true if the zero mics is enabled.
    int        zeroMicsExcept; // this is the mic to not-zero.

    char *rin_filename;
    char *rout_filename;
    char *sin_filename[MAX_SIN_FILES];
    char *sout_filename;
    char *refin_filename;

    cmd_line_diag_settings_t cmdline_diags;
} options_t;

int parseOptions(int argc, char *argv[], options_t *o);
int checkFiles(options_t *o);
int wav_file_read_block_short(wav_file_t *wav, fir_bank_t *fir_bank, int16 *output, int channels, int frames);
void close_files(options_t *o);
void destroy_options(options_t *o);
#endif
