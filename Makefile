BINARY_NAME = aether
VERSION = 2.0.0
MAJOR = 1
MINOR = 0.1

DIR=$(CURDIR)
SRC_DIR = $(DIR)/src
OBJ_DIR = $(DIR)/obj
TARGET_DIR = $(DIR)/bin
DATA_DIR = $(DIR)/data

DEBUG_DIR = debug
RELEASE_DIR = release

DEBUG_OBJ_DIR = $(OBJ_DIR)/$(DEBUG_DIR)
RELEASE_OBJ_DIR = $(OBJ_DIR)/$(RELEASE_DIR)

DEBUG_TARGET_DIR = $(TARGET_DIR)/$(DEBUG_DIR)
RELEASE_TARGET_DIR = $(TARGET_DIR)/$(RELEASE_DIR)

DEBUG_BINARY_NAME = $(DEBUG_TARGET_DIR)/$(BINARY_NAME)
RELEASE_BINARY_NAME = $(RELEASE_TARGET_DIR)/$(BINARY_NAME)

GCC = g++
CPP_STD = c++20

PREPROCESSOR_DEFINITIONS += -DAETHER_HOME=\"$(DATA_DIR)\"

rec_wildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rec_wildcard,$d,$2)$(filter $(subst *,%,$2),$d))

INC_FILES := $(call rec_wildcard,$(SRC_DIR),*.hpp)
SRC_FILES := $(call rec_wildcard,$(SRC_DIR),*.cpp)

DEBUG_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(DEBUG_OBJ_DIR)/%.o,$(SRC_FILES))
DEBUG_OBJ_DIRS := $(sort $(dir $(DEBUG_OBJ_FILES)))
RELEASE_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(RELEASE_OBJ_DIR)/%.o,$(SRC_FILES))
RELEASE_OBJ_DIRS := $(sort $(dir $(RELEASE_OBJ_FILES)))

CPP_LIBRARY_ROOT = /opt/homebrew/Cellar
BOOST_ROOT = $(CPP_LIBRARY_ROOT)/boost/1.80.0
OPENSSL_ROOT = $(CPP_LIBRARY_ROOT)/openssl@3/3.0.7
ZLIB_ROOT = $(CPP_LIBRARY_ROOT)/zlib/1.2.13

INCLUDES += -I$(SRC_DIR)
INCLUDES += -I$(BOOST_ROOT)/include
INCLUDES += -I$(OPENSSL_ROOT)/include
INCLUDES += -I$(ZLIB_ROOT)/include

LIBS += -L$(BOOST_ROOT)/lib
LIBS += -L$(OPENSSL_ROOT)/lib
LIBS += -L$(ZLIB_ROOT)/lib
LIBS += -lssl
LIBS += -lcrypto
LIBS += -lboost_system
LIBS += -lpthread
LIBS += -lz

WARNING_FLAGS += -W
WARNING_FLAGS += -Wall
WARNING_FLAGS += -Wno-missing-field-initializers
WARNING_FLAGS += -Wno-unused-parameter

CXX_FLAGS = -std=$(CPP_STD) $(WARNING_FLAGS) $(PREPROCESSOR_DEFINITIONS) $(INCLUDES)
LD_FLAGS = 

.PHONY: all clean debug prep release remake

all: prep release

debug: CXX_FLAGS += -g -O0
debug: PREPROCESSOR_DEFINITIONS += -DDEBUG
debug: $(DEBUG_BINARY_NAME)

release: CXX_FLAGS += -O1
release: PREPROCESSOR_DEFINITIONS += -DNDEBUG
release: $(RELEASE_BINARY_NAME)

$(DEBUG_BINARY_NAME): $(DEBUG_OBJ_FILES)
	$(GCC) $(LD_FLAGS) -o $@ $^ $(LIBS)

$(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(GCC) $(CXX_FLAGS) -MMD -MP -c -o $@ $<

$(RELEASE_BINARY_NAME): $(RELEASE_OBJ_FILES)
	$(GCC) $(LD_FLAGS) -o $@ $^ $(LIBS)

$(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(GCC) $(CXX_FLAGS) -MMD -MP -c -o $@ $<

$(DEBUG_TARGET_DIR) $(DEBUG_OBJ_DIR) $(RELEASE_TARGET_DIR) $(RELEASE_OBJ_DIR) $(DATA_DIR):
	mkdir -p $@

remake: clean all

prep:
	mkdir -p $(DEBUG_TARGET_DIR) $(RELEASE_TARGET_DIR)

clean:
	rm -f $(DEBUG_BINARY) $(RELEASE_BINARY) $(DEBUG_OBJ_FILES) $(RELEASE_OBJ_FILES)
