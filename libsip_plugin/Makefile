#---------------------------------------------------------------------------------------
APP_NAME	:= libsip_plugin

ifdef _USE_VOD
VOD_INC			:=
VOD_LIB			:=
VOD_DFLAGS		:=-D_VOD_ENABLE
endif

ifdef _USE_GB28181
GB28181_INC     	:= -I../../include/GB28181
#GB28181_LIB		:= -L../../lib/linux -lGB28181
GB28181_DFLAGS		:= -D_USE_GB28181
endif

ifdef _USE_SIP
SIP_INC		:= -I../../3party/osip2/include -I../../3party/eXosip2/include
#SIP_LIB		:= -L../../3party/osip2/linux -L../../3party/eXosip2/linux
#SIP_DFLAGS	:= -D_USE_SIP
endif

#OSTYPE		:= 
#OSTYPE		:= arm-linux-
#OSTYPE		:= arm-none-linux-gnueabi- 
#OSTYPE		:= mipsel-linux-

#TARGET_SO	= true	
#TARGET_AR  = true
#TARGET_APP	= true

#-----------------------------------------------------------------------------------------
LINUX_GCC	:= $(OSTYPE)gcc
LINUX_CXX	:= $(OSTYPE)g++
LINUX_LD	:= $(OSTYPE)ld
LINUX_AR	:= $(OSTYPE)ar
LINUX_RANLIB:= $(OSTYPE)ranlib
LINUX_STRIP	:= $(OSTYPE)strip

ifdef TARGET_SO
TARGET		= ../../bin/linux/$(OSTYPE)bin/plugins/$(APP_NAME).so
endif

ifdef TARGET_AR
TARGET		= ../../bin/linux/$(OSTYPE)lib/$(APP_NAME).a
endif

ifdef TARGET_APP
TARGET		= ../../bin/linux/$(OSTYPE)bin/$(APP_NAME)
endif

CXX     	= $(LINUX_CXX)
CXXSOURCES	= $(wildcard *.cpp) $(wildcard $(RECORD)/*.cpp) $(wildcard $(NXPAPI)/*.cpp)
CXXOBJS    	= $(patsubst %.cpp,%.o,$(CXXSOURCES))
CXXFULLOBJS	= $(patsubst %.cpp,$(OBJPATH)/%.o,$(CXXSOURCES))

CC		= $(LINUX_GCC)
CCSOURCES	= $(wildcard *.c) $(wildcard $(RECORD)/*.c) $(wildcard $(NXPAPI)/*.c)
CCOBJS    	= $(patsubst %.c,%.o,$(CCSOURCES))
CCFULLOBJS 	= $(patsubst %.c,$(OBJPATH)/%.o,$(CCSOURCES))

INCPATH		= -I. -I../../include -I../../include/expat -I../../3party

ifdef TARGET_SO
LIBPATH		= -L../../lib/linux/$(OSTYPE)lib -lnetio -ldts -lGB28181 -lvod_tx
else
LIBPATH		= -L../../lib/linux/$(OSTYPE)lib -lnetec #-ldts
endif

LIBPATH		+= -lstream_transcode -lstream_transfer -lsqldata -lknet -lkdbapi -lkbase -lz -lexpat 
LIBPATH		+= -lpthread -lm -ldl -lstdc++
#DFLAGS		= -D_USE_HIKVISIONVISION

ifdef TARGET_SO
LFLAGS		= -w -shared
endif

ifdef TARGET_AR
LFLAGS		= -w -static
endif

ifdef TARGET_APP
LFLAGS		= -w -rdynamic
endif

ifdef _USE_GB28181
INCPATH		+= $(GB28181_INC)
LIBPATH		+= $(GB28181_INC)
DFLAGS		+= $(GB28181_INC)
endif

ifdef _USE_SIP
INCPATH		+= $(SIP_INC)
#LIBPATH	+= $(SIP_LIB)
#DFLAGS		+= $(SIP_DFLAGS)
endif

ifdef _USE_SIP
#INCPATH	+= $(VOD_INC)
#LIBPATH	+= $(VOD_LIB)
DFLAGS		+= $(VOD_DFLAGS)
endif

all:$(TARGET)

ifdef TARGET_SO

$(TARGET): $(CCOBJS) $(CXXOBJS)
	$(CXX) $(LFLAGS) $(CCFULLOBJS) $(CXXFULLOBJS) -o $(TARGET) $(LIBPATH)

endif

ifdef TARGET_AR

$(TARGET):${CCOBJS} ${CXXOBJS}
	${LINUX_AR} rv $@ ${CCFULLOBJS} ${CXXFULLOBJS}
	${LINUX_RANLIB} $@

endif	

ifdef TARGET_APP

$(TARGET): $(CCOBJS) $(CXXOBJS)
	$(CXX) $(LFLAGS) $(CCFULLOBJS) $(CXXFULLOBJS) -o $(TARGET) $(LIBPATH)

endif
	
$(CXXOBJS) :$(CXXSOURCES)
	$(CXX) $*.cpp -o $(OBJPATH)/$@ $(CFLAGS) $(DFLAGS) $(INCPATH)
	
$(CCOBJS) :$(CCSOURCES)
	$(CXX) $*.c -o $(OBJPATH)/$@ $(CFLAGS) $(DFLAGS) $(INCPATH)
	
clean:
	rm -f ${CCFULLOBJS} 
	rm -f ${CXXFULLOBJS}
	rm -f $(TARGET)


