//////////////////////////////////////////////////////////////////////
// BControl.cpp: CBControl 类的实现
// 实现对话框中的一个控件的各种功能
//   
//////////////////////////////////////////////////////////////////////

#include <tchar.h>
#include <stdio.h>	// 用 _stprintf
#include "BForm.h"

// 仅本模块使用的全局空串字符串空间
TCHAR mEmptyStr[2]={0};	


// 构造函数
CBControl::CBControl( unsigned short int idResControl /*=0*/)
					 // 基类 CBWndBase 的第1个参数（控件hWnd）稍后由 SetResID() 设置
					 // 基类 CBWndBase 的第3个参数设为 NULL，属性空间的地址需要时基类会自动获得
{
	SetResID(idResControl);
}

CBControl::CBControl( HWND hWndCtrl /*= NULL*/ )
					  // 基类 CBWndBase 的第1个参数（控件hWnd）稍后由 SetResID() 设置
					  // 基类 CBWndBase 的第3个参数设为 NULL，属性空间的地址需要时基类会自动获得
{
	SetResID(hWndCtrl);	
}


// 析构函数
CBControl::~CBControl()
{
	// 禁止一切释放系统资源操作，因 CBControl 是动态绑定的
	//   本类对象被卸载时，对应的控件不一定被卸载
	;
}



HWND CBControl::SetResID( unsigned short int idResControl/*=0*/ )
{
	HWND hWndCtrl;
	
	// 记录 m_ResCtrlID，因这个设置后，重载版的 SetResID 不再重复设置
	m_ResCtrlID = idResControl;
	
	// 获得控件 hWnd，方法是从 ms_hashCtrlResIDs 哈希表中按照 ID 获得
	// 因此之前必须加载了窗体，那期间做了 EnumChildProcSubClass，
	//   在此哈希表中添加了控件 ID 信息
	hWndCtrl = (HWND)CBWndBase::ms_hashCtrlResIDs.Item((long)idResControl, false);

	// 调用重载版的 SetResID （第2个参数为控件句柄）设置
	// 如果 hWndCtrl 为 NULL，下面函数会设置 m_ResCtrlID 为 0
	return SetResID(hWndCtrl);
}


HWND CBControl::SetResID( HWND hWndCtrl/*=NULL*/ )
{
	ClearWndBase();		// 基类成员变量清零

	// ==== 根据参数设置成员变量 ====
	if (hWndCtrl)
	{
		// 将相关信息记录到成员变量
		m_hWnd = hWndCtrl;
		
		// 获得类的唯一标识 Atom
		m_atom = GetClassLong(hWndCtrl, GCW_ATOM);
		
		// 获得类名字符串 => m_ClassName
		GetClassName(m_hWnd, m_ClassName, sizeof(m_ClassName)/sizeof(TCHAR)-1);

		// 如 m_ResCtrlID 不为 0，说明已设置过，不再设置它的值；
		// 否则现在通过 GetDlgCtrlID 设置
		if (! m_ResCtrlID) m_ResCtrlID=GetDlgCtrlID(hWndCtrl);
	}
	else
	{
		// hWndCtrl 为 NULL，设置 m_ResCtrlID 为0，不要让 m_ResCtrlID 有内容
		m_ResCtrlID = 0;
	}

	return hWndCtrl;
}



void CBControl::EnabledSet( bool enabledValue )
{
	EnableWindow(m_hWnd, (BOOL)enabledValue);

	// 如果是图片按钮，再在按钮上显示表示对应 Enabled 状态的图片
	m_pPro = PropMem(false);
	if (m_pPro)
	{
		if (m_pPro->ePicBtn)
		{
			if (enabledValue) PicButtonStateSet(0); else PicButtonStateSet(4, -1);
		}
	}
}





// 仅有 CBControl 类有 TextInt；
// Text、TextVal 继承 CBWndBase 基类
int CBControl::TextInt(BOOL * retSuccess/*=0*/)
{
	return GetDlgItemInt(GetParent(m_hWnd), m_ResCtrlID, retSuccess , 1);	
}





// =================== Static 类控件的功能 ===================


bool CBControl::BackStyleTransparent()
{
	m_pPro = PropMem(false);
	if (m_pPro) 
		return m_pPro->bBackStyleTransparent;
	else
		return false;
}

void CBControl::BackStyleTransparentSet( bool bTransparent )
{
	m_pPro = PropMem(true);
	if (m_pPro) m_pPro->bBackStyleTransparent = bTransparent;
}


// 设置一个 Static 控件为图片按钮，
//    按钮大小为控件 Width、Height；各按钮状态的位图组合为一张大位图，组合位图资源ID为 bmpCombResID
// bmpCombResID=0 时，使用 PictureSet() 设置的位图
// btnX, btnY, btnWidth, btnHeight 为图片按钮的位置、大小，如 >=0 则自动调整控件大小为此值（这4个参数只为调用本函数时
//   自动设置控件位置、大小）
// iStateStyle = 1时，使用两态按钮：单击一次按下、再单击一次抬起；可由 ValueChecked() 获得其按下/抬起 状态
void CBControl::PicButtonSet( EPicButtonType style/*=epbNone*/, UINT bmpCombResID/*=0*/, 
							 int btnWidth/*=-1*/, int btnHeight/*=-1 */, int btnX/*=-1*/, int btnY/*=-1*/, 
							 int iStateStyle/*=0*/)
{
	if (bmpCombResID) PictureSet(bmpCombResID);
	if (btnX>=0 && btnY>=0 && btnWidth>=0 && btnHeight>=0)
	{
		// btnX、btnY、btnWidth、btnHeight 四个参数全都给出
		// 通过 Move 设置控件位置、大小，以提高效率
		Move(btnX, btnY, btnWidth, btnHeight);
	}
	else
	{
		// 只分别给出 Left、Top、Width、Height 中一部分参数情况：分别设置
		if (btnX>=0) LeftSet(btnX);
		if (btnY>=0) TopSet(btnY);
		if (btnWidth>=0) WidthSet(btnWidth);
		if (btnHeight>=0) HeightSet(btnHeight);
	}

	// 设置附加属性为：图片按钮
	m_pPro = PropMem(true);	// 获得属性空间的地址
	if (m_pPro)
	{
		m_pPro->ePicBtn = style;
		m_pPro->iPicBtnStateStyle = iStateStyle;
	}
	
	// Static 控件加 SS_NOTIFY 风格已在 EnumChildProcSubClass 中完成

	// 显示常规按钮状态
	PicButtonStateSet(0);
}



// 设置图片按钮的状态：0=常规；1=高亮；2=按下；3=灰色
// iSetOrReleaseCapture：是否同时 SetCapture (=1) 或 ReleaseCapture(=-1) 或不做 (=0)
// 如 iSetOrReleaseCapture 非0，无论如何都会做 SetCapture 或 ReleaseCapture
// 本函数仅设置控件的外观状态，要设置 Checked 状态，请使用 ValueCheckedSet()
void CBControl::PicButtonStateSet( int iState, int iSetOrReleaseCapture/*=0*/ )
{
	// SetCapture 或 ReleaseCapture
	if (iSetOrReleaseCapture>0) SetCapture(m_hWnd);
	else if (iSetOrReleaseCapture<0) ReleaseCapture(); 

	
	m_pPro = PropMem(true);	// 获得属性空间的地址
	if (!m_pPro) return; 
	if (m_pPro->iPicBtnCurrDisp == iState) return;  // 如果当前已经显示为 这种状态，就不重复显示了
	

	RECT rc;  
	unsigned uMaskClpLeft=0, uMaskClpTop=0;    // 用于计算 rcPictureClip.Left 和 .Top 的掩码
	unsigned uMoveLeft=0, uMoveTop=0;	// 用于计算 rcPictureClip.Left 和 .Top 的 >> 移位位数 
	GetWindowRect(m_hWnd, &rc);			// rect.right - rect.left 为控件 width	
										// rect.bottom - rect.top 为控件 height


	// 根据要显示不同的状态，计算掩码 => uMaskClpLeft,uMaskClpTop
	switch(iState)
	{
	case 0:		// 常规
		uMaskClpLeft = 0xf000;	uMaskClpTop = 0xf0000000;
		uMoveLeft = 12;			uMoveTop = 28;
		break;
	case 1:		// 高亮
		uMaskClpLeft = 0x0f00;	uMaskClpTop = 0x0f000000;
		uMoveLeft = 8;			uMoveTop = 24;
		break;
	case 2:		// 按下
		uMaskClpLeft = 0x00f0;	uMaskClpTop = 0x00f00000;
		uMoveLeft = 4;			uMoveTop = 20;
		break;
	case 3:		// 灰色
		uMaskClpLeft = 0x000f;	uMaskClpTop = 0x000f0000;
		uMoveLeft = 0;			uMoveTop = 16;
		break;
	default:
		return;
	}
	
	// 设置表示图片按钮状态的属性
	m_pPro->rcPictureClip.left = (((UINT)m_pPro->ePicBtn & uMaskClpLeft)>>uMoveLeft) * (rc.right - rc.left); 
	m_pPro->rcPictureClip.top = (((UINT)m_pPro->ePicBtn & uMaskClpTop)>>uMoveTop) * (rc.bottom - rc.top);
	m_pPro->rcPictureClip.right = -1;
	m_pPro->rcPictureClip.bottom = -1;
	
	m_pPro->iPicBtnCurrDisp = iState;	// 记录当前已经显示为 这种状态


	// 将在 WM_PAINT 中绘制位图将图片按钮的状态表现出来（并支持 Stretch）
	InvalidateRect(m_hWnd, NULL, TRUE);	
}


int CBControl::PicButtonState()
{
	m_pPro = PropMem(true);	// 获得属性空间的地址
	if (!m_pPro) return -1; else return m_pPro->iPicBtnCurrDisp;
}




// =================== Edit、Rich Edit 类控件的功能 ===================


int CBControl::SelStart(int *pLineIdx/*=NULL*/)
{
	if ( IsClassName(TEXT("Edit")) )
	{
		if (pLineIdx)  // 传回行号
			(*pLineIdx) = SendMessage(m_hWnd, EM_LINEFROMCHAR, -1, 0);

		int iStart=-1, iEnd=-1;
		SendMessage(m_hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
		if (iStart<0) return -1; else return iStart;
	}
	else if ( IsClassName( TEXT("ComboBox") ) )
	{
		int iStart=-1, iEnd=-1;
		SendMessage(m_hWnd, CB_GETEDITSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
		if (iStart<0) return -1; else return iStart;
		// pLineIdx 对 ComboBox 无效
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// 返回 anchor item index
		// A multiple selection spans all items from the anchor item to the caret item.
		return SendMessage(m_hWnd, LB_GETANCHORINDEX, 0, 0) + 1; // +1 转换为编号从1开始
	}
	else if (_tcsstr(ClassName(), TEXT("RICHEDIT")))	// 以 Rich Edit 控件的方式操作
	{
		CHARRANGE cr;
		SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		if (pLineIdx)  // 传回行号
			(*pLineIdx) = SendMessage(m_hWnd, EM_EXLINEFROMCHAR, 0, cr.cpMin);
		return cr.cpMin;
	}
	else
		return -1;	// 不支持该控件
}

int CBControl::SelLength()
{
	if ( IsClassName(TEXT("Edit")) )
	{
		int iStart=-1, iEnd=-1;
		SendMessage(m_hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
		if (iStart<0 || iEnd<0) return -1; else return iEnd-iStart;	
	}
	else if ( IsClassName( TEXT("ComboBox") ) )
	{
		int iStart=-1, iEnd=-1;
		SendMessage(m_hWnd, CB_GETEDITSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
		if (iStart<0 || iEnd<0) return -1; else return iEnd-iStart;	
		// pLineIdx 对 ComboBox 无效
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// A multiple selection spans all items from the anchor item to the caret item.
		int idxAnchor = SendMessage(m_hWnd, LB_GETANCHORINDEX, 0, 0) + 1; // +1 转换为编号从1开始
		int idxCaret = SendMessage(m_hWnd, LB_GETCARETINDEX, 0, 0) + 1; // +1 转换为编号从1开始
		return idxCaret - idxAnchor + 1;
	}
	else if (_tcsstr(ClassName(), TEXT("RICHEDIT")))	// 以 Rich Edit 控件的方式操作
	{
		CHARRANGE cr;
		SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		return cr.cpMax - cr.cpMin;
	}
	else
		return -1;	// 不支持该控件
}


void CBControl::SelSet( int selStart/*=0*/, int selLength/*=-1*/ )
{
	if (IsClassName(TEXT("Edit")))
	{
		SendMessage(m_hWnd, EM_SETSEL, (WPARAM)selStart, (LPARAM)(selStart+selLength));
	}
	else if ( IsClassName( TEXT("ComboBox") ) )
	{
		SendMessage(m_hWnd, CB_SETEDITSEL, 0, MAKELPARAM(selStart, selStart+selLength));
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// List Box 必须具有多选风格，本操作才能成功
		if (selStart==0 && selLength==-1) 
		{ selStart=1; selLength=ListCount();} // 全选

		if (selStart<1) selStart=1;

		if (selLength>0)	// wParam 小，lParam 大，选中项目
			SendMessage(m_hWnd, LB_SELITEMRANGEEX, WPARAM(selStart-1),LPARAM(selStart-1+selLength-1));
		else				// lParam 小，wParam 大，取消选中项目
			SendMessage(m_hWnd, LB_SELITEMRANGEEX, WPARAM(selStart-1-selLength-1),LPARAM(selStart-1));
	}
	else if (_tcsstr(ClassName(), TEXT("RICHEDIT")))	// 以 Rich Edit 控件的方式操作
	{
		CHARRANGE cr;
		cr.cpMin = selStart; cr.cpMax = selStart+selLength;
		SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
	}
}


void CBControl::SelTextSet( LPCTSTR stringRepl, bool bCanUndo /*=true*/ )
{
	SendMessage(m_hWnd,  EM_REPLACESEL, (WPARAM)bCanUndo, (LPARAM)stringRepl );
}

void CBControl::SelTextSet( tstring stringRepl, bool bCanUndo/*=true*/ )
{
	SelTextSet(stringRepl.c_str(), bCanUndo);
}


int CBControl::FindText( LPTSTR stringFind, int rgStart/*=0*/, int rgLength/*=-1*/, bool bCaseSensitive/*=false*/, bool bMatchWholeWord/*=false*/, bool bAutoSelFound/*=true*/ )
{
	if (_tcsstr(ClassName(), TEXT("RICHEDIT"))==0) return -1;	// 不支持控件
	
	WPARAM wpFlag = FR_DOWN;
	FINDTEXTEX ft;
	int ret;
	if (bCaseSensitive) wpFlag |= FR_MATCHCASE;
	if (bMatchWholeWord) wpFlag |= FR_WHOLEWORD;

	ft.chrg.cpMin = rgStart;
	ft.chrg.cpMax = rgStart+rgLength;
	if (rgStart>0 && rgLength<0) ft.chrg.cpMax=0x7fffffff;  // 从 rgStart 开始到全文本末尾
	
	// 设置 stringFind，在其后要加个 \0
	TCHAR * szFind = NULL;
	int iFindLength = _tcslen(stringFind);
	szFind = new TCHAR [iFindLength+2];
	_tcscpy(szFind, stringFind);
	*(szFind+iFindLength)=TEXT('\0');
	ft.lpstrText = szFind; 

	// 查找
	ret = (int)SendMessage(m_hWnd, EM_FINDTEXTEX, wpFlag, (LPARAM)&ft );
	
	// 选中找到的文本
	if (bAutoSelFound)
		SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);

	// 返回值
	return ret;
}


// 设置 Edit 控件的图片效果外观
// 需要一个 图片框控件（Static控件）作为其边框并消除 Edit 控件本身的边框
// 必须在对话框设计时的属性设置中将 Edit 控件的“Border”属性去掉
// idPicBorder 为图片框控件（Static控件）的资源ID
// idBMPBorder 为图片框控件（Static控件）中的背景图片资源ID
// x、y、width、height 为包含图片框控件的总体位置、大小
// xOffset 为 Edit 控件相对图片框控件的水平偏移位置
//   Edit 控件的宽度将被设置为 width - 2*xOffset
//   Edit 控件的高度将被设置为适合控件所用字体（可由 szFontName、
//     fFontSize 设置，默认为“宋体”、10.5磅）的一行文字高度
// 如 bOnlyMoveSize=true 只摆放控件位置，并不再重复设置效果
//   此时忽略 idBMPBorder、fFontSize、x、y、width、height
void CBControl::EditSetApperance( unsigned short idPicBorder, unsigned short idBMPBorder, int x, int y, int width, int height, LPTSTR szFontName/*=NULL*/, float fFontSize/*=10.5*/, int xOffset/*=4*/, bool bOnlyMoveSize/*=false*/ )
{
	CBControl ctPicBorder;
	if (! szFontName) szFontName=TEXT("宋体");  // 默认为“宋体”

	if (!bOnlyMoveSize)
	{
		ctPicBorder.SetResID(idPicBorder);
		ctPicBorder.StretchSet(true);
		ctPicBorder.FontNameSet(szFontName);	// 将 picture 控件的字体也
		ctPicBorder.FontSizeSet(fFontSize);		//   设置为与 Edit 控件相同
		ctPicBorder.PictureSet(idBMPBorder);
		ctPicBorder.MousePointerSet(IDC_IBeam);	// 将 picture 控件鼠标也设为 I 型
	}
	ctPicBorder.Move(x, y, width, height);

	if (!bOnlyMoveSize)
	{
		FontNameSet(szFontName);	// 设置字体
		FontSizeSet(fFontSize);		// 设置字号
 		ZOrder(0);
	}
	Move(x+xOffset, ctPicBorder.Top() + (ctPicBorder.Height()-TextHeight())/2, 
		width-2*xOffset, TextHeight());
}


// =================== 按钮类控件的功能 ===================
// 返回和设置单选或复选按钮是否被勾选了：0=未选；1=已选；2=三态
// 此也可用于 Static 控件制作的“图片按钮”
unsigned int CBControl::ValueChecked()
{
	m_pPro = PropMem(false);
	if (m_pPro)
	{
		if (m_pPro->ePicBtn)
		{
			// 返回“图片按钮”的按下 或 抬起 状态
			return m_pPro->iPicBtnCheckedState;
		}
	}

	// 返回单选或复选按钮的状态
	return IsDlgButtonChecked(GetParent(m_hWnd), m_ResCtrlID);
}

// 设置单选或复选按钮的勾选状态：0=未选；1=已选；2=三态
// 要设置为“2=三态”，复选框必须具有 tri-STATE 风格
// 此也可用于 Static 控件制作的“图片按钮”
void CBControl::ValueCheckedSet( UINT uCheck )
{
	m_pPro = PropMem(false);
	if (m_pPro)
	{
		if (m_pPro->ePicBtn)
		{
			// 设置“图片按钮”的状态
			m_pPro->iPicBtnCheckedState = uCheck;
			if (uCheck) PicButtonStateSet(2,0); else PicButtonStateSet(0,0);
			InvalidateRect(m_hWnd, NULL, TRUE);
			return;
		}
	}
	
	// 设置单选或复选按钮的状态
	CheckDlgButton(GetParent(m_hWnd), m_ResCtrlID, uCheck);
}


// =================== 组合框列表框类控件的功能 ===================

// 添加一个条目，返回添加的条目编号（从1开始），出错返回 0
// index=0 时，在末尾添加；否则在指定的 index 位置添加，后者不能自动排序
int CBControl::AddItem( LPTSTR szText, int index /*= -1*/, int iImage /*= 0*/, int iTagLong /*= 0*/, int iIndent /*= 0x80000000*/ ) const
{
	// ************************
	// If you create the combo box with an owner-drawn style but without the CBS_HASSTRINGS style, 
	// the value of the lpsz parameter is stored as item data rather than the string it would otherwise point to. 
	
	if ( IsClassName( TEXT("ComboBox") ) )
	{
		// ComboBox
		if (index<=0)
			return SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM)szText)+1;
		else
			return SendMessage(m_hWnd, CB_INSERTSTRING, index-1, (LPARAM)szText)+1;
	}
	else if ( IsClassName( TEXT("ListBox") ) ) 
	{
		// ListBox
		if (index<=0)
			return SendMessage(m_hWnd, LB_ADDSTRING, 0, (LPARAM)szText)+1;
		else
			return SendMessage(m_hWnd, LB_INSERTSTRING, index-1, (LPARAM)szText)+1;
	}
	else if (IsClassName( TEXT("SysListView32") ))
	{
		LVITEM itm;
		itm.mask = LVIF_TEXT | LVIF_PARAM;
		if (iImage != -1) itm.mask |= LVIF_IMAGE;
		if (iIndent != 0x80000000) itm.mask |= LVIF_INDENT;
		if (index <= 0) itm.iItem = 0x7FFFFFFF; else itm.iItem = index - 1;  // 转换为 index 从0开始编号
		itm.iSubItem = 0;  // LVM_INSERTITEM 时，the iSubItem member of the LVITEM structure must be zero; zero if this structure refers to an item rather than a subitem
		itm.iImage = iImage - 1;
		itm.lParam = iTagLong;
		itm.iIndent = iIndent;
		itm.pszText = (LPTSTR)szText;
		return SendMessage(m_hWnd, LVM_INSERTITEM, 0, (LPARAM)(&itm)) + 1;    // 返回值转换为从1开始编号
	}

	// 该控件不能用 AddItem
	return -3;
}

int CBControl::AddItem( tstring stringText, int index /*= -1*/, int iImage /*= 0*/, int iTagLong /*= 0*/, int iIndent /*= 0x80000000*/ ) const
{
	//LPTSTR szText = (LPTSTR)stringText.c_str();
	return AddItem((LPTSTR)stringText.c_str(), index, iImage, iTagLong, iIndent);
}


// 删除一个条目，编号从1开始；返回删除后的列表共有条目数，出错返回0
int CBControl::RemoveItem( int index ) const
{
	if ( IsClassName( TEXT("ComboBox") ) )   
	{
		// ComboBox
		return SendMessage(m_hWnd, CB_DELETESTRING, index-1, 0);
	}
	else if ( IsClassName( TEXT("ListBox") ) )    
	{
		// ListBox
		return SendMessage(m_hWnd, LB_DELETESTRING, index-1, 0);
	}
	else if (IsClassName( TEXT("SysListView32") ))
	{
		SendMessage(m_hWnd, LVM_DELETEITEM, index-1, 0);   // 转换为 index 从0开始编号
	}
	
	// 该控件不能用 RemoveItem
	return -3;
}


// 返回共有条目数，出错返回<0的值
int CBControl::ListCount() const
{
	if ( IsClassName( TEXT("ComboBox") ) )
	{
		// ComboBox
		return SendMessage(m_hWnd, CB_GETCOUNT, 0, 0);
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		return SendMessage(m_hWnd, LB_GETCOUNT, 0, 0);
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		return SendMessage(m_hWnd, LVM_GETITEMCOUNT, 0, 0);
	}
	
	// 该控件不能用 ListCount
	return -3;
}


// 返回当前选择的条目编号（编号从1开始），出错返回0
// 若对多选列表框使用，则返回的是具有焦点的项目编号
//   所有项目都没被选择时，返回1（表示第1个项目有焦点）
int CBControl::ListIndex() const
{

	if ( IsClassName( TEXT("ComboBox") ) )
	{
		// ComboBox
		return SendMessage(m_hWnd, CB_GETCURSEL, 0, 0)+1;	// +1 表示转换为编号从1开始
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		return SendMessage(m_hWnd, LB_GETCURSEL, 0, 0)+1;	// +1 表示转换为编号从1开始
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		return ListViewNextItemIdx(0, false, true);
	}
	// 该控件不能用 ListIndex
	return -3;
}


// 选择一个条目，条目编号从1开始
void CBControl::ListIndexSet( int index ) const
{
	if ( IsClassName( TEXT("ComboBox") ) ) 
	{
		// ComboBox
		SendMessage(m_hWnd, CB_SETCURSEL, index-1, 0);
	}
	else if ( IsClassName( TEXT("ListBox") ) )  
	{
		// ListBox
		SendMessage(m_hWnd, LB_SETCURSEL, index-1, 0);
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		ItemSelectedSet(index, true);
//     Me.SelMarkIndex = vNewValue  // 设置 selection mark 为要选项
	}
	
	// 该控件不能用 ListIndexSet
}


// 获得一个条目的内容，index、indexSub 都从 1 开始
// indexSub 仅用于 ListView 控件
LPTSTR CBControl::ItemText( int index/*=-1*/, int indexSub/*=0*/ ) const
{
	// index<0 时获得 ListIndex() 的文本
	if (index<0) index=ListIndex();

	LRESULT ret;
	if ( IsClassName( TEXT("ComboBox") ) )    
	{
		// ********************************************
		// If you create the combo box with an owner-drawn style but without 
		// the CBS_HASSTRINGS style, the buffer pointed to by the lpszBuffer parameter of
		// the message receives the 32-bit value associated with the item (the item data). 

		// ComboBox
		long length = SendMessage(m_hWnd, CB_GETLBTEXTLEN, index-1, 0);
		if (length == CB_ERR) { mEmptyStr[0]=0; return mEmptyStr; }  // 返回空串

		TCHAR * listText = new TCHAR [length + 1];
		HM.AddPtr(listText);
		HM.ZeroMem(listText, sizeof(TCHAR)*(length + 1));
		ret = SendMessage(m_hWnd, CB_GETLBTEXT, index-1, (LPARAM)listText);
		if (ret == CB_ERR)
			{ mEmptyStr[0]=0; return mEmptyStr; }  // 返回空串
		else
			return listText;
	}
	else if  ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		long length = SendMessage(m_hWnd, LB_GETTEXTLEN, index-1, 0);
		if (length == LB_ERR) { mEmptyStr[0]=0; return mEmptyStr; }  // 返回空串

		TCHAR * listText = new TCHAR [length + 1];
		HM.AddPtr(listText);
		HM.ZeroMem(listText, sizeof(TCHAR)*(length + 1));
		ret = SendMessage(m_hWnd, LB_GETTEXT, index-1, (LPARAM)listText);
		if (ret == LB_ERR)
			{ mEmptyStr[0]=0; return mEmptyStr; }  // 返回空串
		else
			return listText;
	}
	else if  ( IsClassName( TEXT("SysListView32") ) )
	{
		// List View
		LVITEM itm;
		TCHAR buff[8192]={0};
		itm.mask = LVIF_TEXT;
		itm.cchTextMax = 8192;
		itm.pszText = buff;
		itm.iItem = index - 1;    // 转换为从 0 开始
		itm.iSubItem = indexSub - 1;
		SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
		
		int len = _tcslen(buff);
		if (len==0)
		{
			// 返回 ""
			mEmptyStr[0] = 0;
			return mEmptyStr;
		}
		else
		{
			TCHAR * p = new TCHAR[_tcslen(buff)+1];
			HM.AddPtr(p);
			_tcscpy(p, buff);
			return p;
		}
	}
	
	// 该控件不能用 List
	return 0;
}


void CBControl::ItemTextSet( int index, LPTSTR szNewText/*=NULL*/, int indexSub/*=0*/ ) const
{
	if ( IsClassName( TEXT("ComboBox") ) || IsClassName( TEXT("ListBox") ))    
	{
		bool blSel = ItemSelected(index) ;
		int idxSel = ListIndex();
		RemoveItem(index);
		AddItem(szNewText, index);
		ItemSelectedSet(index, blSel);
		if (idxSel==index) ListIndexSet(index);
	}
	else if  ( IsClassName( TEXT("SysListView32") ) )
	{
		// 先 获得文本：若内容相同就不更新，避免闪烁
		LVITEM itm;
		TCHAR buff[8192]={0};
		itm.mask = LVIF_TEXT;
		itm.iItem = index - 1;    // 转换为从 0 开始
		itm.iSubItem = indexSub - 1;
		itm.cchTextMax = 8192;
		itm.pszText = buff;
		SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
		if (_tcscmp(buff, szNewText)==0) return;	// 若内容相同就不更新，避免闪烁
		
		itm.mask = LVIF_TEXT;
		itm.iItem = index - 1;    // 转换为从 0 开始
		itm.iSubItem = indexSub - 1;
		itm.pszText = szNewText;
		// .cchTextMax is ignored if the structure specifies item attributes.
		
		SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
	}
}

void CBControl::ItemTextSet( int index, tstring stringNewText/*=NULL*/, int indexSub/*=0*/ ) const
{
	ItemTextSet(index, (LPTSTR)stringNewText.c_str(), indexSub);
}

LPTSTR CBControl::List( int index, int indexSub/*=0*/ ) const
{
	return ItemText(index, indexSub);
}


int CBControl::ItemData( int index ) const
{
	int iRet=0;
	if ( IsClassName( TEXT("ComboBox") ) )    
	{
		iRet = (int)SendMessage(m_hWnd, CB_GETITEMDATA, index-1, 0);
		if (iRet==CB_ERR) return 0; else return iRet;
	}
	else if  ( IsClassName( TEXT("ListBox") ) )
	{
		iRet = (int)SendMessage(m_hWnd, LB_GETITEMDATA, index-1, 0);
		if (iRet==CB_ERR) return 0; else return iRet;
	}
	else if  ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		itm.mask = LVIF_PARAM;
		itm.iItem = index-1;  // 转换为从 0 开始
		itm.iSubItem = -1;
		SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
		return itm.lParam;
	}

	// 该控件不能使用 ItemData
	return 0;	
}

void CBControl::ItemDataSet( int index, int itemData )
{
	if ( IsClassName( TEXT("ComboBox") ) )    
	{
		SendMessage(m_hWnd, CB_SETITEMDATA, index-1, (LPARAM)itemData);
	}
	else if  ( IsClassName( TEXT("ListBox") ) )
	{
		SendMessage(m_hWnd, LB_SETITEMDATA, index-1, (LPARAM)itemData);
	}
	else if  ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		itm.mask = LVIF_PARAM;
		itm.iItem = index-1;  // 转换为从 0 开始
		itm.iSubItem = -1;
		itm.lParam = itemData;
		SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
	}

	// 该控件不能使用 ItemDataSet
}


// 返回多选列表中已选条目数
int CBControl::ListSelCount() const
{
	if  ( IsClassName( TEXT("ListBox") ) )
	{
		// 单选列表框将返回 0
		return SendMessage(m_hWnd, LB_GETSELCOUNT, 0, 0);
	}
	else if  ( IsClassName( TEXT("SysListView32") ) )
	{
		return SendMessage(m_hWnd, LVM_GETSELECTEDCOUNT, 0, 0);
	}

	return 0;
	// 该控件不能用 ListSelCount
}


void CBControl::ItemsCopyToClipboard( bool bOnlySel/*=true*/, LPCTSTR szSpliter/*=TEXT('\t')*/ )
{
	const int c_iStrExpPer = 16383;

	TCHAR * buffResult = NULL;
	int iBuffResultUbound = 0, buffUsedLen = 0;
	int i, j;

	if  ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		TCHAR buff[8192]={0};
		int iLen = 0, iLenSep = _tcslen(szSpliter);
		int iColCount = ListViewColumnsCount();
		if (iColCount<=0) iColCount=1;	// 至少应有一列；iColCount<=0可能是由于非详细资料视图造成的
		
		iBuffResultUbound = c_iStrExpPer;
		Redim(buffResult, iBuffResultUbound, -1, false);
		if (buffResult==NULL) return;
		buffUsedLen = 0;

		i = ListViewNextItemIdx(0, ! bOnlySel, bOnlySel );	// 若没有下一个被选中的项目，返回 0
		while (i)
		{
			// 处理第 i 行的项目
			itm.mask = LVIF_TEXT;
			itm.iItem = i - 1;    // 转换为从 0 开始
			itm.cchTextMax = 8192;
			itm.pszText = buff;

			for (j=1; j<=iColCount; j++)
			{
				// 获取第 j 列的文本
				itm.iSubItem = j - 1;
				SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
				iLen = _tcslen(itm.pszText);

				// 若空间不够，扩大空间
				if (buffUsedLen+iLen+iLenSep+2 > iBuffResultUbound)	// +2 将本行结束的 \r\n 也算在内
				{
					Redim(buffResult, iBuffResultUbound+c_iStrExpPer, iBuffResultUbound, true);
					if (buffResult==NULL) return;
					iBuffResultUbound += c_iStrExpPer;
				}

				// 连接第 j 列的文本
				int aaa=lstrlen(buffResult);
				_tcscat(buffResult, itm.pszText);
				buffUsedLen += iLen;
				
				// 非最后一列时，连接列分隔符 szSpliter
				if (j<iColCount)	
				{
					_tcscat(buffResult, szSpliter);
					buffUsedLen += iLenSep;
				}
			}
			// 连接换行符
			_tcscat(buffResult, TEXT("\r\n"));
			buffUsedLen += 2;

				
			// 获得下一个被选项目的索引号 => i
			i = ListViewNextItemIdx(i, !bOnlySel, bOnlySel);  // 若没有下一个被选中的项目，返回 0
		}	// end while (i)

		ClipboardSetText(buffResult);
		Erase(buffResult);
	}	// end else if  ( IsClassName( TEXT("SysListView32") ) )
}


bool CBControl::ItemSelected( int index ) const
{
	if ( IsClassName( TEXT("ListBox") ) ) 
	{
		if ( SendMessage(m_hWnd, LB_GETSEL, index-1, 0) >0 )	// <0 表示出错(-1)，归于 return false
			return true;
		else
			return false;
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		itm.mask = LVIF_STATE;
		itm.stateMask = LVIS_SELECTED;
		itm.iItem = index - 1;			// 转换为从 0 开始
		itm.iSubItem = - 1;
		SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
		if (itm.state & LVIS_SELECTED) return true; else return false;
	}

	return false;

	// 该控件不能用 ItemSelected（Combo 控件不能用 ItemSelected）
}

void CBControl::ItemSelectedSet( int index, bool bSel ) const
{
	if ( IsClassName( TEXT("ListBox") ) )
	{
		if (index<1) return;	// 不能SendMessage: -1 到控件，否则将全部选中（取消选中）
		SendMessage(m_hWnd, LB_SETSEL, (BOOL)bSel, index-1);
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		itm.mask = LVIF_STATE;
		itm.stateMask = LVIS_SELECTED;
		itm.iItem = index - 1;			// 转换为从 0 开始
		itm.iSubItem = - 1;
		if (bSel) itm.state = LVIS_SELECTED; else itm.state = 0;
		SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
	}

	// 该控件不能用 ItemSelectedSet（Combo 控件不能用 ItemSelectedSet）
}


void CBControl::ItemsSelDo( int iSelStyle/*=1*/ )
{
	int i;
	if  ( IsClassName( TEXT("ListBox") ) )
	{
		switch(iSelStyle)
		{
		case 1:  // 全选
			SendMessage(m_hWnd, LB_SETSEL, 1, -1);
			break;
		case 0:  // 取消选择
			SendMessage(m_hWnd, LB_SETSEL, 0, -1);
			break;
		case -1:  // 反选
			for(i=0; i<ListCount(); i++)
			{
				if (SendMessage(m_hWnd, LB_GETSEL, i, 0) >0)
					SendMessage(m_hWnd, LB_SETSEL, 0, i);
				else
					SendMessage(m_hWnd, LB_SETSEL, 1, i);
			}
			break;
		}
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		LVITEM itm;
		int total = SendMessage(m_hWnd, LVM_GETITEMCOUNT, 0, 0);

		itm.mask = LVIF_STATE;
		itm.stateMask = LVIS_SELECTED;
		itm.iSubItem = -1;
		switch(iSelStyle)
		{
		case 1:   // 全选
			if ( (Style() & LVS_SINGLESEL) == 0) return;	// 非多选风格，退出
			itm.state = LVIS_SELECTED;
			for (i=0; i<total; i++)
			{
				itm.iItem = i;
				SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
			}
			break;

		case 0:  // 取消选择
			itm.state = 0;
			// 查找下一项已选项，i从0开始，若没有下一个被选中的项目，返回 i将=-1
			i = SendMessage(m_hWnd, LVM_GETNEXTITEM, -1, LVNI_SELECTED); 
			while (i>=0)
			{
				// 取消选择第 i 个项目
				itm.iItem = i;
				SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
				// 获得下一个被选项目的索引号 => i
				i = SendMessage(m_hWnd, LVM_GETNEXTITEM, i, LVNI_SELECTED);  // 若没有下一个被选中的项目，返回  -1
			}
			break;

		case -1:  // 反选
			if ( (Style() & LVS_SINGLESEL) == 0) return;	// 非多选风格，退出
			for (i=0; i<total; i++)
			{
				// 获得现在该项目是被选还是未选
				itm.iItem = i;
				SendMessage(m_hWnd, LVM_GETITEM, 0, (LPARAM)(&itm));
				if (itm.state & LVIS_SELECTED) itm.state = 0; else itm.state = LVIS_SELECTED;
				// 设置为反选状态 
				itm.iItem = i;
				SendMessage(m_hWnd, LVM_SETITEM, 0, (LPARAM)(&itm));
			}
			break;
		}
	}

	// 该控件不能用 ListSelItemsDo（Combo 控件不能用 ItemSelectedSet）
}





int * CBControl::ListSelItems( int *pSelCount/*=0*/ ) const
{
	if  ( ! IsClassName( TEXT("ListBox") ) ) 
	{
		if (pSelCount) *pSelCount = 0;
		return 0;	// 该函数只能用于 ListBox 控件
	}

	int count = ListSelCount();
	if (pSelCount) *pSelCount = count;
	if (count == 0) return 0;

	int *pSels = new int [count+1];		// 结果空间（不使用下标为0的元素）
	HM.AddPtr(pSels);
	HM.ZeroMem(pSels, sizeof(int)*(count+1) );

	int *pSelsTemp = new int [count];	// 临时空间
	SendMessage(m_hWnd, LB_GETSELITEMS, (WPARAM)count, (LPARAM)pSelsTemp);

	// 将临时空间的值拷贝到结果空间（不使用结果空间下标为0的元素）
	HM.CopyMem(pSels+1, pSelsTemp, sizeof(int)*count);

	// 将结果索引号各自 +1，使索引号都从 1 开始
	int i;
	for (i=1; i<=count; i++) pSels[i]+=1;

	delete []pSelsTemp;	// 不删除 pSels，它由 HM 管理

	return pSels;
}


int CBControl::ListTopIndex()
{
	if ( IsClassName( TEXT("ComboBox") ) )
	{
		return (int)SendMessage(m_hWnd, CB_GETTOPINDEX, 0, 0) + 1;
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		return (int)SendMessage(m_hWnd, LB_SETTOPINDEX, 0, 0) + 1;
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		return SendMessage(m_hWnd, LVM_GETTOPINDEX, 0, 0) + 1;
	}
	else
		return 0;
}

void CBControl::ListTopIndexSet( int idxTop )
{
	if ( IsClassName( TEXT("ComboBox") ) )
	{
		SendMessage(m_hWnd, CB_SETTOPINDEX, idxTop-1, 0);
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		SendMessage(m_hWnd, LB_SETTOPINDEX, idxTop-1, 0);
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		SendMessage(m_hWnd, LVM_ENSUREVISIBLE, idxTop-1, 0);
	}
}



// 清除列表的所有内容
void CBControl::ListClear() const
{
	if ( IsClassName( TEXT("ComboBox") ) ) 
	{
		// ComboBox
		SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0);
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		SendMessage(m_hWnd, LB_RESETCONTENT, 0, 0);
	}
	else if ( IsClassName( TEXT("SysListView32") ) )
	{
		SendMessage(m_hWnd, LVM_DELETEALLITEMS, 0, 0);
	}

	// 该控件不能用 ListClear
}


int CBControl::ListItemsHeight() const
{
	if ( IsClassName( TEXT("ComboBox") ) ) 
	{
		// ComboBox
		return (int)SendMessage(m_hWnd, CB_GETITEMHEIGHT, 0, 0 );
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		return (int)SendMessage(m_hWnd, LB_GETITEMHEIGHT, 0, 0 );
	}

	// 该控件不能使用 ListItemsHeight
	return 0;
}

void CBControl::ListItemsHeightSet( int newHeight )
{
	if ( IsClassName( TEXT("ComboBox") ) ) 
	{
		// ComboBox
		SendMessage(m_hWnd, CB_SETITEMHEIGHT, 0, MAKELPARAM(newHeight, 0) );
	}
	else if ( IsClassName( TEXT("ListBox") ) )
	{
		// ListBox
		SendMessage(m_hWnd, LB_SETITEMHEIGHT, 0, MAKELPARAM(newHeight, 0) );
	}

	Refresh();
}


// =================== ListView 控件功能 ===================

int CBControl::ListViewAddColumn( LPTSTR szColText, int iWidth /*= 60*/, int iAlign /*= 0*/, int iSubItemIndex /*= 0*/, int iImageIdx /*= -1*/, int iIndex /*= 0x7FFFFFFF*/ )
{
	LVCOLUMN col;
	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
	if (iImageIdx != -1) col.mask |= LVCF_IMAGE;
	col.cx = iWidth;
	col.fmt = iAlign;
	col.iSubItem = iSubItemIndex;
    col.iImage = iImageIdx;
	col.pszText = szColText;

    return SendMessage(m_hWnd, LVM_INSERTCOLUMN, (WPARAM)iIndex, (LPARAM)(&col)) + 1; // 返回值转换为从1开始编号
}

int CBControl::ListViewAddColumn( tstring stringColText, int iWidth /*= 60*/, int iAlign /*= 0*/, int iSubItemIndex /*= 0*/, int iImageIdx /*= -1*/, int iIndex /*= 0x7FFFFFFF*/ )
{
	return ListViewAddColumn((LPTSTR)stringColText.c_str(), iWidth, iAlign, iSubItemIndex,iImageIdx, iIndex);
}

int CBControl::ListViewColumnsCount()
{
	return SendMessage((HWND)SendMessage(m_hWnd, LVM_GETHEADER, 0, 0), HDM_GETITEMCOUNT, 0, 0);
}

int CBControl::ListViewNextItemIdx( int idxStart/*=0*/, bool bFindAll/*=true*/, bool bFindSelected/*=true*/, bool bFindCut/*=true*/, bool bFindDropHilited/*=false*/, bool bFindFocused/*=false*/, bool bFindAbove/*=false*/, bool bFindBelow/*=false*/, bool bFindLeft/*=false*/, bool bFindRight/*=false*/ ) const
{
	int iFlag = 0;
	if (bFindAll) 
		iFlag = LVNI_ALL;
	else
	{
		iFlag = 0;
		if (bFindSelected) iFlag |= LVNI_SELECTED;
		if (bFindCut) iFlag |= LVNI_CUT;
		if (bFindDropHilited) iFlag |= LVNI_DROPHILITED;
		if (bFindFocused) iFlag |= LVNI_FOCUSED;
		if (bFindAbove) iFlag |= LVNI_ABOVE;
		if (bFindBelow) iFlag |= LVNI_BELOW;
		if (bFindLeft) iFlag |= LVNI_TOLEFT;
		if (bFindRight) iFlag |= LVNI_TORIGHT;
	}

	return SendMessage(m_hWnd, LVM_GETNEXTITEM, idxStart-1, iFlag) + 1 ;
}

bool CBControl::ListViewFullRowSelect()
{
	if (SendMessage(m_hWnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0) & LVS_EX_FULLROWSELECT)
		return true;
	else
		return false;
}

void CBControl::ListViewFullRowSelectSet( bool blNewValue )
{
	if (blNewValue)
		SendMessage (m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);    
	else
		SendMessage (m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, 0);
}

bool CBControl::ListViewGridLines()
{
	if (SendMessage(m_hWnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0) & LVS_EX_GRIDLINES)
		return true;
	else
		return false;
}

void CBControl::ListViewGridLinesSet( bool blNewValue )
{
	if (blNewValue)
		SendMessage (m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);  
	else
		SendMessage (m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES, 0);  
}


// ======== ProgressBar 类控件功能 ========

int CBControl::Value()
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		return SendMessage(m_hWnd, PBM_GETPOS, 0, 0 );
	}
	
	return 0;
}

void CBControl::ValueSet( int iNewValue )
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		SendMessage(m_hWnd, PBM_SETPOS, (WPARAM)iNewValue, 0);
	}
}

int CBControl::Max()
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		return SendMessage(m_hWnd, PBM_GETRANGE, 0, 0);
	}
	
	return 0;
}

void CBControl::MaxSet( int iNewValue )
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		SendMessage(m_hWnd, PBM_SETRANGE32, (WPARAM)Min(), (LPARAM)iNewValue );
	}
}

int CBControl::Min()
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		return SendMessage(m_hWnd, PBM_GETRANGE, 1, 0 );
	}

	return 0;
}

void CBControl::MinSet( int iNewValue )
{
	if ( IsClassName( TEXT("msctls_progress32") ) ) 
	{
		// ProgressBar
		SendMessage(m_hWnd, PBM_SETRANGE32, (WPARAM)iNewValue, (LPARAM)Max() );
	}
}






// =================== 高级通用功能 ===================


HWND CBControl::hWnd() const
{
	return m_hWnd;	
}

HWND CBControl::hWndParentForm() const
{
	return (HWND)(CBWndBase::ms_hashCtrls.ItemLong((long)m_hWnd, false));
}



HWND CBControl::Parent()
{
	return GetParent(m_hWnd);
}

HWND CBControl::ParentSet( HWND hWndParent )
{
	if (! hWndParent) return NULL;	// 不允许将新父窗口设置为 NULL

	// 成功返回之前父窗口的句柄，失败返回 NULL
	HWND hWndParentPrev = SetParent(m_hWnd, hWndParent);
	if (! hWndParentPrev) return NULL;

	// 调整 CBWndBase::ms_hashCtrlResIDs 哈希表中对本控件父窗口的记录
	//   如出错不报错
	CBWndBase::ms_hashCtrlResIDs.ItemLongSet( (long)m_hWnd, (long)hWndParent, false );

// 	// For compatibility reasons, SetParent does not modify the WS_CHILD or WS_POPUP 
// 	//   window styles of the window whose parent is being changed. 
// 	if (hWndParent)
// 	{
// 		// if hWndNewParent is not NULL and the window was previously a child 
// 		//   of the desktop, you should clear the WS_POPUP style and set 
// 		//   the WS_CHILD style before calling SetParent.
// 		StyleSet(WS_POPUP, -1);
// 		StyleSet(WS_CHILD, 1);.
// 	}
// 	else
// 	{
// 		// if hWndNewParent is NULL, you should also clear the WS_CHILD bit and set 
// 		//   the WS_POPUP style after calling SetParent.
// 		StyleSet(WS_CHILD, -1);
// 		StyleSet(WS_POPUP, 1);
// 	}


	
	return hWndParentPrev;
}


// 返回一个父窗体的 CBForm 对象的地址指向，用于通过控件访问父窗体
CBForm * CBControl::ParentFormPtr() const
{
	return (CBForm *)(CBForm::ms_hashWnd.Item((long)hWndParentForm(),false));
}

// 获得子窗口控件的默认窗口程序的地址（注实际此地址的函数可能不被使用，因已被子类化）
unsigned long CBControl::PtrWndProcDef() const
{
	return (unsigned long)(CBWndBase::ms_hashCtrls.Item((long)m_hWnd, false));
}












