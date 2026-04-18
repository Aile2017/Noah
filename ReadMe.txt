For the terms of use of this source code, refer to the terms
attached to the Noah binary. Basically, you are free to use it in any way.

This repository contains the current x64/MSVC version of Noah.
It keeps the B2E-script-based external tool runner and removes the
legacy integrated-archiver DLL interface.


== Workspace

 - Noah.sln                 (Visual Studio solution)
 - Noah.vcxproj             ('Noah.exe' project)
 - stdafx.h/cpp             (for pre-compiled header generation)


== Resources

   - Noah.rc                (resource script)
   - Resource.h             (resource ID definition header)
   - Noah.ico               (icon data)


== Source code

 - /
   - NoahApp.h|Noah.cpp     (Noah main routine)
   - NoahCM.h|cpp           (configuration loading, saving, and dialogs)
   - NoahAM.h|cpp           (archive manager / dispatch)
   - SubDlg.h|cpp           (archive viewer / password / progress dialogs)
   - Archiver.h|cpp         (common archive operation interface and external command runner)
   - ArcB2e.h|cpp           (B2E script support)

 - kilib/
   - kilib.h                (K.I.LIB main header)
   - kilibext.h             (K.I.LIB extension features header)
   - kl_app.h|cpp           (startup point and application-wide information)
   - kl_wnd.h|cpp           (window, dialog, property sheet management)
   - kl_reg.h|cpp           (registry and INI file I/O)
   - kl_dnd.h|cpp           (OLE drag & drop processing)
   - kl_find.h|cpp          (file search)
   - kl_wcmn.h|cpp          (utility functions mainly for Windows Shell)
   - kl_cmd.h|cpp           (command line parser)
   - kl_str.h|cpp           (string and path processing)
   - kl_file.h|cpp          (file I/O)
   - kl_misc.h|cpp          (general-purpose classes)
   - kl_rythp.h|cpp         (Rythp script processing used by B2E)
   - kl_carc.h              (legacy integrated-archiver definitions kept only as historical reference)


== Structure

 - CNoahApp : kiApp
   - determines what to do by communicating with ArcMan and CnfMan

   - CNoahArchiverManager
     - routes compression/extraction to the appropriate CArchiver
     - kiArray<CArchiver*>

   - CNoahConfigManager
     - INI read/write processing and settings dialogs
     - CNoahConfigDialog : kiPropSheet
       - CCmprPage : kiPropSheetPage
       - CInfoPage : kiPropSheetPage
       - CMeltPage : kiPropSheetPage

 - CArchiver
   - common interface for archive operations
   - engine-specific classes are derived from here

 - CArcB2e
   - B2E-backed archiver implementation
   - executes external archiver tools through B2E scripts

 - K.I.LIB
   - a very non-portable Win32-specific library
