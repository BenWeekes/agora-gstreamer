
CC = g++  # C compiler

FLAGS        = -std=c++11  -I${AGORA_SDK_DIR}/agora_sdk/include 
CFLAGS       = -fPIC -g -std=c++11 
LDFLAGS      = -shared

TARGET_LIB = libagorac.so
DESTDIR?=/usr/local/lib

HELPER_FILES=helpers/agoradecoder.cpp  helpers/agoraencoder.cpp  helpers/agoralog.cpp  helpers/localconfig.cpp helpers/utilities.cpp

all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS) agorac.cpp 
	$(CC)  $(FLAGS) ${LDFLAGS} -o $(TARGET_LIB) agorac.cpp ${HELPER_FILES}  $(CFLAGS)

clean:
	rm $(TARGET_LIB)

install:
	mv $(TARGET_LIB) ${DESTDIR}


