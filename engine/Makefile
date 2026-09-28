#!/usr/bin/make

KERNEL := /lib/modules/$$(uname -r)

clangd:
	echo "-I$(KERNEL)/build/arch/x86/include" > compile_flags.txt
	echo "-I$(KERNEL)/build/arch/x86/include/generated/uapi" > compile_flags.txt
	echo "-I$(KERNEL)/build/include" >> compile_flags.txt
	echo "-D__KERNEL__" >> compile_flags.txt
	echo "-D__LITTLE_ENDIAN" >> compile_flags.txt
	echo "-D__GNUC__" >> compile_flags.txt
	echo "-DMODULE" >> compile_flags.txt
