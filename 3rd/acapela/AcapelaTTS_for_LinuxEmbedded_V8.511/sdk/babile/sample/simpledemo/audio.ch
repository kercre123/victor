#if !defined(LINUX_UA) && !defined(__EPSON_C33__)
# if defined (PLATFORM_LINUX) 
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <linux/soundcard.h>
#  define O_BINARY 0
# else
#  include <io.h>
# endif
#else
# define O_BINARY 0
#endif

#include <sys/stat.h>
#include <fcntl.h>

static void writeWavHeader(int out,int freq)
{
	long pos=0;
	write(out,"RIFF",4);
	pos=36;
	write(out,&pos,4);	/* total len */
	write(out,"WAVE", 4);
	write(out,"fmt ", 4);
	pos=16;
	write(out,&pos,4);		  /* fmt chunk size */
	pos=1;
	write(out,&pos,2);	        /* FormatTag: WAVE_FORMAT_PCM */
	write(out,&pos,2);         /*  channels"\001\000"        */
	pos=freq;
	write(out,&pos,4); /* SamplesPerSec             */
	pos=2*freq;
	write(out,&pos,4); /*  Average Bytes/sec        */
	pos=2;
	write(out,&pos,2);         /*  BlockAlign "\002\000"    */
	pos=16;
	write(out,&pos,2);	     /* BitsPerSample  "\020\000" */ 
	write(out,"data", 4);
	pos=0;
	write(out,&pos,4);
}

static int open_audio(const char* out_name, int sampling_rate)
{
#if !defined ( WIN32) && !defined (LINUX_UA)
	int size=16;
	int nb_channel=0;
	int tmp= sampling_rate;
	int format;
#endif
	int out=-1;
	if (strcmp(out_name,"/dev/dsp")!=0)
	{
		if (strcmp(out_name,"/dev/null")==0) return 0;
		out= open(out_name, O_WRONLY | O_CREAT | O_TRUNC  | O_BINARY,  S_IWRITE | S_IREAD);
		if (out<0)
		{
			printf("Can't open %s\n",out_name);
		}
		else if (strlen(out_name)>4 && strcmp(&out_name[(strlen(out_name)-4)],".wav")==0) 
		{
			writeWavHeader(out,sampling_rate);
		}
		return out;
	}

#if !defined ( WIN32) && !defined (LINUX_UA)
	// Open linux audio
	out= open(out_name, O_WRONLY); // | O_NDELAY);
	if (out<0)  {printf("Cant open %s\n", out_name); return -1;}

//	if (ioctl(out, SNDCTL_DSP_SAMPLESIZE, &size)<0 || size!=16)
//	{printf("Cant open /dev/dsp 16bits\n"); return -1;}

		format= AFMT_S16_LE;
		ioctl(out,SNDCTL_DSP_SETFMT,&format);
	nb_channel=1;
	if ((ioctl(out, SNDCTL_DSP_STEREO, &nb_channel)<0) )
	{printf("Cant open /dev/dsp mono, channel % d\n",nb_channel); return -1;}

	if ((ioctl(out, SNDCTL_DSP_SPEED, &tmp)<0) || tmp!=sampling_rate)
	{printf("Cant open /dev/dsp Sampling rate\n"); return -1;}

	//if (ioctl(out, SNDCTL_DSP_SETFRAGMENT, &tmp)<0)
#endif /* WIN32 */

	return out;
}

static void closeWav(FILE* out,long tsize)
{	
	long tmp;
		fseek(out,4,SEEK_SET);
		tmp=tsize*sizeof(BB_S16)+44-8;
		fwrite(&tmp,4,1,out);
		fseek(out,40,SEEK_SET);
		tmp=tsize*sizeof(BB_S16);
		fwrite(&tmp,4,1,out);
}

static void close_audio(int hndl, char* filename, long wavSize)
{
	if (hndl)
	{
		close(hndl);
		if (strlen(filename)>4 && strcmp(&filename[(strlen(filename)-4)],".wav")==0) 
		{
			FILE* f=fopen(filename,"r+b");
			if (f)
			{
				closeWav(f,wavSize);
				fclose(f);
			}
		}
	}
}
