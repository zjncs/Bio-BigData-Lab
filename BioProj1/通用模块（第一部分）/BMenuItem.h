//////////////////////////////////////////////////////////////////////
// BMenuItem.h: CBMenuItem 类的定义，用于操作一个菜单项（事件除外）
// 系统菜单与普通菜单的处理方法相同，用 CBForm 类的 Menu 的重载函数
//
// 由于配合菜单所属父窗体的 哈希表（指针为 m_ptrHashMenuIDs），使用本模块
//   处理菜单时，同一窗体的所有菜单项的 ID 都要保持不同，不要有位于不同
//   菜单中的相同 ID 的菜单项存在，否则只有第一个菜单有效
//
// 支持：需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////

#pragma once

#include "BWindows.h"

class CBMenuItem							// 一个菜单项的处理对象
{
private:
	static const int mc_MaxMenuTextLen;		// 菜单项最大文本长度

public:
	// 构造函数
	CBMenuItem(HMENU hMenuParent=NULL, UINT idResMenuItem=0, HWND hWndParent=NULL, CBHashLK * ptrHashMenuIDs=NULL);	
	// 析构函数
	~CBMenuItem();
	
	// 通过 资源ID 设置使用本对象管理的一个菜单项
	// 若 hMenuParent 为0，表示本对象正管理系统菜单的“父菜单项”
	//  （它引出的子菜单句柄为 GetSystemMenu，即它的子菜单的第一项是【还原】）
	//   若 hMenuParent 为0，将忽略 idResMenuItem
	// hWndParent 为菜单项隶属于的父窗口
	// ptrHashMenuIDs 为菜单项隶属 Form 对象（CBForm类）中维护的菜单 ID 的哈希表的地址，
	//   用于增删菜单时更新此哈希表
	void SetFromResID(HMENU hMenuParent, UINT idResMenuItem, HWND hWndParent, CBHashLK *ptrHashMenuIDs );
	
	// 通过 菜单位置 设置使用本对象管理的一个菜单项
	// 第一个菜单项位置为1（注意与API不同）
	// 若 hMenuParent 为0，表示本对象正管理系统菜单的“父菜单项”
	//  （它引出的子菜单句柄为 GetSystemMenu，即它的子菜单的第一项是【还原】）
	//   若 hMenuParent 为0，将忽略 pos
	// hWndParent 为菜单项隶属于的父窗口
	// ptrHashMenuIDs 为菜单项隶属 Form 对象（CBForm类）中维护的菜单 ID 的哈希表的地址，
	//   用于增删菜单时更新此哈希表
	// ptrHashObjs 为菜单项隶属 Form 对象（CBForm类）中维护的位图、光标、图标等对象的
	//   句柄哈希表的地址，用于加载菜单相关位图等资源时将句柄存入其中
	void SetFromPos(HMENU hMenuParent, UINT pos, HWND hWndParent, CBHashLK *ptrHashMenuIDs );

	// 返回直属父菜单的句柄
	HMENU hMenuParent() const;

	// 返回本菜单项的资源ID
	// 如果通过 菜单位置 设置的菜单项，也能用此函数获得资源ID
	// 如果菜单项无资源ID，或是弹出子菜单的菜单项，返回 0xFFFFFFFF
	UINT IDRes() const;

	// 获得本菜单项位于 m_hMenuParent 菜单中的位置（第一个位置为1，注与API从0开始不同）
	// 如通过 菜单资源ID 设置的菜单项，也能用此函数获得位置；失败返回 0
	UINT Position();

	// 删除本菜单项；若本菜单项引出子菜单，不会删除子菜单（子菜单可被
	//   重复利用），但须事先将子菜单句柄妥善保存（调用 hSubMenu 方法）
	BOOL Remove();

	// 在本菜单项同一菜单的最后，添加一个新菜单项或分隔线
	//   newMenuItemText 为菜单文本，为 0 时添加一条分隔线并忽略 bPopupSub
	//   bPopupSub 为 false 时表示添加一个命令菜单项，uIDNewItem 为新菜单项 ID
	//   bPopupSub 为 true 时表示添加一个子菜单，uIDNewItem 为子菜单资源 ID
	BOOL Append(LPCTSTR newMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);
	BOOL Append(tstring stringNewMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);
	
	// 若本菜单项将引出一个子菜单，则在子菜单的最后，添加一个新菜单项或分隔线
	//   若本菜单项不引出子菜单，调用此方法无效
	//   newMenuItemText 为菜单文本，为 0 时添加一条分隔线并忽略 bPopupSub
	//   bPopupSub 为 false 时表示添加一个命令菜单项，uIDNewItem 为新菜单项 ID
	//   bPopupSub 为 true 时表示添加一个子菜单，uIDNewItem 为子菜单资源 ID
	BOOL AppendSubItem(LPCTSTR newMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);
	BOOL AppendSubItem(tstring stringNewMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);

	// 在本菜单项之前（同一菜单中），插入一个新菜单项或分隔线
	//   newMenuItemText 为菜单文本，为 0 时添加一条分隔线并忽略 bPopupSub
	//   bPopupSub 为 false 时表示添加一个命令菜单项，uIDNewItem 为新菜单项 ID
	//   bPopupSub 为 true 时表示添加一个子菜单，uIDNewItem 为子菜单资源 ID
	BOOL Insert(LPCTSTR newMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);
	BOOL Insert(tstring stringNewMenuItemText, UINT uIDNewItem=0, bool bPopupSub=false);

	// 返回和设置该菜单项的 文本（或设置为分隔线；并取消为以位图方式的模式）
	// 如果是分隔线 或 位图形式的菜单项，返回 ""；若将 newText 设置为 0 表示分隔线
	LPTSTR Text();
	void TextSet(LPCTSTR newText);
	void TextSet(tstring stringNewText);

	// 设置该菜单项以位图方式显示
	// 用 idResBmp 参数的重载版可自动 LoadBitmap 并自动管理加载后的位图句柄
	//   在窗体卸载时自动释放此句柄；如使用 hBmp 的重载版本，本类不会自动释放
	// 参数为0时取消位图菜单模式，变回文本模式；但需再重新使用 TextSet 设置文本
	void BitmapSet(HBITMAP hBmp);
	void BitmapSet(UINT idResBmp);

	// 设置非选中、选中状态的位图
	BOOL SetCheckBitmaps(UINT idResBmpUnChecked, UINT idResBmpChecked);
	BOOL SetCheckBitmaps(EOEMBmp bmpUnChecked, EOEMBmp bmpChecked);

	// 返回和设置该菜单项的 使能
	bool Enabled();	
	void EnabledSet( bool enabled=true );	

	// 返回和设置该菜单项的 Checked 状态（对勾或圆点）
	bool Checked();						
	void CheckedSet(bool checked=true,	// bRadio=true 时，将忽略此参数值
					bool bRadio=false,	// bRadio=false 时为对勾， bRadio=true 时为圆点
					UINT itemFrom=0,	// bRadio=true 时，还要设置菜单项组的范围：itemFrom、itemTo
					UINT itemTo=0,
					bool fFromToByPosition=false	// 是否 用位置指定 itemFrom、itemTo，否则 itemFrom、itemTo 是菜单项的资源 ID
					);
	
	// 返回和设置一个菜单项是否为“圆点”的勾选标记（true），
    //   否则（false）是“对勾”的勾选标记
	bool RadioCheck();
	void RadioCheckSet(bool bValue);

	// 返回和设置该菜单项的高亮状态
	bool Hilited();	
	void HilitedSet();	

	// 设置一个菜单条上的菜单，如“文件”、“编辑”，从 pos 项开始的后续菜单
    //   项都被移动到菜单栏的右侧右对齐
	bool RightJustify();
	void RightJustifySet(bool bValue);

	// 返回和设置一个菜单项分栏，为true时，从此菜单项开始的后续菜单项
	//  将在一个“新行”中（对菜单条）或 一个新的"分栏"中（对下拉菜单、
	//  子菜单或快捷菜单）
	// bWithLine 表示栏间是否有竖线；若菜单条分新行，则忽略此参数
	bool BreakMenu(); 
	void BreakMenuSet(bool bBreak, bool bWithLine=true);

	// 返回和设置本菜单项所引出子菜单的默认项位置，位置从1开始
	//   0表示无默认项（本菜单项需引出子菜单才能使用以下2个函数）
	// bGoIntoPopups=true 时，若默认项打开下一级菜单，将从下一级菜单中继续
	//   查找默认项；如下一级无默认项还是返回当前菜单的默认项
	// bUseDisabled=true 时，若默认项被禁用或灰化 也返回该项，否则不返回
	UINT DefaultItemPos(bool bGoIntoPopups = false, bool bUseDisabled = true);
	void DefaultItemPosSet(UINT pos);

	// 返回和设置一个菜单项是否为从右到左的阅读顺序
	bool RightToLeft();
	void RightToLeftSet(bool bValue);

	// 返回和设置一个菜单项是否为 自绘菜单项
	bool OwnerDraw();
	void OwnerDrawSet(bool bValue);

	// 返回和设置该菜单项的附加数据 dwItemData
	unsigned long Tag();
	void TagSet(unsigned long ulNewValue);

	// 返回该菜单项是否为一条分隔线
	bool IsSeparator();

	// 若该菜单项引出一个子菜单，返回子菜单句柄（返回值不为0）
	//   若不引出子菜单，返回0
	HMENU hSubMenu();
	
	// 返回菜单项的位置（窗体坐标系，相对于菜单左上角位置）
	long LeftRelative();
	long TopRelative();
	// 返回菜单项的大小
	long Width();
	long Height();
	
	// 返回该菜单项所引出的子菜单的菜单项总数
	//  （本菜单项需引出子菜单才能使用以下函数）
	int CountSubItems();

private:
	// 获得菜单 hMenu 中，资源ID 为 idResMenuItem 的菜单项的位置
	// （第一个菜单项位置为0），失败返回 0xFFFFFFFF
	UINT GetMenuPositionFromID(HMENU hMenu, UINT idResMenuItem);

	// 获取和设置一个 fType 属性的公用函数
	bool GetfType(UINT fTypeValue);
	void SetfType(UINT fTypeValue, bool bAddOrDelValue);

private:
	HMENU m_hMenuParent;		// 所管理菜单项的直属父菜单句柄
								// 若此项为0，且 m_idMenuItemOrPos >0 时，表示本对象正管理：窗体“顶层菜单项”，它引出的子菜单句柄为 GetMenu，即它的子菜单的第一项是【文件】；调用 hSubMenu 获得窗体顶层菜单句柄
								// 若此项为0，且 m_idMenuItemOrPos ==0 时，表示本对象正管理：系统菜单的“父菜单项”，它引出的子菜单句柄为 GetSystemMenu，即它的子菜单的第一项是【还原】；调用 hSubMenu 获得系统菜单句柄
	UINT m_idMenuItemOrPos;		// 所管理菜单项资源 ID 或 位置（保存的第一个菜单项位置为0（与API一致））
	UINT m_fByPosition;			// 为 MF_BYCOMMAND 时 m_idMenuItemOrPos 为资源ID，为 MF_BYPOSITION 时 m_idMenuItemOrPos 为位置
	HWND m_hWndParent;			// 菜单项隶属于的父窗口
	CBHashLK * m_ptrHashMenuIDs;// 菜单项隶属 Form 对象（CBForm类）中维护的菜单 ID 的哈希表的地址，用于增删菜单时更新此哈希表
};



