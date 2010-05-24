#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "aic14_ioctl.h"

#define AIC14_DEV		"/dev/aic14"

static int 		aic14_fd;

static u_int8_t 	bytereg[4];

static u_int32_t	mainstatus;
static u_int8_t		powerdown;
static u_int8_t		digiloop;
static u_int8_t		turbo;
static u_int8_t		lpfx;
static u_int8_t		dacgain;
static u_int8_t		adcgain;
static u_int8_t		sidetone;
static u_int8_t		preamp;
static u_int8_t		input;
static u_int8_t		dacclip;
static u_int8_t		adcclip;
static u_int8_t		psdo;
static u_int8_t		mute2;
static u_int8_t		mute3;
static u_int8_t		odrct;
static u_int8_t		lpf;
static u_int8_t		micmute;

struct aicregs
{
	u_int32_t	reg1;
	u_int32_t	reg2;
	u_int32_t	reg3;
	u_int32_t	reg4a;
	u_int32_t	reg4b;
	u_int32_t	reg5a;
	u_int32_t	reg5b;
	u_int32_t	reg5c;
	u_int32_t	reg5d;
	u_int32_t	reg6;
};

struct aicregs hwregs;
struct aicregs drvregs;

struct mnpreg
{
	u_int32_t	M;
	u_int32_t	N;
	u_int32_t	P;
};

struct mnpreg mnp;

struct fifostat
{
	u_int32_t	fifo_addr;
	u_int32_t	phys_addr;
	u_int32_t	fifo_size;
	u_int32_t	fragment_size;
	u_int32_t	read_offset;
	u_int32_t	write_offset;
	u_int32_t	fifo_status;
};

struct fifostat fifo_status[2];

static u_int32_t 	dwordregs[16];

static int result, ioctlval;

int aic14_open()
{
	aic14_fd = open(AIC14_DEV, O_RDWR);
	if(aic14_fd == -1)
	{
		printf("can't open aic14 device\r\n");
	}
	return aic14_fd;
}

int aic14_close(void)
{
	if(aic14_fd != -1)
	{
		if(close(aic14_fd) == -1)
		{
			printf("can't close device\r\n");
			return -1;
		}
	}

	return 0;
}

static void clearregs(void)
{
	int i;
	for(i = 0; i < 16; i++)
	{
		dwordregs[i] = 0;
	}
	bytereg[0] = 0;
	bytereg[1] = 0;
	bytereg[2] = 0;
	bytereg[3] = 0;
}

static void dumpregs(int result, u_int32_t ioctlval)
{
	printf("0x%08X - result 0x%08X - 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", ioctlval, result, dwordregs[0], dwordregs[1], dwordregs[2], dwordregs[3], dwordregs[4], dwordregs[5], dwordregs[6], dwordregs[7], dwordregs[8], dwordregs[9], dwordregs[10], dwordregs[11], dwordregs[12], dwordregs[13], dwordregs[14], dwordregs[15]);

	clearregs();
}

static void dump_fifostat(void)
{
	printf("read fifo stats:\r\n");
	printf("FIFO addr:   0x%08X      Phys. addr:   0x%08X\r\n", fifo_status[0].fifo_addr, fifo_status[0].phys_addr);
	printf("read offset: 0x%08X      write offset: 0x%08X\r\n", fifo_status[0].read_offset, fifo_status[0].write_offset);
	printf("FIFO size:     %8i      frag. size:     %8i\r\n", fifo_status[0].fifo_size, fifo_status[0].fragment_size);
	printf("FIFO status: 0x%08X\r\n", fifo_status[0].fifo_status);

	printf("\r\nwrite fifo stats:\r\n");
	printf("FIFO addr:   0x%08X      Phys. addr:   0x%08X\r\n", fifo_status[1].fifo_addr, fifo_status[1].phys_addr);
	printf("read offset: 0x%08X      write offset: 0x%08X\r\n", fifo_status[1].read_offset, fifo_status[1].write_offset);
	printf("FIFO size:     %8i      frag. size:     %8i\r\n", fifo_status[1].fifo_size, fifo_status[1].fragment_size);
	printf("FIFO status: 0x%08X\r\n", fifo_status[1].fifo_status);
}

static void dumpaicregs(struct aicregs *regs)
{
	printf("Reg1:               %8X\r\n",regs->reg1);
	printf("Reg2:               %8X\r\n",regs->reg2);
	printf("Reg3:               %8X\r\n",regs->reg3);
	printf("Reg4A:              %8X\r\n",regs->reg4a);
	printf("Reg4B:              %8X\r\n",regs->reg4b);
	printf("Reg5A:              %8X\r\n",regs->reg5a);
	printf("Reg5B:              %8X\r\n",regs->reg5b);
	printf("Reg5C:              %8X\r\n",regs->reg5c);
	printf("Reg5D:              %8X\r\n",regs->reg5d);
	printf("Reg6:               %8X\r\n",regs->reg6);

}

int aic14_readall(void)
{
	if(aic14_fd != -1)
	{
		clearregs();

		ioctlval = AIC14_GET_MAIN_STATUS;
		result = ioctl(aic14_fd, ioctlval, &mainstatus);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Main Status:        %8X\r\n",mainstatus);

		ioctlval = AIC14_GET_POWER_DOWN;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		powerdown = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Power-Down:         %8X\r\n",powerdown);

		ioctlval = AIC14_GET_DIGITAL_LOOP;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		digiloop = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Digital Loop:       %8X\r\n",digiloop);

		ioctlval = AIC14_GET_TURBO;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		turbo = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Turbo:              %8X\r\n",turbo);

		ioctlval = AIC14_GET_LOWPASS_FILTER_X;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		lpfx = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Lowpass Filter X:   %8X\r\n",lpfx);

		ioctlval = AIC14_GET_DAC_GAIN;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		dacgain = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("DAC Gain:           %8X (%5i dB)\r\n",dacgain, -42 + dacgain);

		ioctlval = AIC14_GET_ADC_GAIN;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		adcgain = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("ADC Gain:           %8X (%5i dB)\r\n",adcgain, -42 + adcgain);

		ioctlval = AIC14_GET_SIDETONE;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		sidetone = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Sidetone:           %8X\r\n",sidetone);

		ioctlval = AIC14_GET_PREAMP;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		preamp = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Preamp:             %8X\r\n",preamp);

		ioctlval = AIC14_GET_INPUT;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		input = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Input:              %8X\r\n",input);

		ioctlval = AIC14_GET_M_N_P;
		result = ioctl(aic14_fd, ioctlval, &mnp);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
		{
			printf("M:                  %8X\r\n",mnp.M);
			printf("N:                  %8X\r\n",mnp.N);
			printf("P:                  %8X\r\n",mnp.P);
		}

		ioctlval = AIC14_GET_DAC_CLIP;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		dacclip = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("DAC Clip:           %8X\r\n",dacclip);

		ioctlval = AIC14_GET_ADC_CLIP;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		adcclip = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("ADC Clip:           %8X\r\n",adcclip);

		ioctlval = AIC14_GET_FIFO_STATUS;
		result = ioctl(aic14_fd, ioctlval, fifo_status);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			dump_fifostat();

		ioctlval = AIC14_1F;
		result = ioctl(aic14_fd, ioctlval, dwordregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			dumpregs(result, ioctlval);

		ioctlval = AIC14_21;
		result = ioctl(aic14_fd, ioctlval, dwordregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			dumpregs(result, ioctlval);

		ioctlval = AIC14_GET_RD_SLEEP_CNT;
		result = ioctl(aic14_fd, ioctlval, dwordregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			dumpregs(result, ioctlval);

		ioctlval = AIC14_GET_WR_SLEEP_CNT;
		result = ioctl(aic14_fd, ioctlval, dwordregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			dumpregs(result, ioctlval);

		// reading HW regs returns different values compared to the aic14_conf binary. Dunno why ...
		ioctlval = AIC14_GET_ALL_HW_REGS;
		result = ioctl(aic14_fd, ioctlval, &hwregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
		{
			printf("aic14 HW regs (result=0x%08X):\r\n",result);
			dumpaicregs(&hwregs);
		}

		ioctlval = AIC14_GET_ALL_DRV_REGS;
		result = ioctl(aic14_fd, ioctlval, &drvregs);
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
		{
			printf("aic14 DRV regs (result=0x%08X):\r\n",result);
			dumpaicregs(&drvregs);
		}

		ioctlval = AIC14_GET_PSDO;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		psdo = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("PSDO:               %8X\r\n",psdo);

		ioctlval = AIC14_GET_MUTE2;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		mute2 = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Mute2:              %8X\r\n",mute2);

		ioctlval = AIC14_GET_MUTE3;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		mute3 = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Mute3:              %8X\r\n",mute3);

		ioctlval = AIC14_GET_ODRCT;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		odrct = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("ODRCT:              %8X\r\n",odrct);

		ioctlval = AIC14_GET_LOWPASS_FILTER;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		lpf = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Low Pass Filter:    %8X\r\n",lpf);

		ioctlval = AIC14_FORCE_MIC_MUTE1;
		result = ioctl(aic14_fd, ioctlval, bytereg);
		micmute = bytereg[0];
		if(result != 0)
			printf("error in IOCTL 0x%08X - result: %i\r\n",ioctlval, result);
		else
			printf("Mic Mute:           %8X\r\n",micmute);
	}
	else
	{
		printf("aic14 codec device not opened, abort register read\r\n");
		return -1;
	}
	return 0;
}

int aic14_set8k(void)
{
	if(aic14_open() == -1)
	{
		printf("failed to open aic14 codec device\r\n");
		return -1;
	}

	ioctlval = AIC14_SET_CODEC_8KHZ;
	result = ioctl(aic14_fd, ioctlval);

	if(aic14_close() == -1)
	{
		printf("failed to close aic14 codec device\r\n");
		return -1;
	}

	return result;
}

int aic14_set16k(void)
{
	if(aic14_open() == -1)
	{
		printf("failed to open aic14 codec device\r\n");
		return -1;
	}

	ioctlval = AIC14_SET_CODEC_16KHZ;
	result = ioctl(aic14_fd, ioctlval);

	if(aic14_close() == -1)
	{
		printf("failed to close aic14 codec device\r\n");
		return -1;
	}

	return result;
}

int aic14_enable_mic(void)
{
	if(aic14_open() == -1)
	{
		printf("failed to open aic14 codec device\r\n");
		return -1;
	}

	ioctlval = AIC14_FORCE_MIC_MUTE0;
	result = ioctl(aic14_fd, ioctlval, 1);
	if(result != 0)
		printf("enable mic failed in force mic mute\r\n");

	ioctlval = AIC14_PRODUCT_MIC;
	result = ioctl(aic14_fd, ioctlval, 1);
	if(result != 0)
		printf("enable mic failed in product mic\r\n");

	ioctlval = AIC14_SET_PREAMP;
	result = ioctl(aic14_fd, ioctlval, 1);
	if(result != 0)
		printf("enable mic failed in set preamp\r\n");

	if(aic14_close() == -1)
	{
		printf("failed to close aic14 codec device\r\n");
		return -1;
	}

	return result;
}
