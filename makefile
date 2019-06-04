
TARGET = epollicmp

LIBS = -L./libs -lpthread -lrt -lm
INCLUDE = 

CC = gcc
CPP = g++ 

CFLAGS = -g -Wall -O0 -std=c++11
CPPFLAGS = -g -Wall -O0 -std=c++11

#########################################################
#DO NOT NEED MODIFY BELOW
#########################################################

MAKEDIR = $(PWD)

TMPLIST = $(shell find $(MAKEDIR) -name "*.h" -or -name "*.hpp")
TMPDIR = $(dir $(TMPLIST))
INCLIST = $(sort $(TMPDIR))
INCLUDE += $(foreach temp, $(INCLIST), -I$(temp))

CSRCLIST = $(shell find $(MAKEDIR) -name '*.c')
COBJS = $(CSRCLIST:%.c=%.o)

CPPSRCLIST = $(shell find $(MAKEDIR) -name '*.cpp')
CPPOBJS = $(CPPSRCLIST:%.cpp=%.o)

OBJS = $(COBJS) ${CPPOBJS}

all: $(TARGET)

$(COBJS): %.o: %.c 
	 @echo "##########COMPILE $<##########"
	 $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
	 @echo
	 
$(CPPOBJS): %.o: %.cpp 
	 @echo "##########COMPILE $<##########"
	 $(CPP) $(CPPFLAGS) $(INCLUDE) -c $< -o $@
	 @echo

$(TARGET): $(OBJS)
	 @echo "##########LINK $@##########"
	 $(CPP) $(CPPFLAGS) $(LIBS) $(INCLUDE) $(OBJS) -o $@ 
	 @echo
	 @echo "##########COMPILE OVER##########"
	 @echo

clean:
	 rm -f $(OBJS)
	 rm -f $(TARGET)

.PHONY: clean



