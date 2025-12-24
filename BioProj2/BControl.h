//////////////////////////////////////////////////////////////////////
// BControl.h: CBControl 类的定义
// 实现对话框中的一个控件的各种功能
// 本类对象用全局 HM 管理动态分配的内存
//
// 支持：
//     需要 BWindows、BWndBase、BForm、BMenuItem 模块的支持
//////////////////////////////////////////////////////////////////////

#pragma once

#include "BWndBase.h"
// BControl.cpp 将包含 #include "BForm.h"，因 BControl.cpp
//   中的 CBControl 类的实现中，将用到 CBForm 类

class CBForm;	// 类的声明


enum EImageListCtrlType
{
	iml_Normal = 0, // LVSIL_NORMAL=0,
	iml_Small = 1,  // LVSIL_SMALL=1,
	iml_State = 2   // LVSIL_STATE=2
};

enum EImageListFlagsColor
{
	imlcol_Color = ILC_COLOR,		// = 0 Use the default behavior if none of the other ILC_COLOR* flags is specified. Typically, the default is ILC_COLOR4, but for older display drivers, the default is ILC_COLORDDB.
	imlcol_Color4 = ILC_COLOR4,		//= 4     Use a 4-bit (16-color) device-independent bitmap (DIB) section as the bitmap for the image list.
	imlcol_Color8 = ILC_COLOR8,		//= 8     Use an 8-bit DIB section. The colors used for the color table are the same colors as the halftone palette.
	imlcol_Color16 = ILC_COLOR16,	//= &H10  Use a 16-bit (32/64k-color) DIB section.
	imlcol_Color24 = ILC_COLOR24,	//= &H18  Use a 24-bit DIB section.
	imlcol_Color32 = ILC_COLOR32,	//= &H20  Use a 32-bit DIB section.
	imlcol_ColorDDB = ILC_COLORDDB	//= &HFE  Use a device-dependent bitmap.
};

enum ELsvGetItemRectCode // Portion of the list view item for which to retrieve the bounding rectangle.
{
	lsvir_Bounds = LVIR_BOUNDS,				//= 0 Returns the bounding rectangle of the entire item, including the icon and label.
	lsvir_Icon = LVIR_ICON,					// = 1 Returns the bounding rectangle of the icon or small icon.
	lsvir_Label = LVIR_LABEL,				//= 2 Returns the bounding rectangle of the item text.
	lsvir_SelectBounds = LVIR_SELECTBOUNDS	//= 3 Returns the union of the LVIR_ICON and LVIR_LABEL rectangles, but excludes columns in details view.
};

// ImageList 控件的创建和维护
class CBImageList
{
public:
	CBImageList();
	~CBImageList();
	// 创建一个 ImageList 控件并返回控件的句柄，失败返回 NULL
	HIMAGELIST CreateImageList(int iImgWidth=16, 
		int iImgHeight=16, 
		EImageListFlagsColor colorType=imlcol_Color32, 
		bool fUseMask=true,
		int imgCountInit=128,
		int imgCountMax=8192 );

	// 在 ImageList 控件中添加一个图标，iconRes 为图标资源 ID，或添加系统图标
	// 成功返回新添加的图标图像的编号（从1开始），失败返回0
	// 若成功，则在 mImgIdxes() 数组中添加一个项目，保存此编号，并使 mImgCount=mImgCount+1
	//   之后主调程序可用 ImageIdx(第几个图像) 获得每个图像的编号，后者（编号）用于其他 API 函数
	int AddImageIcon(EStandardIcon icon);
	int AddImageIcon(unsigned short iconRes);
	int AddImageIcon(HICON hIcon);

	// 给出 1 开始的一个 iOrderNum，返回 ImageList 控件中可以使用的一个编号（后者可用于其他 API 函数）
	int ImageIdx(int iOrderNum);
	// 返回 ImageList 控件中当前共有图像数量
	int ImageCount();
	// 获得 ImageList 控件的句柄
	HIMAGELIST hImageList();		
private:
	HIMAGELIST m_hImgList;			// ImageList 控件的句柄
	int * m_ImgIdxes;				// 数组 m_ImgIndexes[下标从1开始] 保存控件中每个图像的编号
	int m_ImgIdxesUbound;			// 数组 m_ImgIndexes[] 目前已分配空间的下标上界
	int m_ImgCount;					// 控件中共有图像数
};

class CBControl : public CBWndBase
{
public:
	// 构造函数。idResControl 为控件的资源id
	//   也可使用重载版本的 构造函数，给控件的句柄
	CBControl(unsigned short int idResControl=0);		
	CBControl(HWND hWndCtrl);		
	
	// 析构函数
	~CBControl();

	// 设置使用本对象管理的一个控件，如未在构造函数中设置，也可用本函数中设置
	// idResControl 为控件的 资源ID；也可使用重载函数，给控件的句柄
	// 但如通过 资源ID 方式，之前必须加载了窗体，因为那期间做了 EnumChildProcSubClass，
	//   在 CBWndBase::ms_hashCtrlResIDs 哈希表中添加了控件 ID 信息
	// 成功返回子窗口控件句柄，失败返回0
	HWND SetResID(unsigned short int idResControl=0);	
	HWND SetResID(HWND hWndCtrl=NULL);	
	

	// 为支持图片按钮，重写 EnabledSet
	void EnabledSet(bool enabledValue);	



	// 获得控件文本，但返回的不是字符串，而是转换为的整数
	// retSuccess 为一个指针，指向数据表示函数是否执行成功
	// 仅有 CBControl 类有 TextInt；
	// Text、TextVal 继承 CBWndBase 基类
	int TextInt(BOOL * retSuccess=0);


	// ======== Static 类控件功能 ========
	bool BackStyleTransparent();
	void BackStyleTransparentSet(bool bTransparent);

	// 设置一个 Static 控件为图片按钮，
	//    按钮大小为控件 Width、Height；各按钮状态的位图组合为一张大位图，组合位图资源ID为 bmpCombResID
	// bmpCombResID=0 时，使用 PictureSet() 设置的位图
	// btnX, btnY, btnWidth, btnHeight 为图片按钮的位置、大小，如 >=0 则自动调整控件大小为此值（这4个参数只为调用本函数时
	//   自动设置控件位置、大小）
	// iStateStyle = 1时，使用两态按钮：单击一次按下、再单击一次抬起；可由 ValueChecked() 获得其按下/抬起 状态
	void PicButtonSet(EPicButtonType style=epbNone, UINT bmpCombResID=0, 
		int btnWidth=-1, int btnHeight=-1, int btnX=-1, int btnY=-1, 
		int iStateStyle = 0);

	// 设置图片按钮的状态：0=常规；1=高亮；2=按下；3=灰色
	// iSetOrReleaseCapture：是否同时 SetCapture (=1) 或 ReleaseCapture(=-1) 或不做 (=0)
	// 如 iSetOrReleaseCapture 非0，无论如何都会做 SetCapture 或 ReleaseCapture
	// 本函数仅设置控件的外观状态，要设置 Checked 状态，请使用 ValueCheckedSet()
	void PicButtonStateSet(int iState, int iSetOrReleaseCapture=0);
	int PicButtonState();

	// ======== Edit、Rich Edit 类控件功能 ========
	
	// 返回选区的起始位置（第一个字符位置为0）
	// 如给出参数 pLineIdx （DWORD变量的地址），还由参数传回选区
	//   起始位置的行号（第1行编号为0）。
	// 可用于 Edit、Rich Edit、Combo 的文本框部分、List 控件
	// pLineIdx 对 ComboBox、List 控件 无效
	int SelStart(int *pLineIdx=NULL);

	// 返回选区长度
	// 可用于 Edit、Rich Edit、Combo 的文本框部分、List 控件
	int SelLength();

	// 选中一段文本，第一个字符位置为0。
	//   两个参数为 0、-1 时全选；第一个参数为-1时取消选择
	// 可用于 Edit、Rich Edit、Combo 的文本框部分、List 控件
	// 用于 List 控件时，控件必须为多选风格，选中/不选连续多个项目
	//   selLength>0 选中项目，selLength<0 取消选中项目
	//   项目编号从 1 开始；selStart==0,selLength==-1 时全选
	void SelSet(int selStart=0, int selLength=-1);		
	
	// 替换选中文本，bCanUndo 表示替换操作可否被撤销
	void SelTextSet(LPCTSTR stringRepl, bool bCanUndo=true);	
	void SelTextSet(tstring stringRepl, bool bCanUndo=true);	

	// 查找文本（仅支持 Rich Edit）
	// rgStart、rgLength 表示搜索范围，默认搜索在所有文本范围内搜索
	// bCaseSensitive==true 时区分大小写；bMatchWholeWord==false 时不区分大小写
	// bAutoSelFound==true 时，还将自动在控件中选中找到的文本
	// 返回第一个匹配的文本起始位置，位置从0开始；出错返回 -1
	int FindText(LPTSTR stringFind, 
				 int rgStart=0, 
				 int rgLength=-1, 
				 bool bCaseSensitive=false, 
				 bool bMatchWholeWord=false, 
				 bool bAutoSelFound=true);
	
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
	void EditSetApperance(unsigned short idPicBorder,  unsigned short idBMPBorder,  
					 int x, int y, int width, int height, 
					 LPTSTR szFontName=NULL, float fFontSize=10.5, int xOffset=4, bool bOnlyMoveSize=false);

	// ======== 按钮类控件功能 ========

	// 返回和设置单选或复选按钮是否被勾选了：0=未选；1=已选；2=三态
	// 此也可用于 Static 控件制作的“图片按钮”
	unsigned int ValueChecked();			
	void ValueCheckedSet( UINT uCheck );	


	// ======== 列表框、组合框、ListView 类控件功能 ========

	// 添加一个条目，返回添加的条目编号（从1开始），出错返回 0
	// index<=0 时，在末尾添加；否则在指定的 index 位置添加，对列表框、组合框后者不能自动排序
	// iImage、iTagLong、iIndent 参数仅对 ListView 控件有效
	//   iImage 为在 image list 中的大图标和小图标的编号，从 1 开始；0 表示暂不设置此项
	//   iIndent 为项目缩进几个图像的宽度，如 =1 为缩进一个图像的宽度；=-&H80000000 表暂不设置此项
	// 返回新条目的索引号（>=1），0表示出错
	int AddItem(LPTSTR szText, int index = -1, int iImage = 0, int iTagLong = 0, int iIndent = 0x80000000) const;	
	int AddItem(tstring stringText, int index = -1, int iImage = 0, int iTagLong = 0, int iIndent = 0x80000000) const;	

	// 删除一个条目，编号从1开始；返回删除后的列表共有条目数，出错返回0
	int RemoveItem(int index) const;

	// 返回共有条目数，出错返回<0的值
	int ListCount() const;

	// 返回当前选择的条目编号（编号从1开始），出错返回0
	// 若对多选列表框使用，则返回的是具有焦点的项目编号
	//   所有项目都没被选择时，返回1（表示第1个项目有焦点）
	int ListIndex() const;

	// 选择一个条目，条目编号从1开始
	// 该函数对多选列表框使用无效
	void ListIndexSet(int index) const;

	// 获取和设置 一个条目的内容，index 从1开始
	//   （index<0时获得 ListIndex() 的文本）
	// indexSub 参数仅对 ListView 控件有效
	// 对 ListView 控件，subItemIndex 为 1 的就是 ListItem.Text 本身
	//   第二列的 subItemIndex 为 2，
	// 因此可以用 .ItemText(row, col) 直接获取（或用 Set 设置）文本，row、col 都是从 1 开始的
	// 返回文本（ListViewItemText）时，自动开辟和由 HM 自动管理内存空间
	LPTSTR ItemText(int index=-1, int indexSub=0) const;
	void ItemTextSet(int index, LPTSTR szNewText=NULL, int indexSub=0) const;
	void ItemTextSet(int index, tstring stringNewText=NULL, int indexSub=0) const;
	LPTSTR List(int index=-1, int indexSub=0) const;			// 同 ItemText()

	// 获取和设置 ListBox、ComboBox、ListView 中各项目的附加整数数据，index 从1开始
	int ItemData(int index) const;
	void ItemDataSet(int index, int itemData);

	// 获取和设置 多选列表的一个项目是否被选中的状态，index 从1开始
	bool ItemSelected(int index) const;
	void ItemSelectedSet(int index, bool bSel) const;
	
	// 获取和设置 ListView 的一个项目前的勾选状态，index 从1开始
	bool ItemChecked(int index) const;
	void ItemCheckedSet(int index, bool bSel);

	// 获取和设置一个项目的图标（对 ListView 控件有效）
	// 若要支持 SubItem 也能显示图像，需要设置：ListViewAllowSubItemImagesSet(True);
	// iImage 为在 ImageList 中的大图标和小图标的编号，从 1 开始；0 表示暂不设置此项
	// 可以用 .ItemImage(row, col) 直接设置图片，row、col 都是从 1 开始的
	int ItemImage(int indexItem, int indexSubItem=1) const;
	void ItemImageSet(int indexItem, int indexSubItem=1, int iImage=0);
	
	// 全选(iSelStyle=1)、反选(iSelStyle=-1)、取消选择(iSelStyle=0)
	//  ListBox 或 ListView 控件中的项目
	void ItemsSelDo(int iSelStyle=1);	

	// 返回多选列表中已选条目数
	int ListSelCount() const;

	// 将 List、ListView 中的选定项或所有项文本拷贝到剪贴板
	void ItemsCopyToClipboard(bool bOnlySel=true, LPCTSTR szSpliter=TEXT("\t"));

	// 返回和设置列表中第一个可见项的索引号（从 1 开始）
	int ListTopIndex();
	void ListTopIndexSet(int idxTop);


	// 获得一个列表框中所有选定项目的索引号，放入一个 int 型数组
	//   各项目索引号从 1 开始
	// 数组下标从 1 开始，由本函数自动开辟数组空间
	// 函数返回数组首地址，没有选中项或出错返回 0
	// 数组元素个数返回到参数 pSelCount 所指空间，pSelCount 为0时不返回
	//   可用 ListSelCount 获得选择的元素个数
	// 对单选列表框数组中只有一个元素
	int * ListSelItems(int *pSelCount=0) const;

	// 清除列表的所有内容
	void ListClear() const;

	// 获取和设置列表项高度
	int ListItemsHeight() const;
	void ListItemsHeightSet(int newHeight);
	
	// 获得 ListView 控件的一个项目或子项目（indexSubItem>1）的 rectType 内容的矩形区域
	//    indexSubItem 也可看作列号（第1列为1）
	// hwndControlToMove 若不为 NULL，本函数还将自动移动该控件并调整大小到该矩形区域位置
	// rectType=lsvir_SelectBounds 仅对项目（即第1列）有效，对子项目（第2列及以后）无效
	RECT ListViewGetItemRect(int indexItem, 
							 int indexSubItem = 1, 							 
							 HWND hwndControlToMove = NULL, 
							 ELsvGetItemRectCode rectType = lsvir_Label );

	// 视图列表中添加新的一列：iAlign=0,1,2 分别表示左、右、中对齐
	// index 从1开始编号，指定要插入的位置；默认在最后一列新增
	// 成功返回列索引号（从1开始），失败返回0
	int ListViewAddColumn(LPTSTR szColText, int iWidth = 60, int iAlign = 0, int iSubItemIndex = 0, int iImageIdx = -1, int iIndex = 0x7FFFFFFF);
	int ListViewAddColumn(tstring stringColText, int iWidth = 60, int iAlign = 0, int iSubItemIndex = 0, int iImageIdx = -1, int iIndex = 0x7FFFFFFF);

	// 返回或设置列标题的对齐方式（0,1,2 分别表示左、右、中对齐）
	int ListViewColumnAlignment(int indexCol);
	void ListViewColumnAlignmentSet(int indexCol, int iAlign = 0);

	// 返回 ListView 的列数（仅在详细资料视图下有效）
	int ListViewColumnsCount();

	// 返回“下一个 Item”的索引号
	// idxStart 从哪个开始查找（不包含本身，0表示查找所有即查找第一个）
	// bFindAll=true 时，后面参数都被忽略；否则按后面参数组合构成的条件进行查找
	// 找到返回索引号（从1开始），没找到返回 0
	//-------------------------------------------------------------------------------
	// 用此函数可方便地遍历所有被选项目，如：
	// i = ...ListViewNextItemIdx(0, false, true);  // 若没有下一个被选中的项目，返回 0
	// While(i)
	// {
	//	  处理第 i 个项目
	//    // 获得下一个被选项目的索引号 -> i
	//    i = ...ListViewNextItemIdx(i, false, true)  // 若没有下一个被选中的项目，返回 0
	//  }
	//-------------------------------------------------------------------------------
	int ListViewNextItemIdx(int idxStart=0, bool bFindAll=true, bool bFindSelected=true, 
		bool bFindCut=false, bool bFindDropHilited=false, bool bFindFocused=false, 
		bool bFindAbove=false, bool bFindBelow=false, bool bFindLeft=false, bool bFindRight=false) const;

	// 返回或设置 ListView 是否整行选择（详细资料视图有效）：LVS_EX_FULLROWSELECT
	bool ListViewFullRowSelect();
	void ListViewFullRowSelectSet(bool blNewValue);

	// 返回或设置 ListView 是否是否允许 subitem 也可显示图片：LVS_EX_SUBITEMIMAGES
	bool ListViewAllowSubItemImages();
	void ListViewAllowSubItemImagesSet(bool blNewValue);
	

	// 返回或设置 是否有网格线（详细资料视图有效）：LVS_EX_GRIDLINES
	bool ListViewGridLines();
	void ListViewGridLinesSet(bool blNewValue);

	// 返回或设置 各条目前是否有复选框：LVS_EX_CHECKBOXES
	bool ListViewCheckBoxes();
	void ListViewCheckBoxesSet(bool blNewValue);

	// 返回或设置所关联的 ImageList 控件（ListView 控件或 TreeView 控件有效）
	HIMAGELIST hImageListCtrl(EImageListCtrlType imlType);
	void hImageListCtrlSet(EImageListCtrlType imlType, HIMAGELIST hImgList);


	// ======== ProgressBar 类控件功能 ========
	int Value();
	void ValueSet(int iNewValue);

	int Max();
	void MaxSet(int iNewValue);

	int Min();
	void MinSet(int iNewValue);



	// ======== 高级通用功能 ========
	
	// 获得子窗口控件的窗口句柄
	HWND hWnd() const;				
	
	// 获得所属父窗体的窗口句柄
	HWND hWndParentForm() const;		

	// 返回或设置父窗口的句柄
	//  （如不在其他控件内，获得值与 hWndParentForm 相同）
	HWND Parent();
	HWND ParentSet(HWND hWndParent);	// 成功返回之前父窗口的句柄，失败返回 NULL

	// 返回一个父窗体的 CBForm 对象的地址指向，用于通过控件访问父窗体
	// 例如：Form1.Control(ID_cmd1).ParentFormPtr()->BackColorSet(...);
	CBForm * ParentFormPtr() const;		

	// 获得子窗口控件的默认窗口程序的地址（注实际此地址的函数可能不被使用，因已被子类化）
	unsigned long PtrWndProcDef() const;			
	
private:
	// HWND m_hWnd：子窗口控件的句柄，使用基类 BWndBase 的成员，此值在 SetResID 函数中被设置
	// TCHAR m_ClassName[128]：窗口控件的类名，使用基类 BWndBase 的成员
	// long m_atom：窗口控件类的 atom 号，唯一标识一种类，使用基类 BWndBase 的成员
	//				这两个值都在 SetResID 后即自动被设置
	// STWndProp * m_pPro：窗体或控件附件属性结构体变量的地址，使用基类 BWndBase 的成员

	unsigned short int m_ResCtrlID;	// 控件的资源id，此值在 SetResID 函数中被设置
};