// Archiver.h
//-- CArchiver -- common interface in 'Noah' for arhiving routine --

#ifndef AFX_ARCHIVER_H__359A2ED3_2F97_480E_BC94_24834EBA6498__INCLUDED_
#define AFX_ARCHIVER_H__359A2ED3_2F97_480E_BC94_24834EBA6498__INCLUDED_

enum {
	aCheck = 1, aMelt = 2, aList = 4, aMeltEach = 8, aCompress = 16, aArchive = 32, aSfx = 64,
};
enum {
	aUnknown=0, aSingleFile, aSingleDir, aMulti
};

struct arcname {
	arcname( const kiPath& b,const char *s,const char *l )
		: basedir(b),sname(s),lname(l) {}
	const kiPath& basedir;
	const char*   lname;
	const char*   sname;
};

struct arcfile {
	INDIVIDUALINFO inf;
	char rawline[256];
	union {
		bool selected;
		bool isfile;
	};
};

#define aflArray kiArray<arcfile>
#define wfdArray kiArray<WIN32_FIND_DATA>

class CArchiver
{
public: //--< attribute >--

	int ability();
	int cancompressby( const char* ext, const char* mhd, bool sfx );

	const char*     mlt_ext();
	const kiStr&    cmp_ext();
	const StrArray& cmp_mhd_list();
	const int       cmp_mhd_default();
	bool            ver( kiStr& str );
	static bool GetVersionInfoStr( char* name, char* buf, size_t cbBuf );

public: //--< action >--

	bool check( const kiPath& aname );
	int  contents( const kiPath& aname, kiPath& dname );
	int  melt( const arcname& aname, const kiPath& ddir, const aflArray* files=NULL );
	bool list( const arcname& aname, aflArray& files );
	int  compress( const kiPath& base, const wfdArray& files, const kiPath& ddir, int method, bool sfx );
	kiStr arctype_name(const char* an) const { return v_name(an); }

protected: //--< for child >--

	CArchiver( const char* mext );
	void set_cmp_ext( const char* ext );
	void add_cmp_mhd( const char* mhd, bool def=false );

	virtual int  v_load(){return 0;}
	virtual bool v_ver( kiStr& str ){return false;}
	virtual bool v_check( const kiPath& aname ){return false;}
	virtual int  v_contents( const kiPath& aname, kiPath& dname ){return aUnknown;}
	virtual int  v_melt( const arcname& aname, const kiPath& ddir, const aflArray* files ){return false;}
	virtual bool v_list( const arcname& aname, aflArray& files ){return false;}
	virtual int  v_compress( const kiPath& base, const wfdArray& files, const kiPath& ddir, int method, bool sfx ){return false;}
	virtual kiStr v_name(const char*) const { return ""; }

private: //--< private >--

	friend class CNoahArchiverManager;
	bool extCheck( const char* ext );
	kiStr m_MyExtList, m_MyCmpExt;
	StrArray m_Mhd;
	int m_MhdDef,m_Able;
	bool not_loaded;

public: //--< dummy >--

	virtual ~CArchiver(){}
};

inline int CArchiver::ability()
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return m_Able;
}

inline int CArchiver::cancompressby( const char* ext, const char* mhd, bool sfx )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	if( (sfx && !(m_Able&aSfx)) || !(m_Able&aCompress) || !m_MyCmpExt.isSame(ext) )
		return -1; // no
	for( unsigned int i=0; i!=m_Mhd.len(); i++ )
		if( m_Mhd[i] == mhd )
			return (int)i;
	return -2; // only - 'type name' matched
}

inline bool CArchiver::check( const kiPath& aname )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return (m_Able&aCheck)?v_check(aname):false;
}

inline int CArchiver::contents( const kiPath& aname, kiPath& dname )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return (m_Able&aList)?v_contents(aname,dname):false;
}

inline int CArchiver::melt( const arcname& aname, const kiPath& ddir, const aflArray* files )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return (m_Able&aMelt)?v_melt(aname,ddir,files):0xffff;
}

inline bool CArchiver::list( const arcname& aname, aflArray& files )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return (m_Able&aList)?v_list(aname,files):false;
}

inline int CArchiver::compress( const kiPath& base, const wfdArray& files, const kiPath& ddir, int method, bool sfx )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return (m_Able&aCompress)?v_compress(base,files,ddir,method,sfx):0xffff;
}

inline bool CArchiver::ver( kiStr& str )
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return v_ver(str);
}

inline const char* CArchiver::mlt_ext()
{
	return m_MyExtList;
}

inline const kiStr& CArchiver::cmp_ext()
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return m_MyCmpExt;
}

inline const StrArray& CArchiver::cmp_mhd_list()
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return m_Mhd;
}

inline const int CArchiver::cmp_mhd_default()
{
	if( not_loaded )
		m_Able=v_load(), not_loaded=false;
	return m_MhdDef;
}

inline CArchiver::CArchiver( const char* extlist )
		: m_MyExtList( extlist ), m_Mhd(3), m_MhdDef(0), not_loaded(true)
{
	m_MyExtList.lower();
}

inline void CArchiver::set_cmp_ext( const char* ext )
{
	m_MyCmpExt = ext;
}

inline void CArchiver::add_cmp_mhd( const char* method, bool Default )
{
	m_Mhd.add(method);
	if( Default )
		m_MhdDef = m_Mhd.len() - 1;
}

inline bool CArchiver::extCheck( const char* ext )
{
	const char *x=m_MyExtList,*y;
	int ln = ki_strlen(ext);
	while( *x )
	{
		for( y=x+1; *y && *y!='.'; y++ );
		if( *y=='\0' ) break;

		if( y-x == ln )
		{
			while( x!=y )
			{
				if( *x!=ext[ln+(x-y)] )
					break;
				x++;
			}
			if( x==y )
				return true;
		}
		x=y+1;
	}
	return false;
}

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// Archiver module layer. Also handles exe-based archivers.

class CArcModule
{
public:

		// Create by specifying the command name to execute.
		//  - If found as a file, treat it as an executable.
		//  - Otherwise assume it is a shell command and keep it as-is.
	CArcModule( const char* name, bool us=false );
	virtual ~CArcModule() {}
	bool exist();
	bool chk( const char* ) { return false; }

	kiStr name() const { return kiPath::name(m_name); }

	int cmd( const char* cmd, bool mini=false );
	void ver( kiStr& str );

	int  cnt( const kiPath& aname, kiPath& dname, const char* wild="*" );
	bool lst_exe( const char* lstcmd, aflArray& files,
		const char* BL, int BSL, const char* EL, int SL, int dx );

private:
	enum { NOTEXIST, EXE, EXEUS, SHLCMD=0 } m_type;
	char         m_name[MAX_PATH];
};

inline bool CArcModule::exist()
	{ return m_type!=NOTEXIST; }



#endif
