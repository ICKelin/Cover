ifndef CFG
	CFG = _Debug
endif

ifeq ($(CFG), Debug)
	CFLAGS = -g -D __FOR_DEBUG_KO
else
	CFLAGS = -O1 -Wall
endif


CC  = gcc
CXX = g++
AR  = ar

INCLUDE_PATH  	    = ./xml/ ./http ./module ./log
LIB_PATH 	    =  
CFLAGS 		    += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LDFLAGS 	    = -lpthread 
TARGETBIN 	    = Cover


INCLUDE = -I. $(patsubst %, -I%, $(INCLUDE_PATH))
LIB 	= -L. $(patsubst %, -L%, $(LIB_PATH))
CFLAGS 	+= $(INCLUDE) $(LIB) 


SRCS_C 	 := $(shell find . -name '*.c')		
SRCS_C 	 := $(patsubst ./%,%,$(SRCS_C))		

SRCS_CPP :=$(shell find . -name '*.cpp')  
SRCS_CPP :=$(patsubst ./%,%,$(SRCS_CPP))

SRCS_CXX :=$(shell find . -name '*.cxx')
SRCS_CXX :=$(patsubst ./%,%,$(SRCS_CXX))

LOG_FILE :=$(shell find . -name '*.log')
LOG_FILE :=$(patsubst ./%,%,$(LOG_FILE))

BAK_FILE :=$(shell find . -name '*.bak')
BAK_FILE :=$(patsubst ./%,%,$(BAK_FILE))

OBJS := $(patsubst %.o,%.o,$(SRCS_C:.c=.o) $(SRCS_CPP:.cpp=.o) $(SRCS_CXX:.cxx=.o))

.PHONY: all clean clobber distclean depends install

all: $(TARGETBIN)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS)  -c -o $@ $<

$(TARGETBIN):  $(OBJS)
	$(CXX) $(CFLAGS) $(filter %.o,$^) -o $@ $(LDFLAGS)

clean:
	-$(RM) $(OBJS) $(TARGETBIN)

clog:
	-$(RM) -f $(LOG_FILE) $(BAK_FILE)
	
