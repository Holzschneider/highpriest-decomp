# Builds the Ragnarok Online HighPriest client (kRO, 2008-11-05) from src/.
#
#   make          build build/HighPriest.exe
#   make check    build it and compare its SHA-256 with the original's
#   make clean    remove build/
#
# Linux x86-64. The compilers, the assembler and the librarian run under the
# bundled wibo; LINK.EXE and RC.EXE run under the system's Wine, and
# tools/stamp.py needs Python 3.

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c
.DELETE_ON_ERROR:
.SUFFIXES:
ifeq ($(filter -j%,$(MAKEFLAGS)),)
MAKEFLAGS += -j$(shell nproc)
endif

ROOT  := $(CURDIR)
BUILD := build
OBJ   := $(BUILD)/obj

# SHA-256 of the original HighPriest.exe.
ORIGINAL := 9abc36c88376387d85c878c0c879d9af8ef66eb1f5152638085dc6995e3f4742

# The tools see this time (Unix seconds) instead of the clock, and no
# environment but this one.
CLOCK := 1790492747
ENV   := env -i HOME="$(HOME)" PATH="$(PATH)" TMP=. TEMP=. LANG=C LC_ALL=C TZ=UTC
WIBO  := $(ENV) $(ROOT)/tools/bin/wibo --fixed-time $(CLOCK)
WINE  := $(ENV) WINEDEBUG=-all wine

COMPILER  := tools/msvc6/Bin/CL.EXE
CL_8047   := tools/msvc6_8047/Bin/CL.EXE
CL_8943   := tools/msvc6_8943/Bin/CL.EXE
CL_DDK    := tools/ddk2266/Bin/CL.EXE
ASSEMBLER := tools/masm613/ml.exe
LIBRARIAN := $(WIBO) $(ROOT)/tools/msvc6/Bin/LINK.EXE -lib /NOLOGO /MACHINE:IX86

objects = $(addprefix $(OBJ)/,$(addsuffix .obj,$(1)))
member  = $(or $(MEMBER.$(1)),$(notdir $(1)))

include sources.mk

.PHONY: all check clean
all: $(BUILD)/HighPriest.exe
check: $(BUILD)/HighPriest.exe
	@echo "$(ORIGINAL)  $<" | sha256sum --check
clean:
	rm -rf $(BUILD)

# Compiling. Each source compiles in a directory of its own that links to the
# trees it reads, so parallel compiles do not share temporary files. The
# compilers order some symbols and sections by the length of that directory's
# path, so its name is padded until the path has STAGE_LENGTH characters.
INPUTS := $(shell find src vendor -type f) Makefile sources.mk
STAGE_LENGTH := 200

define enter_stage
stage="$(ROOT)/$(BUILD)/stage/$*"; rm -rf "$$stage"; \
pad=$$(( $(STAGE_LENGTH) - $$(printf %s "$$stage" | wc -c) - 1 )); \
if [ $$pad -lt 1 ]; then \
  echo "$<: compiles in a directory whose path has $(STAGE_LENGTH) characters; move the checkout to a shorter path" >&2; exit 1; \
fi; \
stage="$$stage/$$(printf '%*s' $$pad '' | tr ' ' _)"; \
mkdir -p "$$stage" $(@D); ln -s $(ROOT)/src $(ROOT)/tools $(ROOT)/vendor "$$stage"; cd "$$stage"
endef

compile = $(WIBO) $(COMPILER) $(BASE) $(EXTRA) $(INCLUDES) $(PCH)

define compile_c
@echo "  CL    $<"
@$(enter_stage); \
$(if $(PCH_READ),$(compile) /Fopch-create.obj $< > log 2>&1 || { cat log; exit 1; };) \
$(compile) /Fooutput.obj $< > log 2>&1 || { cat log; exit 1; }; \
mv output.obj $(ROOT)/$@
endef

$(OBJ)/%.obj: src/%.cpp $(INPUTS)
	$(compile_c)
$(OBJ)/%.obj: src/%.c $(INPUTS)
	$(compile_c)
$(OBJ)/%.obj: src/%.asm $(INPUTS)
	@echo "  ML    $<"
	@$(enter_stage); \
	$(WIBO) $(ASSEMBLER) $(ASFLAGS) $(ASEXTRA) /c /Fooutput.obj $< > log 2>&1 || { cat log; exit 1; }; \
	mv output.obj $(ROOT)/$@

# The original project compiled through an automatic precompiled header of
# common.h. Four objects show whether their compile made the header or read it.
PCH_FLAGS := /YXcommon.h /Fpcompile.pch
$(call objects,3dlib/Device resource/Action telexInputMode base/Util): PCH := $(PCH_FLAGS)
$(call objects,base/Util): PCH_READ := yes

# Three units of the C++ library have the stock members' section order at
# these lengths only.
$(call objects,library/msvc6/locale): STAGE_LENGTH := 212
$(call objects,library/msvc6/strstrea): STAGE_LENGTH := 229
$(call objects,library/msvc6/xlocale): STAGE_LENGTH := 239

# Import libraries of the DLLs that came without one. An export list in C
# gives the librarian the DLL's own hints.
IMPORTS := binkw32 mss32 granny2 ijl15 cps
IMPORT_FORMAT := /LINK50COMPAT
$(BUILD)/imports/cps.lib: IMPORT_FORMAT :=
$(BUILD)/imports/%.lib: src/imports/%.def $(wildcard src/imports/*.c) Makefile
	@echo "  LIB   $@"
	@stage=$(BUILD)/imports/$*.stage; rm -rf $$stage; mkdir -p $$stage; cp src/imports/$*.* $$stage; cd $$stage; \
	exports=; \
	if [ -f $*.c ]; then \
	  $(WIBO) $(ROOT)/$(COMPILER) /nologo /c /Zl /X /Fo$*.obj $*.c > log 2>&1 || { cat log; exit 1; }; \
	  exports=$*.obj; \
	fi; \
	$(LIBRARIAN) $(IMPORT_FORMAT) /DEF:$*.def $$exports /OUT:$*.lib > log 2>&1 || { cat log; exit 1; }; \
	cp $*.lib $(ROOT)/$@

# Libraries the original linked statically, built from src/ with their
# members in the stock libraries' order.
# $(call archive,object stems,other inputs)
define archive
@echo "  LIB   $@"
@rm -rf $(@D); mkdir -p $(@D)
@$(foreach o,$(1),cp $(OBJ)/$(o).obj $(@D)/$(call member,$(o)).obj;)
@cd $(@D); $(3) $(LIBRARIAN) /OUT:$(@F) $(foreach o,$(1),$(call member,$(o)).obj) $(2) > log 2>&1 || { cat log; exit 1; }
endef

# One member of libcpmt.lib is the stock library's: no compile of
# src/library/msvc6/wlocale.cpp gives its sections in the stock order.
$(OBJ)/stock/wlocale.obj: tools/msvc6/Lib/LIBCPMT.LIB
	@mkdir -p $(@D)
	@cd $(@D); cp $(ROOT)/$< libcpmt.lib; \
	$(LIBRARIAN) '/EXTRACT:build\intel\mt_obj\wlocale.obj' /OUT:wlocale.obj libcpmt.lib > log 2>&1 || { cat log; exit 1; }

PROJECT_LIBS := $(foreach l,libcpmt libcmt npgamelibpro mss32 dinput dxguid,$(BUILD)/libs/$(l)/$(l).lib)
$(BUILD)/libs/libcpmt/libcpmt.lib: $(call objects,$(LIBCPMT))
	$(call archive,$(LIBCPMT))
$(BUILD)/libs/libcmt/libcmt.lib: $(call objects,$(LIBCMT))
	$(call archive,$(LIBCMT))
$(BUILD)/libs/npgamelibpro/npgamelibpro.lib: $(call objects,$(NPGAMELIBPRO))
	$(call archive,$(NPGAMELIBPRO))
$(BUILD)/libs/dxguid/dxguid.lib: $(call objects,library/DxGuid)
	$(call archive,library/DxGuid)
# The Miles import library with the one object the stock library holds.
$(BUILD)/libs/mss32/mss32.lib: $(call objects,library/mss/cleanup) $(BUILD)/imports/mss32.lib
	$(call archive,library/mss/cleanup,mss32-imports.lib,cp $(ROOT)/$(BUILD)/imports/mss32.lib mss32-imports.lib;)
# The SDK's dinput.lib with its first object from source: the other four are not linked.
DINPUT_STOCK := $(foreach n,5 4 3 2 1,'/REMOVE:obj\i386\dilib$(n).obj')
$(BUILD)/libs/dinput/dinput.lib: $(call objects,library/dinput/dilib1) tools/dx8sdk/Lib/dinput.lib
	$(call archive,library/dinput/dilib1,dinput-imports.lib,cp $(ROOT)/tools/dx8sdk/Lib/dinput.lib dinput-stock.lib; $(LIBRARIAN) /OUT:dinput-imports.lib dinput-stock.lib $(DINPUT_STOCK) > log 2>&1;)

# Resources.
$(BUILD)/res/SimpleAssert.res: $(wildcard src/res/*) Makefile
	@echo "  RC    $@"
	@rm -rf $(@D); mkdir -p $(@D); cp src/res/* $(@D); ln -s $(ROOT)/tools $(@D); cd $(@D); \
	$(WINE) tools/msvc6/Bin/RC.EXE /r /x /i tools/msvc6/MFC/Include /i tools/msvc6/Include /fo SimpleAssert.res SimpleAssert.rc > log 2>&1 || { cat log; exit 1; }

# Linking. LINK lays the image out in the reverse order of its command line.
LDFLAGS := /NOLOGO /MACHINE:IX86 /SUBSYSTEM:WINDOWS,4.0 /INCREMENTAL:NO /OPT:NOREF /FIXED \
  /BASE:0x400000 /STACK:0x100000,0x1000 /HEAP:0x100000,0x1000 /DEBUG /PDB:HighPriest.pdb
LINK_DIR := $(BUILD)/link
$(LINK_DIR)/HighPriest.exe: $(call objects,$(GAME)) $(PROJECT_LIBS) $(IMPORTS:%=$(BUILD)/imports/%.lib) $(BUILD)/res/SimpleAssert.res
	@echo "  LINK  $@"
	@rm -rf $(LINK_DIR); mkdir -p $(LINK_DIR)/project $(LINK_DIR)/imports
	@cp $(PROJECT_LIBS) $(LINK_DIR)/project; cp $(IMPORTS:%=$(BUILD)/imports/%.lib) $(LINK_DIR)/imports
	@ln -s $(ROOT)/tools $(ROOT)/$(OBJ) $(ROOT)/$(BUILD)/res $(LINK_DIR)
	@printf '"%s"\n' $(LDFLAGS) /OUT:HighPriest.exe /LIBPATH:project /LIBPATH:tools/dx8sdk/Lib \
	  /LIBPATH:tools/msvc6/Lib /LIBPATH:imports $(GAME:%=obj/%.obj) $(LIBS) \
	  res/SimpleAssert.res > $(LINK_DIR)/link.rsp
	@cd $(LINK_DIR); $(WINE) tools/msvc6/Bin/LINK.EXE @link.rsp > log 2>&1 || { cat log; exit 1; }

# The image records when it was linked and where its program database stood;
# no build repeats those, so the original's values are written over them.
LINK_TIME     := 1225857315
PDB_SIGNATURE := 0x48742D43
PDB_AGE       := 2171
PDB_PATH      := D:\mywork\ragnarok\HighPriest2008\Ragnarok___Win32_HighPriest2008_Release\HighPriest.pdb
$(BUILD)/HighPriest.exe: $(LINK_DIR)/HighPriest.exe tools/stamp.py
	@python3 tools/stamp.py $< $@ --time $(LINK_TIME) --signature $(PDB_SIGNATURE) --age $(PDB_AGE) --pdb '$(PDB_PATH)'
