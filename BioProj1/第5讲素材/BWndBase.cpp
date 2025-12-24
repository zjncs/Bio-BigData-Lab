//////////////////////////////////////////////////////////////////////
// BWndBase.cpp: CBWndBase 类的实现
// 包含常规窗口功能，作为 CBForm、CBControl 类的基类
//   
//////////////////////////////////////////////////////////////////////

#include "BWndBase.h"


// 仅本模块使用的全局空串字符串空间
TCHAR mEmptyStrBase[2]={0};	



//////////////////////////////////////////////////////////////////////////
// 定义类中的 static 成员和 static 函数

CBHashLK CBWndBase::ms_hashCtrls;
CBHashLK CBWndBase::ms_hashCtrlResIDs;

STWndProp * CBWndBase::PropertyMemCtrl( HWND hWndCtrl, bool bCreate/*=false*/ )
{
	STWndProp * pProp = 
		(STWndProp *)ms_hashCtrls.ItemLong2((long)hWndCtrl, false);
	if (bCreate && pProp==NULL)
	{
		pProp=new STWndProp;
		HM.ZeroMem(pProp, sizeof(STWndProp));	// 新空间清0
		
		// 设置新空间的各成员初始属性值
		pProp->backColor = -1;	// -1 表示使用默认颜色
		pProp->hBrushBack = NULL;
		pProp->foreColor = -1;	// -1 表示使用默认颜色
		SetRect(&pProp->rcPictureClip, -1, -1, -1, -1);		// 设为-1 表示不截取图片的一部分显示
		pProp->fDisableContextMenu = false;	// 是否禁用右键菜单（即是否取消 WM_CONTEXTMENU 消息）

		// 新空间地址记录到 ms_hashCtrls.ItemLong2Set
		if (! ms_hashCtrls.ItemLong2Set((long)hWndCtrl, (long)pProp, false))
		{ delete pProp;  pProp = NULL;  }   // 失败容错
	}

	return pProp;
}


//////////////////////////////////////////////////////////////////////////
// 构造和析构

CBWndBase::CBWndBase( HWND hWndToManage /*= NULL*/, 
					  STWndProp * pProperty /*= NULL */ )
{
	// 成员变量清零
	// 清零不能仅在构造函数中进行，因用 CBControl 的一个对象的
	//   SetResID 先后绑定不同控件时，不调用构造函数但要清零成员
	ClearWndBase();

	// 成员变量赋值
	m_hWnd = hWndToManage;
	m_pPro = pProperty;
}


CBWndBase::~CBWndBase()
{
	// 禁止一切释放系统资源操作，因 CBControl 类继承本类时，
	//   CBControl 是动态绑定的，对象被卸载时，对应的控件不一定被卸载
	
}



//////////////////////////////////////////////////////////////////////////
// 成员函数


STWndProp * CBWndBase::PropMem( bool bCreate/*=false*/ )
{
	if (m_pPro)	
	{
		// 如已有空间地址（如 CBForm 对象创建时设置过，或 CBControl 对象上次指定过）
		// 直接返回
		return m_pPro;
	}
	else
	{
		// 还没有空间地址，如属于 CBControl 对象
		// 若上次为同一控件（hWnd）开辟过空间，则还返回那个空间的地址；
		// 否则，若 bCreate==true，新开辟一个空间，返回新空间的地址
		//       若 bCreate==false，返回 NULL
		// 方法是调用 PropertyMemCtrl 静态函数，以控件的 hWnd 获得
		m_pPro = PropertyMemCtrl(m_hWnd, bCreate);	
		return m_pPro;
	}
}


// 设置和返回 Enabled
void CBWndBase::EnabledSet( bool enabledValue ) const
{
	EnableWindow(m_hWnd, (BOOL)enabledValue);
}

bool CBWndBase::Enabled() const
{
	if (IsWindowEnabled(m_hWnd)) return true; else return false;
}

// 设置和返回 Visible
void CBWndBase::VisibleSet( bool visibleValue ) const
{
	if (visibleValue)
		ShowWindow(m_hWnd, SW_SHOWNA);
	else
		ShowWindow(m_hWnd, SW_HIDE);
}

bool CBWndBase::Visible() const
{
	if (IsWindowVisible(m_hWnd)) return true; else return false;
}


bool CBWndBase::HScrollBar()
{
	if (Style() & WS_HSCROLL) return true; else return false;
}

void CBWndBase::HScrollBarSet(bool bValue)
{
	if (bValue) 
		StyleSet(WS_HSCROLL);
	else
		StyleSet(WS_HSCROLL, -1);
	Refresh();
}

bool CBWndBase::VScrollBar()
{
	if (Style() & WS_VSCROLL) return true; else return false;
}

void CBWndBase::VScrollBarSet(bool bValue)
{
	if (bValue) 
		StyleSet(WS_VSCROLL);
	else
		StyleSet(WS_VSCROLL, -1);
	Refresh();
}




bool CBWndBase::TabStop()
{
	if (Style() & WS_TABSTOP) return true; else return false;
}

void CBWndBase::TabStopSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_TABSTOP);
	else
		StyleSet(WS_TABSTOP, -1);
	Refresh();
}


bool CBWndBase::Group()
{
	if (Style() & WS_GROUP) return true; else return false;
}

void CBWndBase::GroupSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_GROUP);
	else
		StyleSet(WS_GROUP, -1);
	Refresh();
}


LPTSTR CBWndBase::TagString()
{
	m_pPro = PropMem(false);
	if (m_pPro)   
		if (m_pPro->tagString) 
			return m_pPro->tagString;
	
	// 没有设置过附加属性，或附加属性的 tagString == NULL，
	//  都返回全局 mEmptyStrBase 空间的 ""
	mEmptyStrBase[0]=0; return mEmptyStrBase; 	
}

void CBWndBase::TagStringSet( LPCTSTR tagString )
{
	if (tagString==NULL)	// 若是空指针就设为空字符串
	{ mEmptyStrBase[0]=0; tagString=mEmptyStrBase;	}
	
	m_pPro = PropMem(false);// 参数为 false，不开辟附加属性的空间
	// 如果是空字符串，且还未设置过附加属性，保持还未设置即可
	if (m_pPro==NULL && *tagString==0) return;

	
	// 设置附加属性：获取空间
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL)  return; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错
	
	// 删除原字符串的空间，如果有的话。m_pPro->tagString 未由 HM 管理
	if (m_pPro->tagString) delete [](m_pPro->tagString);	
	m_pPro->tagString = new TCHAR[lstrlen(tagString)+1];
	_tcscpy(m_pPro->tagString, tagString);	
}

int CBWndBase::TagInt()
{
	m_pPro = PropMem(false);
	if (m_pPro)  
		return m_pPro->iTag1;
	else
		return 0;
}

void CBWndBase::TagIntSet( int iTag )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) m_pPro->iTag1 = iTag;	
}

int CBWndBase::TagInt2()
{
	m_pPro = PropMem(false);
	if (m_pPro)  
		return m_pPro->iTag2;
	else
		return 0;	
}

void CBWndBase::TagInt2Set( int iTag2 )
{
	m_pPro = PropMem(true);
	// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) m_pPro->iTag2 = iTag2;	
}

double CBWndBase::TagDouble()
{
	m_pPro = PropMem(false);
	if (m_pPro)  
		return m_pPro->dTag;
	else
		return 0;	
}

void CBWndBase::TagDoubleSet( double dTag )
{
	m_pPro = PropMem(true);
	// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) m_pPro->dTag = dTag;		
}







// ================================================
// 位置大小
// ================================================

// 控件大小和位置
int CBWndBase::Left() const
{
	RECT rect;
	POINT pt;
	if (GetWindowRect(m_hWnd, &rect)) 
	{
		pt.x = rect.left; pt.y = rect.top; 

		// 若 父窗口 不为 NULL，则转换为它的父窗口的坐标系
		HWND hWndParent = GetParent(m_hWnd);
		if (hWndParent) ScreenToClient(hWndParent, &pt);	

		return pt.x;
	}
	else
	{
		return 0;
	}
}

int CBWndBase::Top() const
{
	RECT rect;
	POINT pt;
	if (GetWindowRect(m_hWnd, &rect)) 
	{
		pt.x = rect.left; pt.y = rect.top; 

		// 若 父窗口 不为 NULL，则转换为它的父窗口的坐标系
		HWND hWndParent = GetParent(m_hWnd);
		if (hWndParent) ScreenToClient(hWndParent, &pt);	

		return pt.y;
	}
	else
	{
		return 0;
	}
}

int CBWndBase::Width() const
{
	RECT rect;
	POINT pt1, pt2;
	if (GetWindowRect(m_hWnd, &rect)) 
	{
		pt1.x = rect.left; pt1.y = rect.top; 
		pt2.x = rect.right; pt2.y = rect.bottom; 

		// 若 父窗口 不为 NULL，则转换为它的父窗口的坐标系
		HWND hWndParent = GetParent(m_hWnd);
		if (hWndParent)
		{
			ScreenToClient(hWndParent, &pt1);	
			ScreenToClient(hWndParent, &pt2);
		}

		return pt2.x - pt1.x;
	}
	else
	{
		return 0;
	}
}

int CBWndBase::Height() const
{
	RECT rect;
	POINT pt1, pt2;
	if (GetWindowRect(m_hWnd, &rect)) 
	{
		pt1.x = rect.left; pt1.y = rect.top; 
		pt2.x = rect.right; pt2.y = rect.bottom; 

		// 若 父窗口 不为 NULL，则转换为它的父窗口的坐标系
		HWND hWndParent = GetParent(m_hWnd);
		if (hWndParent)
		{
			ScreenToClient(hWndParent, &pt1);	
			ScreenToClient(hWndParent, &pt2);
		}

		return pt2.y - pt1.y;
	}
	else
	{
		return 0;
	}

}


int CBWndBase::ClientWidth() const
{
	RECT rect;
	if (GetClientRect(m_hWnd, &rect))
		return rect.right - rect.left;
	else
		return 0;
}

int CBWndBase::ClientHeight() const
{
	RECT rect;
	if (GetClientRect(m_hWnd, &rect))
		return rect.bottom - rect.top;
	else
		return 0;	
}


void CBWndBase::LeftSet(int left) const
{
	Move(left);
}

void CBWndBase::TopSet(int top) const
{
	Move(0x7FFFFFFF, top);
}

void CBWndBase::WidthSet(int width) const
{
	Move(0x7FFFFFFF, 0x7FFFFFFF, width);
}

void CBWndBase::HeightSet(int height) const
{
	Move(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, height);
}

void CBWndBase::Move( int left/*=0x7FFFFFFF*/, 
					  int top/*=0x7FFFFFFF*/, 
					  int width/*=0x7FFFFFFF*/, 
					  int height/*=0x7FFFFFFF*/ ) const
{
	RECT rect;
	POINT pt1, pt2;
	if (! GetWindowRect(m_hWnd, &rect)) 
	{
		// 不能获得窗口当前大小，为容错，设置为一个默认大小
		rect.left=0; rect.top=0;
		rect.right=100; rect.bottom=100;
	}
	pt1.x = rect.left; pt1.y = rect.top; 
	pt2.x = rect.right; pt2.y = rect.bottom; 

	// 若 父窗口 不为 NULL，则转换为它的父窗口的坐标系
	HWND hWndParent = GetParent(m_hWnd);
	if (hWndParent)
	{
		ScreenToClient(hWndParent, &pt1);	
		ScreenToClient(hWndParent, &pt2);
	}

	// 设置新的位置和大小 => rect
	if (left!=0x7FFFFFFF) { pt2.x += (left-pt1.x); pt1.x = left;  }
	if (top!=0x7FFFFFFF) { pt2.y += (top-pt1.y); pt1.y = top; }
	if (width!=0x7FFFFFFF) pt2.x = pt1.x + width;
	if (height!=0x7FFFFFFF) pt2.y = pt1.y + height;
	
	// 移动控件位置和调整大小
	MoveWindow(m_hWnd, pt1.x, pt1.y, 
		pt2.x-pt1.x, pt2.y-pt1.y, 1);

	InvalidateRect(m_hWnd, NULL, 0);
}








// ================================================
// 外观
// ================================================




long CBWndBase::MousePointer()
{
	m_pPro = PropMem(false);
	if (m_pPro==NULL) return 0;

	return m_pPro->cursorIdx;
}

void CBWndBase::MousePointerSet( EStandardCursor cursor )
{
	MousePointerSet((long)cursor, 0);
}

void CBWndBase::MousePointerSet( long idResCursor, LPCTSTR typeRes/*=0*/ )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
		//上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错

	m_pPro->cursorIdx = idResCursor;  m_pPro->cursorIdx2 = idResCursor;  // m_CursorIdx2 是副本
	if (m_pPro->cursorIdx)
	{
		// 加载光标，句柄存入 m_hCursor
		// =============================================================
		// LoadCursor 函数即使重复被调用，也不会重复加载资源；系统会判断
		//   如果对应光标已经加载，LoadCursor 直接返回句柄
		// =============================================================
		if ( m_pPro->cursorIdx > gc_IDStandCursorIDBase)
		{
			// 标准光标
			// m_CursorIdx-gc_IDStandCursorIDBase 才是标准光标的ID号
			m_pPro->hCursor = 
				LoadCursor(NULL, 
				  MAKEINTRESOURCE(m_pPro->cursorIdx - gc_IDStandCursorIDBase));
			m_pPro->hCursor2 = m_pPro->hCursor;		// 保存副本
		}
		else
		{
			// 资源光标
			// m_CursorIdx 就是资源 ID
			if (typeRes==0)
			{
				// 加载 Cursor 类型的资源
				m_pPro->hCursor = LoadCursor(pApp->hInstance, MAKEINTRESOURCE(m_pPro->cursorIdx));
				m_pPro->hCursor2 = m_pPro->hCursor;		// 保存副本
			}
			else
			{
				// 加载自定义类型的资源（typeRes 类型的资源）
				unsigned long size=0; 
				unsigned char * p= LoadResData(m_pPro->cursorIdx, typeRes, &size);
				m_pPro->hCursor = (HCURSOR)CreateIconFromResource(p, size, 0, 0x00030000);
				m_pPro->hCursor2 = m_pPro->hCursor;		// 保存副本
			}

			// 记录该光标句柄，以便程序退出前自动删除
			pApp->AddImageObjHandle((HANDLE)m_pPro->hCursor, eImgCursor);
		}
	}	
	else	// if (m_CursorIdx)
	{
		// 不特殊设置光标，使用默认：设置 m_hCursor 为 0
		m_pPro->hCursor = NULL; 
		m_pPro->hCursor2 = m_pPro->hCursor;		// 保存副本
	}		// end if (m_CursorIdx)


	// 如果是 Static 控件，必须设置为包含 SS_NOTIFY 风格，否则控件不接收 WM_SETCURSOR 消息
	// -------------------------------------------------------------------------------------------------
	// From: http://stackoverflow.com/questions/19257237/reset-cursor-in-wm-setcursor-handler-properly
	//   Static controls are normally transparent (not in the visual sense, but meaning that even when 
	// the mouse is over the transparent window, Windows will consider the mouse to be over the window 
	// underneath the transparent window).
	//   ... did not work when the static control was created without SS_NOTIFY because without this 
	// style the static control was transparent, which meant that the WARAM in the WM_SETCURSOR message 
	// was never equal to the static control (in other words Windows never considered the mouse to be over 
	// the static control because it was transparent).
	// -------------------------------------------------------------------------------------------------
	if (IsClassName(TEXT("Static"))) StyleSet(SS_NOTIFY, 1);

	// 向本窗口发送 WM_SETCURSOR，以使光标立即生效
	SendMessage(m_hWnd, WM_SETCURSOR, (WPARAM)m_hWnd, 0);
	// 在本窗口不断接收到的 WM_SETCURSOR 消息中会改变鼠标光标
}





COLORREF CBWndBase::BackColor()
{
	m_pPro = PropMem(false);
	if (m_pPro==NULL)   // 未设置过属性颜色：返回系统 COLOR_BTNFACE 颜色
		return GetSysColor(COLOR_BTNFACE);

	if (-1 == m_pPro->backColor)
	{
		// 获得窗口类的背景色
		HBRUSH hBrush;
		LOGBRUSH lb;
		hBrush=(HBRUSH)GetClassLong(m_hWnd, GCL_HBRBACKGROUND);
		if (hBrush) 
		{
			GetObject((HGDIOBJ)hBrush, sizeof(lb), &lb);
			return lb.lbColor;
		}
		else
		{
			// GetClassLong 失败，或窗口类无画刷：返回系统 COLOR_BTNFACE 颜色
			return GetSysColor(COLOR_BTNFACE);
		}
	}
	else
		// 直接返回属性中的 backColor
		return m_pPro->backColor;	
}

void CBWndBase::BackColorSet( EColorType color )
{
	BackColorSet( (COLORREF)GetSysColor(color) );
}

void CBWndBase::BackColorSet( COLORREF color )
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar：不使用XP风格6.0控件时才有效
		SendMessage(m_hWnd, PBM_SETBKCOLOR, 0, (LPARAM)color); 
	}

	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错
	
	// 用属性结构体的 backColor 成员记录背景色
	m_pPro->backColor = color;
	// 同时建立刷子，并用新空间的 hBrushBack 成员记录刷子句柄
	if (m_pPro->hBrushBack) DeleteObject(m_pPro->hBrushBack);
	m_pPro->hBrushBack = CreateSolidBrush(color); 

	InvalidateRect(m_hWnd, NULL, TRUE);	// 使设置生效
}

COLORREF CBWndBase::ForeColor()
{
	m_pPro = PropMem(false);
	if (m_pPro)
		if (-1 != m_pPro->foreColor) 
			return m_pPro->foreColor;
	
	return GetSysColor(COLOR_WINDOWTEXT);
}

void CBWndBase::ForeColorSet( EColorType color )
{
	ForeColorSet( (COLORREF)GetSysColor(color) );
}

void CBWndBase::ForeColorSet( COLORREF color )
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar：不使用XP风格6.0控件时才有效
		SendMessage(m_hWnd, PBM_SETBARCOLOR, 0, (LPARAM)color);  
	}

	m_pPro = PropMem(true);
	// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
		//上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错

	m_pPro->foreColor = color;

	InvalidateRect(m_hWnd, NULL, TRUE);	// 使设置生效
}


// ================================================
// 文本
// ================================================

// 直接设置文本（有各种重载版本）
void CBWndBase::TextSet( LPCTSTR newText ) const
{
	SetWindowText(m_hWnd, newText);
}
void CBWndBase::TextSet(char valueChar) const
{
	TCHAR buff[10];
	_stprintf(buff, TEXT("%c"), valueChar);
	TextSet(buff);
}

void CBWndBase::TextSet(unsigned short int valueInt) const	// TCHAR
{
	LPTSTR buff=new TCHAR [20];
#ifdef UNICODE
	*buff=valueInt;		// 按字符串的方式输出一个字符
	*(buff+1)='\0';		// 须用 TEXT 宏赋值字符，例TCHAR tch=TEXT('汉');
#else
	_stprintf(buff, TEXT("%u"), valueInt);
#endif
	TextSet(buff);
}

void CBWndBase::TextSet(int valueInt) const
{ 
	TCHAR buff[20];
	_stprintf(buff, TEXT("%d"), valueInt);
	TextSet(buff);
}	
void CBWndBase::TextSet(long valueLong) const
{ 
	TCHAR buff[20];
	_stprintf(buff, TEXT("%ld"), valueLong);
	TextSet(buff);
}
void CBWndBase::TextSet(unsigned int valueInt) const
{ 
	TCHAR buff[20];
	_stprintf(buff, TEXT("%u"), valueInt);
	TextSet(buff);
}
void CBWndBase::TextSet(unsigned long valueInt) const
{ 
	TCHAR buff[20];
	_stprintf(buff, TEXT("%lu"), valueInt);
	TextSet(buff);
}
void CBWndBase::TextSet(float valueSng) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.7g"), valueSng);
	TextSet(buff);
}
void CBWndBase::TextSet(double valueDbl) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), valueDbl);
	TextSet(buff);
}
void CBWndBase::TextSet(long double valueDbl) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), valueDbl);
	TextSet(buff);
}

void CBWndBase::TextSet( tstring valueString ) const
{
	TextSet(valueString.c_str());
}

// 现有文本基础上添加文本和设置（有各种重载版本）
void CBWndBase::TextAdd( LPCTSTR newText ) const
{
	int lenText;
	lenText=GetWindowTextLength(m_hWnd);
		
	TCHAR * pText=new TCHAR[lenText+lstrlen(newText)+1];
	GetWindowText(m_hWnd, pText, lenText+1);
	_tcscat(pText, newText);

	SetWindowText(m_hWnd, pText);
	delete []pText;
}

void CBWndBase::TextAdd(char valueChar) const
{
	TCHAR buff[10];
	_stprintf(buff, TEXT("%c"), valueChar);
	TextAdd(buff);
}

void CBWndBase::TextAdd(unsigned short int valueInt) const
{
	LPTSTR buff=new TCHAR [20];
#ifdef UNICODE
	*buff=valueInt;		// 按字符串的方式输出一个字符
	*(buff+1)='\0';		// 须用 TEXT 宏赋值字符，例TCHAR tch=TEXT('汉');
#else
	_stprintf(buff, TEXT("%u"), valueInt);
#endif
	TextAdd(buff);
}

void CBWndBase::TextAdd(int valueInt) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%d"), valueInt);
	TextAdd(buff);
}

void CBWndBase::TextAdd(long valueLong) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%ld"), valueLong);
	TextAdd(buff);
}
void CBWndBase::TextAdd(unsigned int valueInt) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%u"), valueInt);
	TextAdd(buff);
}
void CBWndBase::TextAdd(unsigned long valueInt) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%lu"), valueInt);
	TextAdd(buff);
}
void CBWndBase::TextAdd(float valueSng) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.7g"), valueSng);
	TextAdd(buff);
}
void CBWndBase::TextAdd(double valueDbl) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), valueDbl);
	TextAdd(buff);
}
void CBWndBase::TextAdd(long double valueDbl) const
{
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), valueDbl);
	TextAdd(buff);
}

void CBWndBase::TextAdd( tstring valueString ) const
{
	TextAdd(valueString.c_str());
}


// 获得文本（可获得字符串的，以及转换为整型、double型的）
LPTSTR CBWndBase::Text() const
{
	int lenText;
	lenText=GetWindowTextLength(m_hWnd);

	TCHAR * pText=new TCHAR[lenText+1];
	HM.AddPtr(pText);

	GetWindowText(m_hWnd, pText, lenText+1);
	return pText;
}

double CBWndBase::TextVal() const
{
	int lenText;
	double dblRet=0.0;
	lenText=GetWindowTextLength(m_hWnd);

	TCHAR * pText=new TCHAR[lenText+1];
	GetWindowText(m_hWnd, pText, lenText+1);
	
	dblRet = Val(pText);
	delete []pText;
	return dblRet;
}



// ================================================
// 字体
// ================================================



LPTSTR CBWndBase::FontName()
{
	LOGFONT lf;  GetFontMLF(&lf);
	TCHAR * pStr = new TCHAR [lstrlen(lf.lfFaceName) + 1];
	HM.AddPtr(pStr);
	_tcscpy(pStr, lf.lfFaceName);
	return pStr;
}

void CBWndBase::FontNameSet( LPCTSTR szFontName )
{
	LOGFONT lf;  GetFontMLF(&lf);

	if ( lstrlen(szFontName) > LF_FACESIZE )
	{
		// 若字体名字符串过长，截断之
		_tcsncpy(lf.lfFaceName, szFontName, LF_FACESIZE-1 );
		*(lf.lfFaceName + LF_FACESIZE - 1) = 0;	
	}
	else
	{
		_tcscpy( lf.lfFaceName, szFontName );
	}

	SetFontMLF(&lf);
}

void CBWndBase::FontNameSet( tstring stringFontName )
{
	FontNameSet(stringFontName.c_str());
}

float CBWndBase::FontSize()
{
	LOGFONT lf;  GetFontMLF(&lf);

	HDC hDC=GetDC(m_hWnd);
	float fheight =
		(float)(lf.lfHeight * (-72.0) / GetDeviceCaps(hDC, LOGPIXELSY));
	fheight = (float)((int)(fheight * 100 + 0.5)/100.0);	// 保留 2 位小数
	ReleaseDC(m_hWnd, hDC);

	return fheight;
}

void CBWndBase::FontSizeSet( float fSize )
{
	LOGFONT lf;  GetFontMLF(&lf);

	HDC hDC=GetDC(m_hWnd);
	float fheight = 
		(float)(- fSize * GetDeviceCaps(hDC, LOGPIXELSY) / 72.0);
	lf.lfHeight = (long)(fheight+0.5);	// 四舍五入 fheight
	ReleaseDC(m_hWnd, hDC);

	SetFontMLF(&lf);
}

bool CBWndBase::FontBold()
{
	LOGFONT lf;  GetFontMLF(&lf);
	return (lf.lfWeight >= 700);
}

void CBWndBase::FontBoldSet( bool value )
{
	LOGFONT lf;  GetFontMLF(&lf);

	if (value)
		lf.lfWeight = 700;
	else
		lf.lfWeight = 400;

	SetFontMLF(&lf);
}

bool CBWndBase::FontUnderline()
{
	LOGFONT lf;  GetFontMLF(&lf);
	return (lf.lfUnderline != 0);
}

void CBWndBase::FontUnderlineSet( bool value )
{
	LOGFONT lf;  GetFontMLF(&lf);

	if (value)
		lf.lfUnderline = 1;
	else
		lf.lfUnderline = 0;

	SetFontMLF(&lf);
}

bool CBWndBase::FontItalic()
{
	LOGFONT lf;  GetFontMLF(&lf);
	return (lf.lfItalic != 0);	
}

void CBWndBase::FontItalicSet( bool value )
{
	LOGFONT lf;  GetFontMLF(&lf);
	
	if (value)
		lf.lfItalic = 1;
	else
		lf.lfItalic = 0;

	SetFontMLF(&lf);
}

float CBWndBase::FontRotateDegree()
{
	LOGFONT lf;  GetFontMLF(&lf);
	return (float)(lf.lfEscapement / 10);
}

void CBWndBase::FontRotateDegreeSet( float fDegree )
{
	LOGFONT lf;  GetFontMLF(&lf);
	lf.lfEscapement = (long)(fDegree * 10+0.5);
	SetFontMLF(&lf);
}

BYTE CBWndBase::FontCharSet()
{
	LOGFONT lf;  GetFontMLF(&lf);
	return lf.lfCharSet;
}

void CBWndBase::FontCharSetSet( BYTE ucValue )
{
	LOGFONT lf;  GetFontMLF(&lf);
	lf.lfCharSet = ucValue;
	SetFontMLF(&lf);
}


int CBWndBase::TextWidth( LPCTSTR sText )
{
	HDC hDC = GetDC(m_hWnd);
	HFONT hFontOld = (HFONT)SelectObject(hDC, GetFontMLF(NULL));
	SIZE sz;
	GetTextExtentPoint32(hDC, sText, lstrlen(sText), &sz);
	SelectObject(hDC, hFontOld);
	ReleaseDC(m_hWnd, hDC);
	return sz.cx;
}

int CBWndBase::TextHeight(LPCTSTR sText/*=NULL*/)
{
	TEXTMETRIC tm;
	HDC hDC = GetDC(m_hWnd);
	HFONT hFontOld = (HFONT)SelectObject(hDC, GetFontMLF(NULL));
	if (sText==NULL)
	{
		GetTextMetrics(hDC, &tm);
	}
	else
	{
		SIZE sz;
		GetTextExtentPoint32(hDC, sText, lstrlen(sText), &sz);
		tm.tmHeight = sz.cy;	// 将结果也放入 tm.tmHeight 中以与前一种情况统一
	}
	
	SelectObject(hDC, hFontOld);
	ReleaseDC(m_hWnd, hDC);

	return tm.tmHeight;
}


// protected:

HFONT CBWndBase::GetFontMLF( LOGFONT * lpLf /*=NULL*/ )
{
	HFONT hFont = NULL; 

	m_pPro = PropMem(false);
	if (m_pPro==NULL) 
	{
		// ==== 未设置过属性：控件正在使用系统字体 ====
		// 如果是有父窗口，表示是控件（不是窗体），就用 WM_GETFONT 消息获得系统所用字体
		if (GetParent(m_hWnd))
			hFont = (HFONT)SendMessage (m_hWnd, WM_GETFONT, 0, 0);	// hFont 可能为 NULL
		else
			hFont = NULL;
	}
	else
	{
		hFont = m_pPro->hFont;
	}

	// 获取字体信息
	if (lpLf) 	// 若 lpLf 为空指针，不获取字体信息
	{
		HM.ZeroMem((void *)lpLf, sizeof(LOGFONT));		// 全部成员清零
		if (hFont)
		{
			// 成功获得了字体句柄：获得字体信息
			GetObject( (HGDIOBJ)hFont, sizeof(LOGFONT), lpLf );
		}
		else
		{
			// 无字体句柄，填充 lpLf 为默认字体
			lpLf->lfHeight = -15;
			lpLf->lfWeight = 400;
			_tcscpy(lpLf->lfFaceName, TEXT("宋体"));
		}
	}
	
	// 返回字体句柄（可能为 NULL）
	return hFont;
}


HFONT CBWndBase::SetFontMLF( LOGFONT * lpLf )
{
	HFONT hFontNew = CreateFontIndirect(lpLf);

	if (GetParent(m_hWnd))	// 如果有父窗口表示是对控件（不是对窗体），就发送 WM_SETFONT 消息
	{
		// causes the control to redraw itself immediately upon setting the font
		SendMessage (m_hWnd, WM_SETFONT, (WPARAM)hFontNew, 1);
	}

	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return hFontNew; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错，但仍返回 hFontNew
	
	// 将新建立的字体对象句柄保存到属性空间
	if (m_pPro->hFont)			
		DeleteObject(m_pPro->hFont);	// 删除旧字体对象	
	m_pPro->hFont = hFontNew;			// 用属性空间记录新字体对象
	
	// 返回新字体对象句柄
	return hFontNew;
}



// ================================================
// 边框
// ================================================

bool CBWndBase::Border()
{
	if (Style() & WS_BORDER) return true; else return false;
}

void CBWndBase::BorderSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_BORDER);
	else
		StyleSet(WS_BORDER, -1);
	Refresh();
}

bool CBWndBase::BorderFrameDlg()
{
	if (Style() & WS_DLGFRAME) return true; else return false;
}

void CBWndBase::BorderFrameDlgSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_DLGFRAME);
	else
		StyleSet(WS_DLGFRAME, -1);
	Refresh();
}

bool CBWndBase::BorderFrameThick()
{
	if (Style() & WS_THICKFRAME) return true; else return false;
}

void CBWndBase::BorderFrameThickSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_THICKFRAME);
	else
		StyleSet(WS_THICKFRAME, -1);
	Refresh();
}

bool CBWndBase::BorderRaised()
{
	if (StyleEx() & WS_EX_WINDOWEDGE) return true; else return false;
}

void CBWndBase::BorderRaisedSet( bool bValue )
{
	if (bValue) 
	{
		// 必须有 WS_THICKFRAME 或 WS_DLGFRAME，才能起作用
		if ( ! ( Style() & (WS_THICKFRAME | WS_DLGFRAME) ) )
			StyleSet(WS_DLGFRAME);
		StyleExSet(WS_EX_WINDOWEDGE);
	}
	else
		StyleExSet(WS_EX_WINDOWEDGE, -1);
	Refresh();
}


bool CBWndBase::BorderSunken()
{
	if (StyleEx() & WS_EX_CLIENTEDGE) return true; else return false;
}

void CBWndBase::BorderSunkenSet( bool bValue )
{
	if (bValue) 
		StyleExSet(WS_EX_CLIENTEDGE);
	else
		StyleExSet(WS_EX_CLIENTEDGE, -1);
	Refresh();
}


bool CBWndBase::BorderEtched()
{
	if (Style() & SS_ETCHEDFRAME) return true; else return false;
}

void CBWndBase::BorderEtchedSet( bool bValue )
{
	if (bValue) 
		StyleSet(SS_ETCHEDFRAME);
	else
		StyleSet(SS_ETCHEDFRAME, -1);
	Refresh();	
}

bool CBWndBase::BorderStatic()
{
	if (StyleEx() & WS_EX_STATICEDGE) return true; else return false;
}

void CBWndBase::BorderStaticSet( bool bValue )
{
	if (bValue) 
		StyleExSet(WS_EX_STATICEDGE);
	else
		StyleExSet(WS_EX_STATICEDGE, -1);
	Refresh();
}

bool CBWndBase::BorderTitleBar()
{
	if (Style() & WS_CAPTION) return true; else return false;
}

void CBWndBase::BorderTitleBarSet( bool bValue )
{
	if (bValue) 
		StyleSet(WS_CAPTION);
	else
		StyleSet(WS_CAPTION, -1);
	Refresh();
}


bool CBWndBase::BorderToolWindow()
{
	if (StyleEx() & WS_EX_TOOLWINDOW) return true; else return false;
}

void CBWndBase::BorderToolWindowSet( bool bValue )
{
	if (bValue) 
		StyleExSet(WS_EX_TOOLWINDOW);
	else
		StyleExSet(WS_EX_TOOLWINDOW, -1);
	Refresh();
}





// ================================================
// 绘图
// ================================================


void CBWndBase::PictureSet( UINT bmpResID )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) 
	{
		HANDLE hBmp = LoadImage(pApp->hInstance, (LPCTSTR)bmpResID, IMAGE_BITMAP, 0, 0, LR_SHARED);
			// LR_SHARED：Shares the image handle, returning the same handle, 
			//            if the image is loaded multiple times. not load the image again

		m_pPro->hBmpDisp = (HBITMAP)hBmp;

		InvalidateRect(m_hWnd, NULL, true);	// 使设置生效
	}
}

void CBWndBase::PictureSet( LPCTSTR bmpFile )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) 
	{
		HANDLE hBmp = LoadImage(NULL, bmpFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			// Do not use LR_SHARED for images that have non-standard sizes, 
			//   that may change after loading, or that are loaded from a file.

		m_pPro->hBmpDisp = (HBITMAP)hBmp;

		InvalidateRect(m_hWnd, NULL, true);	// 使设置生效	
	}
}

void CBWndBase::PictureSet( HANDLE hPic, EImgObjType picType )
{
	LONG stl = GetWindowLong(m_hWnd, GWL_STYLE);
	if ( IsClassName(TEXT("Button")) )
	{
		//////////////////////////////////////////////////////////////////////////
		// Button 控件

		// 改变 Style 
		// BM_SETSTYLE
		
		if ( picType == IMAGE_BITMAP )
		{
			// 调整 Button 控件为 BS_BITMAP 风格
			stl = stl & ~BS_ICON;
			stl = stl | BS_BITMAP;
			SetWindowLong(m_hWnd, GWL_STYLE, stl);
		}
		else if ( picType == IMAGE_ICON )
		{
			// 调整 Button 控件为 BS_ICON 风格
			stl = stl & ~BS_BITMAP;
			stl = stl | BS_ICON;
			SetWindowLong(m_hWnd, GWL_STYLE, stl);
		}
		
		// 设置图片
		SendMessage(m_hWnd, BM_SETIMAGE, (WPARAM)picType, (LPARAM)hPic);
	}
	else if ( IsClassName(TEXT("Static")) )
	{
		//////////////////////////////////////////////////////////////////////////
		// Static 控件

		bool bRestoreCENTERIMAGE=false; 
		
		// 若有 SS_CENTERIMAGE 风格在显示图片时可能会有问题：
		//   如 hPic 为 0 表示删除图片，不能立即刷新删除
		//   转换为 SS_ICON 风格时原位图不能被清除 
		// 若有此风格，这里要取消 SS_CENTERIMAGE 之后再恢复
		if (stl & SS_CENTERIMAGE)
		{
			// 暂时取消 SS_CENTERIMAGE
			stl = stl & ~SS_CENTERIMAGE;
			SetWindowLong(m_hWnd, GWL_STYLE, stl);
			bRestoreCENTERIMAGE=true;
		}
		
		// 根据图片类型，调整控件风格，并设置图片
		if ( picType == IMAGE_BITMAP )
		{
			// 调整 Static 控件为 SS_BITMAP 风格
			stl = stl & ~SS_ICON;
			stl = stl & ~SS_ENHMETAFILE;
			stl = stl | SS_BITMAP;
			SetWindowLong(m_hWnd, GWL_STYLE, stl);
			
			// 设置位图
			m_pPro = PropMem(true);
			if (m_pPro) m_pPro->hBmpDisp = (HBITMAP)hPic;  // 将位图句柄保存入 pProp->hBmpAsso

			InvalidateRect(m_hWnd, NULL, TRUE);			 // 在 WM_Paint 时绘制位图（并支持 Stretch）
		}
		else if ( picType == IMAGE_ICON )
		{
			// 调整 Static 控件为 SS_ICON 风格
			stl = stl & ~SS_BITMAP;
			stl = stl & ~SS_ENHMETAFILE;
			stl = stl | SS_ICON;
			SetWindowLong(m_hWnd, GWL_STYLE, stl);
			
			// 设置图标
			SendMessage(m_hWnd, STM_SETIMAGE, (WPARAM)picType, (LPARAM)hPic);
		}
		else	// 其他类型图片
		{
			SendMessage(m_hWnd, STM_SETIMAGE, (WPARAM)picType, (LPARAM)hPic);
		}
		
		// 如需要，恢复 SS_CENTERIMAGE
		if (bRestoreCENTERIMAGE)
			SetWindowLong(m_hWnd, GWL_STYLE, stl | SS_CENTERIMAGE);
	}
	else	// 其他类型控件或窗体（仅支持位图）
	{
		//////////////////////////////////////////////////////////////////////////
		// 其他类型控件或窗体（仅支持位图）
		if ( picType == IMAGE_BITMAP )
		{
			// 设置位图
			m_pPro = PropMem(true);
			if (m_pPro) m_pPro->hBmpDisp = (HBITMAP)hPic;  // 将位图句柄保存入 pProp->hBmpAsso
			InvalidateRect(m_hWnd, NULL, TRUE);			 // 在 WM_Paint 时绘制位图（并支持 Stretch）
		}
	}
	
	// 将图像句柄记录到 pApp，以便程序结束前能自动 DeleteObject
	pApp->AddImageObjHandle((HANDLE)hPic, picType);
}

void CBWndBase::PictureClipSet( int xClip/*=-1*/, int yClip/*=-1*/, int widthClip/*=-1*/, int heightClip/*=-1*/ )
{
	m_pPro = PropMem(true);
	// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) 
	{
		m_pPro->rcPictureClip.left = xClip; 
		m_pPro->rcPictureClip.top = yClip;

		if (widthClip>=0) 
			m_pPro->rcPictureClip.right = xClip + widthClip; 
		else 
			m_pPro->rcPictureClip.right=-1; 

		if (heightClip>=0) 
			m_pPro->rcPictureClip.bottom = yClip + heightClip; 
		else 
			m_pPro->rcPictureClip.bottom=-1; 

		InvalidateRect(m_hWnd, NULL, true);	// 使设置生效：在 WM_Paint 时绘制位图（并支持 Stretch）	
	}
}





bool CBWndBase::Stretch()
{
	m_pPro = PropMem(false);
	if (m_pPro==NULL)   // 未设置过属性
		return false;
	else
		return m_pPro->stretch;
}

void CBWndBase::StretchSet( bool stretchVal )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错
	
	// 用属性结构体的 stretch 成员记录
	m_pPro->stretch = stretchVal;
	
	InvalidateRect(m_hWnd, NULL, TRUE);	// 使设置生效
}







void CBWndBase::PictureSetIcon( EStandardIcon iconStd )
{
	HANDLE hIco = (HANDLE)LoadIcon(NULL, (LPCTSTR)iconStd);
	PictureSet(hIco, eImgIcon);
}

void CBWndBase::PictureSetIcon( UINT iconResID, LPCTSTR typeRes/*=0*/ )
{
	HANDLE hIco = 0; 
	if (typeRes==0)
	{
		// 加载 Icon 类型的资源
		hIco = LoadImage(pApp->hInstance, (LPCTSTR)iconResID, IMAGE_ICON, 0, 0, LR_SHARED);	
	}
	else
	{
		// 加载自定义类型的资源（typeRes 类型的资源）
		unsigned long size=0; 
		unsigned char * p= LoadResData(iconResID, typeRes, &size);
		hIco = CreateIconFromResource(p, size, 0, 0x00030000);
	}
	PictureSet(hIco, eImgIcon);	
}

void CBWndBase::PictureSetIcon( LPCTSTR iconFile )
{
	HANDLE hIco = LoadImage(NULL, iconFile, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	PictureSet(hIco, eImgIcon);	
}









bool CBWndBase::PrintText( LPCTSTR sText, int x/*=-65536*/, int y/*=-65536*/, bool fLineFeed/*=true*/, 
						  bool fBkTransparent/*=true*/, bool fSingleLine/*=true*/ )
{
	HDC hDC = GetDC(m_hWnd);
	HFONT hFont = GetFontMLF(), hFontOld = NULL;
	RECT rcClip;

	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return false; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错

	// ======== 选入字体 ========
	if (hFont)
		hFontOld = (HFONT)SelectObject(hDC, hFont);
	// else 是 hFont==NULL，不选新字体，用 hDC 中的默认字体
	
	// ======== 设置文字颜色和背景 ========
	if ( -1 != m_pPro->foreColor )
		SetTextColor (hDC, m_pPro->foreColor);
	if (fBkTransparent)
		SetBkMode(hDC, TRANSPARENT);	// 文字背景透明
	else
		SetBkMode(hDC, OPAQUE);			// 文字背景不透明
	
	// ======== 设置输出范围 ========
	if ( x > -65536 )  
	{ 
		m_pPro->rcTextOutClip.left = x;  
		m_pPro->iTextOutStartLeft = x; 
	}  // 否则保持原样表示上次输出后的状态
	
	if ( y > -65536 )  
		m_pPro->rcTextOutClip.top = y;  
	   // 否则保持原样表示上次输出后的状态
	
	rcClip.left = m_pPro->rcTextOutClip.left;
	rcClip.top = m_pPro->rcTextOutClip.top;
	rcClip.right = m_pPro->rcTextOutClip.right;
	rcClip.bottom = m_pPro->rcTextOutClip.bottom;
	
	// m_pPro->rcTextOutClip.right、m_pPro->rcTextOutClip.bottom 可能在 PrintTextFormat() 中设置过
	//   现看如 <=0 则自动将之设置为客户区大小
	if (m_pPro->rcTextOutClip.right<=0 || m_pPro->rcTextOutClip.bottom<=0)
	{
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		if (m_pPro->rcTextOutClip.right <= 0) rcClip.right = rc.right;
		if (m_pPro->rcTextOutClip.bottom <= 0) rcClip.bottom = rc.bottom;
		// 不修改 m_TextOutClip，保持 <=0 作为自动调整输出范围为客户区大小的标志
		//   因窗体大小可能改变，如也 <=0，这时下次输出时的输出范围也应对应改变
	}
	
	// ======== 打印文本 ========
	RECT rcText;
	// 获得打印文本的宽度和高度 => rectText
    DrawText(hDC, sText, -1, &rcText, DT_CALCRECT);
	// 打印文本
	if (fSingleLine)
		DrawText(hDC, sText, -1, &rcClip, m_pPro->iTextOutFlags | DT_SINGLELINE);
	else
		DrawText(hDC, sText, -1, &rcClip, m_pPro->iTextOutFlags & ~DT_SINGLELINE);
	
	// ======== 调整下次输出位置 => m_TextOutClip ========
	if (fLineFeed)
	{
		m_pPro->rcTextOutClip.top += (rcText.bottom - rcText.top);
		m_pPro->rcTextOutClip.left = m_pPro->iTextOutStartLeft;	// 新行左边界仍应为原始起始左边界
	}
	else
	{
		m_pPro->rcTextOutClip.left += (rcText.right - rcText.left); 
		// 改变了左边界，以后换行时可恢复左边界为 m_TextOutStartLeft
		if ( m_pPro->rcTextOutClip.left > rcClip.right )
		{
			// 宽度超出范围，向下移动一行
			m_pPro->rcTextOutClip.top += (rcText.bottom - rcText.top);
			m_pPro->rcTextOutClip.left = m_pPro->iTextOutStartLeft;  // 新行左边界仍应为原始起始左边界
		}
	}
	
	// ======== 释放资源 ========
	if (hFontOld) SelectObject(hDC, hFontOld);
	ReleaseDC(m_hWnd, hDC);	

	return true;
}

bool CBWndBase::PrintText( tstring stringText, int x/*=-65536*/, int y/*=-65536*/, bool fLineFeed/*=true*/, bool fBkTransparent/*=true*/, bool fSingleLine/*=true*/ )
{
	return PrintText(stringText.c_str(), x, y, fLineFeed, fBkTransparent, fSingleLine);
}

bool CBWndBase::PrintTextFormat( int clipX/*=0*/, int clipY/*=0*/, int clipWidth/*=-1*/, int clipHeight/*=-1*/, 
								EAlign align/*=eAlignLeft*/, EVAlign valign/*=eVAlignVCenter*/, 
								bool fEndEllipsis/*=false*/, bool fPathEllipsis/*=false*/, 
								bool fANDPrefix/*=true*/, bool bEditControlStyle/*=false*/, int iTabSpace/*=8*/ )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return false; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错

	SetRect(&m_pPro->rcTextOutClip, clipX, clipY, clipX+clipWidth, clipY+clipHeight);
	if (clipWidth<0) m_pPro->rcTextOutClip.right = -1;
	if (clipHeight<0) m_pPro->rcTextOutClip.bottom = -1;


	m_pPro->iTextOutFlags = 0;
	m_pPro->iTextOutFlags |= align;
	m_pPro->iTextOutFlags |= valign;
	if (fEndEllipsis) m_pPro->iTextOutFlags |= DT_END_ELLIPSIS;
	if (fPathEllipsis) m_pPro->iTextOutFlags |= DT_PATH_ELLIPSIS;
	if (!fANDPrefix) m_pPro->iTextOutFlags |= DT_NOPREFIX;
	if (bEditControlStyle) m_pPro->iTextOutFlags |= DT_EDITCONTROL;
	if (iTabSpace) 
	{
		m_pPro->iTextOutFlags |= DT_EXPANDTABS;
		if (iTabSpace!=8)
		{
			m_pPro->iTextOutFlags |= DT_TABSTOP;
			m_pPro->iTextOutFlags |= (iTabSpace<<8);
		}
	}	

	return true;
}






void CBWndBase::Cls()
{
	// 本函数通过 InvalidateRect、WM_PAINT 时实现 Cls 的功能
	// 在 InvalidateRect 之前，需设置附加属性的 fFromCls 标志为 true
	//  在响应 WM_PAINT 时，若此成员为 true 则不再生成 Paint 事件
	//  避免在 Paint 事件函数中用户又调用 Cls 造成死递归

	// 必须设置 fFromCls 标志，如无法分配附加属性空间，返回
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
		// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错
	
	m_pPro->fFromCls = true;
	SetRect(&m_pPro->rcTextOutClip, 0, 0, 0, 0);
	m_pPro -> iTextOutStartLeft = 0;

	InvalidateRect(m_hWnd, NULL, TRUE);

// 	HDC hDC = GetDC(m_hWnd);
// 	HBRUSH hBrush = CreateSolidBrush(BackColor());
// 	RECT rectBK;
// 	
// 	GetClientRect(m_hWnd, &rectBK);
// 	FillRect(hDC, &rectBK, hBrush);
// 	
// 	DeleteObject(hBrush);
// 	ReleaseDC(m_hWnd, hDC);
}








bool CBWndBase::RefreshPicture( HDC hDC, int xEdge/*=0*/, int yEdge/*=0*/ )
{
	m_pPro = PropMem(false);
	if (! m_pPro) return false;				// 返回 false 表示窗体或控件没有设置过图片
	
	HBITMAP hBmp = m_pPro->hBmpDisp;		// 获得原始大小的位图句柄
	if (! hBmp) return false;				// 返回 false 表示窗体或控件没有设置过图片
	
	HDC hDcMem = CreateCompatibleDC(hDC);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hDcMem, hBmp);
	BITMAP bm;
	int xSrc=0, ySrc=0, widSrc=0, heiSrc=0;	// 要绘制到控件的位图中的范围
	
	GetObject(hBmp, sizeof(bm), &bm);
	
	// 如果 pProp->rcPicture 中指定了 left、top，现在调整 xSrc、ySrc
	if ( m_pPro->rcPictureClip.left >= 0) 
		xSrc = m_pPro->rcPictureClip.left;
	if ( m_pPro->rcPictureClip.top >= 0) 
		ySrc = m_pPro->rcPictureClip.top;
	
	// 调整 widSrc、heiSrc：先设置它们为右边界、下边界，再让它们分别减去 xSrc、ySrc
	//   当指定了 pProp->rcPicture 中的 left、top 而未指定 right、bottom 时，
	//   将绘制 left、top 开始到位图边界的部分
	if ( m_pPro->rcPictureClip.right >= 0) 
		widSrc = m_pPro->rcPictureClip.right;	
	else 
		widSrc = bm.bmWidth;
	
	if ( m_pPro->rcPictureClip.bottom >= 0) 
		heiSrc = m_pPro->rcPictureClip.bottom;
	else
		heiSrc = bm.bmHeight;
	
	widSrc -= xSrc;
	heiSrc -= ySrc;
	
	// 绘制位图到控件
	if ( m_pPro->stretch )
	{
		SetStretchBltMode(hDC, HALFTONE);
		StretchBlt(hDC, xEdge, yEdge, ClientWidth()-xEdge, ClientHeight()-yEdge, 
			hDcMem, xSrc,ySrc,widSrc,heiSrc, SRCCOPY);
	}
	else
	{
		BitBlt(hDC, xEdge, yEdge, widSrc-xEdge, heiSrc-yEdge, 
			hDcMem, xSrc, ySrc, SRCCOPY);
	}
	
	// 清除临时资源
	SelectObject(hDcMem, hBmpOld);
	DeleteDC(hDcMem);

	return true; // 返回 true 表示窗体或控件设置过图片且上述绘制了图片
}









// ================================================
// 高级
// ================================================


bool CBWndBase::Transparent()
{
	if (StyleEx() & WS_EX_TRANSPARENT) return true; else return false;
}

void CBWndBase::TransparentSet( bool bTransparent )
{
	if (bTransparent)
		StyleExSet(WS_EX_TRANSPARENT);
	else
		StyleExSet(WS_EX_TRANSPARENT, -1);
}	

int CBWndBase::Opacity()
{
	typedef BOOL (WINAPI *PFUNType)(HWND, COLORREF *, BYTE *, DWORD *);

	if (StyleEx() & WS_EX_LAYERED)
	{
		PFUNType pfun = NULL;
		pfun = (PFUNType)GetProcAddress(GetModuleHandle(TEXT("USER32.dll")), 
			"GetLayeredWindowAttributes");
		if (pfun == 0) return -2;	// 无法获得 GetLayeredWindowAttributes 函数的入口地址

		BYTE ret; DWORD retFlags; 
		(*pfun)(m_hWnd, 0, &ret, &retFlags);
		if (retFlags & LWA_ALPHA)
			return (int)ret; 
		else
			return -1;	// 窗口未被设置此样式
	}
	else
	{
		return -1;		// 窗口未被设置此样式
	}
}

void CBWndBase::OpacitySet( int iOpacity )
{
	typedef BOOL (WINAPI *PFUNType)(HWND, COLORREF, BYTE, DWORD);

	if (iOpacity < 0)
	{
		// 取消 WS_EX_LAYERED 样式
		StyleExSet(WS_EX_LAYERED, -1);
	}
	else
	{
		PFUNType pfun = NULL;
		pfun = (PFUNType)GetProcAddress(GetModuleHandle(TEXT("USER32.dll")), 
			"SetLayeredWindowAttributes");
		if (pfun == 0) return;	// 无法获得 SetLayeredWindowAttributes 函数的入口地址

		BYTE byt = (BYTE)iOpacity;
		// 设置 WS_EX_LAYERED 样式
		StyleExSet(WS_EX_LAYERED, 1);
		if (iOpacity > 255) iOpacity = 255;	// 限制范围在 0～255
		(*pfun)(m_hWnd, 0, byt, LWA_ALPHA);
	}
}

COLORREF CBWndBase::TransparencyKey()
{
	typedef BOOL (WINAPI *PFUNType)(HWND, COLORREF *, BYTE *, DWORD *);

	if (StyleEx() & WS_EX_LAYERED)
	{
		PFUNType pfun = NULL;
		pfun = (PFUNType)GetProcAddress(GetModuleHandle(TEXT("USER32.dll")), 
			"GetLayeredWindowAttributes");
		if (pfun == 0) return -2;	// 无法获得 GetLayeredWindowAttributes 函数的入口地址

		BYTE ret; DWORD retFlags; COLORREF color;
		(*pfun)(m_hWnd, &color, &ret, &retFlags);
		if (retFlags & LWA_COLORKEY)
			return (int)ret; 
		else
			return -1;	// 窗口未被设置此样式
	}
	else
	{
		return -1;		// 窗口未被设置此样式
	}
}

void CBWndBase::TransparencyKeySet(COLORREF iTransColor)
{
	typedef BOOL (WINAPI *PFUNType)(HWND, COLORREF, BYTE, DWORD);
	
	if (iTransColor == 0xffffffff)
	{
		// 取消 WS_EX_LAYERED 样式
		StyleExSet(WS_EX_LAYERED, -1);
	}
	else
	{
		PFUNType pfun = NULL;
		pfun = (PFUNType)GetProcAddress(GetModuleHandle(TEXT("USER32.dll")), 
			"SetLayeredWindowAttributes");
		if (pfun == 0) return;	// 无法获得 SetLayeredWindowAttributes 函数的入口地址
		
		// 设置 WS_EX_LAYERED 样式
		StyleExSet(WS_EX_LAYERED, 1);
		(*pfun)(m_hWnd, iTransColor, 0, LWA_COLORKEY);
	}
}



int CBWndBase::TitleBarBehavior()
{
	m_pPro = PropMem(false);
	if (m_pPro==NULL)   // 未设置过属性
		return false;
	else
		return m_pPro->iTitleBarBehav;

}

void CBWndBase::TitleBarBehaviorSet( int iBehav/*=1*/ )
{
	m_pPro = PropMem(true);
		// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro) m_pPro->iTitleBarBehav = true;

	// 增加风格 SS_NOTIFY，否则不能响应 MouseMove 等消息
	if (iBehav && IsClassName(TEXT("Static")))
		StyleSet(SS_NOTIFY, 1);
}



// 获得窗口的类名（不能通过指针改变类名字符串的内容）
const TCHAR * CBWndBase::ClassName() const
{
	return m_ClassName;
}


bool CBWndBase::IsClassName( LPCTSTR lpTestClassName ) const
{
	return (_tcscmp(m_ClassName, lpTestClassName)==0); 
}

bool CBWndBase::IsClassName( tstring stringTestClassName ) const
{
	return IsClassName(stringTestClassName.c_str());
}



unsigned long CBWndBase::Style()
{
	return (unsigned long)GetWindowLong(m_hWnd, GWL_STYLE);
}

void CBWndBase::StyleSet( unsigned long newStyle, int bOr/*=1*/ )
{
	unsigned long r, rNew;
	r = (unsigned long)GetWindowLong(m_hWnd, GWL_STYLE);
	if (bOr > 0) 
		rNew = r | newStyle;	// 若 bOr > 0， 则在现有风格上增加
	else if (bOr < 0)
		rNew = r & ~newStyle;	// 若 bOr < 0，则在现有风格上取消 newStyle
	else	// bOr == 0
		rNew = newStyle;		// 若 bOr == 0，则将现有风格改为 newStyle

	if (rNew != r) 
		SetWindowLong(m_hWnd, GWL_STYLE, (LONG)rNew);	
}

unsigned long CBWndBase::StyleEx()
{
	return (unsigned long)GetWindowLong(m_hWnd, GWL_EXSTYLE);	
}

void CBWndBase::StyleExSet( unsigned long newStyleEx, int bOr/*=1*/ )
{
	unsigned long r, rNew;
	r = (unsigned long)GetWindowLong(m_hWnd, GWL_EXSTYLE);
	if (bOr > 0) 
		rNew = r | newStyleEx;	// 若 bOr > 0， 则在现有扩展风格上增加
	else if (bOr < 0)			
		rNew = r & ~newStyleEx;	// 若 bOr < 0，则在现有扩展风格上取消 newStyleEx
	else
		rNew = newStyleEx;		// 若 bOr < 0，则在现有扩展风格上取消 newStyleEx

	if (rNew != r) 
		SetWindowLong(m_hWnd, GWL_EXSTYLE, (LONG)rNew);
}




// ================================================
// 方法
// ================================================

void CBWndBase::Refresh()
{
	SetWindowPos(m_hWnd, 0, 0, 0, 0, 0, 
		SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE); 
	InvalidateRect(m_hWnd, NULL, 1);
	UpdateWindow(m_hWnd);
}

void CBWndBase::SetFocus()
{
	::SetFocus(m_hWnd);		// 调用全局 API 函数（与本方法同名）
}

void CBWndBase::ZOrder( int position/*=0*/ )
{
	// 设置窗口或控件的 Z-顺序，即是覆盖其他控件，还是被其他控件所覆盖
	// position=0，则位于其他控件的最前面；否则 位于最后面
	if (m_hWnd==NULL) return;
	UINT flags = SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
	if (position==0)
		SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, flags);
	else
		SetWindowPos(m_hWnd, HWND_BOTTOM, 0, 0, 0, 0, flags);
}












void CBWndBase::Cut()
{
	SendMessage(m_hWnd, WM_CUT, 0, 0);
}

void CBWndBase::Copy()
{
	SendMessage(m_hWnd, WM_COPY, 0, 0);
}

void CBWndBase::Paste()
{
	SendMessage(m_hWnd, WM_PASTE, 0, 0);
}






void CBWndBase::ClearWndBase()
{
	m_hWnd = NULL;
	HM.ZeroMem(m_ClassName, sizeof(m_ClassName));
	m_atom = 0;
	m_pPro = NULL;
}

bool CBWndBase::DisableContextMenu()
{
	m_pPro = PropMem(false);
	if (m_pPro==NULL) 
		return false;   // 未设置过属性
	else
		return m_pPro->fDisableContextMenu;	
}

void CBWndBase::DisableContextMenuSet( bool newValue )
{
	m_pPro = PropMem(true);
	// 参数为 true，若没有设置过附加属性，现在开辟附加属性的空间
	if (m_pPro==NULL) return; 
	// 上条语句参数已为 true，若 m_pPro 仍为 NULL 表示出错

	// 用属性结构体的 fDisableContextMenu 成员记录背景色
	m_pPro->fDisableContextMenu = newValue;
}




