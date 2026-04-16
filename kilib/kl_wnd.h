//--- K.I.LIB ---
// kl_wnd.h : window information manager

#ifndef AFX_KIWINDOW_H__26105B94_1E36_42FA_8916_C2F7FB9EF994__INCLUDED_
#define AFX_KIWINDOW_H__26105B94_1E36_42FA_8916_C2F7FB9EF994__INCLUDED_

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// kiWindow : Window�̊ȒP�ȊǗ�

class kiWindow
{
friend void kilib_startUp();

private: //-- �O���[�o���ȏ����������Ȃ� ---------------------

	static void init();
	static void finish();

public: //-- �O�����C���^�[�t�F�C�X --------------------------

	// �֘A�t�����Ă���HWND
	HWND hwnd()
		{
			return m_hWnd;
		}

	// ����Window�p�ɃA�N�Z�����[�^�����[�h
	void loadAccel( UINT id );

	// �E�C���h�E���܂��c���Ă��邩�ǂ����`�F�b�N
	bool isAlive()
		{
			if( !m_hWnd )
				return false;
			if( ::IsWindow(m_hWnd) )
				return true;
			m_hWnd = NULL;
			return false;
		}

	// �e
	kiWindow* parent()
		{
			return kiwnd( ::GetParent( hwnd() ) );
		}

	// ���b�Z�[�W���M
	int sendMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 )
		{
			return ::SendMessage( hwnd(), msg, wp, lp );
		}

	// [static] �L���[�ɂ��郁�b�Z�[�W��S�ď���
	static void msg();

	// [static] ���b�Z�[�W���[�v���܂킷�B
	enum msglooptype {PEEK, GET};
	static void msgLoop( msglooptype type = GET );

	// [static] Window�������I�� front ��
	static void setFront( HWND wnd );

	// [static] Window�𒆉���
	static void setCenter( HWND wnd, HWND rel=NULL );

	// [static] HWND -> kiWindow ( ��������� )
	static kiWindow* kiwnd( HWND wnd )
		{
			kiWindow* ptr = (kiWindow*)::GetWindowLongPtr( wnd, GWLP_USERDATA );
			if( !ptr ) return NULL;
			if( ::IsBadCodePtr((FARPROC)&ptr) ) return NULL;
			return ptr;
		}

protected: //-- �h���N���X���� -----------------------------

	// �h���N���X�́A�쐬���O�ɃR�����ĂԂ��ƁB
	static void preCreate( kiWindow* wnd )
		{ st_pCurInit = wnd; }
	// �j�����O�ɃR�����ĂԂ��ƁB
	void detachHwnd();
	// ��������GET/POST���b�Z�[�W���[�v���~
	static void loopbreak()
		{
			loopbreaker = true;
		}

private: //-- �������� -------------------------------------

	// �E�C���h�E�n���h���ݒ�
	static LRESULT CALLBACK CBTProc( int code, WPARAM wp, LPARAM lp );
	static HHOOK st_hHook;
	static kiWindow* st_pCurInit;
	void setHwnd( HWND wnd )
		{
			m_hWnd = wnd;
		}

	// �E�C���h�E���ێ��p�ϐ�
	HWND m_hWnd;
	HACCEL m_hAccel;
	// �_�C�A���O���b�Z�[�W
	virtual bool isDlgMsg( MSG* msg )
		{ return false; }
	// GET���[�v�ꎞ��������
	static bool loopbreaker;

protected:
	kiWindow();
public:
	virtual ~kiWindow();
};

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// kiDialog : Dialog��kiWindow�Ƃ��ĊǗ�����

class kiDialog : public kiWindow
{
public: //-- �O�����C���^�[�t�F�C�X --------------------------

	// ���[�_���_�C�A���O�Ƃ��Ď��s
	virtual void doModal( HWND parent=NULL );

	// ���[�h���X�_�C�A���O�Ƃ��č쐬
	virtual void createModeless( HWND parent=NULL );

	// �I���R�[�h�擾
	UINT getEndCode()
		{
			return m_EndCode;
		}

	// ���[�_�����ۂ�
	bool isModal()
		{
			return m_bStateModal;
		}

	// �_�C�A���O�A�C�e��
	int sendMsgToItem( UINT id, UINT msg, WPARAM wp=0, LPARAM lp=0 )
		{
			return ::SendDlgItemMessage( hwnd(), id, msg, wp, lp );
		}
	HWND item( UINT id )
		{
			return ::GetDlgItem( hwnd(), id );
		}

protected: //-- �h���N���X���� -----------------------------

	// ���\�[�XID�ŏ�����
	kiDialog( UINT id );

	// ���\�[�XID�擾
	UINT getRsrcID()
		{
			return m_Rsrc;
		}

	// �I���R�[�h���Z�b�g
	void setEndCode( UINT endcode )
		{
			m_EndCode = endcode;
		}

	// ���[�_�����ۂ��̃t���O"�̂�"��ؑ�
	void setState( bool modal )
		{
			m_bStateModal = modal;
		}

	// �I���R�[�h���Z�b�g���āA�I������( IDOK ��n���Ă� onOK() �͌Ă΂�Ȃ����Ƃɒ��ӁI )
	virtual void end( UINT endcode );

	// �R�}���h�E���b�Z�[�W�������ɌĂ΂��

		// �n�j -> onOK     -> if true end(IDOK)
		virtual bool onOK() {return true;}
		// ��� -> onCancel -> if true end(IDCANCEL)
		virtual bool onCancel() {return true;}
		// WM_INITDIALOG      -> onInit
		virtual BOOL onInit() {return FALSE;}
		// WM_????            -> proc
		virtual BOOL CALLBACK proc( UINT msg, WPARAM wp, LPARAM lp ) {return FALSE;}

private: //-- �������� -------------------------------------

	UINT m_EndCode;
	UINT m_Rsrc;
	bool m_bStateModal;
	bool isDlgMsg( MSG* msg )
		{
			return (!!::IsDialogMessage( hwnd(), msg ));
		}
	static INT_PTR CALLBACK commonDlg( HWND dlg, UINT msg, WPARAM wp, LPARAM lp );
};

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// kiPropSheet : PropertySheet��kiWindow�Ƃ��ĊǗ�����B

#define IDAPPLY      (0x3021)
#define ID_KIPS_HELP (0x0009)

class kiPropSheetPage : public kiDialog
{
friend class kiPropSheet;

protected: //-- �h���N���X���� ----------------------------

	// �_�C�A���O��A�C�R����ID�ŏ�����
	kiPropSheetPage( UINT dlgid )
		: kiDialog( dlgid ), m_hIcon( NULL ) {}
	void setIcon( HICON h )
		{ m_hIcon = h; }

	// �n�j/�K�p -> page::onOK -> sheet::onOK -> (if ok �I��)
	// virtual bool onOK()
	// WM_INITDIALOG
	// virtual BOOL onInit()
	// ���̑�
	// virtual BOOL CALLBACK proc( UINT msg, WPARAM wp, LPARAM lp )

private: //-- �������� -------------------------------------

	void end( UINT endcode ) {}
	void setInfo( PROPSHEETPAGE* p );
	HICON m_hIcon;
};

class kiPropSheet : public kiDialog
{
friend class kiPropSheetPage;

public: //-- �O�����C���^�[�t�F�C�X --------------------------

	// ���[�_���_�C�A���O�Ƃ��Ď��s
	void doModal( HWND parent );

	// ���[�h���X�_�C�A���O�Ƃ��č쐬
	void createModeless( HWND parent );

protected: //-- �h���N���X���� ----------------------------

	// �R���X�g���N�^�ӂ�Ł������������ׂ�
	PROPSHEETHEADER m_Header;
	kiArray<kiPropSheetPage*> m_Pages;

	// �I��
	void end( UINT endcode );
	// 
	void sendOK2All()
	{
		for( unsigned int i=0;i!=m_Pages.len(); i++ )
			if( m_Pages[i]->isAlive() )
				m_Pages[i]->onOK();
	}

	// �n�j/�K�p -> page::onOK -> sheet::onOK -> (if ok �I��)
	// virtual void onOK()
	// �L�����Z�� -> sheet::onCancel -> �I��
	// virtual void onCancel()
	// PSCB_INITIALIZED
	// virtual BOOL onInit()
	// �K�p
	virtual void onApply() {}
	// �w���v
	virtual void onHelp() {}
	// ���̑��R�}���h
	virtual void onCommand( UINT id ) {}
	// �t�@�C���h���b�v
	virtual void onDrop( HDROP hdrop ) {}

private: //-- �������� ---------------------------------------

	void begin();
	bool m_bStateModal;

	static kiPropSheet* st_CurInitPS;
	WNDPROC m_DefProc;
	bool isDlgMsg( MSG* msg )
		{ return !!PropSheet_IsDialogMessage( hwnd(),msg ); }
	static int CALLBACK main_initProc( HWND dlg, UINT msg, LPARAM lp );
	static LRESULT CALLBACK main_cmmnProc( HWND dlg, UINT msg, WPARAM wp, LPARAM lp );
	static INT_PTR CALLBACK page_cmmnProc( HWND dlg, UINT msg, WPARAM wp, LPARAM lp );
	static UINT CALLBACK page_initProc( HWND dlg, UINT msg, LPPROPSHEETPAGE ppsp );

protected:
	kiPropSheet();
public:
	~kiPropSheet()
		{ for( unsigned int i=0; i!=m_Pages.len(); i++ ) delete m_Pages[i]; }
};

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// kiListView : ListView�R���g���[���̊ȒP�ȃ��b�p

class kiListView
{
public:
	kiListView( kiDialog* dlg, UINT id )
		{
			m_hWnd = ::GetDlgItem( dlg->hwnd(), id );
		}

	void insertColumn( int y, const char* title,
						int width=100, int fmt=LVCFMT_LEFT )
		{
			LVCOLUMN col;
			col.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
			col.pszText = const_cast<char*>(title);
			col.cx = width;
			col.fmt = fmt;
			::SendMessage( m_hWnd, LVM_INSERTCOLUMN, y, (LPARAM)&col );
		}

	void insertItem( int x, const char* str, LPARAM param=0, int iImage=-1 )
		{
			LVITEM item;
			item.mask = LVIF_TEXT | LVIF_PARAM | (iImage!=-1 ? LVIF_IMAGE : 0);
			item.pszText = const_cast<char*>(str);
			item.iItem = x;
			item.iSubItem = 0;
			item.iImage = iImage;
			item.lParam = param; 
			::SendMessage( m_hWnd, LVM_INSERTITEM, 0, (LPARAM)&item );
		}

	void setSubItem( int x, int y, const char* str )
		{
			LVITEM item;
			item.mask = LVIF_TEXT;
			item.pszText = const_cast<char*>(str);
			item.iItem = x;
			item.iSubItem = y;
			::SendMessage( m_hWnd, LVM_SETITEM, 0, (LPARAM)&item );
		}

	void setImageList( HIMAGELIST Large, HIMAGELIST Small )
		{
			::SendMessage( m_hWnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)Large );
			::SendMessage( m_hWnd, LVM_SETIMAGELIST, LVSIL_SMALL,  (LPARAM)Small );
		}

private:
	HWND m_hWnd;
};

#endif
