CC=g++
OPT=-O0

TARGET = server

DEPFLAGS = -MP -MD

LIB_FLAGS = -llog -lpthread -lmysqlclient # log --> 日志库  mysqlclient --> mysql库

SRC = ./src
CODE_DIR_IN_SRC = $(wildcard $(SRC)/*)
CODEDIRS = ./src $(CODE_DIR_IN_SRC)

INCLUDE = ./include
HEAD_DIR_IN_INCLUDE = $(wildcard $(INCLUDE)/*)
HEAD_DIRS = ./src $(HEAD_DIR_IN_INCLUDE)

HEADFILES = $(foreach D, $(HEAD_DIRS),$(wildcard $(D)/*.hpp))

CFILES = $(foreach D,$(CODEDIRS),$(wildcard $(D)/*.cc))

FLAGS=-g $(OPT) $(DEPFLAGS) $(foreach D, $(HEAD_DIRS),-I$(D))

OBJECTS=$(patsubst %.cc,%.o,$(CFILES))
DEPFILES=$(patsubst %.cc,%.d,$(CFILES))

all:$(TARGET)


$(TARGET):$(OBJECTS)
	$(CC) $^ -o $@ $(LIB_FLAGS)

%.o:%.cc
	$(CC) $(FLAGS) -c -o $@ $<
clean:
	rm -rf $(TARGET) $(OBJECTS) $(DEPFILES)

-include $(DEPFILES)
