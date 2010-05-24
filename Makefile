ARMBASE=/opt/VP5500/toolchain/usr/local/arm/3.3.2
INCLUDEPATH=$(ARMBASE)/include
LIBPATH=$(ARMBASE)/arm-linux/lib
ARMPATH=$(ARMBASE)/bin
TOOLPREFIX=/arm-linux-

###############################################################
#####
##### Compiler, Linker and Tools
#####
###############################################################

CC=$(ARMPATH)$(TOOLPREFIX)gcc
CPP=$(ARMPATH)$(TOOLPREFIX)cpp
CXX=$(ARMPATH)$(TOOLPREFIX)g++
AS=$(ARMPATH)$(TOOLPREFIX)as
LD=$(ARMPATH)$(TOOLPREFIX)gcc
OC=$(ARMPATH)$(TOOLPREFIX)objcopy
OD=$(ARMPATH)$(TOOLPREFIX)objdump

CPUFLAGS=-mcpu=arm9
OPTFLAGS=-Os
#OPTFLAGS=

CFLAGS=$(CPUFLAGS) -c -Wall -I$(INCLUDEPATH)
ASFLAGS=$(CPUFLAGS) -D --gstabs
ROMLDFLAGS=-lc -s -Wl,-warn-common
THUMBFLAGS=

PROJECT=aic_dsp_test

-include Makefile.local

all: $(PROJECT)

$(PROJECT): main.o aic14.o
	$(LD) $(ROMLDFLAGS) -o $(PROJECT) main.o aic14.o 

main.o: main.c
	$(CC) $(CFLAGS) $(THUMBFLAGS) -L $(LIBPATH) -o main.o main.c

aic14.o: aic14.c aic14.h aic14_ioctl.h
	$(CC) $(CFLAGS) $(THUMBFLAGS) -L $(LIBPATH) -o aic14.o aic14.c

clean:
	$(RM) -v $(PROJECT) *.o *.elf *.bin *.hex *~

### EOF
