#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     UNIX Makefile for MIDAS slow control frontend
#
#  $Id$
#
#####################################################################

# get OS type from shell
OSTYPE = $(shell uname)

#----------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSFLAGS = -DOS_LINUX -Dextname
CFLAGS = -g -O2 -Wall
LIBS = -lm -lz -lutil -lnsl -lpthread -lrt
endif

#-----------------------
# MacOSX/Darwin
#
ifeq ($(OSTYPE),Darwin)
FF = cc
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -fPIC -Wno-unused-function
LIBS = -lpthread -lz
endif

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
INC_DIR 	= ../../include
LIB_DIR 	= ../../lib

####################################################################
# Lines below here should not be edited
####################################################################

LIB = $(LIB_DIR)/libmidas.a

# compiler
CXX = c++
CFLAGS = -O2 -g -Wall -Wuninitialized -I$(INC_DIR) -L$(LIB_DIR)

PROGS = produce consume rpc_test rpc_clnt rpc_srvr
all: $(PROGS)

$(PROGS): %: %.cxx $(LIB)
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< -lmidas $(LIBS)

clean:
	rm -f $(PROGS) *~ \#*

