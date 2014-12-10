# Makefile: morse
#

MODULENAME 	:= morse


#KDIR		:= /opt/PHYTEC_BSPs/phyCORE-AM335x-PD12.1.0/platform-phyCORE-AM335x/build-target/linux-3.2
KDIR		:= /home/linux/seminar/linux-3.12.31-rt45
PWD		:= $(shell pwd)

CC		:= ${CROSS_COMPILE}gcc -Wall

ifeq ($(KERNELRELEASE),)
all:
	$(MAKE) V=1 -C $(KDIR) SUBDIRS=$(PWD) modules
else
  obj-m	:= $(MODULENAME).o
endif


clean:
	-rm -f *.o *.ko modules.order Module.symvers *.mod.c .*.cmd
	-rm -rf .tmp_versions

copy:
	scp morse.ko root@10.10.4.104:/home

