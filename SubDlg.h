#ifndef AFX_ARCVIEWDLG_H__91EDF9F6_142E_4E25_BCE3_448E937E29D9__INCLUDED_
#define AFX_ARCVIEWDLG_H__91EDF9F6_142E_4E25_BCE3_448E937E29D9__INCLUDED_

#include "NoahApp.h"
#include "Archiver.h"
#include "resource.h"

class CArcViewDlg : public kiDialog, kiDataObject
{
public:
	CArcViewDlg( CArchiver* ptr,arcname& fnm,const kiPath& ddir )
		: kiDialog( IDD_ARCVIEW ), m_pArc( ptr ),
		m_fname( fnm ), m_ddir( ddir ), m_hFont( NULL ), m_hHeader( NULL ),
		m_hTree( NULL ), m_listLeft( 0 ), m_listTop( 0 ),
		m_bDragging( false ), m_dragX( 0 ), m_dragListLeft( 0 ), m_ghostX( -1 ),
		m_folderIconIdx( 0 )
		{
			AddRef();
			myapp().get_tempdir( m_tdir );
		}

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
			m_ddir.mkdir();
			m_ddir.beShortPath();
		}
	struct listrow { bool isFolder; int idx; };
	bool setSelection();
	int hlp_cnt_check();
	bool m_bAble;
	HFONT m_hFont;
	HWND  m_hHeader;
	HWND  m_hTree;
	int   m_listLeft;
	int   m_listTop;
	bool  m_bDragging;
	int   m_dragX;
	int   m_dragListLeft;
	int   m_ghostX;
	StrArray m_folderPaths;
	kiArray<HTREEITEM> m_treeNodes;
	kiArray<unsigned int> m_folderSorted;  // sorted indices into m_folderPaths (for binary search)
	kiArray<unsigned int> m_fileIndices;
	StrArray m_iconExtCache;
	kiArray<int> m_iconIdxCache;
	kiArray<listrow> m_rows;
	int m_folderIconIdx;

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

private: //-- Drag & drop processing

	bool giveData( const FORMATETC& fmt, STGMEDIUM* stg, bool firstcall );

private: //-- Sort processing

	void DoSort( int col );
	static int CALLBACK lv_compare( LPARAM p1, LPARAM p2, LPARAM type );
	bool m_bSmallFirst[6];

private: //-- Right click

	void DoRMenu();
	void GenerateDirMenu( HMENU m, int& id, StrArray* sx, const kiPath& pth );
	void BuildFolderTree( HTREEITEM hRoot, int folderIconIdx );
	void FilterListByFolder( int folderIdx );
	void LayoutTopRow( int dlgW );
	void LayoutPanes( int dlgW, int paneH );
	void DrawSplitterGhost( int x );

private: //-- Extraction

	CArchiver* m_pArc;
	arcname m_fname;
	kiPath m_ddir, m_tdir;
	aflArray m_files;

//-- Window instance count management.
public:	static void clear() { st_nLife=0; }
private:static void hello() { st_nLife++; }
		static void byebye() { if(--st_nLife==0) kiWindow::loopbreak(); }
		static int st_nLife;
};


#endif
