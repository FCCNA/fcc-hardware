#####################################################################
#
#  Name:         Makefile
#  Created by:   Pierre-Andre Amaudruz
#
#  Contents:     Makefile for mevb MIDAS example 
#
#  $Id$
#
#####################################################################

#ifndef MIDASSYS
MIDASSYS:=../..
#endif

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

#-----------------------------------------
# This is for OSF1
#LIBS = -lbsd -lzlib
#OSFLAGS = -DOS_OSF1 -Dextname
#FF = f77
#FFLAGS = -nofor_main -D 40000000 -T 20000000

#-----------------------------------------
# This is for Linux
#OSFLAGS = -DOS_LINUX -Dextname
#CFLAGS = 
#MIDAS_OS = linux
#LIBS = -lm -lz -lutil -lnsl -lpthread -lrt

# This is for MacOS
OSFLAGS = -DOS_DARWIN -Dextname
CFLAGS =
MIDAS_OS = darwin
LIBS = -lm -lz -lutil -lpthread

# for f2c, uncomment following:
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a
#FF = cc
# for egcs g77, use this
#FF = g77

#-------------------------------------------------------------------
# The following lines define direcories. Adjust it if necessary.
#                 
DRV_DIR         = $(MIDASSYS)/drivers
INC_DIR         = $(MIDASSYS)/include
LIB_DIR         = $(MIDASSYS)/lib
CERNLIB_DIR     = /cern/pro/lib

#-------------------------------------------------------------------
# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVER = camacnul

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = g++
CFLAGS += -g -I$(INC_DIR) -I$(DRV_DIR) -std=c++11
LDFLAGS +=

all: mevb ebfe

ebfe: $(LIB) $(LIB_DIR)/mfe.o $(DRIVER).o ebfe.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o ebfe ebfe.c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

$(DRIVER).o: $(DRV_DIR)/camac/$(DRIVER).cxx
	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/camac/$(DRIVER).cxx

mdsupport.o: $(MIDASSYS)/src/mdsupport.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

mevb:   $(LIB) mevb.c ebuser.o mdsupport.o
	$(CC) $(CFLAGS) $(OSFLAGS) -o mevb mevb.c ebuser.o mdsupport.o $(LIB) $(LDFEFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ mevb \#*


