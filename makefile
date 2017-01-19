#	@(#)$Id: makefile,v 1.6 2016/06/13 04:01:20 jleffler Exp $
#
#	Makefile for SCC - Strip C Comments

.SECONDEXPANSION:		# Needed by GNU Make

GCC_STD   = -std=c11 -pedantic
GCC_FLAGS = -Werror -Wall -Wshadow -Wpointer-arith -Wcast-qual \
			-Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition

BITS      = 64
CC        = gcc -m${BITS}
JLLIBDIR  = ${HOME}/lib/${BITS}
JLLIBBASE = jl
JLLIBNAME = lib${LIBJLBASE}.a
UFLAGS    = # Set on command line only
OFLAGS    = -O -Wall -Wshadow -DDEBUG -ansi
OFLAGS    = -g -O
WFLAGS    = ${GCC_STD} ${GCC_FLAGS}
CPPFLAGS  = -I${HOME}/inc
CFLAGS    = ${CPPFLAGS} ${OFLAGS} ${WFLAGS} ${UFLAGS}
STRIP     = #-s
JLLIBFLAG = -L${JLLIBDIR}
JLLIB     = -l${JLLIBBASE}
LDFLAGS   = ${JLLIBFLAG} ${STRIP}
LDLIBS    = ${JLLIB}
PROGRAM   = scc
OUTPUT    = Output
BASH      = bash

RM_F      = rm -f
RM_FR     = rm -fr
DEBRIS    = a.out core *~ *.o *.a
DEBRIS_D  = *.dSYM

TEST_SCRIPTS = \
	scc.test-01.sh \
	scc.test-02.sh \
	scc.test-03.sh \
	scc.test-04.sh \
	scc.test-05.sh \
	scc.test-06.sh \
	scc.test-07.sh \

TEST_SOURCE = \
	scc-bogus.binary.cpp \
	scc-bogus.rawstring.cpp \
	scc-bogus.ucns.c \
	scc-test.binary.cpp \
	scc-test.comment.cpp \
	scc-test.example1.c \
	scc-test.example2.c \
	scc-test.example3.c \
	scc-test.hexfloat.cpp \
	scc-test.numpunct.cpp \
	scc-test.rawstring.cpp \
	scc-test.ucns.c \

all:	${PROGRAM}

test:	${PROGRAM} test-output dev-test

dev-test: ${TEST_SCRIPTS}
	for test in ${TEST_SCRIPTS}; \
	do echo $$test; ${BASH} $$test; \
	done

test-output:
	+cd ${OUTPUT}; ${MAKE}

clean:
	${RM_F} ${TEST_SCRIPTS} ${TEST_SOURCE}
	${RM_F} ${DEBRIS}
	${RM_FR} ${DEBRIS_D}

realclean: clean
	${RM_F} ${PROGRAM:=.c}

${PROGRAM}: $$@.c
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS} ${LDLIBS}
