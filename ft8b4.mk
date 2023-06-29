# ft8b4 makefile
# April 16, 2023
# G. Keith Cambron 

CPPFLAGS= -Wall -std=c++11 -O2 -lsqlite3 -lncurses 
HOME=.
INCFLAGS= -I$(INC) -I $(HOME)/Logger/inc -I $(HOME)/Config/inc
BUILD=build
DIST=dist
SRC=$(HOME)/src
INC=$(HOME)/inc 

OBJECTS=$(BUILD)/main.o $(BUILD)/messageBuffer.o \
  $(BUILD)/proxyChannel.o $(BUILD)/sqlLiteDb.o  \
  $(BUILD)/udpRecvPort.o $(BUILD)/udpXmitPort.o  \
  $(BUILD)/logView.o $(BUILD)/logMessage.o

all: $(OBJECTS) $(BUILD)/config.o $(BUILD)/logger.o
	g++ -Wall -o $(DIST)/ft8b4 \
  $(OBJECTS) $(BUILD)/config.o $(BUILD)/logger.o -lsqlite3 -lncurses

$(BUILD)/config.o: $(HOME)/Config/src/configFile.cpp $(HOME)/Config/inc/configFile.h
	g++ -c -o $(BUILD)/config.o $(CPPFLAGS) $(INCFLAGS) \
   $(HOME)/Config/src/configFile.cpp

$(BUILD)/logger.o: $(HOME)/Logger/src/logger.cpp $(HOME)/Logger/inc/logger.h
	g++ -c -o $(BUILD)/logger.o $(CPPFLAGS) $(INCFLAGS) \
   $(HOME)/Logger/src/logger.cpp

$(OBJECTS): $(BUILD)/%.o: $(SRC)/%.cpp
	@ mkdir -p build dist
	g++ -c $(CPPFLAGS) $(INCFLAGS) $< -o $@\

clean:
	rm -f $(BUILD)/* $(DIST)/*

