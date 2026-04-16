// NoahAM.cpp
//-- control many archiver routines --

#include "stdafx.h"
#include "resource.h"
#include "NoahApp.h"
#include "NoahAM.h"
#include "ArcB2e.h"

//----------------------------------------------//
//------ ���������̃f�[�^�ŏ��������Ă��� ------//
//----------------------------------------------//

void CNoahArchiverManager::init()
{
	char prev_cur[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, prev_cur);
	::SetCurrentDirectory( CArcB2e::init_b2e_path() );
	kiFindFile find;
	find.begin( "*.b2e" );
	WIN32_FIND_DATA fd;
	for( int t=0; find.next(&fd); t++ )
		m_AList.add( new CArcB2e(fd.cFileName) );
	m_b2e = (t>0);
	::SetCurrentDirectory(prev_cur);
}

//----------------------------------------------//
//------------ �t�@�C�����X�g���L�� ------------//
//----------------------------------------------//

unsigned long CNoahArchiverManager::set_files( const cCharArray& files )
{
	//-- �N���A
	m_FName.empty();
	m_BasePathList.empty();

	//-- ���p�X���擾( �o���邾�����p�͈͂��L���邽�߁A8.3�`���� )
	if( files.len() != 0 )
	{
		char spath[MAX_PATH];
		m_BasePath =
			( 0!=::GetShortPathName( files[0], spath, MAX_PATH ) )
			? spath : "";
		if( !m_BasePath.beDirOnly() )
		{
			m_BasePath.beSpecialPath( kiPath::Cur );
			m_BasePath.beBackSlash( true );
		}
	}

	//-- �Z���t�@�C�����ƒ����̂𗼕��擾���Ă���
	m_FName.alloc( files.len() );
	m_BasePathList.alloc( files.len() );
	for( unsigned int i=0,c=0; i!=files.len(); i++ )
		if( kiFindFile::findfirst( files[i], &m_FName[c] ) )
		{
			if( m_FName[c].cAlternateFileName[0] == '\0' )
				::lstrcpy(m_FName[c].cAlternateFileName,m_FName[c].cFileName);
			m_BasePathList[c] = files[i];
			if( !m_BasePathList[c].beDirOnly() )
			{
				m_BasePathList[c].beSpecialPath( kiPath::Cur );
				m_BasePathList[c].beBackSlash( true );
			}
			++c;
		}
	m_FName.forcelen( c );
	m_BasePathList.forcelen( c );
	return c;
}

//----------------------------------------------//
//--- �t�@�C�����X�g�ɉ𓀃��[�`�������蓖�� ---//
//----------------------------------------------//

// �w�肳�ꂽ�g���q�ɑΉ����Ă��郋�[�`������`�T��
CArchiver* CNoahArchiverManager::fromExt( const char* ext )
{
	kiStr tmp = ext;
	tmp.lower();

	for( unsigned int i=0; i!=m_AList.len(); i++ )
		if( m_AList[i]->extCheck( tmp ) 
		 && (m_AList[i]->ability() & aMelt) )
			return m_AList[i];
	return NULL;
}

bool CNoahArchiverManager::map_melters( int mode ) // 1:cmp 2:mlt 3:must_mlt
{
	// �N���A
	m_Melters.empty();

#define attrb (m_FName[ct].dwFileAttributes)
#define lname (m_FName[ct].cFileName)
#define sname (m_FName[ct].cAlternateFileName[0]==0 ? m_FName[ct].cFileName : m_FName[ct].cAlternateFileName)

	kiPath fnm;
	const char* ext;
	for( unsigned int ct=0, bad=0; ct!=file_num(); ct++ )
	{
//		fnm = m_BasePath, fnm += sname;
		fnm = m_BasePathList[ct], fnm += sname;

		//-- 0byte�t�@�C�� / �f�B���N�g���͒e��
		if( !(attrb & FILE_ATTRIBUTE_DIRECTORY) && 0!=kiFile::getSize( fnm, 0 ) )
		{
			//-- �܂��Ή��g���q���ǂ����Ō��`����I�o
			CArchiver* x = fromExt( ext=kiPath::ext(lname) );

			//-- ���`�ŁA�t�@�C�����e�ɂ��`�F�b�N
			if( x && x->check( fnm ) )
			{
				m_Melters.add( x );
				continue;
			}

			//-- ���`�����e�`�F�b�N�s�Ȃ��̂������炻����g��
			if( x && !(x->ability() & aCheck) )
			{
				m_Melters.add( x );
				continue;
			}

			//-- ���`���_���Ȃ�A���̑��̓��e�`�F�b�N�\�ȃ��[�`���S�ĂŎ���
			if( mode!=1 || 0==ki_strcmpi( "exe", ext ) )
			{
				for( unsigned long j=0; j!=m_AList.len(); j++ )
					if( m_AList[j]!=x && m_AList[j]->check( fnm ) )
					{
						m_Melters.add( m_AList[j] );
						break;
					}
				if( m_Melters.len() == ct+1 )
					continue;
			}
		}

		//-- �`�F�b�N�̌��ʁA�𓀕s�\�ł����Ƃ�
		if( mode!=3 )
			return false; //-- �𓀐�p���[�h�łȂ���ΏI��
		m_Melters.add( NULL ), bad++;
	}
#undef sname
#undef lname
#undef attrb

	return (ct!=bad);
}

//----------------------------------------------//
//--- �t�@�C�����X�g�Ɉ��k���[�`�������蓖�� ---//
//----------------------------------------------//

bool CNoahArchiverManager::map_compressor( const char* ext, const char* method, bool sfx )
{
	int m;
	m_Method = -1;
	m_Sfx    = sfx;

	for( unsigned int i=0; i!=m_AList.len(); i++ )
		if( -1 != (m=m_AList[i]->cancompressby(ext,method,sfx)) )
			if( m!=-2 ) // ���S��v
			{
				m_Compressor = m_AList[i];
				m_Method = m;
				break;
			}
			else if( m_Method == -1 ) // �`�����݈̂�v�����ŏ��̃��m
			{
				m_Compressor = m_AList[i];
				m_Method = m_AList[i]->cmp_mhd_default();
			}
	return (m_Method != -1);
}

//----------------------------------------------//
//------------ �o�[�W������񕶎��� ------------//
//----------------------------------------------//

void CNoahArchiverManager::get_version( kiStr& str )
{
	kiStr tmp;
	for( unsigned int i=0; i!=m_AList.len(); i++ )
		if( m_AList[i]->ver( tmp ) )
			str+=tmp, str+="\r\n";
}

//----------------------------------------------//
//--------------- ���k�`�����X�g ---------------//
//----------------------------------------------//

static unsigned int find( const cCharArray& x, const char* o )
{
	for( unsigned int i=0; i!=x.len(); i++ )
		if( 0==ki_strcmp( x[i], o ) )
			return i;
	return 0xffffffff;
}

static unsigned int find( const StrArray& x, const char* o )
{
	for( unsigned int i=0; i!=x.len(); i++ )
		if( x[i]==o )
			return i;
	return 0xffffffff;
}

void CNoahArchiverManager::get_cmpmethod(
		const char* set,
		int& def_mhd,
		StrArray& mhd_list,
		bool need_ext,
		cCharArray* ext_list )
{
	def_mhd = -1;

	const char* x;
	for( unsigned int i=0; i!=m_AList.len(); i++ )
	{
		if( *(x = m_AList[i]->cmp_ext())=='\0' )
			continue;
		if( need_ext )
		{
			if( -1 == find( *ext_list, x ) )
				ext_list->add( x );
		}
		if( 0 == ki_strcmp( set, x ) )
		{
			if( mhd_list.len()==0 )
			{
				def_mhd = m_AList[i]->cmp_mhd_default();
				for( unsigned int j=0; j!=m_AList[i]->cmp_mhd_list().len(); j++ )
					mhd_list.add( (m_AList[i]->cmp_mhd_list())[j] );
			}
			else
			{
				for( unsigned int j=0; j!=m_AList[i]->cmp_mhd_list().len(); j++ )
					if( -1 == find( mhd_list, (m_AList[i]->cmp_mhd_list())[j] ) )
						mhd_list.add( (m_AList[i]->cmp_mhd_list())[j] );
			}
		}
	}

	if( def_mhd == -1 )
		def_mhd = 0;
}

//----------------------------------------------//
//--------------- ���Ɉꗗ���[�h ---------------//
//----------------------------------------------//

#include "SubDlg.h"

void CNoahArchiverManager::do_listing( kiPath& destdir )
{
	kiWindow* mptr = app()->mainwnd();
	kiPath ddir;
	int    mdf = mycnf().mkdir();
	bool   rmn = mycnf().mnonum();
	destdir.beBackSlash( true );

	//-- �_�C�A���O�̌��J�E���^���N���A
	kiArray<CArcViewDlg*> views;
	CArcViewDlg::clear();

	//-- �_�C�A���O�N��
	for( unsigned int i=0; i!=m_FName.len(); i++ )
	{
		if( !m_Melters[i] )
			continue;

		arcname an(
			m_BasePathList[i],
//			m_BasePath,
			m_FName[i].cAlternateFileName[0]==0 ? m_FName[i].cFileName : m_FName[i].cAlternateFileName,
			m_FName[i].cFileName );
		ddir = destdir;

		if( mdf )
			generate_dirname( m_FName[i].cFileName, ddir, rmn );

		CArcViewDlg* x = new CArcViewDlg( m_Melters[i],an,ddir );
		views.add( x );
		x->createModeless( NULL );
	}

	//-- �S���I������܂őҋ@
	kiWindow::msgLoop( kiWindow::GET );

	//-- ���I��
	app()->setMainWnd( mptr );
	for( i=0; i!=views.len(); i++ )
		delete views[i];
}

//----------------------------------------------//
//----------------- �𓀍�� -------------------//
//----------------------------------------------//

void CNoahArchiverManager::do_melting( kiPath& destdir )
{
	//-- �ݒ胍�[�h
	const int  mdf = mycnf().mkdir();  // Make Directory Flag( 0:no 1:no1file 2: noddir 3:yes )
	const bool rmn = mycnf().mnonum(); // Remove NuMber ?

	//-- �o�͐�
	destdir.beBackSlash( true );
	destdir.mkdir(), destdir.beShortPath();

	for( unsigned int i=0; i!=m_FName.len(); i++ )
		if( m_Melters[i] )
		{
			//-- �o�͐�

			int mk=2; // 0:no 1:yes 2:???
			kiPath ddir( destdir ), dnm;
			if( mdf==0 )
				mk=0;
			else if( mdf==3 )
				mk=1;
			else
			{
				kiPath anm(m_BasePathList[i]);
//				kiPath anm(m_BasePath);
				anm+=m_FName[i].cFileName;
				int c = m_Melters[i]->contents( anm, dnm );
				if( c==aSingleDir || (c==aSingleFile && mdf==1) )
					mk=0; // �Q�d�t�H���_�h�~����(��)
				else if( c==aMulti )
					mk=1;
			}
			if( mk )
			{
				generate_dirname( m_FName[i].cFileName, ddir, rmn );
				if( mk==2 && kiSUtil::exist(ddir) )
					mk=1;
				ddir+='\\';
				ddir.mkdir();
				ddir.beShortPath();
			}

			//-- �𓀁I

			arcname an( m_BasePathList[i],
//			arcname an( m_BasePath,
				m_FName[i].cAlternateFileName[0]==0 ? m_FName[i].cFileName : m_FName[i].cAlternateFileName,
				m_FName[i].cFileName );
			int result = m_Melters[i]->melt( an, ddir );
			if( result<0x8000 )
			{
				if( mk==2 ) // �Q�d�t�H���_�h�~����(��)
					break_ddir( ddir, mdf==2 );
				else if( mk==0 && dnm.len() ) // �Q�d�t�H���_�h�~����(��)
					if( dnm.len()<=1 || dnm[1]!=':' ) // ��΃p�X�͊J���Ȃ�
						ddir+=dnm, ddir+='\\';
				// �o�͐���J������
				myapp().open_folder( ddir, 1 );
			}
			else if( result!=0x8020 )
			{
				//�G���[�I
				char str[255];
				wsprintf( str, "%s\nError No: [%x]",
					(const char*)kiStr().loadRsrc( IDS_M_ERROR ), result );
				app()->msgBox( str );
			}
		}
}

void CNoahArchiverManager::generate_dirname( const char* src, kiPath& dst, bool rmn )
{
	// src�Ŏ����ꂽ���ɖ�����f�B���N�g�����𐶐����A
	// dst�֑����Brmn==true�Ȃ疖���̐������폜

	// ��ԍ��� . �ƍ������Ԗڂ� . ��T��
	const char *fdot=NULL, *sdot=NULL, *tail;
	for( tail=src; *tail; tail=kiStr::next(tail) )
		if( *tail=='.' )
			sdot=fdot, fdot=tail;

	// .tar.xxx ���A.xxx.gz/.xxx.z/.xxx.bz2 �Ȃ��폜
	if( fdot )
	{
		tail = fdot;
		if( sdot )
			if( 0==::lstrcmpi(fdot,".gz")
			 || 0==::lstrcmpi(fdot,".z")
			 || 0==::lstrcmpi(fdot,".bz2")
			 || (sdot+4==fdot
			 && (sdot[1]=='t'||sdot[1]=='T')
			 && (sdot[2]=='a'||sdot[2]=='A')
			 && (sdot[3]=='r'||sdot[3]=='R')
			))
				tail = sdot;
	}

	// �����̐�����'-'��'_'��'.'�폜�B���p�X�y�[�X���B
	bool del[256];
	ki_memzero( del, sizeof(del) );
	if( rmn )
	{
		del['-'] = del['_'] = del['.'] = true;
		for( char c='0'; c<='9'; ++c )
			del[c] = true;
	}
	del[' '] = true;

	const char* mjs=NULL;
	for( const char *x=src; x<tail; x=kiStr::next(x) )
		if( !del[(unsigned char)(*x)] )
			mjs = NULL;
		else if( !mjs )
			mjs = x;
	if( mjs && mjs!=src )
		tail = mjs;

	// ��ɂȂ��Ă��܂����� "noahmelt" �Ƃ������O�ɂ��Ă��܂��B
	if( src==tail )
		dst += "noahmelt";
	else
		while( src!=tail )
			dst += *src++;
}

bool CNoahArchiverManager::break_ddir( kiPath& dir, bool onlydir )
{
// �Q�d�t�H���_ or �P��t�@�C�� ��Ԃ�����
//
// �f���Ɋi�[�t�@�C��������x��������̂��{���Ȃ�ł����A
// ���ɋ��发�ɂ̎����x�ቺ���������̂ƁAFindFirst�n��
// �T�|�[�g����DLL������G���W���ȊO�ɑΉ��ł��Ȃ��Ƃ���
// ���_�����邽�߁A���ς�炸 Noah 2.xx �Ɠ�����@�ł��B

//-- ���ɂP���������ĂȂ����Ƃ��m�F -----------------
	char wild[MAX_PATH];
	ki_strcpy( wild, dir );
	ki_strcat( wild, "*.*" );
	kiFindFile find;
	if( !find.begin( wild ) )
		return false;
	WIN32_FIND_DATA fd,fd2,fd3;
	find.next( &fd );
	if( find.next( &fd2 ) )
		return false;
	find.close();
//----------------------------------------------------

//-- to:�ŏI�ړ���t�@�C�����B���łɁA�J�����gDir�͏����Ȃ����̉���� -----
	kiPath to(dir); to.beBackSlash( false ), to.beDirOnly();
	::SetCurrentDirectory( to );
	to += fd.cFileName;
//-------------------------------------------------------------------------

//-- �t�@�C���������ꍇ --------------------------------------
	if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	{
		if( !onlydir )
		{
			// now ���݂̃t�@�C����
			kiStr now=dir; now+=fd.cFileName;

			// now -> to �ړ�
			if( ::MoveFile( now, to ) )
			{
				dir.remove();
				dir.beBackSlash( false ), dir.beDirOnly();
				return true;
			}
		}
	}
//-- �t�H���_�������ꍇ ----------------------------------------
	else
	{
		// 'base/aaa/aaa/' ���ƒ���aaa���O��move�ł��Ȃ��B
		// ����āA�����B-> 'base/aaa_noah_tmp_178116/aaa/'

		dir.beBackSlash( false );
		kiFindFile::findfirst( dir, &fd3 );
		kiPath dirx( dir ); dirx+="_noah_tmp_178116";

		if( ::MoveFile( dir, dirx ) )
		{
			// now ���݂̃t�@�C����
			kiStr now( dirx ); now+='\\', now+=fd.cFileName;

			// �f�B���N�g�����ړ�
			if( ::MoveFile( now, to ) )
			{
				dirx.remove();
				dir=to, dir.beBackSlash( true );
				return true;
			}
			else
			{
				// 'base/aaa_noah_tmp_178116/aaa/' -> 'base/aaa/aaa/'
				dir.beDirOnly(), dir+=fd3.cFileName;
				::MoveFile( dirx, dir );
			}
		}

		dir.beBackSlash( true );
	}
//------------------------------------------------------------
	return false;
}

//----------------------------------------------//
//----------------- ���k��� -------------------//
//----------------------------------------------//

void CNoahArchiverManager::do_compressing( kiPath& destdir, bool each )
{
	int result = 0xffff, tr;

	// �o�͐���m���ɍ���Ă���
	destdir.beBackSlash( true );
	destdir.mkdir();
	destdir.beShortPath();

	// �ʈ��k���[�h���AArchiving�s�̌`���Ȃ�����
	if( each || !(m_Compressor->ability() & aArchive) )
	{
		wfdArray templist;

		for( unsigned int i=0; i!=m_FName.len(); i++ )
		{
			templist.empty();
			templist.add( m_FName[i] );
			tr = m_Compressor->compress( m_BasePath,templist,destdir,m_Method,m_Sfx );
			if( tr<0x8000 || tr==0x8020 )
				result = tr;
		}
	}
	else
		result = m_Compressor->compress( m_BasePath,m_FName,destdir,m_Method,m_Sfx );

	// �J������
	if( result<0x8000 )
		myapp().open_folder( destdir, 2 );
	else if( result!=0x8020 )
	{
		//�G���[�I
		char str[255];
		wsprintf( str, "%s\nError No: [%x]", "Compression Error", result );
		app()->msgBox( str );
	}
}
