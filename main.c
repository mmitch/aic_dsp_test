/*
 *
 * 2010 (c) by Christian Klippel
 *
 * licensed under GNU GPL
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "aic14.h"

int16_t sound_data[1000];
int16_t sound_data2[2000];
int16_t rec_data[160000];

#define DSP_DEV	"/dev/dsp1"

int main(void)
{
	int sound_fd, dsp_fd, cnt, cnt1, lastsamp;

	printf("probing /dev/aic14\r\n");

	if(aic14_open("/dev/aic14") == -1)
	{
		printf("failed to open aic14 codec device, exiting...\r\n");
		return -1;
	}

	aic14_readall();

	if(aic14_close() == -1)
	{
		printf("failed to close aic14 codec device...\r\n");
		return -1;
	}

	sound_fd = open("sig1k.le", O_RDONLY);
	if(sound_fd == -1)
	{
		printf("failed to open soundfile\r\n");
		return -1;
	}

	read(sound_fd, sound_data, 2000);
	close(sound_fd);

	cnt1 = 0;
	lastsamp = 0;

	for(cnt=0; cnt<1000; cnt++)
	{
		lastsamp += sound_data[cnt];
		lastsamp >>= 1;
		sound_data2[cnt1++] = lastsamp;
		lastsamp = sound_data[cnt];

		lastsamp += sound_data[cnt];
		lastsamp >>= 1;
		sound_data2[cnt1++] = lastsamp;
		lastsamp = sound_data[cnt];
	}

	printf("testing 8khz output\r\n");

	if(aic14_set8k() != 0)
		printf("failed to set 8 kHz sample rate\r\n");

	dsp_fd = open(DSP_DEV, O_WRONLY | O_NONBLOCK);
	if(dsp_fd == -1)
	{
		printf("failed to open dsp device\r\n");
		return -1;
	}
	
	write(dsp_fd, sound_data, 2000);
	printf("audio data written to device\r\n");
	close(dsp_fd);
	printf("device closed\r\n");

	sleep(1);

	printf("testing 16khz output\r\n");

	if(aic14_set16k() != 0)
		printf("failed to set 16 kHz sample rate\r\n");

	dsp_fd = open(DSP_DEV, O_WRONLY | O_NONBLOCK);
	if(dsp_fd == -1)
	{
		printf("failed to open dsp device\r\n");
		return -1;
	}
	
	write(dsp_fd, sound_data2, 4000);
	printf("audio data written to device\r\n");
	close(dsp_fd);
	printf("device closed\r\n");

	sleep(1);

	printf("testing read from audio device\r\n");

	dsp_fd = open("/dev/aud1", O_RDONLY);
	if(dsp_fd == -1)
	{
		printf("failed to open dsp device\r\n");
		return -1;
	}

	aic14_enable_mic();

	read(dsp_fd, rec_data, 320000);
	close(dsp_fd);
	printf("recording device closed\r\n");

	dsp_fd = open(DSP_DEV, O_WRONLY);
	if(dsp_fd == -1)
	{
		printf("failed to open dsp device\r\n");
		return -1;
	}
	
	write(dsp_fd, rec_data, 320000);
	printf("audio data written to device\r\n");
	close(dsp_fd);
	printf("device closed\r\n");

	return 0;
}
