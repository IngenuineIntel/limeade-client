#!/usr/bin/make

obj-m += lce.o
lce-objs := main.o hooks.o procfile.o ready.o

KERNEL := /lib/modules/$(shell uname -r)
KDIR ?= $(KERNEL)/build
CWD  := $(shell pwd)

SRC := $(CWD)
INC := $(CWD)/include
OUT := $(CWD)/build

ccflags-y += $(CFLAGS_EXTRA)
ccflags-y += -I$(INC)

all:
	@mkdir -p $(OUT)
	$(MAKE) -C $(KDIR) M=$(SRC) MO=$(OUT) modules

clangd:
	echo "-I$(KERNEL)/build/arch/x86/include" >> compile_flags.txt
	echo "-I$(KERNEL)/build/arch/x86/include/generated/uapi" >> compile_flags.txt
	echo "-I$(KERNEL)/build/include" >> compile_flags.txt
	echo "-D__KERNEL__" >> compile_flags.txt
	echo "-D__LITTLE_ENDIAN" >> compile_flags.txt
	echo "-D__GNUC__" >> compile_flags.txt
	echo "-DMODULE" >> compile_flags.txt

load: all
	sudo insmod build/lce.ko

