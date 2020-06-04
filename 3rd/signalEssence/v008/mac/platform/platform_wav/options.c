#include "options.h"
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mmglobalsizes.h"
#include "se_diag.h"
#include "mmif.h"
#include "se_error.h"

//const char *argp_program_version = "signal-essence-aec 1.0";
//const char *argp_program_bug_address = "<caleb@signalessence.com>";
static char doc[] = "Signal Essence AEC command line processsor.";
static char args_doc[] = "..some stuff...";
extern void deinterleave(int16 *non_interleaved, const int16 *interleaved, int channels, int blocksize);
extern void deinterleave_float(float *non_interleaved, const float *interleaved, int channels, int blocksize);
void cmd_line_diags_destroy(cmd_line_diag_settings_t *cd);
int cmd_line_diags_insert(cmd_line_diag_settings_t *cd, const char *str);

void setup_fir_filters(options_t *o);

/*
argp_option fields:
===================
const char *name - The name of this option’s long option (may be zero)
int key          - The KEY to pass to the PARSER function when parsing this option,
                   *and* the name of this option’s short option, if it is a
                   printable ascii character
const char *arg - The name of this option’s argument, if any
                  if non-zero, name of argument associated with option; optional if associated with OPTION_ARG_OPTIONAL
int flags -       Flags describing this option; some of them are:
                  OPTION_ARG_OPTIONAL – The argument to this option is optional
                  OPTION_ALIAS        – This option is an alias for the
                                        previous option
                  OPTION_HIDDEN       – Don’t show this option in –help output
const char *doc - A documentation string for this option, shown in –help output
                  If both the name and key fields are zero, this string will be printed tabbed left from the normal option column
                  This will be the first thing printed in its group. In this usage, it’s conventional to end the string with a ‘:’ character.
int group - Group identity for this option.
 */
static struct argp_option options[] = {
    {"beam",            'b', "n", 0,
                        "Sets the selected beam number to process", 0},

    {"sin",             's', "sin.wav[:ch-start:[ir.txt][:gaindB]]",  0,
                        "sin .wav file.  The sin.wav file may be  mono (1ch) or multi-channel."
                        "If mono, you can optionally specify an impulse response ir.txt so the microphone "
                        "array is presented with a realistic input signal.  If sin is multi-channel,"
                        "it's presented directly to the microphone array as-is.  Gain applies a gain"
                        "to the signal.  use ch-start as the start channel", 0},

    {"rin",             'r', "rin.wav[:ch-start]", 0,
                        "rin .wav file.  can be multi-channel file, but only 1 channel will"
                        "be used, starting at ch-start ", 0},

    {"refin",           'F', "refin.wav[:ch-start]", 0,
                        "reference input .wav file.  can be multi-channel file, but only 1"
                        "channel will be used, starting at ch-start ", 0},

    {"refinIsRout",     'c', 0, 0,
                        "Use rout as refin (Copy channel 0 of rout to refin);"
                        "Only works if rout sample rate == rin sample rate", 0},

    {"rout",            'R', "rout.wav", 0, 
                        "specify output rout .wav file, mono (1ch)", 0},

    {"refin-delay",      'x', "0", 0, 
                        "extra refin delay, specified in seconds, e.g. 5.0e-3 = 5 msec", 0},
    {"sout",            'S', "sout.wav", 0, 
                        "sout .wav file, mono (1ch)", 0},

    {"forceAdaptation", 'f', 0, 0, 
                        "specify to force adaptation", 0},

    {"profile",         'p', "NUM_BLOCKS", OPTION_ARG_OPTIONAL, 
                        "run without input files for profiling.  Forces AEC adaptation (--forceAdaptation)."
                        "By default, runs for 1000 blocks but you can specify NUM_BLOCKS. "
                        "Specify -1 to run forever.", 0},

    {"hw",              'h', 0, 0, 
                        "Show the high water marks for the scratch buffers, and exit. "
                        "Do ./wav --profile --hw to get highwater mark without specifying real input files", 0},

    {"diag",            'd', "DIAG", 0, 
                        "Dump the value of a single SEDIAG then quit.", 0},
    {"set-diag",        'g', "DIag", 0,
                        "Sets a diag to a value.  Format is \"diag_name=diag_value\"."
                        "  diag_value can be a scalar or comma separated list thats compatible with json format.", 0},

    {"docs",             '!', 0, 0,
                        "Write detailed documents in HTML format to stdout; redirect into file and open with browser, "
                        "e.g. ./wav --docs > docs.html",0},

    {"dump-diag",       'u', 0, 0, 
                        "Dump all SE Diag names.", 0},
    {"zero-mics",       'z', "mic", 0, 
                        "Zero out all mics except for this one", 0},

    {"diag-settings",   'j', "my_settings.json", 0, 
                        "Specify json file of se diag values."
                        "Look in ./README or mmfx/platform/platform_wav/settings for examples", 0},

    {"skipsamps",       'k', "n", 0, 
                        "skip (n) samples in all input files prior to processing", 0},
    {0, 0, 0, 0, 0, 0},
};



// get_int:  convert arg into an int and put results into value
// on error, print the argp help message and quit.
error_t get_int(struct argp_state *state, const char *arg, int *value) {
    char *end;
    UNUSED(state);
    *value = strtod(arg, &end);
    if (arg == end) {
        fprintf(stderr, "Could not parse %s as an integer.\n", arg);
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

error_t get_float(struct argp_state *state, const char *arg, float *value) {
    char *end;
    UNUSED(state);
    *value = strtof(arg, &end);
    if (arg==end) 
    {
        fprintf(stderr, "Could not parse %s as a float\n", arg);
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

#define STUPID_STR_LEN 1000
typedef struct {
    char wav_file_name[STUPID_STR_LEN]; // be stupid & wasteful.  this is only a throwaway struct anyway.
    char ir_file_name[STUPID_STR_LEN];
    char gain_str[STUPID_STR_LEN];
    float gaindB;
    float gain;
    int       start_channel;
} wav_info_t;


/** parse_wav_in_string 
 * This will parse its argument and fill the wav_info_t structure (info)
 * The format of 'arg' should be in the format:
 *    <filename.wav>[:[<ir_filename.[wav|txt]>][:gain_dB]]
 * That is:
 *    test.wav                 // unity gain, unity impulse
 *    test.wav:4               // unity gain, unity impulse, start at channel 4
 *    test.wav:4:test.txt      // unity gain, start at channel 4, test.txt as the impulse response.
 *    test.wav:5:test.txt:3.2  // start at channel 5, test.txt as the impulse response, 3.2dB gain
 *    test.wav:6::3.2          // 3.2dB, unity impulse, 3.2dB gain.
 */
error_t parse_wav_in_string(const char *arg, wav_info_t *info)
{
    char *p;
    char arg_g[STUPID_STR_LEN];
    char *arg_t = arg_g;
    strncpy(arg_t, arg, STUPID_STR_LEN);
    arg_t[STUPID_STR_LEN-1]=0; // make sure it's null terminated.

    memset(info, 0, sizeof(*info));

    p = strchr(arg_t, ':'); // find the .wav file name
    if (p == NULL) {
        // No colons at all, the whole thing is the filename.
        strcpy(info->wav_file_name, arg_t);
    } else {
        // there is a colon.  Only copy a partial string.
        *p = 0; // null terminate it. 
        p++;
        strcpy(info->wav_file_name, arg_t);

        // Now, look for the start channel
        // we have a colon, which means the next thing is a channel start.
        info->start_channel = strtol(p, &p, 10);
        // Now, search for the impulse response file.
        p++;
        arg_t = p;
        p = strchr(arg_t, ':');
        if (p == NULL) {
            // no more colons.
            strcpy(info->ir_file_name, arg_t);
        } else {
        
            *p = 0;
            p++;
            strcpy(info->ir_file_name, arg_t);
            arg_t = p;
            {
                char *end;
                info->gaindB = strtod(arg_t, &end);
                if (arg_t == end) {
                    // no conversion occurred.
                    info->gaindB = 0;
                }
            }
        }
    }
    info->gain = pow(10, info->gaindB/20.0);
    return 0;
}

// Read the impulse response file.
error_t get_ir_in(char *ir_file_name, ir_file_t *ir) {
    char *txt_found, *wav_found;
    error_t err = 0;
    if (strlen(ir_file_name) == 0) {
        if (ir) {
            ir->filename = NULL;
            ir->is_valid = 0;
            ir->data = NULL;
        }
        return 0;
    }
    txt_found = strstr(ir_file_name, ".txt");
    wav_found = strstr(ir_file_name, ".wav");
    ir->filename = malloc(strlen(ir_file_name) + 1);
    strcpy(ir->filename, ir_file_name);
    if (txt_found) {
        err = ARGP_ERR_UNKNOWN;
        fprintf(stderr, "Couldn't understand the format of ir file %s -- can't parse .txt files yet.\n", ir_file_name);
        // load up the .txt file
    } else if (wav_found) {
        SNDFILE *sf;
        SF_INFO  sf_info;
        memset(&sf_info, 0, sizeof(sf_info));
        sf = sf_open(ir_file_name, SFM_READ, &sf_info);
        if (sf) {
            // This is good, the file opened.
            sf_count_t count;
            int bytes;
            float *temp_data;
            ir->channels = sf_info.channels;
            ir->frames   = sf_info.frames;
            ir->is_valid = 1;
            bytes = ir->channels*ir->frames*sizeof(*ir->data);
            ir->data     = malloc(bytes);
            temp_data    = malloc(bytes);
            memset(ir->data, 0, bytes);
            memset(temp_data, 0, bytes);
            count = sf_readf_float(sf, temp_data, ir->frames);
            deinterleave_float(ir->data, temp_data, ir->channels, ir->frames);
            free(temp_data);
            if  (count != ir->frames) {
                err = ARGP_ERR_UNKNOWN;
                fprintf(stderr, "Didn't read all %d samples.  Only read %d\n", 
                        (int)count,
                        (int)ir->frames);
                ir->is_valid = 0;
            }
            sf_close(sf);
        } else {
            err = ARGP_ERR_UNKNOWN;
            fprintf(stderr, "Couldn't open the file %s for reading\n", ir_file_name);
        }
    } else {
        // No IR file found.
        if (strlen(ir_file_name) == 0) {
            ir->is_valid = 0;
        } else {
            err = ARGP_ERR_UNKNOWN;
            fprintf(stderr, "Couldn't understand the format of ir file %s\n", ir_file_name);
        }
    }
    return err;
}
// get an input .wav file argument.  
// open the file, verify it exists, etc.
error_t get_wav_in(char *arg, wav_file_t *wav, ir_file_t *ir) {
    wav_info_t wav_info;
    error_t err;
    err = parse_wav_in_string(arg, &wav_info);
    if (err)
        return err;

    // Deal with the .wav file.
    wav->sf_info.format = 0;
    wav->filename = malloc(strlen(wav_info.wav_file_name)+1);
    strcpy(wav->filename, wav_info.wav_file_name);

    wav->sf = sf_open(wav->filename, SFM_READ, &(wav->sf_info));
    if (!wav->sf) {
        fprintf(stderr, "Couldn't open %s for reading.\n", arg);
        return ARGP_ERR_UNKNOWN;
    }

    // Deal with the impulse response file
    err = get_ir_in(wav_info.ir_file_name, ir);
    if (err)
        return err;

    // deal with the gain.
    wav->gaindB = wav_info.gaindB;
    wav->gain   = wav_info.gain;
    wav->start_channel = wav_info.start_channel;
    return 0;
}

void close_wav_in(wav_file_t *wav)
{
    if (wav->sf != NULL)
    {
        free(wav->filename);
        sf_close(wav->sf);
    }
}

void close_ir_file(ir_file_t *ir)
{
    if (ir->is_valid != 0)
    {
        free(ir->filename);
        free(ir->data);
    }
}

void close_wav_out(wav_file_t *wav)
{
    if (wav->sf)
    {
        sf_close(wav->sf);
    }
}

void close_files(options_t *o)
{
    int m;

    close_wav_in(&o->rin);

    for (m=0;m < o->sin_num;m++)
    {
        close_wav_in(&(o->sin[m]));
        close_ir_file(&(o->sin_ir[m]));
    }

    close_wav_in(&o->refin);
    close_wav_out(&o->rout);
    close_wav_out(&o->sout);
}

//
// skip N samples in wav file
void skip_samples(int numSkipSamples, SNDFILE *sf)
{
    if ((sf != NULL) && (numSkipSamples > 0))
    {
        sf_seek(sf, numSkipSamples, SEEK_SET);
    }
}

// open the file, verify it exists, etc.
error_t get_wav_out(char *arg, wav_file_t *wav, int samplerate, int channels) {
    wav->sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    wav->sf_info.samplerate = samplerate;
    wav->sf_info.channels = channels;
    wav->filename = arg;
    wav->sf = sf_open(wav->filename, SFM_WRITE, &(wav->sf_info));
    if (!wav->sf) {
        fprintf(stderr, "Couldn't open %s for writing.\n", arg);
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    options_t *o = state->input;
    error_t retval = ARGP_ERR_UNKNOWN ; // return an error unless otherwise handled.
    switch (key) {

        // ryu 2013-01-04
        // mmfx doesn't currently support running injecting a single band of narrowband noise
        // (and hasn't for a while now)
        //
        // case 'n':
        //     retval = get_int(state, arg, &(o->narrowbandNoisegenChannel));
        //     break;
    case 'b':
        retval =  get_int(state, arg, &(o->forcedBeam));
        break;
    case 'r':
        o->rin_filename = malloc(strlen(arg)+1);
        strcpy(o->rin_filename, arg);
        retval = 0;
        break;
    case 'F':
        o->refin_filename = malloc(strlen(arg)+1);
        strcpy(o->refin_filename, arg);
        retval = 0;
        break;
    case 'c':
        o->copyRoutToRefin = 1;
        retval = 0;
        break;
    case 's':
        o->sin_filename[o->sin_num] = malloc(strlen(arg)+1);
        strcpy(o->sin_filename[o->sin_num], arg);
        o->sin_num++;
        retval = 0;
        break;
    case 'x':
        retval = get_float(state, arg, &(o->ref_delay_sec));
        
        // sanity check
        if ((o->ref_delay_sec < 0.0f) || (o->ref_delay_sec >= 2.0f))
        {
            fprintf(stderr,"nonsensical reference delay %f\n",o->ref_delay_sec);
            retval = ARGP_ERR_UNKNOWN;
        }
        break;
    case 'R':
        o->rout_filename = malloc(strlen(arg)+1);
        strcpy(o->rout_filename, arg);
        retval = 0;
        break;
    case 'S':
        o->sout_filename = malloc(strlen(arg)+1);
        strcpy(o->sout_filename, arg);
        retval = 0;
        break;
    case 'f':
        retval = 0;
        o->forceAdaptation = 1;
        break;
    case 'p':
        retval = 0;
        o->profileOnly = 1;
        o->forceAdaptation = 1;

        // how many blocks to process?
        // we actually specify the number of samples as "frames" because
        // libwav deals with multichannel WAV files
        o->inputframes = (NULL!=arg) ? (sf_count_t)atoi(arg) : (sf_count_t)1000;
        break;
    case 'h':
        retval = 0;
        o->showHighwaterMark = 1;
        o->forceAdaptation = 1;
        break;
    case 'j':
        o->diagSettingsFile = malloc(strlen(arg)+1);
        strcpy(o->diagSettingsFile, arg);
        retval = 0;
        break;

    case 'u':
        retval = 0;
        o->dumpAllSeDiagNames = 1;
        break;

    case 'z':
        retval = 0;
        o->zeroMics = 1;
        o->zeroMicsExcept = atoi(arg);
        break;

    case 'd':
        o->dumpSingleSeDiag = malloc(strlen(arg)+1);
        strcpy(o->dumpSingleSeDiag, arg);
        retval = 0;
        break;

    case 'g':
        // set a diag from the command line.  It will happen at block 0
        cmd_line_diags_insert(&o->cmdline_diags, arg);
        retval = 0;
        break;

    case 'k':
        retval =  get_int(state, arg, &(o->numSkipSamples));
        break;

    case '!':
        o->printDocs=1;
        retval=0;
        break;

    case ARGP_KEY_ARG:
        retval = 0;
        break;
    case ARGP_KEY_END:
        retval = 0;
        break;
    case ARGP_KEY_INIT:
        retval = 0;
        // printf("ARGP_KEY_INIT\n");
        break;
    case ARGP_KEY_NO_ARGS:
        break;
    case ARGP_KEY_ARGS:
        retval = 0;
        printf("ARGP_KEY_ARGS\n");
        break;
    case ARGP_KEY_FINI:
        retval = 0;
        //printf("ARGP_KEY_FINI\n");
        break;
    case ARGP_KEY_SUCCESS:
        retval = 0;
        // printf("ARGP_KEY_SUCCESS\n");
        break;
    case ARGP_KEY_ERROR:
        retval = 0;
        printf("ARGP_KEY_ERROR\n");
        break;
    default:
        retval = ARGP_ERR_UNKNOWN;
        break;
    }
    return retval;
}

struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

int parseOptions(int argc, char *argv[], options_t *o) 
{
    memset(o, 0, sizeof(*o));
    o->forcedBeam = -1;
    argp_parse(&argp, argc, argv, 0, 0, o);
    return 0;
}

void destroy_options(options_t *o)
{
    int m;
    for (m=0;m<MAX_SIN_FILES;m++)
    {
        if (o->sin_filename[m] != NULL)
        {
            free(o->sin_filename[m]);
        }
    }
    if (o->rout_filename != NULL)
    {
        free(o->rout_filename);
    }
    if (o->sout_filename != NULL)
    {
        free(o->sout_filename);
    }
    if (o->diagSettingsFile != NULL)
    {
        free(o->diagSettingsFile);
    }
    if (o->dumpSingleSeDiag != NULL)
    {
        free(o->dumpSingleSeDiag);
    }
    cmd_line_diags_destroy(&o->cmdline_diags);
}

/*
  sanity check for files

  return 1 if anything smells bad
  otherwise return 0
  
*/
int checkFiles(options_t *o) {
    error_t retval = 0;
    int i;

    // check MMFx and WAV sample rates
    if (o->rin_filename) {
        retval = get_wav_in(o->rin_filename, &(o->rin), NULL);
        if ((int)MMIfGetSampleRateHz(PORT_RCV_RIN) != o->rin.sf_info.samplerate) {
            printf("rin wav file sample rate (%d) does not match expected sample rate (%d)\n",
                   o->rin.sf_info.samplerate,
                   (int)MMIfGetSampleRateHz(PORT_RCV_RIN)
                );
            return 1;
        }
    }
    if (o->refin_filename) {
        retval = get_wav_in(o->refin_filename, &(o->refin), NULL);
        if ((int)MMIfGetSampleRateHz(PORT_SEND_REFIN) != o->refin.sf_info.samplerate) {
            printf("refin wav file sample rate (%d) does not match expected sample rate (%d)\n",
                   o->refin.sf_info.samplerate,
                   (int)MMIfGetSampleRateHz(PORT_SEND_REFIN)
                );
            return 1;
        }
    }
    for (i = 0; i < o->sin_num; i++) {
        retval = get_wav_in(o->sin_filename[i], &(o->sin[i]), &(o->sin_ir[i]));
        if ((int)MMIfGetSampleRateHz(PORT_SEND_SIN) != o->sin[i].sf_info.samplerate) {
            printf("sin wav file sample rate (%d) does not match expected sample rate (%d)\n",
                   o->sin[i].sf_info.samplerate,
                   (int)MMIfGetSampleRateHz(PORT_SEND_SIN)
                );
            return 1;
            break;
        }
    }
    if (o->rout_filename) {
        retval = get_wav_out(o->rout_filename, &(o->rout),
                             (int)MMIfGetSampleRateHz(PORT_RCV_ROUT), 
                             MMIfGetNumChans(PORT_RCV_ROUT));
        if (retval)
        {
            return 1;
        }
    }
    if (o->sout_filename) {
        retval = get_wav_out(o->sout_filename, &(o->sout),
                             (int)MMIfGetSampleRateHz(PORT_SEND_SOUT),
                             MMIfGetNumChans(PORT_SEND_SOUT));
        if (retval)
        {
            return 1;
        }
    }
    
    if ((o->rin.sf && (o->rin.start_channel > o->rin.sf_info.channels-1)) && ! o->profileOnly) {
        printf("Rin contains %d channels, but you specified to start at channeld %d.  Cowardly quitting\n",
               o->rin.sf_info.channels,
               o->rin.start_channel);
        return 1;
    }

    // how many frames (sample blocks) to process?
    if (o->profileOnly==0) {
        //
        // figure out how many samples are in input files
        // take skip samples into account
        //
        int inputframes; // where "frames" means samples

        inputframes = o->sin[0].sf_info.frames;   // consider only number of samples in sin
        printf("input frames=(%d) numSkipSamples=(%d)\n",inputframes,o->numSkipSamples);
        inputframes -= o->numSkipSamples;

        o->inputframes = inputframes;

        //
        // if user requested skip samples, then do so now
        {
            int n;
            for (n=0;n<o->sin_num;n++)
            {
                skip_samples(o->numSkipSamples, o->sin[n].sf);
            }
            skip_samples(o->numSkipSamples, o->rin.sf);
            skip_samples(o->numSkipSamples, o->refin.sf);
        }
    }
    {
        int warn_input_files = 0;
        warn_input_files = !(o->profileOnly || o->dumpAllSeDiagNames || (o->dumpSingleSeDiag != NULL));
        if (warn_input_files) {
            if ((!o->sin[0].sf) && (!o->rin.sf) && (!o->profileOnly)) {
                fprintf(stderr, "No input files.  Please try again.\n");
                return 1;
            }
            if ((!o->sout.sf) && (!o->rout.sf) && (!o->profileOnly)) {
                fprintf(stderr, "No output files.  Please try again.\n");
                return 1;
            }
        }
    }
    
    // All files opened.  good.

    setup_fir_filters(o);
    return 0;

}


typedef struct {
    int junk;
} fir_struct;
/** run an FIR on the input data 'in', sending the data to the output
 * buffer.
 * @param[in] ir       Impulse response structure
 * @param[in] in       The input data, in deinterleaved form.
 * @param[out] out     Output data, in deinterleaved form.
 * @param[in] channels Number of channels in *out
 * @param[in] frames   Number of frames in *out
 */
void ir_run_fir(float *ir, int16 *in, int16 *out, int channels, int frames)
{
    UNUSED(ir);
    UNUSED(in);
    UNUSED(out);
    UNUSED(channels);
    UNUSED(frames);
//  if (ir->delay_line_bytes == 0) {
//      ir->delay_line_bytes = delay_line_bytes;
//      ir->delay_line = malloc(delay_line_bytes);
//      memset(ir->delay_line, 0, delay_line_bytes);
//  }
}


/** Read a block of data from the wave file wav. The data is processed
    through the impulse response imbedded in the .wav
    paramter. Generally, the .wav file should either be mono, and have
    an attached impulse response that transforms the mono signal to a
    multi channel signal for microphone array processing, or the .wav
    file should be multi-channel (i.e. already processed) and not
    require any impulse response molestation.
    
    @param[in]  wav    The .wav file to read the data from.  the # of
    channels in the .wav must be >= the channels
    paramter.  
    @param[in]  fir_bank A fir_bank object that the input file will be
    filtered through. 
    @param[out] output A storage location to put the data.  Must be
    at least channels*frames*sizeof*(*block)
    bytes. 
    @param[in]  channels The number of channels in 'block'  This must
    be less than or equal to the number of channels
    in the .wav file, or if the .wav file is mono,
    must be less than or equal to the number of
    channels in the impulse repsonse file. 
    @param[in]  frames   The number of frames to stick into *block
    @return            the number of frames read.
*/
int wav_file_read_block_short(wav_file_t *wav, fir_bank_t *fir_bank, int16 *output, int channels, int frames)
{
    int wav_buffer_size_bytes = wav->sf_info.channels * frames * sizeof(*output);
    int output_bytes          = sizeof(*output) * channels * frames;
    int16 *wav_buffer    = malloc(wav_buffer_size_bytes);
    int16 *wav_buffer_di = malloc(wav_buffer_size_bytes);   // DeInterleaved samples
    sf_count_t read;
    int wav_channels = wav->sf_info.channels;

    memset(wav_buffer, 0, wav_buffer_size_bytes);
    read = sf_readf_short(wav->sf, wav_buffer, frames);
    deinterleave(wav_buffer_di, wav_buffer, wav->sf_info.channels, frames);
    if ((wav_channels > 1) ||
        (fir_bank->nfirs==0))   // or no fir filters are configured
    {   
        // The input is a multi-channels sin buffer.  In this
        // case we don't want to run the FIR bank, but rather
        // just copy the input to the output unchanged.

        // the wav may have more or fewer channels than the
        // 'output'.  Take the minimum.
        int n_channels = channels < wav_channels ? channels : wav_channels;
        int n_bytes_to_copy = sizeof(*output) * frames * n_channels;
        memset(output, 0, output_bytes);
        memmove(output, &wav_buffer_di[wav->start_channel * (frames)], n_bytes_to_copy);
    } else {
        assert(fir_bank->nfirs > 0);
        fir_bank_run(fir_bank, wav_buffer_di, output, channels, frames, frames);
    }
    free(wav_buffer);
    free(wav_buffer_di);
    return read;
}


void setup_fir_bank(fir_bank_t *fir_bank, ir_file_t *ir) {
    int channel;
    int channels = ir_file_get_channels(ir);
    fir_bank_init(fir_bank, channels);
    for (channel = 0; channel < channels; channel++) {
        fir_bank_set_ir(fir_bank, channel, ir_file_get_taps(ir, channel), ir_file_get_ntaps(ir));
    }
}

/** Set up all the FIR filters based on how the options need them to be.
 */
void setup_fir_filters(options_t *o) {
    int i;
    // Set up the speaker FIR.
    setup_fir_bank(&o->spk_fir, &o->spk_ir);
    for (i = 0; i < o->sin_num; i++) {
        setup_fir_bank(&o->sin_firs[i], &o->sin_ir[i]);
    }
}


int ir_file_get_channels(ir_file_t *ir) {
    return ir->channels;
}

int ir_file_get_ntaps(ir_file_t *ir) {
    return ir->frames;
}

float *ir_file_get_taps(ir_file_t *ir, int channel) {
    return &(ir->data[channel * ir->frames]);
}

int cmd_line_diags_insert(cmd_line_diag_settings_t *cd, const char *str)
{
    cd->diag_setting[cd->ndiags] = malloc(strlen(str)+1);
    strcpy(cd->diag_setting[cd->ndiags], str);
    cd->ndiags++;
    return 0;
}

void cmd_line_diags_destroy(cmd_line_diag_settings_t *cd)
{
    int i;
    for(i = 0; i < cd->ndiags; i++) {
        free(cd->diag_setting[i]);
        cd->diag_setting[i] = NULL;
    }
}
