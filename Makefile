# define the Cpp compiler to use
CXX = g++
#CC = gcc
# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra -g
CFLAGS	:= -std=c11 -Wall -Wextra -g
# define library paths in addition to /usr/lib
LFLAGS = -lpthread -lyaml-cpp

# define output directory
OUTPUT	:= output

# define build directory
BUILD	:= build

# define source directory
SRC		:= src
# define include directory
INCLUDE	:= include

# define lib directory
LIB		:= 

# Ragel 支持
RLSOURCES	:= $(shell find $(INCLUDE) -name '*.rl')
RL2C		:= $(patsubst $(INCLUDE)/%.rl,$(BUILD)/%.c, $(RLSOURCES))

ifeq ($(OS),Windows_NT)
MAIN	:= main.exe
SOURCEDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
MD	:= mkdir
else
MAIN	:= main
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
MD	:= mkdir -p
endif

# define any directories containing header files other than /usr/include
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

# define the C libs
LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

# define the C source files
SOURCES		:= $(shell find $(SRC) -name '*.cpp')

# define the C object files
#OBJECTS		:= $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o, $(SOURCES))
OBJECTS		:= $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o, $(SOURCES)) $(RL2C:.c=.o)
#ragel
$(BUILD)/%.c: $(INCLUDE)/%.rl
	@$(MD) $(dir $@)
	ragel -C -I./$(INCLUDE)/http $< -o $@
# define the dependency output files
DEPS		:= $(OBJECTS:.o=.d)

# define the output main file
OUTPUTMAIN	:= $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(OUTPUT) $(BUILD) $(MAIN)
	@echo Executing 'all' complete!
	@echo Executing 'run: all' complete!
	objdump -d $(OUTPUTMAIN) > $(OUTPUT)/$(MAIN).objdump
	@echo Objdump complete!
	readelf -a $(OUTPUTMAIN) > $(OUTPUT)/$(MAIN).readelf
	@echo Readelf complete!

$(OUTPUT) $(BUILD):
	$(MD) $@

$(MAIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $^ $(LFLAGS) $(LIBS)

# include all .d files
-include $(DEPS)

# this is a suffix replacement rule for building .o's and .d's from .c's
$(BUILD)/%.o: $(SRC)/%.cpp
	@$(MD) $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -MMD $<  -o $@
%.o: %.c
	@$(MD) $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -MMD $< -o $@

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)*
	$(RM) $(call FIXPATH,$(OBJECTS))
	$(RM) $(call FIXPATH,$(DEPS))
	@echo Cleanup complete!

run: all 
	./$(OUTPUTMAIN)



