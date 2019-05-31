
TARGET = epollicmp

LIBS = -L./libs -lpthread -lrt -lm
INCLUDE = 

CPP = g++ 
CFLAGS = -g -Wall -O0 -std=c++11

#########################################################
#DO NOT NEED MODIFY BELOW
#########################################################

MAKEDIR = $(PWD)
TMPLIST = $(shell find $(MAKEDIR) -name "*.h")
TMPDIR = $(dir $(TMPLIST))
INCLIST = $(sort $(TMPDIR))
SRCLIST = $(shell find $(MAKEDIR) -name '*.cpp')

INCLUDE += $(foreach temp, $(INCLIST), -I$(temp))
OBJS = $(SRCLIST:%.cpp=%.o)

all: $(TARGET)

$(OBJS): %.o: %.cpp
	 @echo "##########COMPILE $<##########"
	 $(CPP) $(CFLAGS) $(INCLUDE) -c $< -o $@
	 @echo

$(TARGET): $(OBJS)
	 @echo "##########LINK $@##########"
	 $(CPP) $(CFLAGS) $(LIBS) $(INCLUDE) $(OBJS) -o $@ 
	 @echo
	 @echo "##########COMPILE OVER##########"
	 @echo

clean:
	 rm -f $(OBJS)
	 rm -f $(TARGET)

.PHONY: clean



