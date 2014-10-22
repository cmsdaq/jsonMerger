program_NAME := jsonmerger

#SRC_FILES       = src/json_value.cpp src/json_reader.cpp src/json_writer.cpp src/FileIO.cc src/DataPointDefinition.cc src/DataPoint.cc src/JsonMonitorable.cc src/JSONSerializer.cc src/main.cc
SRC_FILES       = src/main.cc src/DataPointCollection.cc src/json_value.cpp src/json_reader.cpp src/json_writer.cpp src/FileIO.cc src/JSONSerializer.cc src/JsonMonitorable.cc src/DataPoint.cc src/DataPointDefinition.cc
#O_FILES       = json_value.o json_reader.o json_writer.o FileIO.o DataPointDefinition.o DataPoint.o JsonMonitorable.o JsonSerializer.o main.o
O_FILE       = jsonMerger

CFLAGS += -m64 -I. -fPIC -Wall -O3 -std=c++0x  -lstdc++
program_INCLUDE_DIRS := .

LIBDIR=./lib

CC=cc

CC.c := $(CC) $(CFLAGS)

.PHONY: all

all: $(program_NAME)

$(program_NAME):
	make clean
	$(CC.c) $(SRC_FILES) -o $(O_FILE)

clean:
	rm -rf jsonMerger
