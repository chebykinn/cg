# Ultimate Makefile v3.0.0 (C) 2017 Ivan Chebykin
# NOTE: GNU Make only
-include Makefile.config
# Variables--------------------------------------------------------------------
ARCH?=
PROJECT?=bsp-loader
VERSION?=v0.0.1
DEBUG?=true
VERBOSE?=

# TODO: examples
# Supported: project, test
BUILD_TYPE?=project
# Supported: bin, dylib
PROJECT_TYPE?=bin

EXCLUDE_LANGUAGES?=

ifndef CC
CC=gcc
endif
ifndef CXX
CXX=g++
endif

LIBS?=
TEST_LIBS?=
DEBUG_LIBS?=
RELEASE_LIBS?=

# Flags
PROJECT_CFLAGS?=
PROJECT_CXXFLAGS?=-std=c++14 -Wextra -D_DEBUG -fsanitize=address -DSPDLOG_TRACE_ON -DSPDLOG_DEBUG_ON -DGLM_FORCE_CXX11 -DGLM_FORCE_SWIZZLE $(shell pkg-config --cflags gl sdl2 glu SDL2_image)
PROJECT_LDFLAGS?=$(shell pkg-config --libs gl glu glew sdl2 SDL2_image)

TEST_CFLAGS?=
TEST_LDFLAGS?=

DEBUG_CFLAGS?=
DEBUG_LDFLAGS?=

RELEASE_CFLAGS?=
RELEASE_LDFLAGS?=

EXCLUDE_FILES?=
TEST_EXCLUDE_FILES?=

ifdef DEBUG
PROJECT_STATUS=debug
else
PROJECT_STATUS=release
endif

# Directories------------------------------------------------------------------
SRCDIR?=src
OBJDIR?=obj
TESTDIR?=tests
BUILDROOT?=target
# Directory for build artifacts
BUILDARTDIR?=release
ifdef DEBUG
    BUILDARTDIR=debug
endif
MKINCLUDEDIR?=mk
TARGETSDIR?=targets
LANGUAGESDIR?=languages
EXTENSIONSDIR?=extensions
ifndef INCLUDEDIRS
    INCLUDEDIRS:=include
endif
INCLUDEDIRS+=lib/glm
LIBDIRS:=
TEST_LIBDIRS?=
DEBUG_LIBDIRS?=
RELEASE_LIBDIRS?=

# Tools------------------------------------------------------------------------
# Remote host configuration
SCP?=scp
SCPFLAGS?=
REMOTE_HOST?=
REMOTE_PATH?=

# GDB configuration
GDB?=gdb
GDBARGS?=

# Formatter configuration
FMT?=clang-format
FMTFLAGS?=-i

DOC?=doxygen
DOCFLAGS?=Doxyfile

ENABLE_EXTENSIONS=true

# Install environment
INSTALL?=install
CP?=cp
DESTDIR?=
BINDIR?=/usr/bin
LIBDIR?=/usr/lib
INCLUDEDIR?=/usr/include
SYSCONF?=/etc

INSTALL_INCLUDES?=$(firstword $(INCLUDEDIRS))

ENABLED_LANGUAGES := c cpp

# Everything after this comment is ultimake internals

# Ultimake public constants
ULTIMAKE_VERSION := 3.0

# Ultimake config variables
ULTIMAKE_LOG_LEVEL                  ?= warn
ULTIMAKE_FORMATTED_PROGRESS_ENABLED ?= true
ULTIMAKE_COLOR_PROGRESS_ENABLED     ?= true
ULTIMAKE_IS_LOG_ENABLED             ?= true
ifndef VERBOSE
ULTIMAKE_HIDE_PROGRESS              ?= true
endif


# Support variables
um_space=
um_space+=

ifdef ULTIMAKE_COLOR_PROGRESS_ENABLED
    um_color_green  := \e[32m
    um_color_yellow := \e[33m
    um_text_normal  := \e[0m
    um_text_bold    := \e[1m
    um_msg_info     := $(um_color_green)$(um_text_bold)
    um_msg_progress := $(um_color_yellow)$(um_text_bold)
endif

um_log_prefix = ultimake:
ifdef DEBUG
    um_build_target := debug
else
    um_build_target := release
endif

um_is_library :=
ifeq ($(PROJECT_TYPE),dylib)
    um_is_library := true
endif
ifeq ($(PROJECT_TYPE),staticlib)
    um_is_library := true
endif

# Library functions
um_count_words = $(shell echo "$(1)" | wc -w)
um_add         = $(shell echo $$(( $(1) + $(2) )))
um_inc         = $(call um_add,$(1),1)

um_prefix_words   = $(2)$(subst $(um_space),$(um_space)$(2),$(1))
um_append_words   = $(subst $(um_space),$(2)$(um_space),$(1))$(2)
um_log_fn_enabled = $(filter $(1), $(ULTIMAKE_LOG_LEVEL))

um_has_flag = $(filter $(1), $(2))

define um_add_flag_impl =
ifeq ($(call um_has_flag,$(1),$($(2))),)
    override $(2) += $(1)
endif
endef

um_add_flag = $(eval $(call um_add_flag_impl,$(1),$(2)))

um_locate_files = $(shell find -L $(1) -type f -name "*.$(2)" $(3))

um_print_var = $(info $(1) = "$($(1))")

um_print_obj = $(subst $(objpath)/,,$(1))

# args: compiler, input_file, output_file, total_count, build_counter
define um_print_progress =
	@echo -e "$(um_msg_progress)[$(5)/$(4)]$(um_text_normal) $(1) $(call um_print_obj,$(3))"
endef

# args: compiler, input_files, output_file
define um_print_link_progress =
	@echo -e "$(um_msg_info)[LINK]$(um_text_normal) $(1) $(3)"
endef

define um_ld_library_path =
LD_LIBRARY_PATH=$(subst $(um_space),:,$(strip $(LIBDIRS)))
endef

ifdef ULTIMAKE_DEBUG_ENABLED
$(info Ultimake $(ULTIMAKE_VERSION))
$(info Debug is enabled)
ULTIMAKE_IS_LOG_ENABLED := true
ULTIMAKE_LOG_LEVEL      := debg

endif

ifdef ULTIMAKE_IS_LOG_ENABLED

ifneq ($(call um_log_fn_enabled,debg),)
ULTIMAKE_LOG_LEVEL += warn
endif
ifneq ($(call um_log_fn_enabled,warn),)
ULTIMAKE_LOG_LEVEL += erro
endif
ifneq ($(call um_log_fn_enabled,erro),)
ULTIMAKE_LOG_LEVEL += crit
endif

ifneq ($(call um_log_fn_enabled,debg),)
debg = $(warning $(um_log_prefix) DEBG: $(1))
endif
ifneq ($(call um_log_fn_enabled,warn),)
warn = $(warning $(um_log_prefix) WARN: $(1))
endif
ifneq ($(call um_log_fn_enabled,erro),)
erro = $(warning $(um_log_prefix) ERRO: $(1))
endif
ifneq ($(call um_log_fn_enabled,crit),)
crit = $(error $(um_log_prefix) CRIT: $(1))
endif

endif


ifndef PROJECT
    $(call crit,PROJECT is not defined)
endif
ifndef PROJECT_TYPE
    $(call crit,PROJECT_TYPE is not defined)
endif
ifndef BUILD_TYPE
    $(call crit,BUILD_TYPE is not defined)
endif

ifndef OBJECTS
    OBJECTS:=
else
ifeq ($(flavor OBJECTS),recursive)
    $(call crit,OBJECTS should be assigned with := operator)
endif
endif

ifeq ($(PROJECT_TYPE),dylib)
ifneq ($(BUILD_TYPE),test)
    $(call um_add_flag,-shared,LDFLAGS)
endif
endif

host_arch := $(shell uname -m)
ifndef ARCH
    ARCH := $(host_arch)
endif

LIBEXT  ?= so
SLIBEXT ?= a

# Check target
build_target_arch := native
ifneq ($(ARCH),$(host_arch))
    build_target_arch := cross
endif

# Target-specific variables
PREFIX           ?=
TOOLCHAIN_PATH   ?=
BIN_PATH         ?=

outname_bin       = $(PROJECT)
outname_dylib     = lib$(outname_bin).$(LIBEXT)
outname_staticlib = lib$(outname_bin).$(SLIBEXT)

outname           = $(outname_$(PROJECT_TYPE))

# Select build directory for currect arch
ifneq ($(build_target_arch),native)
    BUILDROOT := $(BUILDROOT)/$(ARCH)
else
    BUILDROOT := $(BUILDROOT)
endif
build_dir := $(BUILDROOT)/$(BUILDARTDIR)

objpath   := $(build_dir)/$(OBJDIR)

TARGET     = $(build_dir)/$(outname)

# Check if building tests
ifeq ($(BUILD_TYPE),test)
    ifdef TEST_LIBDIRS
        LIBDIRS += $(TEST_LIBDIRS)
    endif

    ifdef TEST_LIBS
        LIBS += $(TEST_LIBS)
    endif

    ifeq ($(PROJECT_TYPE), dylib)
        LIBDIRS += $(build_dir)
        LIBS    += $(PROJECT)
    endif
    SRCDIR       := $(TESTDIR)
    CFLAGS        = $(TEST_CFLAGS)
    LDFLAGS       = $(TEST_LDFLAGS)
    EXCLUDE_FILES = $(TEST_EXCLUDE_FILES)
    build_dir    := $(build_dir)/test
    TARGET        = $(build_dir)/$(outname)-test
endif

# Add trailing dash to prefix
ifdef PREFIX
    PREFIX := $(PREFIX)-
endif
# Add trailing slash to compiler path
ifdef BIN_PATH
    BIN_PATH := $(BIN_PATH)/
endif

override CC  := $(BIN_PATH)$(PREFIX)$(CC)
override CXX := $(BIN_PATH)$(PREFIX)$(CXX)

# Add linker flags
LIBDIRS     += $(LIBDIRS_$(um_build_target))
LIBS        += $(LIBS_$(um_build_target))
LIBDIRS     := $(strip $(LIBDIRS))
LIBS        := $(strip $(LIBS))

ifdef LIBDIRS
    lib_dirs_flags := $(call um_prefix_words,$(LIBDIRS),-L)
    override LDFLAGS += $(lib_dirs_flags)
endif

ifdef LIBS
    libs_flags := $(call um_prefix_words,$(LIBS),-l)
    override LDFLAGS += $(libs_flags)
endif

override LDFLAGS := $(strip $(LDFLAGS))

LINK_CC := $(CC)

ifdef DEBUG
    flags_prefix := DEBUG
else
    flags_prefix := RELEASE
endif

ifdef $(flags_prefix)_CFLAGS
override CFLAGS   += $($(flags_prefix)_CFLAGS)
endif
ifdef $(flags_prefix)_CXXFLAGS
override CXXFLAGS += $($(flags_prefix)_CXXFLAGS)
endif
ifdef $(flags_prefix)_LDFLAGS
override LDFLAGS  += $($(flags_prefix)_LDFLAGS)
endif

ifdef PROJECT_CFLAGS
override CFLAGS   += $(PROJECT_CFLAGS)
endif
ifdef PROJECT_CXXFLAGS
override CXXFLAGS += $(PROJECT_CXXFLAGS)
endif
ifdef PROJECT_LDFLAGS
override LDFLAGS  += $(PROJECT_LDFLAGS)
endif

ifneq ($(EXCLUDE_FILES),)
    TMP_EXCLUDE  := $(strip $(EXCLUDE_FILES))
    exclude_opts := $(call um_prefix_words,$(TMP_EXCLUDE),! -path ./)
endif


# Define first target
all: $(TARGET)

# Prepare list of language includes
lang_dir := $(MKINCLUDEDIR)/$(LANGUAGESDIR)/
languages_list := $(filter-out $(EXCLUDE_LANGUAGES),$(ENABLED_LANGUAGES))
languages_list := $(call um_prefix_words,$(languages_list),$(lang_dir))
languages_list := $(call um_append_words,$(languages_list),.mk)

SRCEXT_c ?= c
INCEXT_c ?= h
OBJEXT_c ?= o

SRCDIR_c ?= $(SRCDIR)
OBJDIR_c ?= $(OBJDIR)

CFLAGS   ?= -std=c99 -Wall

flags_var_c := CFLAGS
cc_var_c    := CC
c_in        := c
std_var_c   := c99

ifndef c_in
	$(call crit,No c_in defined, c-common shouldn\'t be included directly)
endif
ifndef flags_var_$(c_in)
	$(call crit,No flags_var_$(c_in) defined)
endif
ifndef cc_var_$(c_in)
	$(call crit,No cc_var_$(c_in) defined)
endif
ifndef std_var_$(c_in)
	$(call crit,No std_var_$(c_in) defined)
endif
# Add default flags

ifeq ($(call um_has_flag,-std=%,$(CFLAGS)),)
    $(call um_add_flag,-std=$(std_var_$(c_in)),CFLAGS)
endif

$(call um_add_flag,-Wall,$(flags_var_$(c_in)))

$(call um_add_flag,-fPIC,$(flags_var_$(c_in)))

ifndef DEBUG
    opt_lvl_$(c_in) := 3
else
    $(call um_add_flag,-g,$(flags_var_$(c_in)))
    opt_lvl_$(c_in) := 0
endif
ifeq ($(call um_has_flag,-O%,$($(flags_var_$(c_in)))),)
    $(call um_add_flag,-O$(opt_lvl_$(c_in)),$(flags_var_$(c_in)))
endif

$(call um_add_flag,-D_VERSION=$(VERSION), $(flags_var_$(c_in)))

# Prepare flags variable
INCLUDEDIRS_$(c_in) += $(INCLUDEDIRS)

SRCDIR_$(c_in)      := $(strip $(SRCDIR_$(c_in)))
INCLUDEDIRS_$(c_in) := $(strip $(INCLUDEDIRS_$(c_in)))

ifdef INCLUDEDIRS_$(c_in)
    include_flags_$(c_in) := $(call um_prefix_words,$(INCLUDEDIRS_$(c_in)),-I)
    override $(flags_var_$(c_in))  += $(include_flags_$(c_in))
endif

override $(flags_var_$(c_in))  := $(strip $($(flags_var_$(c_in))))

ifndef SOURCES_$(c_in)
FIND_SOURCES_$(c_in) := true
endif

ifdef FIND_SOURCES_$(c_in)
locate_fn_$(c_in) = $(call um_locate_files,$(1),$(2),$(exclude_opts))

ifdef FIND_HEADERS_$(c_in)
    HEADERS_$(c_in) := $(call locate_fn_$(c_in),$(INCLUDEDIRS_$(c_in)),$(INCEXT_$(c_in)))
endif
    SOURCES_$(c_in) := $(call locate_fn_$(c_in),$(SRCDIR_$(c_in)),$(SRCEXT_$(c_in)))
endif

# TODO: fix rules
replace_pattern = $(subst $(um_space),\|,$(SRCDIR_$(c_in)))

ifneq ($(SOURCES_$(c_in)),)

objects_list := $(SOURCES_$(c_in):.$(SRCEXT_$(c_in))=.$(SRCEXT_$(c_in)).$(OBJEXT_$(c_in)))
OBJECTS_$(c_in) := $(shell echo "$(objects_list)" | sed "s@$(replace_pattern)@$(objpath)@g")

build_counter_$(c_in) := 1
objects_count_$(c_in) := $(call um_count_words,$(OBJECTS_$(c_in)))

OBJECTS += $(OBJECTS_$(c_in))
OBJECTS := $(strip $(OBJECTS))
endif


ifneq ($(SOURCES_$(c_in)),)

$(objpath)/%.$(SRCEXT_c).$(OBJEXT_c): $(SRCDIR_c)/%.$(SRCEXT_c)
	@mkdir -p $(dir $@)
ifdef ULTIMAKE_FORMATTED_PROGRESS_ENABLED
ifndef ULTIMAKE_HIDE_PROGRESS
	@if [ "$(build_counter_c)" -eq 1 ]; then                                   \
	echo "CC     = $(CC)";                                                     \
	echo "CFLAGS = $(CFLAGS)";                                                 \
	fi

	$(call um_print_progress,$(CC),$<,$@,$(objects_count_c),$(build_counter_c))
	$(eval build_counter_c := $(call um_inc,$(build_counter_c)))
endif
	@$(CC) $(CFLAGS) -c $< -o $@
else
	$(CC) $(CFLAGS) -c $< -o $@
endif

endif
SRCEXT_cpp ?= cpp
INCEXT_cpp ?= h
OBJEXT_cpp ?= o

SRCDIR_cpp ?= $(SRCDIR)
OBJDIR_cpp ?= $(OBJDIR)

CXXFLAGS   ?= -std=c++11 -Wall

flags_var_cpp := CXXFLAGS
cc_var_cpp    := CXX
c_in          := cpp
std_var_cpp   := c++14

ifndef c_in
	$(call crit,No c_in defined, c-common shouldn\'t be included directly)
endif
ifndef flags_var_$(c_in)
	$(call crit,No flags_var_$(c_in) defined)
endif
ifndef cc_var_$(c_in)
	$(call crit,No cc_var_$(c_in) defined)
endif
ifndef std_var_$(c_in)
	$(call crit,No std_var_$(c_in) defined)
endif
# Add default flags

ifeq ($(call um_has_flag,-std=%,$(CFLAGS)),)
    $(call um_add_flag,-std=$(std_var_$(c_in)),CFLAGS)
endif

$(call um_add_flag,-Wall,$(flags_var_$(c_in)))

$(call um_add_flag,-fPIC,$(flags_var_$(c_in)))

ifndef DEBUG
    opt_lvl_$(c_in) := 3
else
    $(call um_add_flag,-g,$(flags_var_$(c_in)))
    opt_lvl_$(c_in) := 0
endif
ifeq ($(call um_has_flag,-O%,$($(flags_var_$(c_in)))),)
    $(call um_add_flag,-O$(opt_lvl_$(c_in)),$(flags_var_$(c_in)))
endif

$(call um_add_flag,-D_VERSION=$(VERSION), $(flags_var_$(c_in)))

# Prepare flags variable
INCLUDEDIRS_$(c_in) += $(INCLUDEDIRS)

SRCDIR_$(c_in)      := $(strip $(SRCDIR_$(c_in)))
INCLUDEDIRS_$(c_in) := $(strip $(INCLUDEDIRS_$(c_in)))

ifdef INCLUDEDIRS_$(c_in)
    include_flags_$(c_in) := $(call um_prefix_words,$(INCLUDEDIRS_$(c_in)),-I)
    override $(flags_var_$(c_in))  += $(include_flags_$(c_in))
endif

override $(flags_var_$(c_in))  := $(strip $($(flags_var_$(c_in))))

ifndef SOURCES_$(c_in)
FIND_SOURCES_$(c_in) := true
endif

ifdef FIND_SOURCES_$(c_in)
locate_fn_$(c_in) = $(call um_locate_files,$(1),$(2),$(exclude_opts))

ifdef FIND_HEADERS_$(c_in)
    HEADERS_$(c_in) := $(call locate_fn_$(c_in),$(INCLUDEDIRS_$(c_in)),$(INCEXT_$(c_in)))
endif
    SOURCES_$(c_in) := $(call locate_fn_$(c_in),$(SRCDIR_$(c_in)),$(SRCEXT_$(c_in)))
endif

# TODO: fix rules
replace_pattern = $(subst $(um_space),\|,$(SRCDIR_$(c_in)))

ifneq ($(SOURCES_$(c_in)),)

objects_list := $(SOURCES_$(c_in):.$(SRCEXT_$(c_in))=.$(SRCEXT_$(c_in)).$(OBJEXT_$(c_in)))
OBJECTS_$(c_in) := $(shell echo "$(objects_list)" | sed "s@$(replace_pattern)@$(objpath)@g")

build_counter_$(c_in) := 1
objects_count_$(c_in) := $(call um_count_words,$(OBJECTS_$(c_in)))

OBJECTS += $(OBJECTS_$(c_in))
OBJECTS := $(strip $(OBJECTS))
endif


ifneq ($(SOURCES_$(c_in)),)

$(objpath)/%.$(SRCEXT_cpp).$(OBJEXT_cpp): $(SRCDIR_cpp)/%.$(SRCEXT_cpp)
	@mkdir -p $(dir $@)
ifdef ULTIMAKE_FORMATTED_PROGRESS_ENABLED
ifndef ULTIMAKE_HIDE_PROGRESS
	@if [ "$(build_counter_cpp)" -eq 1 ]; then                                 \
	echo "CXX      = $(CXX)";                                                  \
	echo "CXXFLAGS = $(CXXFLAGS)";                                             \
	fi

	$(call um_print_progress,$(CXX),$<,$@,$(objects_count_cpp),$(build_counter_cpp))
	$(eval build_counter_cpp := $(call um_inc,$(build_counter_cpp)))
endif
	@$(CXX) $(CXXFLAGS) -c $< -o $@
else
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif

LINK_CC := $(CXX)
endif


ifdef ULTIMAKE_FORMATTED_PROGRESS_ENABLED
    $(info $(shell echo -e "$(um_msg_info)[BUILD]$(um_text_normal) $(PROJECT) $(VERSION)"))
endif

ifdef ULTIMAKE_DEBUG_ENABLED
    $(call um_print_var,OBJECTS)
endif

# Include target-specific variables
-include $(MKINCLUDEDIR)/$(TARGETSDIR)/$(ARCH).mk

# Rules-------------------------------------------------------------------------
$(PROJECT): $(TARGET)

objects: $(OBJECTS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(build_dir)/$(OBJDIR)
ifdef ULTIMAKE_FORMATTED_PROGRESS_ENABLED
ifndef ULTIMAKE_HIDE_PROGRESS
	$(info OBJECTS = $(call um_print_obj,$(OBJECTS)))
	$(info LDFLAGS = $(LDFLAGS))
endif
	$(call um_print_link_progress,$(LINK_CC),$^,$@)
	@$(LINK_CC) $^ -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)
else
	$(LINK_CC) $^ -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)
endif

clean:
	rm -rf $(BUILDROOT)

cleanobj:
	rm -rf $(objpath)

cleantest:
	rm -rf $(BUILDROOT)/test

remake: clean all

test: $(TARGET)
	LD_LIBRARY_PATH=$(build_dir) $(MAKE) \
	BUILD_TYPE=test run

run: $(TARGET)
	$(call um_ld_library_path) ./$(TARGET) $(ARGS)

debug: $(TARGET)
	$(call um_ld_library_path) $(GDB) $(GDBARGS) --args ./$(TARGET) $(ARGS)

fmt: c_fmt cpp_fmt

doc: $(TARGET)
	$(DOC) $(DOCFLAGS)

ifneq ($(SCP),)
copy-remote: $(TARGET)
		$(SCP) $(SCPFLAGS) $(TARGET) $(REMOTE_HOST):$(REMOTE_PATH)
endif

version:
	@echo $(VERSION)

install: $(TARGET)
ifeq ($(PROJECT_TYPE),bin)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)
endif
ifdef um_is_library
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
ifdef INSTALL_INCLUDES
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	@$(CP) -vr $(INSTALL_INCLUDES)/* $(DESTDIR)$(INCLUDEDIR)
endif
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(LIBDIR)
endif

.PHONY: all clean cleanobj remake run fmt version install


ifdef ENABLE_EXTENSIONS
    -include $(MKINCLUDEDIR)/$(EXTENSIONSDIR)/*/Makefile.include
endif
