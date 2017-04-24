#---------------------------------------------------------------------------------------
APP_NAME := libGBPlatformDownSingle

SIP_INC := -I../../3party/osip2/include -I../../3party/eXosip2/include -I./include
SIP_LIB	:= -L../../3party/osip2/linux -L../../3party/eXosip2/linux
SIP_DFLAGS := -DRTP_REORDER

LIBEVENT2_INC := -I../../3party/event2/include
LIBEVENT2_LIB := -I../../3party/event2/lib/linux

#OSTYPE := 
#OSTYPE := arm-linux-
#OSTYPE	:= arm-none-linux-gnueabi-
#OSTYPE := mipsel-linux-

#TARGET_SO	= true
#TARGET_AR	= true
#TARGET_APP = true

#-----------------------------------------------------------------------------------------
LINUX_GCC    := $(OSTYPE)gcc
LINUX_CXX    := $(OSTYPE)g++
LINUX_LD     := $(OSTYPE)ld
LINUX_AR     := $(OSTYPE)ar
LINUX_RANLIB := $(OSTYPE)ranlib
LINUX_STRIP  := $(OSTYPE)strip

ifdef TARGET_SO
TARGET = ../../lib/linux/$(OSTYPE)lib/$(APP_NAME).so
endif

ifdef TARGET_AR
TARGET = ../../lib/linux/$(OSTYPE)lib/$(APP_NAME).a
endif

ifdef TARGET_APP
TARGET = ../../bin/linux/$(OSTYPE)bin/$(APP_NAME)
endif

CXX     	  = $(LINUX_CXX)
CXXSOURCES	= $(wildcard *.cpp) $(wildcard $(RECORD)/*.cpp) $(wildcard $(NXPAPI)/*.cpp)
CXXOBJS    	= $(patsubst %.cpp,%.o,$(CXXSOURCES))
CXXFULLOBJS = $(patsubst %.cpp,$(OBJPATH)/%.o,$(CXXSOURCES))

CC			   = $(LINUX_GCC)
CCSOURCES	 = $(wildcard *.c) $(wildcard $(RECORD)/*.c) $(wildcard $(NXPAPI)/*.c)
CCOBJS     = $(patsubst %.c,%.o,$(CCSOURCES))
CCFULLOBJS = $(patsubst %.c,$(OBJPATH)/%.o,$(notdir $(CCSOURCES) ) )

INCPATH = -I. -I../../include -I../../include/expat -I../../3party

ifdef TARGET_SO
LIBPATH = -L../../lib/linux/$(OSTYPE)lib -lnetio -ldts -losip2 -losipparser2 -leXosip2 -levent_core
else
LIBPATH = -L../../lib/linux/$(OSTYPE)lib -lnetec #-ldts
endif

LIBPATH += -lkbase -lz -lpthread -lm -ldl -lstdc++ -lexpat
CFLAGS  = -w -c -fPIC
DFLAGS	=

ifdef TARGET_SO
LFLAGS = -w -shared
endif

ifdef TARGET_AR
LFLAGS = -w -static
endif

ifdef TARGET_APP
LFLAGS = -w -rdynamic
endif

INCPATH	+= $(SIP_INC)
LIBPATH	+= $(SIP_LIB)

ifeq (1, $(_USE_SIP))
DFLAGS	+= $(SIP_DFLAGS)
endif

INCPATH	+= $(LIBEVENT2_INC)
LIBPATH += $(LIBEVENT2_LIB)

all: $(TARGET)

ifdef TARGET_SO
$(TARGET): $(CCOBJS) $(CXXOBJS)
	$(CXX) $(LFLAGS) $(CCFULLOBJS) $(CXXFULLOBJS) -o $(TARGET) $(LIBPATH)
endif

ifdef TARGET_AR
$(TARGET): ${CCOBJS} ${CXXOBJS}
	${LINUX_AR} rv $@ ${CCFULLOBJS} ${CXXFULLOBJS}
	${LINUX_RANLIB} $@
endif	

ifdef TARGET_APP
$(TARGET): $(CCOBJS) $(CXXOBJS)
	$(CXX) $(LFLAGS) $(CCFULLOBJS) $(CXXFULLOBJS) -o $(TARGET) $(LIBPATH)
endif
	
$(CXXOBJS): $(CXXSOURCES)
	$(CXX) $*.cpp -o $(OBJPATH)/$@ $(CFLAGS) $(DFLAGS) $(INCPATH)
	
$(CCOBJS): $(CCSOURCES)
	$(CXX) $*.c -o $(OBJPATH)/$(notdir $@) $(CFLAGS) $(DFLAGS) $(INCPATH)
	
clean:
	rm -f ${CCFULLOBJS} 
	rm -f ${CXXFULLOBJS}
	rm -f $(TARGET)
	
install:
