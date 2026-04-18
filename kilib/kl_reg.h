//--- K.I.LIB ---
// kl_reg.h : registry and ini-file operation

#ifndef AFX_KIREGKEY_H__4FD5E1B3_B8FE_45B3_B19E_3D30407C94BA__INCLUDED_
#define AFX_KIREGKEY_H__4FD5E1B3_B8FE_45B3_B19E_3D30407C94BA__INCLUDED_

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// Registry and INI file operations

class kiRegKey
{
public: //-- Public interface --------------------------

	// Open & close
	bool open( HKEY parent, LPCTSTR keyname, REGSAM access = KEY_ALL_ACCESS );
	bool create( HKEY parent, LPCTSTR keyname, REGSAM access = KEY_ALL_ACCESS );
	void close()
		{
			if( m_hKey )
				RegCloseKey( m_hKey );
		}

	// Whether a subkey exists
	bool exist( LPCTSTR keyname )
		{
			HKEY k;
			if( ERROR_SUCCESS==RegOpenKeyEx( m_hKey,keyname,0,KEY_READ,&k ) )
			{
				RegCloseKey( k );
				return true;
			}
			return false;
		}
	// Cast to HKEY
	operator HKEY() const
		{
			return m_hKey;
		}

	// Get value
	bool get( LPCTSTR valname, DWORD* val );
	bool get( LPCTSTR valname, BYTE* val, DWORD siz );
	bool get( LPCTSTR valname, kiStr* val );

	// Set value
	bool set( LPCTSTR valname, DWORD val );
	bool set( LPCTSTR valname, BYTE* val, DWORD siz );
	bool set( LPCTSTR valname, LPCTSTR val );

	// Delete
	bool del( LPCTSTR valname );
	bool delSubKey( LPCTSTR keyname );

public: //-- Internal processing -----------------------------------

	kiRegKey()
		{
			m_hKey = NULL;
		}

	virtual ~kiRegKey()
		{
			close();
		}

private:

	HKEY m_hKey;
	static bool delSubKeyRecursive( HKEY k, LPCTSTR n );
};

class kiIniFile
{
public: //-- Public interface --------------------------

	// Set ini filename
	void setFileName( const char* ini, bool exepath=true );
	void setSection( const char* section )
		{ m_CurSec = section; }

	// Read
	// Note: the return value of getStr is an internal buffer;
	// its contents are only valid immediately after the call.
	int getInt( const char* key, int defval );
	bool getBool( const char* key, bool defval );
	const char* getStr( const char* key, const char* defval );

	// Write
	bool putStr( const char* key, const char* val );
	bool putInt( const char* key, int val );
	bool putBool( const char* key, bool val );

private: //-- Internal processing -----------------------------------

	kiPath m_FileName;
	kiStr m_CurSec;
	char m_StrBuf[256];
};

#endif
