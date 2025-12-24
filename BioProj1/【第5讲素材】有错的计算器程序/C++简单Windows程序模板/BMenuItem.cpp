//////////////////////////////////////////////////////////////////////
// BMenuItem.cpp: CBMenuItem 类的实现，用于操作一个菜单项（事件除外）
//
//////////////////////////////////////////////////////////////////////

#include "BMenuItem.h"

// 常量赋值
const int CBMenuItem::mc_MaxMenuTextLen = 1024;		// 菜单项最大文本长度


CBMenuItem::CBMenuItem( HMENU hMenuParent/*=NULL*/, 
					    UINT idResMenuItem/*=0*/, 
						HWND hWndParent/*=NULL*/, 
						CBHashLK *ptrHashMenuIDs/*=NULL*/ )
{
	m_hMenuParent = NULL;
	m_idMenuItemOrPos = 0;
	m_fByPosition = MF_BYCOMMAND;
	m_hWndParent = NULL;
	m_ptrHashMenuIDs = NULL;
	SetFromResID(hMenuParent, idResMenuItem, hWndParent, ptrHashMenuIDs);
}

CBMenuItem::~CBMenuItem()
{
	
}

void CBMenuItem::SetFromResID( HMENU hMenuParent, UINT idResMenuItem, HWND hWndParent, CBHashLK *ptrHashMenuIDs )
{
	m_hMenuParent = hMenuParent;
	m_idMenuItemOrPos = idResMenuItem;
	m_fByPosition = MF_BYCOMMAND;
	m_hWndParent = hWndParent;
	m_ptrHashMenuIDs = ptrHashMenuIDs; 
}

void CBMenuItem::SetFromPos( HMENU hMenuParent, UINT pos, HWND hWndParent, CBHashLK *ptrHashMenuIDs )
{
	m_hMenuParent = hMenuParent;
	m_idMenuItemOrPos = pos-1;		// 转换为位置从0开始保存
	m_fByPosition = MF_BYPOSITION;
	m_hWndParent = hWndParent;
	m_ptrHashMenuIDs = ptrHashMenuIDs; 
}

HMENU CBMenuItem::hMenuParent() const
{ 
	return m_hMenuParent;
}

UINT CBMenuItem::IDRes() const
{
	if (MF_BYPOSITION == m_fByPosition)
	{
		// 是用 菜单位置 方式设置的管理菜单项：用 API 函数获得资源 ID
		return GetMenuItemID(m_hMenuParent, m_idMenuItemOrPos);
	}
	else
		// 是用 菜单资源ID 方式设置的管理菜单项：直接返回资源 ID
		return m_idMenuItemOrPos;
}


UINT CBMenuItem::Position()
{
	if (MF_BYPOSITION == m_fByPosition)
	{
		// 是用 菜单位置 方式设置的管理菜单项：直接返回位置
		return m_idMenuItemOrPos+1;
	}
	else
	{
		// 是用 菜单资源ID 方式设置的管理菜单项：用 GetMenuPositionFromID 获得位置
		UINT r=GetMenuPositionFromID(m_hMenuParent, m_idMenuItemOrPos);
		if (r==0xFFFFFFFF) return 0; else return r+1;	// 转换为从1开始
	}
}


BOOL CBMenuItem::Remove()
{
	BOOL ret;
	ret = RemoveMenu(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition);
	if (ret)
		m_ptrHashMenuIDs->Remove(this->IDRes(), false);	// 维护哈希表
	return ret;
}


BOOL CBMenuItem::Append( LPCTSTR newMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	BOOL ret;
	// 在本菜单项的同一菜单的最后添加新项
	if (newMenuItemText)
	{
		if (bPopupSub)
		{
			// 添加子菜单
			ret = AppendMenu(m_hMenuParent, MF_POPUP, 
				(UINT)LoadMenu(pApp->hInstance, MAKEINTRESOURCE(uIDNewItem)), newMenuItemText); 
		}
		else
		{
			// 添加命令菜单项
			ret = AppendMenu(m_hMenuParent, MF_STRING, uIDNewItem, newMenuItemText);
			m_ptrHashMenuIDs->Add((int)m_hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);	// 维护哈希表
		}
	}
	else
	{
		// newMenuItemText == 0，添加一条分隔线
		ret = AppendMenu(m_hMenuParent, MF_SEPARATOR, uIDNewItem, NULL);
		m_ptrHashMenuIDs->Add((int)m_hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);
	}

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);

	return ret;
}

BOOL CBMenuItem::Append( tstring stringNewMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	return Append(stringNewMenuItemText.c_str(), uIDNewItem, bPopupSub);
}


BOOL CBMenuItem::AppendSubItem( LPCTSTR newMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	// 若本菜单项引出子菜单，在子菜单的最后添加新项
	BOOL ret;
	HMENU hMenuParent;
	hMenuParent = hSubMenu();		// 本菜单项所引子菜单的句柄
	if (hMenuParent == 0) return 0;	// 本菜单项不引出子菜单

	if (newMenuItemText)
	{
		if (bPopupSub)
		{
			// 添加子菜单
			ret = AppendMenu(hMenuParent, MF_POPUP, 
				(UINT)LoadMenu(pApp->hInstance, MAKEINTRESOURCE(uIDNewItem)), newMenuItemText); 
		}
		else
		{
			// 添加命令菜单项
			ret = AppendMenu(hMenuParent, MF_STRING, uIDNewItem, newMenuItemText);
			m_ptrHashMenuIDs->Add((int)hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);
		}
	}
	else
	{
		// newMenuItemText == 0，添加一条分隔线
		ret = AppendMenu(hMenuParent, MF_SEPARATOR, uIDNewItem, NULL);
		m_ptrHashMenuIDs->Add((int)hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);
	}

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == hMenuParent) DrawMenuBar(m_hWndParent);

	return ret;
}

BOOL CBMenuItem::AppendSubItem( tstring stringNewMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	return AppendSubItem(stringNewMenuItemText.c_str(),uIDNewItem,bPopupSub);
}


BOOL CBMenuItem::Insert( LPCTSTR newMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	BOOL ret;
	// 在本菜单项之前（同一菜单）插入新项
	if (newMenuItemText)
	{
		if (bPopupSub)
		{
			// 插入子菜单
			ret = InsertMenu(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition | MF_POPUP, 
				(UINT)LoadMenu(pApp->hInstance, MAKEINTRESOURCE(uIDNewItem)), newMenuItemText); 
		}
		else
		{
			// 插入命令菜单项
			ret = InsertMenu(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition | MF_STRING, uIDNewItem, newMenuItemText);
			m_ptrHashMenuIDs->Add((int)m_hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);
		}
	}
	else
	{
		// newMenuItemText == 0，插入一条分隔线
		ret = InsertMenu(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition | MF_SEPARATOR, uIDNewItem, NULL);
		m_ptrHashMenuIDs->Add((int)m_hMenuParent, uIDNewItem, 0, 0, 0, 0, 0, false);
	}
	
	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
		
	return ret;	
}

BOOL CBMenuItem::Insert( tstring stringNewMenuItemText, UINT uIDNewItem/*=0*/, bool bPopupSub/*=false*/ )
{
	return Insert(stringNewMenuItemText.c_str(), uIDNewItem, bPopupSub);
}



LPTSTR CBMenuItem::Text()
{
	MENUITEMINFO mi;
	TCHAR *buff = new TCHAR [mc_MaxMenuTextLen];	
	HM.AddPtr(buff);		// 使用 HM 管理动态分配的内存（最终回收）

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.fType = 0;
    mi.cch = mc_MaxMenuTextLen;
    mi.dwTypeData = buff;

	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);

	if ((mi.fType & MFT_SEPARATOR) || (mi.fType & MFT_BITMAP))
	{
		// 是分隔线，或位图形式的菜单项，返回 ""
		*buff = 0;
	}
	return buff;
}

void CBMenuItem::TextSet( LPCTSTR newText )
{
	MENUITEMINFO mi;

	// 先获得菜单项信息（因要保护原来的 mi.fType）
	TCHAR buff[mc_MaxMenuTextLen];
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.fType = 0;
	mi.cch = mc_MaxMenuTextLen;
    mi.dwTypeData = buff;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	
	// 根据是否要新设置为分隔线，做不同处理
	if (newText == 0)	// 设置为分割线
	{
		mi.fType = mi.fType | MFT_SEPARATOR;
		mi.dwTypeData=0;  mi.cch=0;			 // 将忽略 dwTypeData 和 cch 成员
	}
	else
	{
		mi.fType = mi.fType & ~MFT_SEPARATOR;
		mi.dwTypeData = (LPTSTR)newText;	
        mi.cch = 1024;							// 将忽略 cch 成员

	}

	// 设置菜单项
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	SetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}

void CBMenuItem::TextSet( tstring stringNewText )
{
	TextSet(stringNewText.c_str());
}


void CBMenuItem::BitmapSet( HBITMAP hBmp )
{
	MENUITEMINFO mi;
	TCHAR buff[mc_MaxMenuTextLen] = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = buff;
	mi.cch = mc_MaxMenuTextLen;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	
	if (hBmp==0)	// hBmp 为0时取消位图菜单（变回文字菜单）
	{
		mi.fType = mi.fType & ~MFT_BITMAP; 

		// 菜单无文字，需稍后主调程序再设置其文字
		buff[0] = TEXT('\0');	// 让文字缓冲区内容为 ""
		mi.dwTypeData = buff;	// 让 dwTypeData 成员值为文字缓冲区地址
	}
	else 
	{
		mi.fType = mi.fType | MFT_BITMAP;
		mi.dwTypeData = (LPTSTR)hBmp;	// 让 dwTypeData 成员值为位图句柄
	}
	
	SetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);	
	
	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}

void CBMenuItem::BitmapSet( UINT idResBmp )
{
	if (idResBmp==0) { BitmapSet((HBITMAP)0); return; }  // 参数为0取消菜单位图（通过调用 HBITMAP 参数的重载版实现）
	HBITMAP hBmp = LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(idResBmp));
	if (hBmp)
	{
		pApp->AddImageObjHandle((HANDLE)hBmp, eImgBitmap);	// 记录句柄以便程序退出前自动释放它
		BitmapSet(hBmp);
	}
}


BOOL CBMenuItem::SetCheckBitmaps( UINT idResBmpUnChecked, UINT idResBmpChecked )
{
	HBITMAP hBmpUnCheck, hBMPCheck; 
	BOOL ret;

	// 获得位图所需大小：LOWORD(lSize) 是所需位图宽度；HIWORD(lSize) 是所需位图高度
	LONG lSize = GetMenuCheckMarkDimensions();
	
	// 按所需大小加载位图
	hBmpUnCheck = (HBITMAP)LoadImage(pApp->hInstance, MAKEINTRESOURCE(idResBmpUnChecked),
		IMAGE_BITMAP, LOWORD(lSize), HIWORD(lSize), LR_SHARED ); 
	hBMPCheck = (HBITMAP)LoadImage(pApp->hInstance, MAKEINTRESOURCE(idResBmpChecked),
		IMAGE_BITMAP, LOWORD(lSize), HIWORD(lSize), LR_SHARED );

	// 设置位图
	ret=SetMenuItemBitmaps(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, hBmpUnCheck, hBMPCheck);
	
	// 位图句柄存入 pApp 所指对象中，以在程序退出前自动 DeleteObject
	pApp->AddImageObjHandle((HANDLE)hBmpUnCheck, eImgBitmap); 
	pApp->AddImageObjHandle((HANDLE)hBMPCheck, eImgBitmap);	 
	
	return ret;
}

BOOL CBMenuItem::SetCheckBitmaps( EOEMBmp bmpUnChecked, EOEMBmp bmpChecked )
{
	HBITMAP hBmpUnCheck, hBMPCheck; 
	BOOL ret;
	
	// 获得位图所需大小：LOWORD(lSize) 是所需位图宽度；HIWORD(lSize) 是所需位图高度
	LONG lSize = GetMenuCheckMarkDimensions();
	
	// 按所需大小加载位图
	hBmpUnCheck = (HBITMAP)LoadImage(NULL, MAKEINTRESOURCE(bmpUnChecked),
		IMAGE_BITMAP, LOWORD(lSize), HIWORD(lSize), 0 ); 
	hBMPCheck = (HBITMAP)LoadImage(NULL, MAKEINTRESOURCE(bmpChecked),
		IMAGE_BITMAP, LOWORD(lSize), HIWORD(lSize), 0 );
	
	// 设置位图
	ret=SetMenuItemBitmaps(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, hBmpUnCheck, hBMPCheck);
	
	// 位图句柄存入 pApp 所指对象中，以在程序退出前自动 DeleteObject
	pApp->AddImageObjHandle((HANDLE)hBmpUnCheck, eImgBitmap); 
	pApp->AddImageObjHandle((HANDLE)hBMPCheck, eImgBitmap);	 
	
	return ret;	
}

bool CBMenuItem::Enabled()
{
	UINT r;
	r = GetMenuState(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition);
	if (r & MFS_GRAYED || r & MFS_DISABLED)
		return false;
	else
		return true;
}

void CBMenuItem::EnabledSet( bool enabled/*=true*/ )
{
	if (enabled)
		EnableMenuItem(m_hMenuParent, m_idMenuItemOrPos, MF_ENABLED | m_fByPosition);
	else
		EnableMenuItem(m_hMenuParent, m_idMenuItemOrPos, MF_GRAYED  | m_fByPosition);
	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}

bool CBMenuItem::Checked()
{
	UINT r;
	r = GetMenuState(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition);
	return (r & MF_CHECKED) !=0;
}

void CBMenuItem::CheckedSet( bool checked/*=true*/,		// bRadio=true 时，将忽略此参数值
							 bool bRadio/*=false*/,		// bRadio=false 时为对勾， bRadio=true 时为圆点
							 UINT itemFrom/*=0*/,		// bRadio=true 时，还要设置菜单项组的范围：itemFrom、itemTo 
							 UINT itemTo/*=0 */,
							 bool fFromToByPosition/*=false*/	// 是否 用位置指定 itemFrom、itemTo，否则 itemFrom、itemTo 是菜单项的资源 ID
							)
{
	if (bRadio)
	{
		// 以圆点方式勾选，此时忽略参数 checked

		UINT idMenuItem=m_idMenuItemOrPos; 
		if (fFromToByPosition)
		{
			// ======== itemFrom、itemTo 使用位置 ========
			UINT posItem = m_idMenuItemOrPos;
			// 将目标菜单项转换为位置
			if (m_fByPosition==MF_BYCOMMAND)
				posItem = GetMenuPositionFromID(m_hMenuParent, m_idMenuItemOrPos);
			// 用位置设置
			CheckMenuRadioItem(m_hMenuParent, itemFrom, itemTo, posItem, MF_BYPOSITION);
		}
		else
		{
			// ======== itemFrom、itemTo 使用资源ID ========
			if (m_fByPosition==MF_BYPOSITION)
			{
				// 将 itemFrom、itemTo 转换为位置
				itemFrom = GetMenuPositionFromID(m_hMenuParent, itemFrom);
				itemTo = GetMenuPositionFromID(m_hMenuParent, itemTo);
				// 用位置设置
				CheckMenuRadioItem(m_hMenuParent, itemFrom, itemTo, m_idMenuItemOrPos, MF_BYPOSITION);
			}
			else
			{
				// 用资源ID设置
				CheckMenuRadioItem(m_hMenuParent, itemFrom, itemTo, m_idMenuItemOrPos,MF_BYCOMMAND);
			}
		}
	}
	else
	{
		// 以对勾方式勾选，此时忽略参数 itemFrom、itemTo
		if (checked)
			CheckMenuItem(m_hMenuParent, m_idMenuItemOrPos, MF_CHECKED | m_fByPosition);
		else
			CheckMenuItem(m_hMenuParent, m_idMenuItemOrPos, MF_UNCHECKED | m_fByPosition);
	}	

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}


bool CBMenuItem::RadioCheck()
{
	return GetfType(MFT_RADIOCHECK);
}

void CBMenuItem::RadioCheckSet( bool bValue )
{
	SetfType(MFT_RADIOCHECK, bValue);
}


bool CBMenuItem::Hilited()
{
	UINT r;
	r = GetMenuState(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition);
	return (r & MF_HILITE) != 0;
}

void CBMenuItem::HilitedSet()
{ 
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STATE;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	mi.fState = mi.fState | MFS_HILITE;
	SetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	// 不用 HiliteMenuItem，否则可能刷新有问题
	// 不能取消高亮

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}


bool CBMenuItem::RightJustify()
{
	return GetfType(MFT_RIGHTJUSTIFY);
}


void CBMenuItem::RightJustifySet( bool bValue )
{
	SetfType(MFT_RIGHTJUSTIFY, bValue);
}


bool CBMenuItem::OwnerDraw()
{
	return GetfType(MF_OWNERDRAW);
}

void CBMenuItem::OwnerDrawSet( bool bValue )
{
	SetfType(MF_OWNERDRAW, bValue);
}


unsigned long CBMenuItem::Tag()
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_DATA;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	return mi.dwItemData;
}

void CBMenuItem::TagSet(unsigned long ulNewValue)
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_DATA;
	mi.dwItemData = ulNewValue;
	SetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
}



bool CBMenuItem::IsSeparator()
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = 0;
	mi.cch = 0;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	if (mi.fType & MFT_SEPARATOR) return true; else return false;
}



HMENU CBMenuItem::hSubMenu()
{
	if (m_hMenuParent==NULL)
	{
		if (m_idMenuItemOrPos>0)
			// 返回子菜单句柄为：顶层菜单句柄
			return GetMenu(m_hWndParent);
		else
			// 返回子菜单句柄为：系统菜单句柄
			return GetSystemMenu(m_hWndParent, 0);
	}
	else
	{
		MENUITEMINFO mi;
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_SUBMENU;
		GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
		return mi.hSubMenu;
	}
}

long CBMenuItem::LeftRelative()
{
	RECT rRc; POINT pt;
	rRc.left=0; rRc.top=0;
	rRc.right=0; rRc.bottom=0;
	GetMenuItemRect(m_hWndParent,m_hMenuParent, this->Position()-1, &rRc);
	
	// 转换为窗体坐标
	pt.x = rRc.left; pt.y = rRc.top;
	ScreenToClient(m_hWndParent, &pt);
	return pt.x; 
}

long CBMenuItem::TopRelative()
{
	RECT rRc; POINT pt;
	rRc.left=0; rRc.top=0;
	rRc.right=0; rRc.bottom=0;
	GetMenuItemRect(m_hWndParent,m_hMenuParent, this->Position()-1, &rRc);
	
	// 转换为窗体坐标
	pt.x = rRc.left; pt.y = rRc.top;
	ScreenToClient(m_hWndParent, &pt);
	return pt.y; 	
}

long CBMenuItem::Width()
{
	RECT rRc; 
	rRc.left=0; rRc.top=0;
	rRc.right=0; rRc.bottom=0;
	GetMenuItemRect(m_hWndParent,m_hMenuParent, this->Position()-1, &rRc);
	return rRc.right-rRc.left; 	
}

long CBMenuItem::Height()
{
	RECT rRc; 
	rRc.left=0; rRc.top=0;
	rRc.right=0; rRc.bottom=0;
	GetMenuItemRect(m_hWndParent,m_hMenuParent, this->Position()-1, &rRc);
	return rRc.bottom-rRc.top; 		
}



bool CBMenuItem::BreakMenu()
{
	return GetfType(MFT_MENUBARBREAK) | GetfType(MFT_MENUBREAK);
}

void CBMenuItem::BreakMenuSet( bool bBreak, bool bWithLine/*=true*/ )
{
	if (bBreak)
		if (bWithLine)
			SetfType(MFT_MENUBARBREAK, true);
		else
			SetfType(MFT_MENUBREAK, true);
	else
		SetfType(MFT_MENUBARBREAK | MFT_MENUBREAK, false); 	
}


UINT CBMenuItem::DefaultItemPos( bool bGoIntoPopups /*= false*/, bool bUseDisabled /*= True*/ )
{
	HMENU hSub;
	hSub = hSubMenu();
	if (hSub==0) return 0;

	UINT flags = 0;
	if (bGoIntoPopups) flags = flags | GMDI_GOINTOPOPUPS;
	if (bUseDisabled) flags = flags | GMDI_USEDISABLED;

    return GetMenuDefaultItem(hSub, true, flags) + 1;
}

void CBMenuItem::DefaultItemPosSet( UINT pos )
{
	HMENU hSub;
	hSub = hSubMenu();
	if (hSub==0) return;
	
	SetMenuDefaultItem(hSub, pos - 1, true);
}

bool CBMenuItem::RightToLeft()
{
	return GetfType(MFT_RIGHTJUSTIFY);
}

void CBMenuItem::RightToLeftSet( bool bValue )
{
	SetfType(MFT_RIGHTJUSTIFY, bValue);
}



int CBMenuItem::CountSubItems()
{ 
	HMENU hSub;
	hSub = hSubMenu();
	if (hSub==0) return 0;
	
	return GetMenuItemCount(hSub);
}






















UINT CBMenuItem::GetMenuPositionFromID( HMENU hMenu, UINT idResMenuItem )
{
	int i, iCount;
	UINT idItem;

	iCount = GetMenuItemCount(hMenu); 
	if (iCount == -1) return 0xFFFFFFFF;		// GetMenuItemCount 调用失败，或无菜单项
	
	// 逐个扫描 hMenu 下的所有菜单项
	for(i=0; i<=iCount-1; i++)
	{
		idItem = GetMenuItemID(hMenu, i);
		if (idItem==idResMenuItem) return i;
		// hMenu 下的第 i 项有级联菜单时，idItem == 0xFFFFFFFF
	}

	return 0xFFFFFFFF;	// 未找到资源 ID 为 idResMenuItem 的菜单项
}

bool CBMenuItem::GetfType( UINT fTypeValue )
{
	MENUITEMINFO mi;
	TCHAR buff[mc_MaxMenuTextLen];
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = buff;
	mi.cch = mc_MaxMenuTextLen;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	
	if (mi.fType & fTypeValue) return true; else return false;
}

void CBMenuItem::SetfType( UINT fTypeValue, bool bAddOrDelValue )
{
	MENUITEMINFO mi;
	TCHAR buff[mc_MaxMenuTextLen];
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.dwTypeData = buff;
	mi.cch = mc_MaxMenuTextLen;
	GetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);
	
	if (bAddOrDelValue)
		mi.fType = mi.fType | fTypeValue;
	else
		mi.fType = mi.fType & ~fTypeValue;
	SetMenuItemInfo(m_hMenuParent, m_idMenuItemOrPos, m_fByPosition, &mi);	

	// 如果是顶层菜单被修改了外观，自动重绘菜单条（仅在 m_hWndParent 被设置了有效）
	if (m_hWndParent)
		if (GetMenu(m_hWndParent) == m_hMenuParent) DrawMenuBar(m_hWndParent);
}









