#include <alsa/asoundlib.h>
#include <unistd.h>

static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char buffer[16*1024];                          /* some random data */

int main(void)
{
        int err;
        unsigned int i;
        snd_pcm_t *handle;
        snd_pcm_sframes_t frames;
        for (i = 0; i < sizeof(buffer); i++)
                buffer[i] = random() & 0x03;
        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = snd_pcm_set_params(handle,
                                      SND_PCM_FORMAT_U8,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      48000,
                                      1,
                                      500000)) < 0) {   /* 0.5sec */
                fprintf(stderr,"Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        for (i = 0; i < 16; i++) {
                printf("loop %d\n",i);
                frames = snd_pcm_writei(handle, buffer, sizeof(buffer));
                if (frames < 0) {
                        fprintf(stderr,"snd_pcm_writei failed: %s --> recover\n", snd_strerror(frames));
                        frames = snd_pcm_recover(handle, frames, 0);
                        frames = snd_pcm_recover(handle, frames, 0);
                }
                if (frames < 0) {
                        fprintf(stderr,"snd_pcm_recover failed: %s\n", snd_strerror(frames));
                        break;
                }
                if (frames > 0 && frames < (long)sizeof(buffer))
                        fprintf(stderr,"Short write (expected %li, wrote %li)\n", (long)sizeof(buffer), frames);

                usleep(500000); // provocate buffer underrun
        }
        snd_pcm_close(handle);
        return 0;
}
