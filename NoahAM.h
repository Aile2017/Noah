// NoahAM.h
//-- CNoahArchiverManager -- control many archiver routines --

#ifndef AFX_NOAHAM_H__CCE30761_D91B_4570_931A_6C165B19B22F__INCLUDED_
#define AFX_NOAHAM_H__CCE30761_D91B_4570_931A_6C165B19B22F__INCLUDED_

#include "Archiver.h"

class CNoahArchiverManager
{
public: //-- Public interface ------------

	// Initialize
	void init();

	// Store file list
	unsigned long set_files( const cCharArray& files );
	unsigned long file_num() { return m_FName.len(); }
	const kiPath& get_basepath() { return m_BasePathList[0]; }
	bool map_melters( int mode );
	bool map_compressor( const char* ext, const char* method, bool sfx );

	// Extract (destination dir required; other info derived internally)
	void do_melting( kiPath& destdir );
	// List/view (destination dir required; other info derived internally)
	void do_listing( kiPath& destdir );
	// Compress
	void do_compressing( kiPath& destdir, bool each );


	// Version information
	void get_version( kiStr& str );
	// Compression format list
	void get_cmpmethod( const char* set, int& def_mhd, StrArray& mhd_list, bool need_ext=false, cCharArray* ext_list=NULL );
	// Returns true if at least one b2e archiver is loaded
	bool b2e_enabled() { return m_b2e; }


private: //-- Internal processing ---------------------------

	// Double-folder elimination etc.
	bool break_ddir( kiPath& dir, bool onlydir );
	CArchiver* fromExt( const char* ext );
	void generate_dirname( const char* src, kiPath& dst, bool rmn );

	wfdArray m_FName;
	kiArray<CArchiver*> m_AList;
	kiArray<kiPath> m_BasePathList;
	bool m_b2e;

	// Extraction assignment
	kiArray<CArchiver*> m_Melters;
	// Compression assignment
	CArchiver* m_Compressor;
	int        m_Method;
	bool       m_Sfx;

public:
	~CNoahArchiverManager()
		{
			for( unsigned int i=0; i!=m_AList.len(); i++ )
				delete m_AList[i];
		}
};

#endif
