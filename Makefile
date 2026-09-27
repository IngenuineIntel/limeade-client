#!/usr/bin/make

SRC := src/*.d
LIBLIMEADE_H_URI := "https://raw.githubusercontent.com/IngenuineIntel/liblimeade/refs/heads/stable/include/liblimeade/liblimeade.h"

liblimeade.h:
	-@rm liblimeade.h
	curl $(LIBLIMEADE_H_URI) > liblimeade.h

liblimeade.di: liblimeade.h
	dmd -c liblimeade.h -Hf=liblimeade.di


clean:
	-rm -r liblimeade.*
