#ifndef AFX_ARCVIEWDLG_H__91EDF9F6_142E_4E25_BCE3_448E937E29D9__INCLUDED_
#define AFX_ARCVIEWDLG_H__91EDF9F6_142E_4E25_BCE3_448E937E29D9__INCLUDED_

#include "NoahApp.h"
#include "Archiver.h"
#include "resource.h"

class CArcViewDlg : public kiDialog, kiDataObject
{
public:
	// Open with a specific archive
	CArcViewDlg( CArchiver* ptr, const kiPath& base, const char* sname,
	             const char* lname, const kiPath& ddir )
		: kiDialog( IDD_ARCVIEW ), m_pArc( ptr ),
		m_arcBaseDir( base ), m_arcLongName( lname ), m_arcShortName( sname ),
		m_ddir( ddir ), m_hFont( NULL ),
		m_hBmpExtract( NULL ), m_hBmpView( NULL ), m_hBmpSettings( NULL ),
		m_hHeader( NULL ),
		m_hTree( NULL ), m_listLeft( 0 ), m_listTop( 0 ),
		m_bDragging( false ), m_dragX( 0 ), m_dragListLeft( 0 ), m_ghostX( -1 ),
		m_folderIconIdx( 0 ), m_bAble( false ),
		m_bShowToolbar( true ), m_bShowTree( true ),
		m_topRowH( 0 ), m_defaultListLeft( 0 ), m_hMenu( NULL ),
		m_bLoading( false ), m_sortDir( 0 ), m_curFolderIdx( -1 )
		{
			AddRef();
			myapp().get_tempdir( m_tdir );
		}

	// Empty viewer (no archive loaded at start)
	CArcViewDlg( const kiPath& ddir )
		: kiDialog( IDD_ARCVIEW ), m_pArc( NULL ),
		m_ddir( ddir ), m_hFont( NULL ),
		m_hBmpExtract( NULL ), m_hBmpView( NULL ), m_hBmpSettings( NULL ),
		m_hHeader( NULL ),
		m_hTree( NULL ), m_listLeft( 0 ), m_listTop( 0 ),
		m_bDragging( false ), m_dragX( 0 ), m_dragListLeft( 0 ), m_ghostX( -1 ),
		m_folderIconIdx( 0 ), m_bAble( false ),
		m_bShowToolbar( true ), m_bShowTree( true ),
		m_topRowH( 0 ), m_defaultListLeft( 0 ), m_hMenu( NULL ),
		m_bLoading( false ), m_sortDir( 0 ), m_curFolderIdx( -1 )
		{
			AddRef();
			myapp().get_tempdir( m_tdir );
		}

	static void rememberMRU( const char* fullpath );

private: //-- Processing as dialog

	BOOL CALLBACK proc( UINT msg, WPARAM wp, LPARAM lp );
	BOOL onInit();
	bool onOK();
	bool onCancel();
	void setdir()
		{
			char str[MAX_PATH];
			sendMsgToItem( IDC_DDIR, WM_GETTEXT, MAX_PATH, (LPARAM)str );
			m_ddir = str;
			m_ddir.beBackSlash( true );
			if( mycnf().mkdir() )
			{
				myarc().generate_dirname( (const char*)m_arcLongName, m_ddir, mycnf().mnonum() );
				m_ddir += '\\';
			}
			m_ddir.mkdir();
			m_ddir.beShortPath();
		}
	struct listrow { bool isFolder; int idx; };
	bool setSelection();
	int hlp_cnt_check();
	void openSelected();
	bool    m_bAble;
	HFONT   m_hFont;
	HBITMAP m_hBmpExtract;
	HBITMAP m_hBmpView;
	HBITMAP m_hBmpSettings;
	HWND    m_hHeader;
	HWND    m_hTree;
	int   m_listLeft;
	int   m_listTop;
	bool  m_bDragging;
	int   m_dragX;
	int   m_dragListLeft;
	int   m_ghostX;
	StrArray m_folderPaths;
	StrArray m_folderRawlines;
	kiArray<HTREEITEM> m_treeNodes;
	kiArray<unsigned int> m_folderSorted;  // sorted indices into m_folderPaths (for binary search)
	kiArray<unsigned int> m_fileIndices;
	StrArray m_iconExtCache;
	kiArray<int> m_iconIdxCache;
	kiArray<listrow> m_rows;
	int m_folderIconIdx;

	// Toolbar/tree toggle state
	bool  m_bShowToolbar;
	bool  m_bShowTree;
	int   m_topRowH;         // saved m_listTop from onInit (for toolbar restore)
	int   m_defaultListLeft; // saved m_listLeft from onInit (for tree restore)
	HMENU m_hMenu;
	StrArray m_mruList;      // MRU paths, index 0 = most recent (max 10)
	bool  m_bLoading;        // true while archive list is being built
	int   m_sortDir;         // 0=archive order, 1=name asc, -1=name desc
	int   m_curFolderIdx;    // folder currently shown in the list (-1 = root)

private: //-- Helper

	int cachedIconFor( const char* filename )
	{
		const char* ext = kiPath::ext( filename );
		if( !ext ) ext = "";
		for( unsigned i=0; i<m_iconExtCache.len(); i++ )
			if( m_iconExtCache[i] == ext )
				return m_iconIdxCache[i];
		SHFILEINFO fi; ::ZeroMemory( &fi, sizeof(fi) );
		::SHGetFileInfo( filename, FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES );
		m_iconExtCache.add( kiStr(ext) );
		m_iconIdxCache.add( fi.iIcon );
		return fi.iIcon;
	}

	void clearSelections()
	{
		for( unsigned _fi=0; _fi<m_fileIndices.len(); _fi++ )
			m_files[ m_fileIndices[_fi] ].selected = false;
	}

	arcname makeArcName() const
	{
		return arcname( m_arcBaseDir, (const char*)m_arcShortName, (const char*)m_arcLongName );
	}

	void updateDdirDisplay();
	void setOperationControls( bool enable );
	void reportMeltResult( int result );

private: //-- Drag & drop processing

	bool giveData( const FORMATETC& fmt, STGMEDIUM* stg, bool firstcall );

private: //-- Sort processing


private: //-- Right click

	void DoRMenu(POINT pt);
	void BuildFolderTree( HTREEITEM hRoot, int folderIconIdx );
	void FilterListByFolder( int folderIdx );
	void LayoutTopRow( int dlgW );
	void LayoutPanes( int dlgW, int paneH );
	void DrawSplitterGhost( int x );

private: //-- Archive load/unload

	void rebuildContent();
	void loadArchive( CArchiver* parc, const kiPath& base,
	                  const char* sname, const char* lname );
	void unloadArchive();
	void doFileOpen();
	void openFromPath( const char* fullpath );
	void handleDroppedFile( const char* path );
	void updateMRU( const char* fullpath );
	void rebuildMRUMenu();
	void applyToolbarVisibility();
	void applyTreeVisibility();

private: //-- Extraction

	CArchiver* m_pArc;
	kiPath m_arcBaseDir;
	kiStr  m_arcLongName;
	kiStr  m_arcShortName;
	kiPath m_ddir, m_tdir;
	aflArray m_files;

//-- Window instance count management.
public:	static void clear() { st_nLife=0; }
private:static void hello() { st_nLife++; }
		static void byebye() { if(--st_nLife==0) kiWindow::loopbreak(); }
		static int st_nLife;
};


#endif
