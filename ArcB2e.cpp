
#include "stdafx.h"
#include "ArcB2e.h"
#include "resource.h"
#include "NoahApp.h"

//----------------- ArcB2e�N���X�S�̓I�ȏ��� ------------------------------

char CArcB2e::st_base[MAX_PATH];
int  CArcB2e::st_life=0;
CArcB2e::CB2eCore* CArcB2e::rvm=NULL;

const char* CArcB2e::init_b2e_path()
{
	kiPath dir( kiPath::Exe );
	ki_strcpy( st_base, dir+="b2e\\" );
	return st_base;
}

CArcB2e::CArcB2e( const char* scriptname ) : CArchiver( scriptname )
{
	st_life++;
	exe = NULL;
	m_LstScr = m_DcEScr = m_EncScr =
	m_DecScr = m_SfxScr = m_LoadScr= m_ScriptBuf = NULL;
}

CArcB2e::~CArcB2e()
{
	if( !(--st_life) )
		delete rvm;
	delete [] m_ScriptBuf;
}

//------------------- �X�N���v�g���ɂ��܂�֌W���Ȃ����� -------------------------

bool CArcB2e::v_ver( kiStr& str )
{
	if( !exe )
		return false;
	exe->ver( str );

	kiStr tmp;
	for( int i=0,e=m_subFile.len(); i<e; ++i )
	{
		str += "\r\n";
		CArcModule(m_subFile[i]).ver( tmp );
		str += tmp;
	}
	return true;
}

bool CArcB2e::v_check( const kiPath& aname )
{
	return exe ? exe->chk( aname ) : false;
}

int CArcB2e::v_contents( const kiPath& aname, kiPath& dname )
{
	return exe ? exe->cnt( aname, dname ) : aUnknown;
}

//------------------- �X�N���v�g��ǂݍ��݁�eval( load: ) -------------------

bool CArcB2e::load_module( const char* name )
{
	exe = new CArcModule( name, m_usMode );
	return exe->exist();
}

int CArcB2e::v_load()
{
	//-- �g���X�N���v�g�t�@�C�����J��
	kiStr fname( st_base ); fname += mlt_ext();
	kiFile fp;
	if( fp.open( fname ) )
	{
		//-- �t�@�C���S�̂�ǂݍ���
		unsigned int ln=fp.getSize();
		m_ScriptBuf = new char[ ln+1 ];
		ln = fp.read( (unsigned char*)m_ScriptBuf, ln );
		m_ScriptBuf[ ln ] = '\0';

		//-- section���ɐ؂蕪����
		bool pack1,chk=false;
		for( char* p=m_ScriptBuf; *p; p++ )
		{
			switch( *p )
			{
			case 'c': case 'd': case 'e': case 'l': case 's':
				if( ki_memcmp(p,"load:",5) )
					*p='\0', m_LoadScr = (p+=4)+1;
				else if( ki_memcmp(p,"encode:",7) )
					*p='\0', m_EncScr = (p+=6)+1, pack1=false;
				else if( ki_memcmp(p,"encode1:",8) )
					*p='\0', m_EncScr = (p+=7)+1, pack1=true;
				else if( ki_memcmp(p,"decode:",7) )
					*p='\0', m_DecScr = (p+=6)+1;
				else if( ki_memcmp(p,"sfx:",4) )
					*p='\0', m_SfxScr = (p+=3)+1, m_SfxDirect=false;
				else if( ki_memcmp(p,"sfxd:",5) )
					*p='\0', m_SfxScr = (p+=4)+1, m_SfxDirect=true;
				else if( ki_memcmp(p,"check:",6) )
					*p='\0', (p+=5), chk=true;
				else if( ki_memcmp(p,"decode1:",8) )
					*p='\0', m_DcEScr = (p+=7);
				else if( ki_memcmp(p,"list:",5) )
					*p='\0', m_LstScr = (p+=4);
			}
			while( *p && *p!='\n' && *p!='\r' )
				p++;
			if( *p=='\0' )
				break;
		}

		//-- [load:]�����s�I
		if( m_LoadScr )
		{
			//-- RythpVM �N��
			if( !rvm )
				rvm = new CB2eCore;

			//-- ������
			m_Result=0;
			rvm->setPtr( this,mLod );

			//-- ���s
			rvm->eval( m_LoadScr );

			//-- ����
			if( m_Result==0 )
				return (m_DecScr?aMelt|(m_DcEScr?aList|aMeltEach:0)|(chk?aCheck:0):0)
					 | (m_EncScr?aCompress|(pack1?0:aArchive)|(m_SfxScr?aSfx:0):0);
		}
	}
	return 0;
}

int CArcB2e::exec_script( const char* scr, scr_mode mode )
{
	//-- ������
	m_Result = 0;
	rvm->setPtr( this, mode );

	//-- ���s
	char* script = new char[ki_strlen(scr)+8];
	ki_strcpy( script, "(exec " );
	ki_strcat( script, scr );
	ki_strcat( script, ")" );
	rvm->eval( script );
	delete [] script;

	//-- ����
	return m_Result;
}

//-------------------- ���X�g�A�b�v eval( list: ) -----------------------

bool CArcB2e::v_list( const arcname& aname, aflArray& files )
{
	//-- �X�N���v�g�����ŉ��Ƃ��ł���Ȃ炷��B
	if( !exe )
		return false;
	else if( !m_LstScr )
		return false;

//-- ���X�e�B���O�X�N���v�g�ɕK�v�ȃf�[�^

	// ���ɖ�
	m_psArc = &aname;
	// �t�@�C�����X�g
	m_psAInfo = &files;

//-- ���s�I ---------------------

	return 0==exec_script( m_LstScr, mLst );
}

//-------------------- �W�J���� eval( decode: ) -----------------------

int CArcB2e::v_melt( const arcname& aname, const kiPath& ddir, const aflArray* files )
{
//-- �𓀃X�N���v�g�ɕK�v�ȃf�[�^

	// �J�����g
	::SetCurrentDirectory( ddir );
	// ���ɖ�
	m_psArc = &aname;
	// �o�͐�f�B���N�g��
	m_psDir = &ddir;
	// �t�@�C�����X�g
	m_psAInfo = files;

//-- ���s�I ---------------------

	return exec_script( files ? m_DcEScr : m_DecScr,
						files ? mDc1     : mDec );
}

//-------------------- ���k���� eval( encode: sfx: ) -----------------------

int CArcB2e::cmpr( const char* scr, const kiPath& base, const wfdArray& files, const kiPath& ddir, const int method )
{
//-- ���k�X�N���v�g�ɕK�v�ȃf�[�^

	arcname aname(
		ddir,
		files[0].cAlternateFileName,
		files[0].cFileName );
	int mhd=method+1;

	// �J�����g
	::SetCurrentDirectory( base );
	// ���ɖ�
	m_psArc = &aname;
	// �x�[�X�f�B���N�g��
	m_psDir = &base;
	// ���\�b�h
	m_psMhd = &mhd;
	// ���X�g
	m_psList = &files;

//-- ���s�I --------------------

	return exec_script( scr, mEnc );
}

bool CArcB2e::arc2sfx( const kiPath& temp, const kiPath& dest )
{
//-- SFX�ϊ��X�N���v�g�ɕK�v�ȃf�[�^

	kiFindFile f;
	WIN32_FIND_DATA fd;
	kiPath wild( temp );
	f.begin( wild += "*" );
	if( !f.next( &fd ) )
		return false;
	kiPath from, to, oldname( fd.cFileName );
	arcname aname( temp, fd.cAlternateFileName[0] ? fd.cAlternateFileName : fd.cFileName, fd.cFileName );

	// �J�����g
	::SetCurrentDirectory( temp );
	// ���ɖ�
	m_psArc = &aname;
	// �f�B���N�g��
	m_psDir = &temp;

//-- ���s�I ----------------------

	if( 0x8000<=exec_script( m_SfxScr, mSfx ) )
		return false;

//-- �R�s�[ ----------------------

	bool skipped=false, ans=false;
	f.begin( wild );
	while( f.next( &fd ) )
	{
		if( !skipped && oldname == fd.cFileName ) // �e���|�������ɂ̓R�s�[���Ȃ��B
		{
			skipped=true;
			continue;
		}
		from = temp, from += fd.cFileName;
		to   = dest, to   += fd.cFileName;
		if( ::CopyFile( from, to, FALSE ) )
			ans = true;
	}
	return ans;
}

int CArcB2e::v_compress( const kiPath& base, const wfdArray& files, const kiPath& ddir, int method, bool sfx )
{
	const char* theScript = m_EncScr;

	if( sfx )
	{
		if( m_SfxDirect )
			theScript = m_SfxScr;
		else
		{
			kiPath tmp;
			myapp().get_tempdir( tmp );

			// �e���|�����ֈ��k
			int ans = cmpr( m_EncScr, base, files, tmp, method );
			if( ans < 0x8000 )
				// �e���|�����ɗ����Ă�t�@�C����SFX�ɕϊ����R�s�[�I
				ans = (arc2sfx( tmp, ddir ) ? 0 : 0x8020);

			// �J�����g��߂��Ă����Ȃ��ƍ폜�ł��Ȃ��c(;_;)
			::SetCurrentDirectory( base );
			tmp.remove();
			return ans;
		}
	}

	// �o�͐�֕��ʂɈ��k
	return cmpr( theScript, base, files, ddir, method );
}

//-----------------------------------------------------------------//
//-------------------- RythpVM�̕��̎��� --------------------------//
//-----------------------------------------------------------------//

bool CArcB2e::CB2eCore::exec_function( const kiVar& name, const CharArray& a, const BoolArray& b, int c, kiVar* r )
{
	bool processed = false;

	if( m_mode==mLod ){ //**���[�h����pfunctions****************************
		if( name=="name" ){
			processed=true;

			//---------------------------//
			//-- (name module_filename)--//
			//---------------------------//
			if( c>=2 )
			{
				x->m_usMode = false;
				if( c>=3 )
				{
					getarg( a[2],b[2],&t );
					x->m_usMode = ( t=="us" );
				}

				getarg( a[1],b[1],&t );
				if( x->load_module(t) )
					*r = "exec";
				else
					*r = "", x->m_Result=0xffff;
			}

		}else if( name=="type" ){
			processed=true;

			//-----------------------------------//
			//-- (type ext method1 method2 ...)--//
			//-----------------------------------//
			for( int i=1; i<c; i++ )
			{
				getarg( a[i],b[i],&t );
				if( i==1 )
					x->set_cmp_ext( t );
				else
				{
					const char* ptr=t;
					x->add_cmp_mhd( *ptr=='*' ? ptr+1 : ptr, *ptr=='*' );
				}
			}
		}else if( name=="use" ){
			processed=true;

			//-------------------------------//
			//-- (use module1 module2 ...) --//
			//-------------------------------//
			for( int i=1; i<c; i++ )
			{
				getarg( a[i],b[i],&t );
				x->m_subFile.add( t );
			}
		}
	}else{//************ ���[�h���ɂ͎g���Ȃ�functions *********************
		if( ki_memcmp( (const char*)name, "arc", 3 ) ){
			processed=true;

			//---------------------------//
			//-- (arc[+-].xxx [slfrd]) --//
			//---------------------------//
			arc( ((const char*)name)+3, a, b, c, r );

		}else if( ki_memcmp( (const char*)name, "list", 4 ) ){
			processed=true;

			//----------------------------//
			//-- (list[\*|\*.*] [slfn]) --//
			//----------------------------//
			list( ((const char*)name)+4, a, b, c, r );

		}else if( name=="method" ){
			processed=true;

			//-------------------//
			//-- (method [no]) --//
			//-------------------//
			if( c>=2 )
			{
				getarg( a[1],b[1],&t );
				*r = t.getInt()==*x->m_psMhd ? "1" : "0";
			}
			else
				r->setInt( *x->m_psMhd );

		}else if( name=="dir" ){
			processed=true;

			//-----------//
			//-- (dir) --//
			//-----------//
			*r = (x->m_psDir ? *x->m_psDir : (const char*)"");

		}else if( name=="del" ){
			processed=true;

			//-------------------//
			//-- (del filenam) --//
			//-------------------//
			if( c>=2 )
			{
				getarg( a[1],b[1],&t );
				::DeleteFile( kiPath( t.unquote() ) );
			}

		}else if( ki_memcmp( (const char*)name, "resp", 4 )
			||	  ki_memcmp( (const char*)name, "resq", 4 ) ){
			processed=true;

			//----------------------------//
			//-- (resp[@|-o] (list a)) ---//
			//----------------------------//
			resp( name[3]=='p', ((const char*)name)+4, a, b, c, r );

		}else if( name=="cd" ){
			processed=true;

			//-------------------//
			//-- (cd directory)--//
			//-------------------//
			if( c>=2 )
			{
				getarg( a[1],b[1],&t );
				::SetCurrentDirectory( t.unquote() );
			}

		}else if( name=="cmd" || name=="xcmd" ){
			processed=true;

			//----------------------------//
			//-- (cmd command line ...)---//
			//-- (xcmd command line ...)--//
			//----------------------------//
			if( name[0]=='x' && c<2 )
				x->m_Result = 0xffff;
			else
			{
				CArcModule* xxx = x->exe;
				kiVar       cmd;
				int         i=1;

				if( name[0] == 'x' )
				{
					kiVar mm;
					getarg( a[i],b[i],&mm );
					i++;
					xxx = new CArcModule( mm, x->m_usMode );
				}
				for( ; i<c; i++ )
					getarg( a[i],b[i],&t ), cmd+=t, cmd+=' ';

				bool m = (mycnf().miniboot() || m_mode==mDc1);
				x->m_Result = xxx->cmd( cmd, m );
				r->setInt( x->m_Result );

				if( name[0] == 'x' )
					delete xxx;
			}
		}else if( name=="scan" || name=="xscan" ){
			processed=true;

			//----------------------------------------//
			//-- (scan BL BSL EL SL dx cmd...) -------//
			//-- (xscan BL BSL EL SL dx CMD cmd...) --//
			//----------------------------------------//
			if( c<6 || (name[0]=='x'&&c<7) )
				x->m_Result = 0xffff;
			else
			{
				CArcModule* xxx = x->exe;

				kiVar BL, EL;
				getarg( a[1],b[1],&BL );
				getarg( a[2],b[2],&t );
				int BSL = t.getInt();
				getarg( a[3],b[3],&EL );
				getarg( a[4],b[4],&t );
				int SL = t.getInt();
				getarg( a[5],b[5],&t );
				int dx = t.getInt();

				int i=6;
				if( name[0] == 'x' )
				{
					kiVar mm;
					getarg( a[i],b[i],&mm );
					i++;
					xxx = new CArcModule( mm, x->m_usMode );
				}

				kiVar cmd;
				for( ; i<c; ++i )
					getarg( a[i],b[i],&t ), cmd+=t, cmd+=' ';

				x->m_Result = xxx->lst_exe(
					cmd, *const_cast<aflArray*>(x->m_psAInfo),
					BL, BSL, EL, SL, dx ) ? 0 : -1;

				if( name[0] == 'x' )
					delete xxx;
			}
		}else if( name=="input" ){
			processed=true;

			//-------------------------//
			//-- (input MSG DEFUALT) --//
			//-------------------------//
			kiVar msg, defval;
			if( c>=2 )
				getarg( a[1],b[1],&msg );
			if( c>=3 )
				getarg( a[2],b[2],&defval );
			input( msg, defval, r );
		}else if( name=="size" ){
			processed=true;

			//---------------------//
			//-- (size FILENAME) --//
			//---------------------//
			if( c>=2 )
			{
				kiVar fnm;
				getarg( a[1],b[1],&fnm );
				r->setInt( kiFile::getSize( fnm.unquote() ) );
			}
		}else if( name=="is_file" ){
			processed=true;

			//---------------------//
			//-- (is_file) --------//
			//---------------------//
			if( c==1 )
				*r = (x->m_psList->len()==1
					  && !kiSUtil::isdir( (*x->m_psList)[0].cFileName )) ? "1" : "0";
		}else if( name=="is_folder" ){
			processed=true;

			//---------------------//
			//-- (is_folder) ------//
			//---------------------//
			if( c==1 )
				*r = (x->m_psList->len()==1
					  && kiSUtil::isdir( (*x->m_psList)[0].cFileName )) ? "1" : "0";
		}else if( name=="is_multiple" ){
			processed=true;

			//---------------------//
			//-- (is_multiple) ----//
			//---------------------//
			if( c==1 )
				*r = x->m_psList->len()>1 ? "1" : "0";
		}else if( name=="find" ){
			processed=true;

			//---------------------//
			//-- (find FILENAME) --//
			//---------------------//
			if( c>=2 )
			{
				kiVar fnm;
				getarg( a[1],b[1],&fnm );
				char buf[MAX_PATH];
				if( 0==::SearchPath( NULL,fnm.unquote(),NULL,MAX_PATH,buf,NULL ) )
					*r = "";
				else
					*r = buf, r->quote();
			}
		}
	}

	return processed ? true : kiRythpVM::exec_function(name,a,b,c,r);
}

void CArcB2e::CB2eCore::arc( const char* opt, const CharArray& a, const BoolArray& b,int c, kiVar* r )
{
	//---------------------------//
	//-- (arc[+-].xxx [slfrd]) --//
	//---------------------------//

	// �f�t�H���g�I�v�V�����ݒ�
	const char* anm=x->m_psArc->lname;
	enum{ full, nam, dir } part=full;
	if( m_mode==mSfx )	part=nam; // sfx

	// �w�肪����Ώ㏑
	if( c>=2 )
	{
		getarg( a[1],b[1],&t );
		for( const char* p=t; *p; p++ )
			switch(*p)
			{
			case 's': anm=x->m_psArc->sname; break;
			case 'l': anm=x->m_psArc->lname; break;
			case 'f': part=full; break;
			case 'n': part=nam;  break;
			case 'd': part=dir;  break;
			}
	}

	// �f�B���N�g������
	*r = (part==nam ? (const char*)"" : x->m_psArc->basedir);

	// ���O����
	if( part != dir )
	{
		if( *opt=='\0' || *opt=='+' )
		{
			// (arc)       : anm�����̂܂ܕԂ�
			*r += anm;
			// (arc+XXX)   : anmXXX��Ԃ�
			if( *opt=='+' )
				*r += (opt+1);
		}
		else
		{
			const char* ext = kiPath::ext(anm);
			const char* add = "";
			if( opt[0]=='-' && opt[1]=='.' )
			{
				// (arc-.XXX) : ������.XXX��������폜�B
				//            : �����łȂ���Ό���.decompressed
				if( 0!=ki_strcmpi( ext, opt+2 ) )
					ext = anm + ki_strlen(anm), add = ".decompressed";
			}
			else if( opt[1]!='\0' )
			{
				// (arc.XXX)  : �Ō�̊g���q��.XXX�Ɏ��ւ�
				add = opt;
				switch(mycnf().extnum())
				{
				case 0: ext = anm + ::lstrlen(anm);break; 
				case 1: ext = kiPath::ext(anm);    break;
				default:ext = kiPath::ext_all(anm);break;
				}
			}
			else
			{
				// (arc.)     : �g���q��S�Ď�菜��
				switch(mycnf().extnum())
				{
				case 0: ext = anm + ::lstrlen(anm);break; 
				case 1: ext = kiPath::ext(anm);    break;
				default:ext = kiPath::ext_all(anm);break;
				}
			}
			if( *ext )
				ext--;

			char buf[MAX_PATH];
			ki_memcpy( buf, anm, ext-anm );
			buf[ ext-anm ] = '\0';
			*r += buf;
			*r += add;
		}

		// �K�v�Ȃ炭����
		if( part==full )
			r->quote();
	}
}

static void selfR(
	const char* writedir, const char* fullpath, bool lfn, kiVar* r )
{
	kiFindFile       f;
	WIN32_FIND_DATA fd;
	f.begin( kiStr(fullpath) += "\\*" );

	kiVar t, t2, t3;
	while( f.next(&fd) )
	{
		t = writedir;
		t+= '\\';
		t+= (lfn ? fd.cFileName : fd.cAlternateFileName);
		if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			t2 = t;
			t  = "";
			t3 = fullpath;
			t3+= '\\';
			t3+= (lfn ? fd.cFileName : fd.cAlternateFileName);
			selfR( t2, t3, lfn, &t );
		}
		else
		{
			if( lfn )
				t.quote();
		}
		*r += t;
		*r += ' ';
	}
}

void CArcB2e::CB2eCore::list( const char* opt, const CharArray& a, const BoolArray& b,int c, kiVar* r )
{
	//---------------------------//
	//-- (list[r|\*.*] [slfn]) --//
	//---------------------------//

	if( m_mode!=mEnc ) // �𓀂̏ꍇ
	{
		*r = "";

		for( unsigned int i=0; i!=x->m_psAInfo->len(); i++ )
			if( (*x->m_psAInfo)[i].selected )
			{
				// - �Ŏn�܂郄�c�΍�����邩�H
				t = (*x->m_psAInfo)[i].inf.szFileName;
				t.quote();
				*r += t;
				*r += ' ';
			}
	}
	else // ���k�̏ꍇ
	{
		// �f�t�H���g�I�v�V�����ݒ�
		bool lfn=true;
		enum{ full, nam } part=nam;
		// �w�肪����Ώ㏑
		if( c>=2 )
		{
			getarg( a[1],b[1],&t );
			for( const char* p=t; *p; p++ )
				switch(*p)
				{
				case 's': lfn=false; break;
				case 'l': lfn=true;  break;
				case 'f': part=full; break;
				case 'n': part=nam;  break;
				}
		}
		// ���O�ōċA���X�g�A�b�v���s�����ۂ�
		bool selfrecurse = (*opt=='r');

		// �f�B���N�g�����̌��ɕt���������́B
		if( *opt=='\\' || *opt=='/' )
			opt++;

		// ���X�g�A�b�v
		kiVar t2,t3;
		*r = "";
		for( unsigned int i=0; i!=x->m_psList->len(); i++ )
		{
			// �t�@�C��������
			t = ( part==full ? *x->m_psDir : (const char*)"");
			t += lfn ? (*x->m_psList)[i].cFileName : (*x->m_psList)[i].cAlternateFileName;

			if( selfrecurse && ((*x->m_psList)[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				// �Z���t�ċA
				t2 = t;
				t  = "";
				t3 = *x->m_psDir;
				t3+= lfn ? (*x->m_psList)[i].cFileName : (*x->m_psList)[i].cAlternateFileName;
				selfR( t2, t3, lfn, &t );
			}
			else
			{
				// �m�[�}������
				if( *opt && ((*x->m_psList)[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
					t += '\\', t += opt;
				if( lfn )
					t.quote();
			}
			*r += t;
			*r += ' ';
		}
	}

	r->removeTrailWS();
}

void CArcB2e::CB2eCore::resp( bool needq, const char* opt, const CharArray& a, const BoolArray& b,int c, kiVar* r )
{
	//-----------------------------//
	//-- (resp[@|-o] (list) ...) --//
	//-----------------------------//

	// ���X�|���X�t�@�C�����쐬
	kiPath rspfile;
	myapp().get_tempdir(rspfile);
	rspfile += "filelist";

	// �I�v�V�����ƌ������ĕԂ�
	*r  = opt;
	*r += rspfile;

	// �t�@�C���֏�������
	kiFile fp;
	if( !fp.open( rspfile,false ) )
		return;

	kiVar tmp;
	for( int i=1; i<c; i++ )
	{
		// fp�֊e������split���Ȃ��珑������
		getarg( a[i],b[i],&tmp );

		for( const char *s,*p=tmp; *p; p++ )
		{
			// �]���ȋ󔒂̓X�L�b�v
			while( *p==' ' )
				p++;
			if( *p=='\0' )
				break;

			// �����̏I���ցc
			s=p;
			for( int q=0; *p!='\0' && (*p!=' ' || (q&1)!=0); p++ )
				if( *p=='"' )
					q++;

			// "�̂��܍��킹�ꍆ
			if( !needq && *s=='"' )
			{
				s++;
				if( p!=s && *(p-1)=='"' )
					p--;
			}

			fp.write( s, p-s );
			fp.write( "\r\n", 2 );

			// "�̂��܍��킹��
			if( *p=='"' )
				p++;
			if( *p=='\0' )
				break;
		}
	}
}

void CArcB2e::CB2eCore::input( const char* msg, const char* defval, kiVar* r )
{
	struct CB2eInputDlg : public kiDialog
	{
		const char* msg;
		const char* def;
		kiVar*      res;

		CB2eInputDlg( const char* m, const char* d, kiVar* r )
			: kiDialog( IDD_PASSWORD ), msg(m), def(d), res(r) {}
		BOOL onInit()
			{
				sendMsgToItem( IDC_EDIT, WM_SETTEXT, 0, (LPARAM)def );
				sendMsgToItem( IDC_MESSAGE, WM_SETTEXT, 0, (LPARAM)msg );
				::ShowWindow( item(IDC_MASK), SW_HIDE );
				::ShowWindow( item(IDCANCEL), SW_HIDE );
				::EnableWindow( item(IDC_MASK), FALSE );
				::EnableWindow( item(IDCANCEL), FALSE );
				::SetFocus( item(IDC_EDIT) );
				return TRUE;
			}
		bool onOK()
			{
				char* buf = new char[32768];
				sendMsgToItem( IDC_EDIT, WM_GETTEXT, 32768, (LPARAM)buf );
				*res = buf;
				delete [] buf;
				return true;
			}
	};

	CB2eInputDlg d( msg, defval, r );
	d.doModal( app()->mainhwnd() );
}

