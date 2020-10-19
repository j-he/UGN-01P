# Project Name
TARGET = lfsr

# Sources
CPP_SOURCES = lfsr.cpp

# Library Locations
LIBDAISY_DIR = /Users/jhe1/sdks/DaisyExamples/libdaisy
DAISYSP_DIR = /Users/jhe1/sdks/DaisyExamples/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
