// NoahCM.h
//-- CNoahConfigManager -- save / load / modify the setting of 'Noah' --

#ifndef AFX_NOAHCM_H__ACE475C1_D925_4F9E_BDCA_783B921E6FD5__INCLUDED_
#define AFX_NOAHCM_H__ACE475C1_D925_4F9E_BDCA_783B921E6FD5__INCLUDED_

class CNoahConfigManager;

class CNoahConfigDialog : public kiPropSheet
{
public:
	class CCmprPage : public kiPropSheetPage
	{
	public:
		CCmprPage();
	private:
		BOOL onInit();
		bool onOK();
		bool onCancel();
		BOOL CALLBACK proc( UINT msg, WPARAM wp, LPARAM lp );
		void correct( const char* ext, bool first );
		void SetUpToolTip();
	private:
		HWND m_tooltip;
	};
	class CMeltPage : public kiPropSheetPage
	{
	public:
		CMeltPage();
	private:
		BOOL onInit();
		bool onOK();
		BOOL CALLBACK proc( UINT msg, WPARAM wp, LPARAM lp );
		void correct();
	};
	class CInfoPage : public kiPropSheetPage
	{
	public:
		CInfoPage();
	private:
		BOOL onInit();
	};

public:
	CNoahConfigDialog();

private:
	bool onOK();
	bool onCancel();
	void onApply();
	void onHelp();
	void onCommand( UINT id );
	BOOL onInit();
	void onDrop( HDROP hdrop );
	static void shift_and_button();
};

enum loading_flag
{
	Mode    = 1,
	Melt    = 2,
	Compress= 4,
	OpenDir =16,
	All     =23,
};

class CNoahConfigManager
{
public: //-- Operations

	void init();
	void load( loading_flag what );
	void save();
	void dialog();

public: //-- Generic INI accessors (for per-dialog persistence)
	int  getInt( const char* key, int defval ) { return m_Ini.getInt(key, defval); }
	void putInt( const char* key, int val )    { m_Ini.putInt(key, val); }

public: //-- Interface for getting settings items

	// Section: Mode
	const int     mode()  { return m_Mode; }  // 0:compress-only 1:compress-preferred 2:extract-preferred 3:extract-only
	const bool  miniboot(){ return m_MiniBoot; } // Start minimized?
	const bool  oldver()  { return m_OldVer; }// Display version in old format
	const int   extnum()  { return m_OneExt ? 1 : m_ZeroExt ? 0 : -1; } // Number of extensions to treat as part of archive name
	const int multiboot_limit() { return m_MbLim; } // Multiple-instance limit
	// Section: Melt
	const kiPath& mdir()  { return m_MDir; }  // Extraction destination
	const bool    mdirsm(){ return m_MDirSm; }// Extract to same directory?
	const int     mkdir() { return m_MkDir; } // 0:no 1:file 2:dir 3:always
	const bool    mnonum(){ return m_MNoNum; }// Omit numeric suffix
	const char*   kill()  { return m_Kill; }  // Built-in routines to disable
	// Section: Compress
	const kiPath& cdir()  { return m_CDir; }  // Compression destination
	const bool    cdirsm(){ return m_CDirSm; }// Compress to same directory?
	const kiStr&  cext()  { return m_CExt; }  // Compression format
	const kiStr&  cmhd()  { return m_CMhd; }  // Compression method
	// Section: OpenDir
	const bool    modir() { return m_MODir; } // Open folder after extraction?
	const bool    codir() { return m_CODir; } // Open folder after compression?
	const kiStr&  openby(){ return m_OpenBy; }// Program to open folder (hidden)

private: //-- Internal variables

	unsigned long m_Loaded;
	kiIniFile m_Ini;
	kiStr m_UserName;

	// Settings items
	int    m_Mode;
	kiPath m_MDir, m_CDir;
	bool   m_MODir,m_CODir,m_MDirSm,m_CDirSm;
	int    m_MkDir;
	int    m_MbLim;
	kiStr  m_CExt;
	kiStr  m_OpenBy;
	kiStr  m_CMhd;
	bool   m_MNoNum;
	kiStr  m_Kill;
	bool   m_MiniBoot;
	bool   m_OldVer;
	bool   m_OneExt, m_ZeroExt;

public:
	CNoahConfigManager() {}

friend class CNoahConfigDialog::CCmprPage;
friend class CNoahConfigDialog::CMeltPage;
friend class CNoahConfigDialog::CInfoPage;
};

#endif
