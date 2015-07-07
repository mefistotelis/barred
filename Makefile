#******************************************************************************
#  Binary Arrays Editor
#******************************************************************************
#   @file Makefile
#      A script used by GNU Make to recompile the project.
#  @par Purpose:
#      Allows to invoke "make all" or similar commands to compile all
#      source code files and link them into executable file.
#  @par Comment:
#      None.
#  @author   Tomasz Lis
#  @date     01 Mar 2002 - 01 Jul 2013
#  @par  Copying and copyrights:
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#******************************************************************************
OS = $(shell uname -s)
ifneq (,$(findstring MINGW,$(OS)))
BARRED_RES  = obj/barred_stdres.res
EXEEXT = .exe
else
TD8XBMP_RES  = 
EXEEXT =
endif
CPP  = g++
CC   = gcc
WINDRES = windres
DLLTOOL = dlltool

BARRED_BIN  = bin/barred$(EXEEXT)
BARRED_LIBS =
BARRED_OBJS = \
obj/barred.o \
$(BARRED_RES)

LINKLIB = -static -lpdcurses
INCS = 
CXXINCS = 
# flags to generate dependency files
DEPFLAGS = -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" 
# code optimization flags
OPTFLAGS = -O3
DBGFLAGS =
# linker flags
LINKFLAGS = -std=c++11
# compiler warning generation flags
WARNFLAGS = -Wall -Wno-sign-compare -Wno-unused-parameter
# disabled warnings: -Wextra -Wtype-limits
CXXFLAGS = $(CXXINCS) -std=c++11 -c -fmessage-length=0 $(WARNFLAGS) $(DEPFLAGS) $(OPTFLAGS)
CFLAGS = $(INCS) -c -fmessage-length=0 $(WARNFLAGS) $(DEPFLAGS) $(OPTFLAGS)
LDFLAGS = $(LINKLIB) $(OPTFLAGS) $(DBGFLAGS) $(LINKFLAGS)
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BARRED_BIN) all-after

clean: clean-custom
	-${RM} $(BARRED_OBJS) $(BARRED_BIN) $(BARRED_LIBS)
	-@echo ' '

$(BARRED_BIN): $(BARRED_OBJS) $(BARRED_LIBS)
	@echo 'Building target: $@'
	$(CPP) $(BARRED_OBJS) -o "$@" $(LDFLAGS)
	@echo 'Finished building target: $@'
	@echo ' '

obj/%.o: src/%.cpp
	@echo 'Building file: $<'
	$(CPP) $(CXXFLAGS) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

obj/%.o: src/%.c
	@echo 'Building file: $<'
	$(CC) $(CFLAGS) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

obj/%.res: res/%.rc
	@echo 'Building resource: $<'
	$(WINDRES) -i "$<" --input-format=rc -o "$@" -O coff
	@echo 'Finished building: $<'
	@echo ' '
#******************************************************************************
