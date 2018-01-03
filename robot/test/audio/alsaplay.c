/* tinyplay.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#include "tinyalsa/asoundlib.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

static int shutdown = 0;


/*****************
tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 1 
tinymix "RX3 MIX1 INP1" "RX1"
tinymix "SPK DAC Switch" 1
tinymix "RX3 Digital Volume" 50

 *****************/


int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                       unsigned int period_count);



static unsigned int gDEFAULT_DEVICE = 0;
static unsigned int gDEFAULT_CARD= 0;

static FILE* gDataFile = NULL;

#define MAX_AUDIO_BUFS 1

typedef void(*AudioBufferCallback)(void*,int);

typedef struct AlsaEngine_t {
   struct pcm *pcm;
   int bufsize;
   uint8_t* freebufs[MAX_AUDIO_BUFS];
   int freebuf_ct;
   int runflag;
   pthread_t thread_id;
   long period_us;
   AudioBufferCallback callback;
   
} AlsaEngine;

void pcm_engine_destroy(AlsaEngine* engine);
int pcm_engine_tick(AlsaEngine* engine);


int pcm_engine_create(AlsaEngine* engine, struct pcm_config* config)
{

   engine->pcm = pcm_open(gDEFAULT_CARD, gDEFAULT_DEVICE, PCM_OUT,config);
   if (!engine->pcm || !pcm_is_ready(engine->pcm)) {
      fprintf(stderr, "Unable to open PCM device %u (%s)\n",
              gDEFAULT_DEVICE, pcm_get_error(engine->pcm));
      return -1;
   }
   engine->period_us = 1e6/config->rate;

   engine->bufsize = pcm_frames_to_bytes(engine->pcm, pcm_get_buffer_size(engine->pcm));
   engine->freebuf_ct = 0;

   int i;
   for (i=0; i<config->period_count && i < MAX_AUDIO_BUFS; i++) {
      uint8_t* buffer = calloc(engine->bufsize, 1);  
      if (!buffer) {
         fprintf(stderr, "Unable to allocate %d bytes\n", engine->bufsize);
         free(buffer);
         pcm_engine_destroy(engine);
         return -2;
      }
      engine->freebufs[engine->freebuf_ct++]=buffer;
   }

   return 0;

}


static void* pcm_playback_thread(void* data) {
   AlsaEngine* engine = (AlsaEngine*)data;
   printf("threadstart: %p\n", engine);
   while (engine->runflag) {
      pcm_engine_tick(engine);
      usleep(engine->period_us/2);
   }
   return NULL;
}

void pcm_engine_play(AlsaEngine* engine)
{
   printf("play: %p\n", engine);
   engine->runflag = 1;
   pthread_create(&engine->thread_id, NULL, pcm_playback_thread, engine);
   
}

void pcm_engine_stop(AlsaEngine* engine)
{
   engine->runflag = 0;
   pthread_join(engine->thread_id, NULL);
}




void pcm_engine_destroy(AlsaEngine* engine)
{
   pcm_engine_stop(engine);
   
   while (engine->freebuf_ct>0) {
      free(engine->freebufs[--engine->freebuf_ct]);
   }
   
   pcm_close(engine->pcm);
   engine->pcm = NULL;
}



void pcm_engine_register_callback(AlsaEngine* engine, AudioBufferCallback cb)
{
   engine->callback = cb;
   printf("register: %p\n", engine->callback);
}


int pcm_engine_tick(AlsaEngine* engine)
{
   printf("tick: %p\n", engine->callback);
   
   uint8_t* active_buffer = engine->freebufs[0];
   engine->callback(active_buffer, engine->bufsize);
   
   int err = pcm_write(engine->pcm, active_buffer, engine->bufsize);
   if (err) {
      fprintf(stderr, "Error playing sample %d: %s\n",err, pcm_get_error(engine->pcm));
      return -2;
   }
   return 0;
}

//temp: should be a pthread_condition
#define pcm_buffer_wait(PE)  usleep((PE)->period_us/2)


void stream_close(int sig)
{
    /* allow the stream to be closed gracefully */
    signal(sig, SIG_IGN);
    shutdown = 1;
}


void my_callback(void *stream, int len) // len is in BYTES
{
   int num_read = fread(stream, 1, len, gDataFile);
   if (num_read > 0 && num_read < len) {
      memset(stream, 0, len-num_read); //finish with silence;
   }
   if (feof(gDataFile) ) {
      shutdown = 1;
   }
   
}

int main(int argc, char **argv)
{
    FILE *file;
    struct riff_wave_header riff_wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    unsigned int period_size = 2048;//1024;
    unsigned int period_count = 4;
    char *filename;
    int more_chunks = 1;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s file.wav [-D card] [-d device] [-p period_size]"
                " [-n n_periods] \n", argv[0]);
        return 1;
    }

    filename = argv[1];
    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        return 1;
    }

    fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
    if ((riff_wave_header.riff_id != ID_RIFF) ||
        (riff_wave_header.wave_id != ID_WAVE)) {
        fprintf(stderr, "Error: '%s' is not a riff/wave file\n", filename);
        fclose(file);
        return 1;
    }

    do {
        fread(&chunk_header, sizeof(chunk_header), 1, file);

        switch (chunk_header.id) {
        case ID_FMT:
            fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            break;
        default:
            /* Unknown chunk, skip bytes */
            fseek(file, chunk_header.sz, SEEK_CUR);
        }
    } while (more_chunks);

    /* parse command line arguments */
    argv += 2;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                gDEFAULT_DEVICE = atoi(*argv);
        }
        if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        }
        if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        }
        if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                gDEFAULT_CARD = atoi(*argv);
        }
        if (*argv)
            argv++;
    }

    AlsaEngine theEngine;
    struct pcm_config config = {
       chunk_fmt.num_channels,
       chunk_fmt.sample_rate,
       period_size,
       period_count,
       (chunk_fmt.bits_per_sample == 32)?PCM_FORMAT_S32_LE:PCM_FORMAT_S16_LE
    };

    printf("Checking playable\n");
    if (!sample_is_playable(gDEFAULT_CARD, gDEFAULT_DEVICE, config.channels, config.rate, chunk_fmt.bits_per_sample, config.period_size, config.period_count)) {
        return -1;
    }

    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);

    
    printf("Creating engine\n");
    pcm_engine_create(&theEngine, &config);


    gDataFile = file;
    pcm_engine_register_callback(&theEngine, my_callback);

 
    printf("starting playback\n");
    pcm_engine_play(&theEngine);
   

    int num_read = 1;
    do {
       printf("sleeping\n");
       sleep(1);
    } while (!shutdown && num_read > 0);
    
    fclose(file);

    pcm_engine_stop(&theEngine);
    pcm_engine_destroy(&theEngine);

    return 0;
}

int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}
