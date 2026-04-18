# Makefile for Noah - nmake
# Usage: nmake [CFG=Release|Debug]

!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified. Defaulting to Release.
!ENDIF

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!ERROR Invalid CFG="$(CFG)". Use Release or Debug.
!ENDIF

TARGET   = Noah.exe
OUTDIR   = $(CFG)
INTDIR   = obj\$(CFG)

CC       = cl.exe
LINK     = link.exe
RC       = rc.exe
MT       = mt.exe

LIBS     = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
           advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \
           odbc32.lib odbccp32.lib comctl32.lib lz32.lib version.lib \
           ucrt.lib vcruntime.lib

PCH_HDR  = stdafx.h
PCH_FILE = $(INTDIR)\stdafx.pch

COMMON_CFLAGS = /nologo /W3 /EHs-c- /c /Fo"$(INTDIR)\\" /Fp"$(PCH_FILE)"

!IF "$(CFG)" == "Debug"
CFLAGS  = $(COMMON_CFLAGS) /Od /Zi /MDd \
          /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fd"$(INTDIR)\\"
LFLAGS  = /nologo /SUBSYSTEM:WINDOWS /DEBUG /MACHINE:X64 \
          /PDB:"$(INTDIR)\Noah.pdb" /ENTRY:kilib_startUp
!ELSE
CFLAGS  = $(COMMON_CFLAGS) /O1 /Os /Gy /Gw /GL /GS- /GR- /MD \
          /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fd"$(INTDIR)\\"
LFLAGS  = /nologo /SUBSYSTEM:WINDOWS /MACHINE:X64 \
          /PDB:"$(INTDIR)\Noah.pdb" /ENTRY:kilib_startUp \
          /manifest:no /LTCG /OPT:ICF /OPT:REF
!ENDIF

OBJS = \
    $(INTDIR)\stdafx.obj \
    $(INTDIR)\ArcB2e.obj \
    $(INTDIR)\Archiver.obj \
    $(INTDIR)\Noah.obj \
    $(INTDIR)\NoahAM.obj \
    $(INTDIR)\NoahCM.obj \
    $(INTDIR)\SubDlg.obj \
    $(INTDIR)\kl_app.obj \
    $(INTDIR)\kl_cmd.obj \
    $(INTDIR)\kl_dnd.obj \
    $(INTDIR)\kl_file.obj \
    $(INTDIR)\kl_find.obj \
    $(INTDIR)\kl_reg.obj \
    $(INTDIR)\kl_rythp.obj \
    $(INTDIR)\kl_str.obj \
    $(INTDIR)\kl_wcmn.obj \
    $(INTDIR)\kl_wnd.obj \
    $(INTDIR)\Noah.res

all: "$(OUTDIR)" "$(INTDIR)" "$(OUTDIR)\$(TARGET)"

"$(OUTDIR)" :
    @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    @if not exist "$(INTDIR)" mkdir "$(INTDIR)"

# Link and embed manifest
"$(OUTDIR)\$(TARGET)": $(OBJS)
    $(LINK) $(LFLAGS) /OUT:"$(OUTDIR)\$(TARGET)" $(LIBS) $(OBJS)
    $(MT) -nologo -manifest manifest.xml -outputresource:"$(OUTDIR)\$(TARGET);1"

# Precompiled header (must build first)
$(PCH_FILE) $(INTDIR)\stdafx.obj: stdafx.cpp stdafx.h
    $(CC) $(CFLAGS) /Yc"$(PCH_HDR)" stdafx.cpp

# Source files using precompiled header
$(INTDIR)\ArcB2e.obj: ArcB2e.cpp ArcB2e.h Archiver.h $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" ArcB2e.cpp

$(INTDIR)\Archiver.obj: Archiver.cpp Archiver.h $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" Archiver.cpp

$(INTDIR)\Noah.obj: Noah.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" Noah.cpp

$(INTDIR)\NoahAM.obj: NoahAM.cpp NoahAM.h Archiver.h SubDlg.h $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" NoahAM.cpp

$(INTDIR)\NoahCM.obj: NoahCM.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" NoahCM.cpp

$(INTDIR)\SubDlg.obj: SubDlg.cpp SubDlg.h Archiver.h $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" SubDlg.cpp

$(INTDIR)\kl_app.obj: kilib\kl_app.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_app.cpp

$(INTDIR)\kl_cmd.obj: kilib\kl_cmd.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_cmd.cpp

$(INTDIR)\kl_dnd.obj: kilib\kl_dnd.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_dnd.cpp

$(INTDIR)\kl_file.obj: kilib\kl_file.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_file.cpp

$(INTDIR)\kl_find.obj: kilib\kl_find.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_find.cpp

$(INTDIR)\kl_reg.obj: kilib\kl_reg.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_reg.cpp

$(INTDIR)\kl_rythp.obj: kilib\kl_rythp.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_rythp.cpp

$(INTDIR)\kl_str.obj: kilib\kl_str.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_str.cpp

$(INTDIR)\kl_wcmn.obj: kilib\kl_wcmn.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_wcmn.cpp

$(INTDIR)\kl_wnd.obj: kilib\kl_wnd.cpp $(PCH_FILE)
    $(CC) $(CFLAGS) /Yu"$(PCH_HDR)" kilib\kl_wnd.cpp

# Resource
$(INTDIR)\Noah.res: Noah.rc Resource.h Noah.ico
    $(RC) /nologo /fo"$(INTDIR)\Noah.res" Noah.rc

clean:
    @if exist "$(INTDIR)" rmdir /s /q "$(INTDIR)"
    @if exist "$(OUTDIR)\$(TARGET)" del /q "$(OUTDIR)\$(TARGET)"
    @if exist "$(OUTDIR)\$(TARGET:.exe=.pdb)" del /q "$(OUTDIR)\$(TARGET:.exe=.pdb)"
