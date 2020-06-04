#include "mmif.h"
#include "se_error.h"
#include "se_diag.h"
#include "mmfxpub.h"
#include <string.h>

#include "vfixlib.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include "audio-process.h"
#include "options.h"
#include "scratch_mem_pub.h"
#include "process_json.h"
#include "se_timer.h"
#include "docs.h"
#include "se_snprintf.h"


#define BLOCKS_PER_DOT 100
typedef struct  {
    int16 *Sin;
    int16 *Rin;
    int16 *Refin;
    int16 *Sout;
    int16 *Rout;
    int16 *RoutInterleaved;
    int16 *SoutInterleaved;
} wav_platform_t;

int write_buffer(options_t *o, int16 *sout, int16 *rout) {
    // remember the writef writes full frames.
    // channels already specified during sout and rout open.
    if (o->sout.sf) {
        sf_writef_short(o->sout.sf, sout,
                        MMIfGetBlockSize(PORT_SEND_SOUT));
    }
    if (o->rout.sf) {
        sf_writef_short(o->rout.sf, rout,
            MMIfGetBlockSize(PORT_RCV_ROUT));
    }
    return 0;
}

void interleave(int16 *dest_interleaved, int16 *source_deinterleaved, int channels, int frames) 
{
    int frame, channel;
    for (frame = 0; frame < frames; frame++) {
        for(channel = 0; channel < channels; channel++) {
            int dst_index = frame * channels + channel;
            int src_index = channel * frames + frame;
            dest_interleaved[dst_index] = source_deinterleaved[src_index];
        }
    }
}
void deinterleave(int16 *non_interleaved, const int16 *interleaved, int channels, int blocksize) {
    int frame;
    int channel;
    for (frame = 0; frame < blocksize; frame++) {
        for (channel = 0; channel < channels; channel++) {
            non_interleaved[channel*blocksize+frame] = interleaved[frame*channels+channel];
        }
    }
}
void deinterleave_float(float *non_interleaved, const float *interleaved, int channels, int blocksize) {
    int frame;
    int channel;
    for (frame = 0; frame < blocksize; frame++) {
        for (channel = 0; channel < channels; channel++) {
            non_interleaved[channel*blocksize+frame] = interleaved[frame*channels+channel];
        }
    }
}

int read_one_channel(wav_file_t *in_wav, int16 *out, int blocksize)
{
    int start_channel = in_wav->start_channel;
    int channels_in_wav_file = in_wav->sf_info.channels;
    
    // read only 1 channel.  rin may be multichannel.
    int j;
    int16 *rin_tmp = malloc(blocksize * sizeof(int16) * channels_in_wav_file);
    int rin_frames_read = sf_readf_short(in_wav->sf, rin_tmp, blocksize);
    for (j = 0; j < rin_frames_read; j++) {
        // move just the needed channel over.
        out[j] = rin_tmp[start_channel + j * channels_in_wav_file];
    }
    free(rin_tmp);
    return rin_frames_read;
}

int read_buffer(options_t *o, int16 *sin, int16 *rin, int16 *rout, int16 *refin) {
    int rin_read = 0;
    int sin_read = 0;
    int16 *sin_temp;
    int i;

    int num_mics = MMIfGetNumChans(PORT_SEND_SIN);
    int blocksize_sin = MMIfGetBlockSize(PORT_SEND_SIN);
    int blocksize_chans_sin = blocksize_sin * MMIfGetNumChans (PORT_SEND_SIN);

    int blocksize_rin = MMIfGetBlockSize(PORT_RCV_RIN);
    int blocksize_chans_rin = blocksize_rin * MMIfGetNumChans (PORT_RCV_RIN);
    
    int blocksize_rout = MMIfGetBlockSize(PORT_RCV_ROUT);
    // int blocksize_chans_out = blocksize_rout * MMIfGetNumChans (PORT_RCV_ROUT);

    int blocksize_refin = MMIfGetBlockSize(PORT_SEND_REFIN);
    
    int sin_temp_bytes = sizeof(*sin_temp) * blocksize_chans_sin;
    int rin_bytes = sizeof(*rin) * blocksize_chans_rin;

    sin_temp = malloc(sin_temp_bytes);
    memset(sin_temp, 0, sin_temp_bytes);
    memset(sin     , 0, sin_temp_bytes);

    for (i = 0; i < o->sin_num; i++) {
        // loop through each sin, read the dtata, and sum it up.
        sin_read += wav_file_read_block_short(&o->sin[i],
                                              &o->sin_firs[i],
                                              sin_temp, 
                                              num_mics, // allocate enough for num mics
                                              blocksize_sin);
        VAdd_i16(sin, sin_temp, sin, blocksize_chans_sin);
    }

    // simulate additive echo by applying transfer function to rout
    // if the speaker->mic transfer function is 0 (i.e. no spk_fir was configured), then
    // the simulated echo will be all zeros
    fir_bank_run(&o->spk_fir, rout, sin_temp, num_mics, blocksize_rout, blocksize_sin); // apply the speaker->microphone transform
    VAdd_i16(sin, sin_temp, sin, blocksize_chans_sin); // sum it in too.
    
    memset(rin, 0, rin_bytes);
    if (o->rin.sf) {
        rin_read = read_one_channel(&o->rin, rin, blocksize_rin);
    }
    if (o->refin.sf) {
        /*refin_read = */read_one_channel(&o->refin, refin, blocksize_refin);
    }
    free(sin_temp);
    if (rin_read == 0 && sin_read == 0) {
        return 0;
    }
    else
    {
        return 1;
    }
}
void showHighwaterMark(platform_data_t *platform) {
    int i;
    int block = 0;
    wav_platform_t *priv = platform->private;
    SEDiagSetEnumAsString(SEDiagGetIndex("mmfx_aec_adaptation_mode"), "SE_AF_FORCE_ADAPTATION");
    for (i = 0; i < platform->num_mics*10; i++) {
        audio_process_rx(platform, priv->Rin, priv->Rout);
        audio_process_tx(platform, priv->Sin, priv->Sout, priv->Refin);// process a block, force adaptation is turned on;
        block++;
    }
    SEDiagSetEnumAsString(SEDiagGetIndex("mmfx_aec_adaptation_mode"), "SE_AF_DISABLE_ADAPTATION");
    audio_process_rx(platform, priv->Rin, priv->Rout);
    audio_process_tx(platform, priv->Sin, priv->Sout,priv->Refin); // process a block, force adaptation is turned on;

    printf("***********************\n");
    ScratchGetHighWaterMarkSummary(NULL, NULL, NULL);
    printf("***********************\n");
}

int16 *alloc_and_se_diag(const char *base_name, int port, const char *help)
{
    int16 *result = calloc(MMIfGetBlockSize(port) *
                           MMIfGetNumChans(port), sizeof(*result));
    SeAssert(result != NULL);
    if (base_name != NULL) {
        // only allocate an SE Diag if there's a name.
        int channel;
        for (channel = 0; channel < MMIfGetNumChans(port); channel++) {
            char name[100];
            int blocksize = MMIfGetBlockSize(port);
            se_snprintf(name, 100, "%s_%03d", base_name, channel);
            SEDiagNewInt16Array(name, result+(channel*blocksize), blocksize, SE_DIAG_RW, NULL, help);
        }
    }
    return result;
}

void allocate_buffers(wav_platform_t *priv) 
{
    priv->Sin    = alloc_and_se_diag("wav_sin"  , PORT_SEND_SIN  , "wav file sin");
    priv->Rin    = alloc_and_se_diag("wav_rin"  , PORT_RCV_RIN   , "wav file rin");
    priv->Refin  = alloc_and_se_diag("wav_refin", PORT_SEND_REFIN, "wav file refin");
    priv->Sout   = alloc_and_se_diag("wav_sout" , PORT_SEND_SOUT , "wav file sout");
    priv->Rout   = alloc_and_se_diag("wav_rout" , PORT_RCV_ROUT  , "wav file rout");
    priv->RoutInterleaved   = alloc_and_se_diag(NULL, PORT_RCV_ROUT  , "wav file rout interleaved");
    priv->SoutInterleaved   = alloc_and_se_diag(NULL, PORT_SEND_SOUT  , "wav file sout interleaved");
}

void free_buffers(wav_platform_t *priv)
{
    free(priv->Sin);
    free(priv->Rin);
    free(priv->Refin);
    free(priv->Sout);
    free(priv->Rout);
    free(priv->RoutInterleaved);
    free(priv->SoutInterleaved);
}

void copy_refin(wav_platform_t *priv, const options_t *options) {
    if (options->refin.sf)  {
        // There is a separate refin already read in, use it... i.e. do nothing.
    } else if (options->copyRoutToRefin) {
        // user requested to
        // copy rout to refin
        SeAssert(MMIfGetNumChans(PORT_SEND_REFIN) == 1); // only know how to deal with 1 channel of refin in the .wav processor for now.
        SeAssertString(MMIfGetSampleRateHz(PORT_SEND_REFIN) == MMIfGetSampleRateHz(PORT_RCV_ROUT),"refin sample rate != rout sample rate");
        SeAssertString(MMIfGetBlockSize(PORT_SEND_REFIN) == MMIfGetBlockSize(PORT_RCV_ROUT), "refin blocksize != rout blocksize");
        memcpy(priv->Refin, priv->Rout, MMIfGetBlockSize(PORT_SEND_REFIN) * MMIfGetNumChans(PORT_SEND_REFIN) * sizeof(*priv->Refin));
    }
    else {
        // fill refin with zeros
        memset(priv->Refin, 0, MMIfGetBlockSize(PORT_SEND_REFIN) * MMIfGetNumChans(PORT_SEND_REFIN) * sizeof(*priv->Refin));
    }
}

void set_platform(platform_data_t *platform_ptr, wav_platform_t *priv_ptr, const options_t *options_ptr) {
    platform_ptr->private = priv_ptr;
    platform_ptr->num_mics = 16;
    platform_ptr->ref_delay_sec = options_ptr->ref_delay_sec;
}

static void dump_all_sediag_names()
{
    int i;
    // dump the names of all diags.
    fprintf(stdout, "%-45s %-5s %-90s\n", "sediag ID", "RW", "description");
    fprintf(stdout, "%-45s %-5s %-90s\n", "---------", "--", "-----------");
    
    for (i = 0; i < SEDiagGetNumDiags(); i++) {
        char read_write[3];
        SEDiagRW_t read_write_perm;

        strncpy(read_write, "  \0", 3);
        read_write_perm = SEDiagGetRW(i);
        if ((read_write_perm & SE_DIAG_R)==SE_DIAG_R) {
            read_write[0]='R';
        }
        if ((read_write_perm & SE_DIAG_W)==SE_DIAG_W) {
            read_write[1]='W';
        }
        
        fprintf(stdout, "%-45s|%-5s|%-90s", SEDiagGetName(i), read_write, SEDiagGetHelp(i));
        if (SEDiagGetType(i) == SE_DIAG_ENUM) {
            // Print out the legal values:
            int j = 0;
            const char *enum_value_string;
            fprintf(stdout, "\n");
            fprintf(stdout, "%-45s|%-5s|","","");
            while ((enum_value_string = SEDiagGetEnumValueAsString(i, j++)) != NULL) {
                fprintf(stdout, "%s<%d>|", enum_value_string, j-1);
            }
        }
        fprintf(stdout, "\n");
    }
}

/*
handle actions which do not process data

returns 1 if we shouldnt process data afterwards
*/
int do_post_init_actions(wav_platform_t *priv_ptr, options_t *options_ptr) {
    int stop_flag = 0;
    UNUSED(priv_ptr);

    printf("project ID:  %s\n",MMIfGetVersionString());
    fflush(stdout);

    if (options_ptr->printDocs) {
        //
        // print docs and do nothing else
        render_docs();
        stop_flag = 1;
    }
    else {
        // dont run Files until after MMFx initialized because we need to check sample rates
        stop_flag |= checkFiles(options_ptr);

        if (options_ptr->dumpSingleSeDiag) {
            // Dump the values of a specific diag.
            int diagNum = SEDiagGetIndex(options_ptr->dumpSingleSeDiag);
            if (diagNum < 0) {
                printf("Couldn't find a diag named, '%s'\n", options_ptr->dumpSingleSeDiag);
            } else {
                printf("# Help for %s: %s\n", SEDiagGetName(diagNum), SEDiagGetHelp(diagNum));
                SEDiagPrintDiag(stdout, SEDiagGet(diagNum));
            }
            stop_flag |= 1;
        }

        if (options_ptr->dumpAllSeDiagNames) {
            dump_all_sediag_names();
            stop_flag |= 1; // dont process data
        }
    }
    return stop_flag;
}

void force_adaptation(int forceAdaptation)
{
    if (forceAdaptation)
        SEDiagSetEnumAsString(SEDiagGetIndex("mmfx_aec_adaptation_mode"), "SE_AF_FORCE_ADAPTATION");
}

void force_beam(int forcedBeam)
{
    if (forcedBeam >= 0) {
        fprintf(stderr, "Forcing beam to beam %d\n", forcedBeam);
        MMIfSetForcedSpatialBeamAllSubbands( forcedBeam );
    }
}


//
// zero all microphones except zeroMicsExcept... if zeroMics is enabled.
void zero_mics(int zeroMics, int zeroMicsExcept)
{
    int sb, soln, contrib;
    if (zeroMics) {
        SpatialFilterSpec_t *pSF = MMFxGetSpatialFilterConfig(MMFxGetSingletonPubObject());
        for (sb = 0; sb < MAX_SUBBANDS_EVER; sb++) {
            for (soln = 0; soln < MAX_SOLUTIONS; soln++) {
                for (contrib = 0; contrib < MAX_MIC_CONTRIBS; contrib++) {
                    if (pSF->MicCombineIndexTable[sb][soln][contrib] != zeroMicsExcept)
                        pSF->CombineGain[sb][soln][contrib] = 0;  // zero out everybody except the one except
                }
            }
        }
    }
}

/* 
handle actions which process data
*/
void do_processing_actions(platform_data_t *platform_ptr, process_json_t *pj_ptr, wav_platform_t *priv_ptr, options_t *options_ptr) {
    int block = 0;


    printf("processing (%d) blocks\n",(int)options_ptr->inputframes);
    if (options_ptr->numSkipSamples>0)
        printf("skipping (%d) samples\n", options_ptr->numSkipSamples);

    force_adaptation(options_ptr->forceAdaptation);
    force_beam(options_ptr->forcedBeam);
    zero_mics(options_ptr->zeroMics, options_ptr->zeroMicsExcept);
    if (options_ptr->profileOnly) {
        int i;
        if (options_ptr->inputframes >= 0)
        {
            printf("profile for %d blocks\n",(int)options_ptr->inputframes);
            for (i = 0; i < options_ptr->inputframes; i++) 
            {
                process_json_set(pj_ptr, block);
                audio_process_rx(platform_ptr, priv_ptr->Rin, priv_ptr->Rout);
                audio_process_tx(platform_ptr, priv_ptr->Sin, priv_ptr->Sout, priv_ptr->Refin);
                process_json_dump(pj_ptr, block, (i == options_ptr->inputframes-1));
                block++;
            }
        }
        else
        {
            //
            // inputFrames < 0 -> run forever
            printf("profile forever\n");
            while(1)
            {
                process_json_set(pj_ptr, block);
                audio_process_rx(platform_ptr, priv_ptr->Rin, priv_ptr->Rout);
                audio_process_tx(platform_ptr, priv_ptr->Sin, priv_ptr->Sout, priv_ptr->Refin);
                process_json_dump(pj_ptr, block, 0);
                block++;
            }
        }
    } else {
        while(read_buffer(options_ptr,
                          priv_ptr->Sin,
                          priv_ptr->Rin,
                          priv_ptr->Rout,
                          priv_ptr->Refin)) {
            process_json_set(pj_ptr, block);
            copy_refin(priv_ptr, options_ptr);
            audio_process_rx(platform_ptr, priv_ptr->Rin, priv_ptr->Rout);
            audio_process_tx(platform_ptr, priv_ptr->Sin, priv_ptr->Sout, priv_ptr->Refin);
            process_json_dump(pj_ptr, block, 0);
            block++;
            interleave(priv_ptr->RoutInterleaved, priv_ptr->Rout,
                       MMIfGetNumChans(PORT_RCV_ROUT),
                       MMIfGetBlockSize(PORT_RCV_ROUT));
            interleave(priv_ptr->SoutInterleaved, priv_ptr->Sout,
                       MMIfGetNumChans(PORT_SEND_SOUT),
                       MMIfGetBlockSize(PORT_SEND_SOUT));
            write_buffer(options_ptr, priv_ptr->SoutInterleaved, priv_ptr->RoutInterleaved);
            if ((block % BLOCKS_PER_DOT) == 0) {
                printf("."); fflush(stdout);
            }
        }
        process_json_dump(pj_ptr, block-1, 1);
    }
}

/*
  do stuff after processing data
*/
void do_post_proc_actions(platform_data_t *platform_ptr, options_t *options_ptr)
{
    if (options_ptr->showHighwaterMark) {
        showHighwaterMark(platform_ptr);
    }

}

int main(int argc, char *argv[]) {
    process_json_t pj;
    options_t options;
    int err;
    platform_data_t platform;
    wav_platform_t  priv;
    int no_process_flag;
    int i;
    
    memset(&priv, 0, sizeof(priv));

    if (parseOptions(argc, argv, &options)) {
        printf("error n parseOptions\n");
        exit(1);
    }

    set_platform(&platform, &priv, &options);

    audio_process_init(&platform);

    allocate_buffers(&priv) ;
    
    err = process_json_init(&pj, options.diagSettingsFile);
    if (err) {
        printf("Error processing your json file.\n");
        exit(-1);
    }

    for (i = 0; i < options.cmdline_diags.ndiags; i++) {
        const char *setting = options.cmdline_diags.diag_setting[i];
        err = process_json_set_append(&pj, setting);
        if (err) {
            printf("Error processing your setting %s.\n", setting);
            exit(-1);
        }
    }
    
    no_process_flag = do_post_init_actions(&priv, &options);
    if (no_process_flag == 0) {
        // go ahead and process blocks
        do_processing_actions(&platform, &pj, &priv, &options);
        do_post_proc_actions(&platform, &options);
    }
    audio_process_destroy();

    process_json_done(&pj);
    close_files(&options);

    se_timer_show_all_timers();

    free_buffers(&priv);
    destroy_options(&options);
    printf("\ndone\n");

    return 0;
}
