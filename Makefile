#UNAME := $(shell uname)

ifeq ($(OS),Windows_NT) #making sure that you can make the files properly on windowsgit
MKDIR_P = cmd.exe /c mkdir
BUILD_DIR ?= bin
SRC_DIRS ?= src lib
#MKDIR_P = mkdir
else
MKDIR_P = mkdir -p
BUILD_DIR ?= ./bin
SRC_DIRS ?= ./src ./lib
endif

TARGET_EXEC ?= telemetrycore 


C_V ?= 17
CPP_V ?= 17




ifeq ($(OS),Windows_NT)
# Exclude serial osx 		note: can't we just wrap serialosx header in an ifdef __APPLE__? -jus
SRCS := $(shell find $(SRC_DIRS) -type f \( -name "*.cc" -or -name "*.c" -or -name "*.s" \) ! -path "*/lib/serialosx/*") #wrapped all serialosx files in an #ifdef __APPLE__ statement
else
SRCS := $(shell find $(SRC_DIRS) -name *.cc -or -name *.c -or -name *.s)
endif

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

ifeq ($(OS),Windows_NT)
INC_DIRS := $(SRC_DIRS) $(shell powershell -Command "Get-ChildItem -Path ./lib -Recurse -Directory | Select-Object -ExpandProperty FullName")
	# Define the include directories (INC_DIRS) by searching for all subdirectories within the specified source directories.

else
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
endif

INC_FLAGS := $(addprefix -I,$(INC_DIRS)) 

CFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c$(C_V) -g  -Wno-format-security -Wall -Wextra -pedantic-errors -Weffc++ -Wno-unused-parameter
# Adding _WEBSOCKETPP_MINGW_THREAD_ to the CFLAGS to make it deffined in the build and make windows stick with it 
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c++$(CPP_V) -g -O3 -Wno-format-security -Wall -Wextra -pedantic-errors -Weffc++ -Wno-unused-parameter -D_WEBSOCKETPP_CPP11_THREAD_
CXXFLAGS += -Wno-effc++ -Wno-template-id-cdtor #only show errors and remove warnings for now, delete when done
#ifeq ($(UNAME),MINGW64_NT-10.0-19043)
	#LDFALGS ?= -L/c/MinGW/msys/1.0/lib/libdl.a
#else
#ifeq ($(UNAME),MINGW32_NT-6.2)
	#LDFLAGS ?= -L/lib/libdl.a
#else
#LDFLAGS ?= -ldl 

#endif
#endif

ifeq ($(OS),Windows_NT)
    LDFLAGS := -lws2_32 -lmswsock -Llib -lftd2xx
	# taking out ldl and adding winsock2 library
else
    LDFLAGS := -ldl
endif

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) 

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	#-$(MKDIR_P) $(subst /,\,$(dir $@))
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	#-$(MKDIR_P) $(subst /,\,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cc.o: %.cc
	$(MKDIR_P) $(dir $@)
	# -$(MKDIR_P) $(subst /,\,$(dir $@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

#clean:
	#$(RM) -r $(BUILD_DIR)

ifeq ($(OS),Windows_NT)
clean:
		cmd.exe /c if exist $(subst /,\,$(BUILD_DIR)) rmdir /s /q $(subst /,\,$(BUILD_DIR))
else
clean:	
		$(RM) -r $(BUILD_DIR)
endif

-include $(DEPS)

