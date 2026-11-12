CC      ?= cc
CFLAGS  ?= -O2 -Wall

SHELL   := bash
MAKEFLAGS += -s

LABEL_OUTPUTS := \
 +LEqu.s \
 +Internal_Jumps.s \
 +ILib_Size.s \
 +ILib_Functions.s \
 +Lib_Size.s

EDITOR_CONFIG_OUTPUTS := bin/Editor_Config.o AMOS/Tutorial/Extra_Configs/Editor_Config.Backup AMOS/AMOSPro_Editor_Config
INTERPRETER_CONFIG_OUTPUTS := bin/Interpreter_Config.o AMOS/Tutorial/Extra_Configs/Interpreter_Config.Backup AMOS/AMOSPro_Interpreter_Config
COMPILER_CONFIG_OUTPUTS := bin/Compiler_Config.o AMOS/Tutorial/Extra_Configs/Compiler_Config.Backup AMOS/AMOSPro_Compiler_Config
AMOS_INCLUDES_DEPS := +Debug.s +LibOffsets.s +Equ.s +CEqu.s +WEqu.s +LEqu.s +Music_Labels.s
ACLIB_OUTPUTS := +CLib_Labels.s +CLib_Size.s AMOS/APSystem/Compiler.Lib
ACHEADER_OUTPUTS := AMOS/APSystem/Header_CLI.Lib AMOS/APSystem/Header_Backstart.Lib
ACOMPEXT_OUTPUTS := +CompExt_Labels.s +CompExt_Size.s AMOS/APSystem/AMOSPro_Compiler.Lib
AREQUESTEXT_OUTPUTS := +Request_Labels.s +Request_Size.s AMOS/APSystem/AMOSPro_Request.Lib
MUSICEXT_OUTPUTS := +Music_Labels.s +Music_Size.s AMOS/APSystem/AMOSPro_Music.Lib
COMPACTEXT_OUTPUTS := +Compact_Labels.s +Compact_Size.s AMOS/APSystem/AMOSPro_Compact.Lib
AIOEXT_OUTPUTS := +IO_Ports_Labels.s +IO_Ports_Size.s AMOS/APSystem/AMOSPro_IOPorts.Lib


# AmigaDOS return codes: 0=OK, 5=WARN, 10=ERROR, 20=FAIL. Genam/Genim2 exit
# 5 on things like "Warning sign extended operand" even with 0 errors found,
# which vamos passes straight through - only treat >5 (ERROR/FAIL) as fatal.
# Output is piped through iconv (the tools' banners/messages are Latin-1)
# for a UTF-8 terminal; PIPESTATUS[0] (a bash-ism, hence SHELL:=bash above)
# gets vamos's exit code rather than iconv's.
define run_vamos
vamos -- $(1) | iconv -f latin1 -t utf8; st=$${PIPESTATUS[0]}; if [ $$st -gt 5 ]; then exit $$st; fi
endef

# Prints a status line, bolded/colored when stdout is a terminal that
# supports it (and NO_COLOR isn't set), plain text otherwise (a log file,
# `make | cat`, a dumb terminal, ...).
define status
if [ -t 1 ] && [ -z "$$NO_COLOR" ] && tput setaf 1 >/dev/null 2>&1; then printf '\n\033[1;36m==> %s\033[0m\n' "$(1)"; else printf '\n==> %s\n' "$(1)"; fi
endef

.PHONY: ALL

TARGETS := \
  AMOS/APSystem/AMOSPro.Lib \
  AMOS/APSystem/AMOS.library \
  AMOS/AMOSPro \
  AMOS/AMOSPro_Editor_Config \
  AMOS/AMOSPro_Interpreter_Config \
  AMOS/AMOSPro_Compiler_Config \
  AMOS/APSystem/AMOSPro_Editor \
  AMOS/APSystem/AMOSPro_Monitor \
  AMOS/APSystem/APCmp \
  AMOS/APSystem/Compiler.Lib \
  AMOS/APSystem/AMOSPro_Compiler.Lib \
  AMOS/APSystem/AMOSPro_Request.Lib \
  AMOS/APSystem/AMOSPro_Music.Lib \
  AMOS/APSystem/AMOSPro_Compact.Lib \
  AMOS/APSystem/AMOSPro_IOPorts.Lib \
  AMOS/APSystem/AMOSPro_3d.Lib \
  Instruction_List.Asc

ALL: $(TARGETS)

# --- tools ---------------------------------------------------------------
tools/%: tools/%.c tools/common.h tools/common.c
	$(CC) $(CFLAGS) -I tools -o $@ tools/common.c $<

# --- main build ------------------------------------------------------------
+Toktab_Verif.Bin: +Lib.s tools/Make_Toktable
	tools/Make_Toktable +Lib.s +Toktab_Verif.Bin

$(LABEL_OUTPUTS) &: +ILib.s +Lib.s tools/Make_Labels
	tools/Make_Labels

AMOS/APSystem/AMOSPro.Lib: +Lib.s +Lib_Size.s $(AMOS_INCLUDES_DEPS) +ILib.s $(LABEL_OUTPUTS) +Toktab_Verif.Bin
	$(call status,Assembling AMOSPro.Lib)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genam FROM +lib.s TO AMOS/APSystem/AMOSPro.Lib)

AMOS/APSystem/AMOS.library: +W.s $(AMOS_INCLUDES_DEPS) bin/+WFont.bin bin/ReqPic.bin bin/+AMOSPro_Mouse.abk
	$(call status,Assembling AMOS.library)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genim2 +W.s -oAMOS/APSystem/AMOS.library -C)

AMOS/AMOSPro: +B.s +Version.s +Edit.s +Monitor.s +Internal_Jumps.s +Verif.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling AMOSPro)
	mkdir -p AMOS
	$(call run_vamos,c/Genam FROM +B.s TO AMOS/AMOSPro)

$(EDITOR_CONFIG_OUTPUTS) &: tools/Get_Chunk +Editor_Config.s
	$(call status,Assembling the Editor Configuration)
	$(call run_vamos,c/Genim2 +Editor_Config.s -obin/Editor_Config.o)
	mkdir -p AMOS/Tutorial/Extra_Configs
	tools/Get_Chunk bin/Editor_Config.o AMOS/Tutorial/Extra_Configs/Editor_Config.Backup
	cp AMOS/Tutorial/Extra_Configs/Editor_Config.Backup AMOS/AMOSPro_Editor_Config

$(INTERPRETER_CONFIG_OUTPUTS) &: tools/Get_Chunk +Interpreter_Config.s
	$(call status,Assembling the Interpreter Configuration)
	$(call run_vamos,c/Genim2 +Interpreter_Config.s -obin/Interpreter_Config.o)
	mkdir -p AMOS/Tutorial/Extra_Configs
	tools/Get_Chunk bin/Interpreter_Config.o AMOS/Tutorial/Extra_Configs/Interpreter_Config.Backup
	cp AMOS/Tutorial/Extra_Configs/Interpreter_Config.Backup AMOS/AMOSPro_Interpreter_Config


$(COMPILER_CONFIG_OUTPUTS) &: tools/Get_Chunk +Compiler_Config.s
	$(call status,Assembling the Compiler Configuration)
	$(call run_vamos,c/Genim2 +Compiler_Config.s -obin/Compiler_Config.o)
	mkdir -p AMOS/Tutorial/Extra_Configs
	tools/Get_Chunk bin/Compiler_Config.o AMOS/Tutorial/Extra_Configs/Compiler_Config.Backup
	cp AMOS/Tutorial/Extra_Configs/Compiler_Config.Backup AMOS/AMOSPro_Compiler_Config

AMOS/APSystem/AMOSPro_Editor &: +Edit.s +Version.s +Debug.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling AMOSPro_Editor)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genam FROM +Edit.s TO AMOS/APSystem/AMOSPro_Editor)

AMOS/APSystem/AMOSPro_Monitor: +Monitor.s +Version.s +Debug.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling AMOSPro_Monitor)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genam FROM +Monitor.s TO AMOS/APSystem/AMOSPro_Monitor)

AMOS/APSystem/APCmp: +APComp.s +Version.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling APCmp Command Line Compiler)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genam FROM +APComp.s TO AMOS/APSystem/APCmp)

$(ACLIB_OUTPUTS) &: +CLib.s tools/Library_Digest tools/Check_Clib
	$(call status,Assembling Compiler.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +CLib.s
	tools/Check_Clib
	$(call run_vamos,c/Genam FROM +CLib.s TO AMOS/APSystem/Compiler.Lib)

$(ACHEADER_OUTPUTS) &: +Header.s +Header_Backstart.s
	$(call status,Assembling Header Files)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genam FROM +Header.s TO AMOS/APSystem/Header_CLI.Lib)
	$(call run_vamos,c/Genam FROM +Header_Backstart.s TO AMOS/APSystem/Header_Backstart.Lib)

$(ACOMPEXT_OUTPUTS) &: +CompExt.s tools/Library_Digest
	$(call status,Assembling AMOSPro_Compiler.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +CompExt.s
	$(call run_vamos,c/Genam FROM +CompExt.s TO AMOS/APSystem/AMOSPro_Compiler.Lib)

$(AREQUESTEXT_OUTPUTS) &: +Request.s tools/Library_Digest
	$(call status,Assembling AMOSPro_Request.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +Request.s
	$(call run_vamos,c/Genam FROM +Request.s TO AMOS/APSystem/AMOSPro_Request.Lib)

$(MUSICEXT_OUTPUTS) &: +Music.s tools/Library_Digest +Version.s
	$(call status,Assembling AMOSPro_Music.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +Music.s
	$(call run_vamos,c/Genam FROM +Music.s TO AMOS/APSystem/AMOSPro_Music.Lib)

$(COMPACTEXT_OUTPUTS) &: +Compact.s tools/Library_Digest +Version.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling AMOSPro_Compact.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +Compact.s
	$(call run_vamos,c/Genam FROM +Compact.s TO AMOS/APSystem/AMOSPro_Compact.Lib)

$(AIOEXT_OUTPUTS) &: +IO_Ports.s tools/Library_Digest +Version.s $(AMOS_INCLUDES_DEPS)
	$(call status,Assembling AMOSPro_IOPorts.Lib)
	mkdir -p AMOS/APSystem
	tools/Library_Digest +IO_Ports.s
	$(call run_vamos,c/Genam FROM +IO_Ports.s TO AMOS/APSystem/AMOSPro_IOPorts.Lib)

AMOS/APSystem/AMOSPro_3d.Lib: +3d.s +Equ.s _Pointe.s _WEqu.s +CEqu.s _LEqu.s
	$(call status,Assembling AMOSPro_3d.Lib)
	mkdir -p AMOS/APSystem
	$(call run_vamos,c/Genim2 +3d.s -oAMOS/APSystem/AMOSPro_3d.Lib -c)

Instruction_List.Asc: tools/Make_Checklist +Lib.s +Music.s +Compact.s +Request.s +IO_Ports.s
	$(call status,Creating checklist)
	tools/Make_Checklist

# --- top-level targets -----------------------------------------------------
TOOL_SRCS := $(wildcard tools/*.c)
TOOL_BINS := $(TOOL_SRCS:.c=)

.PHONY: clean
clean:
	rm -f $(TOOL_BINS) \
	+Toktab_Verif.Bin \
	$(LABEL_OUTPUTS) \
	$(TARGETS) \
	$(EDITOR_CONFIG_OUTPUTS) \
	$(MUSICEXT_OUTPUTS) \
	$(ACLIB_OUTPUTS) \
	$(ACHEADER_OUTPUTS) \
	$(ACOMPEXT_OUTPUTS) \
	$(AREQUESTEXT_OUTPUTS) \
	$(COMPACTEXT_OUTPUTS) \
	$(AIOEXT_OUTPUTS)
