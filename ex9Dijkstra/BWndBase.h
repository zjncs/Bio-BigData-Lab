//////////////////////////////////////////////////////////////////////
// BWndBase.h: CBWndBase 类的定义
// 包含常规窗口功能，作为 CBForm、CBControl 类的基类
//
// 支持：
//     需要 BWindows 模块的支持
//////////////////////////////////////////////////////////////////////

#pragma once
#pragma warning(disable:4996) // 关闭 warning C4996: 如 'wcscpy': This function or variable may be unsafe. Consider using wcscpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.

#if _MSC_VER > 1200   // MSVC++ 6.0  _MSC_VER == 1200
	// VC6 会出现 warning LNK4044: 如 warning LNK4044: unrecognized option "manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' ...
	#ifdef _M_IX86
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#elif _M_IA64
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#elif _M_X64
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#else
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#endif
#endif

#include "BWindows.h"
#include <commctrl.h>
#include <richedit.h>

// 定义常量（这些常量 VC6 的头文件没有）
// 在高版本的 VC 中，头文件中已包含，应不再此处定义
#ifndef WS_EX_LAYERED
	#define	WS_EX_LAYERED	0x80000
#endif

#ifndef LWA_COLORKEY
	#define	LWA_COLORKEY	0x1
#endif

#ifndef LWA_ALPHA
	#define	LWA_ALPHA		0x2
#endif

enum EAlign	// 文本水平对齐方式
{
	eAlignLeft = DT_LEFT, 
	eAlignCenter = DT_CENTER,
	eAlignRight = DT_RIGHT
};


enum EVAlign	// 文本垂直对齐方式
{
	eVAlignTop = DT_TOP, 
	eVAlignVCenter = DT_VCENTER,
	eVAlignBottom = DT_BOTTOM
};


// 图片按钮的类型
//   按钮的常规状态、高亮状态、按下状态、灰色状态的图片组成同一张大图片
//   本枚举类型表示这些状态图片如何组成这一张大图片
// 枚举数值：高16位表示垂直偏移，低16位表示水平偏移
//           4个位表示一种状态的图片的偏移，每16位中从高到低依次为（高位常规使高位为0）：
//			 常规状态偏移量、高亮状态偏移量、按下状态偏移量、灰色状态偏移量
enum EPicButtonType
{
	epbNone = 0,
	epbVertNormHiltDownDisb = 0x01230000,	// 垂直排列 常规、高亮、按下、灰色
	epbVertNormHiltDisb = 0x01020000,		// 垂直排列 常规、高亮、灰色
	epbVertNormHiltDown = 0x01200000,		// 垂直排列 常规、高亮、按下
	epbVertNormDown = 0x00100000,			// 垂直排列 常规、按下
	
	epbHoriNormHiltDownDisb = 0x00000123,	// 水平排列 常规、高亮、按下、灰色
	epbHoriNormHiltDisb = 0x00000102,		// 水平排列 常规、高亮、灰色
	epbHoriNormHiltDown = 0x00000120,		// 水平排列 常规、高亮、按下
	epbHoriNormDown = 0x00000010			// 水平排列 常规、按下
};


// 窗体或控件的附加属性
// 窗体的空间使用 CBForm 的数据成员，控件的空间动态开辟
typedef struct _WndProp			 
{
	long cursorIdx, cursorIdx2;	// cursorIdx 为窗体当前的鼠标光标索引号，对应句柄为 hCursor。0 表示使用系统默认
								// cursorIdx2 为 cursorIdx 的副本，在窗体允许自动拖动边框调整大小（m_iBorderAutoResizable）
								//   时，响应 WM_MouseMove 消息时，当移动到非边框区域用 cursorIdx2 恢复鼠标指针
	HCURSOR hCursor, hCursor2;	// hCursor 为窗体当前的鼠标光标句柄（在 cursorIdx 非 0 时才有效）
								// hCursor2 为 hCursor 的副本，意义同上


	COLORREF backColor;			// 窗体或控件 背景色，-1 表示使用默认色
	HBRUSH hBrushBack;			// 背景色画刷。设置背景色的同时就创建
								//   对窗体：CBForm 对象析构时 DeleteObject
								//   对控件：EnumChildProcSubClass 中的卸载部分 DeleteObject
	COLORREF foreColor;			// 窗体或控件 前景色，-1 表示使用默认色
	bool bBackStyleTransparent;	// 控件背景是否透明（仅对Static等某些控件有效）
	HFONT hFont;				// 所用字体（为0表示用系统字体；否则表示用自定义字体，最后需 DeleteObject ）
	RECT rcTextOutClip;			// PrintText 输出文本时的输出位置（.left、.top）和输出范围
	int iTextOutStartLeft;		// 输出文本的范围的原始左边界，供 PrintText 换行时确定新行左边界
	unsigned int iTextOutFlags;	// 输出文本的选项


	bool stretch;					// 窗体或控件 被设置背景图片时使用，是否自动拉伸图片大小以适应 窗体或控件 的客户区
	HBITMAP hBmpDisp;				// （不作为属性使用）要作为背景的位图图片。stretch=true 时，此为原始未缩放的位图句柄
	RECT rcPictureClip;				// 各成员>=0时有效，要截取图片的一部分而不是把图片全部显示到 窗体或控件 上
	
	int iTitleBarBehav;				// 是否具有标题栏行为（=0不具有；=1 左键拖动将移动 窗体或控件 所在的窗体）
	
	bool fFromCls;					// （不作为属性使用）为一个标志变量
									//  在 Cls() 时将成员量设为 true，并 InvalidateRect。通过响应 WM_PAINT 来实现 Cls
									//  在响应 WM_PAINT 时，若此成员为 true 则不再生成 Paint 事件
									//  避免在 Paint 事件函数中用户又调用 Cls 造成死递归
	
	bool fDisableContextMenu;		// 是否禁用右键菜单（即是否取消 WM_CONTEXTMENU 消息）

	LPTSTR tagString;				// 附加字符串数据
	int iTag1;						// 附加整数数据1
	int iTag2;						// 附加整数数据2
	double dTag;					// 附加 double 型数据


	// 以下仅用于控件（不用于窗体）
	EPicButtonType ePicBtn;			// 是否作为图片按钮（作为则此值不为0）及图片按钮如何显示位图的类型
	int iPicBtnCurrDisp;			// 如果作为图片按钮，当前显示的按钮状态：0=常规；1=高亮；2=按下；3=灰色
	int iPicBtnStateStyle;			// 是否作为 Checked 类型按钮(=1)（单击一次按下去，再单击一次抬起）；
	int iPicBtnCheckedState;		// iPicBtnStateStyle<>0 时有效，现在按钮 Checked() 状态
									// 可由控件的 ValueChecked() 方法获得目前按钮状态


	// ======================================================================
	// 添加新属性方法：
	//	1. 这里添加新成员；
	//  2. CBForm 构造函数初始化 m_WndProp.成员 的值
	//     本类 PropertyMemCtrl() 静态函数中初始化新空间 成员的值（如为0可不初始化因已清零）
	//  3. CBForm 类的 ClearResource() 函数中，清除成员的值或释放成员对应的资源
	//     EnumChildProcSubClass 中的卸载部分卸载控件的 成员的值或相应资源
	//	4. 本类中增加获取属性和设置属性的函数，函数中通过调用
	//	   m_pPro = PropMem(false); 分配和获得本结构体数据的空间，
	//	   然后使用动态空间中的本成员
	//	   （可参见 BackColor() 和 BackColorSet(COLORREF color)）
	//	5. 实现功能：在 BForm.cpp 的 CBForm_DlgProc 或 CBForm_ControlProc 函数中
	//	   或相关 EventsGenerator 函数中添加处理相关消息的代码。
	//     派生类对象（CBForm 或 CBControl）中，用 PropMem() 获得属性空间，也可：
	//     对窗体，直接用 m_pPro 获得属性空间；
	//     对控件，用 CBWndBase 的静态函数 PropertyMemCtrl 根据 控件句柄 获得属性空间；
	// ======================================================================



} STWndProp ;


class CBWndBase
{
protected:
	// ======================================================================================
	// 静态成员和成员函数
	// ======================================================================================

	// 以下两个静态哈希表（所有本类对象共用）：ms_hashCtrls、ms_hashCtrlResIDs
	//   保存所有子窗口控件信息（位于所有窗体上的控件都统一用此保存）
	// 任何窗体被加载时（WM_INITDIALOG）递归枚举所有子窗口，将之子类化并将
	//   子窗口信息保存于此；任何窗体被卸载时（ClearResource），递归枚举所
	//   有子窗口，从该哈希表中删除这些子窗口信息


	// Key = 控件hwnd，
	// Item = 控件默认窗口程序地址，
	// ItemLong = 所位于窗体的 hWnd，可以此为 key 到 ms_hashWnd 中获得窗体
	//   对应的 CBForm 对象的地址。注意此值不是控件父窗口的句柄，
	//   而直接是所位于窗体的 hWnd。父窗口的句柄可用 GetParent 获得
	// ItemLong2 = 动态分配的一个 STRControlProp 类型的数据的地址，表示控件
	//    的附加属性。只有在设置了控件的某个附加属性时，才会动态开辟一个
	//    STRControlProp 的空间，ItemLong2 才会不为0；
	//    否则 ItemLong2 为 0 表示控件没有被设置过这些属性
	static CBHashLK ms_hashCtrls;	


	// Key = 控件资源ID
	// Item = 控件本身的 hWnd
	// ItemLong = 直接所属父窗口的 hWnd（如不位于其他控件内，与 ItemLong2 相同）
	// ItemLong2 = 所位于窗体的 hWnd，可以此为 key 到 ms_hashWnd 中获得窗体
	//   对应的 CBForm 对象的地址
	static CBHashLK ms_hashCtrlResIDs;


	// ============================================================
	// 有关附加属性的说明：
	// 无论是窗体还是控件，一个窗体和一个控件都用 一个 STWndProp 类型
	//   空间保存该窗体或该控件的所有附加属性。不同的是：
	// 窗体（CBForm类的对象）使用一个 STWndProp 类型的数据成员成员 m_WndProp
	// 控件（CBControl类的对象）类中无 STWndProp 类型的数据成员，因为
	//   CBControl 类的对象将被动态创建和销毁，而对象销毁控件不一定销毁
	// 对一个控件（CBControl类的对象），在需要时动态开辟一个 STWndProp 类型
	//   的空间，并由静态哈希表 ms_hashCtrls 保存控件 hWnd 和该空间的对应关系
	//   这样属性便与控件的 hWnd 对应起来，而 CBControl 类的对象的创建和销毁
	//   与该属性空间的开辟和释放无关。
	// 动态开辟控件的属性空间，是由 PropertyMemCtrl() 静态函数实现的，但
	//   PropMem() 成员函数也自动调用 PropertyMemCtrl()
	// 释放这些动态开辟的控件的属性空间，在 CBForm 类的 EnumChildProcSubClass
	//   中实现
	//
	// 尽管 窗体和控件 的属性空间有此不同，但在 BWndBase 中窗体和控件
	//   都有的方法中，处理是相同的，如在 BackColorSet() 中。这是因为 
	//   BWndBase 中有一个 STWndProp * 类型的指针 m_pPro，既可指向窗体的
	//   属性空间，也可指向控件的属性空间。
	// ============================================================


	// 从一个控件的 hWnd，获得一个 控件 的附加属性 STWndProp 空间的地址（不用于窗体）
	// 对窗体应该在创建 CBForm 对象时向基类传递 m_WndProp 的地址，然后调用对象的
	//   PropMem() 成员函数（也是基类的成员函数）获得
	// 对控件：若还没有附加属性空间，当 bCreate=false 时返回 NULL，
	//   当 bCreate=true 时新开辟附加属性的空间，返回 新空间地址
	// 对控件：若已有附加属性空间，不会重新开辟新空间，而直接返回原来
	//   空间的地址
	// 对控件：如从一个 CBControl 对象本身直接获得该对象对应控件的属性空间
	//   也可调用 CBControl 对象的 PropMem() 成员函数，结果相同
	// 原理是管理 ms_hashCtrls.ItemLong2
	static STWndProp * PropertyMemCtrl(HWND hWndCtrl, bool bCreate=false);


public:
	
	// ======================================================================================
	// 友元函数
	// ======================================================================================
	
	// WinMain 函数
	int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char * lpCmdLine, int nShowCmd );

	// 公用窗口过程：所有本类对象（窗体）都用此函数作为窗口过程
	friend BOOL WINAPI CBForm_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	// 公用窗口过程：所有本类对象（窗体）中的子窗口控件，都用此函数作为窗口过程，所有子窗口控件都被子类处理
	friend BOOL WINAPI CBForm_ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	// 枚举子窗口控件、子类处理（取消子类处理）所有子窗口控件的回调函数：
	// lParam 为控件所属窗体的句柄（非0）时，表示加载窗体时的子类处理；
	// lParam 为 0 时，表示卸载窗体时的恢复子类处理
	friend BOOL CALLBACK EnumChildProcSubClass(HWND hwnd, LPARAM lParam);
	
	// 消息循环函数
	friend void MessageLoop(int iExitStyle/*=0*/);

public:
	// 构造函数和析构函数
	// pProperty 是保存窗体或控件属性的结构体空间的地址
	//   如果是 CBForm 继承，应将 pProperty 指向 CBForm 对象中的 m_WndProp 成员
	//   如果是 CBControl 继承，可保持 pProperty 为空，在需要属性空间时，本类会
	//    自动动态分配；对同一控件（hWnd 相同的）多次创建不同的 CBControl 对象
	//    本类会自动获得上次为它分配的空间，保留上次的属性数据，不会每次分配新空间
	CBWndBase( HWND hWndToManage = NULL, STWndProp * pProperty = NULL );	
	virtual ~CBWndBase();

	// 获得和设置窗口的 使能 状态
	void EnabledSet(bool enabledValue) const;
	bool Enabled() const;	
	
	// 获得和设置窗口的 隐藏 状态
	void VisibleSet(bool visibleValue) const;	
	bool Visible() const;
	
	// 返回和设置窗口是否有滚动条
	bool HScrollBar();
	void HScrollBarSet(bool bValue);
	bool VScrollBar();
	void VScrollBarSet(bool bValue);
	
	// 返回和设置 控件是否接收按 Tab 键移动焦点
	bool TabStop();
	void TabStopSet(bool bValue);
	
	// 返回和设置Group属性
	// 从第一个 Group=true 的控件开始 至下一个 Group=True 的控
    //   件为止，之间的所有控件为一组
	bool Group();
	void GroupSet(bool bValue);

	bool DisableContextMenu();
	void DisableContextMenuSet(bool newValue);

	// 附加字符串数据
	LPTSTR TagString();
	void TagStringSet(LPCTSTR tagString);
	
	// 附加整数1
	int TagInt();
	void TagIntSet(int iTag);

	// 附加整数2
	int TagInt2();
	void TagInt2Set(int iTag2);

	// 附加 double
	double TagDouble();
	void TagDoubleSet(double dTag);


	// ================================================
	// 位置、大小
	// ================================================

	// 获得和改变窗口大小、位置（移动窗口、改变窗口大小）
	void Move(int left=0x7FFFFFFF, int top=0x7FFFFFFF, 
		int width=0x7FFFFFFF, int height=0x7FFFFFFF) const;
	int Left() const;
	int Top() const;
	int Width() const;
	int Height() const;
	int ClientWidth() const;
	int ClientHeight() const;
	void LeftSet(int left) const;
	void TopSet(int top) const;
	void WidthSet(int width) const;
	void HeightSet(int height) const;


	

	// ================================================
	// 外观
	// ================================================

	// 返回和设置 窗体或控件 鼠标光标形状的索引值（不修改窗口类，而是通过响应 WM_SETCURSOR 实现）
	//   typeRes 为 0 时，设置 idResCursor 为 Cursor 类型的资源 ID
	//   也可设置其他类型的资源，此时 typeRes 为类型字符串（动画光标时使用）
	// 调用窗体的 MousePointerSet 方法设置窗体的鼠标指针不影响子窗口控件，如需设置子窗口控件
	//   的光标，应调用对应控件的 MousePointerSet
	long MousePointer();
	void MousePointerSet(EStandardCursor cursor);		
	void MousePointerSet(long idResCursor, LPCTSTR typeRes=0);


	// 返回和设置 窗体或控件 的背景色（设为 -1 表示使用默认颜色）
	// 也可用于设置进度条控件背景色（不使用XP风格6.0控件时才有效）
	COLORREF BackColor();
	void BackColorSet(EColorType color);
	void BackColorSet(COLORREF color);

	// 返回和设置 窗体或控件 的前景色（如用于输出文本的文本颜色）
	// 也可用于设置进度条控件前景色（“条”的颜色）（不使用XP风格6.0控件时才有效）
	COLORREF ForeColor();
	void ForeColorSet(EColorType color);
	void ForeColorSet(COLORREF color);




	// ================================================
	// 文本
	// ================================================

	// 设置窗口文本（各种重载版本）
	void TextSet(LPCTSTR newText) const;
	void TextSet(char valueChar) const;
	void TextSet(unsigned short int valueInt) const;	// TCHAR
	void TextSet(int valueInt) const;
	void TextSet(unsigned int valueInt) const; 
	void TextSet(long valueLong) const;
	void TextSet(unsigned long valueInt) const;
	void TextSet(float valueSng) const;
	void TextSet(double valueDbl) const;
	void TextSet(long double valueDbl) const;
	void TextSet(tstring valueString) const;
	
	// 设置窗口文本（各种重载版本）
	void TextAdd(LPCTSTR newText) const;
	void TextAdd(char valueChar) const;
	void TextAdd(unsigned short int valueInt) const;
	void TextAdd(int valueInt) const;
	void TextAdd(unsigned int valueInt) const;
	void TextAdd(long valueLong) const;
	void TextAdd(unsigned long valueInt) const;
	void TextAdd(float valueSng) const;
	void TextAdd(double valueDbl) const;
	void TextAdd(long double valueDbl) const;
	void TextAdd(tstring valueString) const;

	// 获得窗口文本，返回字符串
	TCHAR * Text() const;
	// 获得窗口文本转换为的 double 型数
	double TextVal() const; 


	// ================================================
	// 字体
	// ================================================
	
	// 返回和设置 窗口字体的字体名称字符串
	// 字符串缓冲区自动开辟、由 HM 自动管理
	LPTSTR FontName();
	void   FontNameSet(LPCTSTR szFontName);
	void   FontNameSet(tstring stringFontName);

	// 返回和设置 窗口字体的大小
	float FontSize();
	void  FontSizeSet(float fSize);

	// 返回和设置 窗口字体是否加粗
	bool FontBold();
	void FontBoldSet(bool value);

	// 返回和设置 窗口字体是否加下划线
	bool FontUnderline();
	void FontUnderlineSet(bool value);

	// 返回和设置 窗口字体是否倾斜
	bool FontItalic();
	void FontItalicSet(bool value);

	// 返回和设置 窗口字体的旋转角度（单位：1/10 度）
	float FontRotateDegree();
	void  FontRotateDegreeSet(float fDegree);

	// 返回和设置 窗口字体的字符集
	BYTE FontCharSet();
	void FontCharSetSet(BYTE ucValue);

	// 获得当前所用字体下的文字宽度、高度
	// 获得高度时，调用 TextHeight() 可省略字符串 sText
	int TextWidth(LPCTSTR sText);
	int TextHeight(LPCTSTR sText=NULL);

	// 字体颜色用 ForeColor() 和 ForeColorSet()
	


	// ================================================
	// 边框
	// ================================================

	// 返回和设置窗口是否有边框
	bool Border();
	void BorderSet(bool bValue);
	
	// 返回和设置 是否是对话框类型的边框
	// A border of a style typically used with dialog boxes. 
	//   A window with this style cannot have a title bar.
	bool BorderFrameDlg();
	void BorderFrameDlgSet(bool bValue);
	
	// 返回和设置 边框可以被拖动以改变窗口大小
	// A sizing border. Same as the WS_SIZEBOX style.
	bool BorderFrameThick();
	void BorderFrameThickSet(bool bValue);

	// 返回和设置 是否为有凸起感的边框
	// A border with a raised edge.
	bool BorderRaised();
	void BorderRaisedSet(bool bValue);

	// 返回和设置 是否为有凹陷感的边框
	// Specifies that a window has a 3D look 
	//   — that is, a border with a sunken edge.
	bool BorderSunken();
	void BorderSunkenSet(bool bValue);

	// 返回和设置 是否为有蚀刻的边框
	bool BorderEtched();
	void BorderEtchedSet(bool bValue);

	// 返回和设置 是否为静态边框
	// Creates a window with a three-dimensional border style
	//   intended to be used for items that do not accept user input.
	bool BorderStatic();
	void BorderStaticSet(bool bValue);

	// 返回和设置窗口是否有标题栏
	bool BorderTitleBar();
	void BorderTitleBarSet(bool bValue);
	
	// 返回和设置 是否为浮动工具窗口样式（窄标题栏）
	// A tool window, which is a window intended to be 
	//   used as a floating toolbar. A tool window has a title bar 
	//   that is shorter than a normal title bar, and the window 
	//   title is drawn using a smaller font. A tool window does not 
	//   appear in the task bar or in the window that appears when 
	//   the user presses ALT+TAB.
	bool BorderToolWindow();
	void BorderToolWindowSet(bool bValue);



	// ======== 绘图 ========

	// 设置位图图片（有重载版本）
	// 设置为位图时，为支持缩放，是在 WM_PAINT 中绘制的，不是关联位图句柄
	// 支持 窗体(窗体仅支持位图)、Picture 控件、Button 控件、单选框、复选框
	void PictureSet(UINT bmpResID);
	void PictureSet(LPCTSTR bmpFile);
	
	// 直接以句柄设置图片，
	// picType 为图片类型：位图、图标、光标、enh图元
	// 设置为位图时，为支持缩放，是在 WM_PAINT 中绘制的，不是关联位图句柄
	// 支持 窗体(窗体仅支持位图)、Picture 控件、Button 控件、单选框、复选框
	void PictureSet(HANDLE hPic, EImgObjType picType);	
	
	// 设置截取位图的一部分显示到 窗体或控件 上，否则位图的全部内容都将显示到 窗体或控件 上
	// 所有值均可为 -1，xClip、yClip 为 -1 时将自动从图片左上角开始截取
	//    widthClip、heightClip 为 -1 时将自动截取到图片的右、下边界位置
	// 仅支持设置了的图片类型为位图
	void PictureClipSet(int xClip=-1, int yClip=-1, int widthClip=-1, int heightClip=-1);
	
	
	// 显示图片时使用：是否自动缩放位图图片以适应控件大小
	bool Stretch();
	void StretchSet( bool stretchVal );


	// PictureSetIcon 可加载图标，也可加载动画光标
	//   typeRes 不为0时，指定自定义资源的类型（为0表示Icon类型）
	// 支持 Picture 控件、Button 控件、单选框、复选框（非Picture控件动画光标不能运动）
	// ==== 加载动画光标的两种方法：=====
	// ① Form1.Control(ID_pic1).PictureSetIcon(TEXT("APPLE.ANI")); 
	//	// 加载动画光标文件
	// ② Form1.Control(ID_pic1).PictureSetIcon(IDR_MYANI1, TEXT("MYANI"));   
	//	// 从资源文件中加载 "MYANI" 类型的一个资源（ani文件事先被导入为该资源）
	void PictureSetIcon(EStandardIcon iconStd);
	void PictureSetIcon(UINT iconResID, LPCTSTR typeRes=0);
	void PictureSetIcon(LPCTSTR iconFile);
	
	



	// 输出文本
	// x、y 缺省从 (0,0) 位置开始
	// 如设置了 x，则自动换行后的下一行也将从 x 起始；但若文本中有 \n 换行则下一行从 0 起始
	// fLineFeed 输出后是否自动换行，即将下次输出位置自动移动到该文本的下一行开头
	//   如 fLineFeed=false 则将下次输出位置自动移动到紧贴该文本的最后
	// fBkTransparent: 文字背景是否透明
	// fSingleLine: 是否只输出单行文本（如为 false 不支持垂直对齐）
	bool PrintText(LPCTSTR sText, int x=-65536, int y=-65536, 
		bool fLineFeed=true, bool fBkTransparent=true, bool fSingleLine=true);
	bool PrintText(tstring stringText, int x=-65536, int y=-65536, 
		bool fLineFeed=true, bool fBkTransparent=true, bool fSingleLine=true);
	

	// 设置输出文本格式，设置后再用 PrintText 输出文本
	// clipX、clipY、clipWidth、clipHeight 设置文本输出范围
	//   clipX、clipY 缺省默认为(0,0)，clipWidth、clipHeight 缺省默认为到客户区边界
	//   如设置了 clipX、clipY，在 PrintText 时缺省 x、y 参数即可
	// align: 在输出范围中的水平对齐方式
	// valign: 在输出范围中的垂直对齐方式
	// fEndEllipsis: 字符串是否可被压缩，末尾添加 ...
	// fPathEllipsis: 文件路径字符串是否可被压缩，中间变成 ...
	// fANDPrefix: 是否将 & 变成下划线
	// iTabSpace: Tab 制表符间距，为 0 时不扩展 Tab 符
	// bEditControlStyle: 像多行文本框控件那样显示文本：平均字符宽度的计算与文本框相同，
	//   如最后一行部分可见则不显示最后一行
	bool PrintTextFormat(int clipX=0, int clipY=0, int clipWidth=-1, int clipHeight=-1, 
		EAlign align=eAlignLeft, EVAlign valign=eVAlignVCenter,
		bool fEndEllipsis=false, bool fPathEllipsis=false, bool fANDPrefix=true, 
		bool bEditControlStyle=false, int iTabSpace=8);


	// 用背景纯色填充
	void Cls();

	// 刷新位图的显示（在用 PictureSet 设置位图后有效）
	// xEdge、yEdge 为水平、垂直方向图片四周留出的空白
	//   为 0 时图片在 窗口或控件 的整个客户区绘制
	// 如该窗体或控件被设置了图片、绘制图片返回 true
	//   如该窗体或控件没有被设置图片，返回 false
	// 不需用户调用，用户只要 PictureSet 即可
	bool RefreshPicture(HDC hDC, int xEdge=0, int yEdge=0);	



	// ================================================
	// 高级
	// ================================================

	// 返回和设置控件是否透明
	bool Transparent();
	void TransparentSet(bool bTransparent);
	
	// 返回和设置 窗体半透明度：0-255，0为完全透明，255为不透明；
	//							设置为负数取消此效果
	//            窗口未被设置此样式返回-1，系统不支持返回 -2
	// win2000以上可设置；winXP以上可返回
	int Opacity();
	void OpacitySet(int iOpacity);

	// 返回和设置 窗体“挖空”的颜色：设置为 0xffffffff（或-1） 取消此效果
	// 返回 0xffffffff（或-1） 表无此效果
	// win2000以上可设置；winXP以上可返回
	COLORREF TransparencyKey();
	void TransparencyKeySet(COLORREF iTransColor);


	// 是否拖动窗体任意位置，或拖动控件对其所在窗体
	//   都具有标题栏行为（=0不具有；=1 左键拖动将移动窗体）
	int TitleBarBehavior();
	void TitleBarBehaviorSet(int iBehav=1);


	// 获得窗口的类名（不能通过指针改变类名字符串的内容）
	const TCHAR * ClassName() const;	
	
	// 判断窗口的类是否是一种类名
	bool IsClassName(LPCTSTR lpTestClassName) const;
	bool IsClassName(tstring stringTestClassName) const;

	// 返回或设置窗口风格；
	// 设置时：若 bOr > 0， 则在现有风格上增加
	//         若 bOr < 0，则在现有风格上取消 newStyle
	//         若 bOr == 0，则将现有风格改为 newStyle
	unsigned long Style();
	void StyleSet(unsigned long newStyle, int bOr=1);
	
	// 返回或设置窗口的扩展风格；
	// 设置时：若 bOr > 0， 则在现有扩展风格上增加
	//         若 bOr < 0，则在现有扩展风格上取消 newStyleEx
	//         若 bOr == 0，则将现有扩展风格改为 newStyleEx
	unsigned long StyleEx();
	void StyleExSet(unsigned long newStyleEx, int bOr=1);





	// ================================================
	// 方法
	// ================================================

	// 刷新窗口显示
	void Refresh();
	
	// 设置焦点到本窗口
	void SetFocus();
	
	// 设置窗口或控件的 Z-顺序，即是覆盖其他窗口控件，还是被其他窗口控件所覆盖
	// position=0，则位于其他窗口控件的最前面；否则 位于最后面
	void ZOrder(int position=0);

	
	// 剪切复制粘贴
	void Cut();
	void Copy();
	void Paste();


	// 成员变量清零
	// 清零不能仅在构造函数中进行，因用 CBControl 的一个对象的
	//   SetResID 先后绑定不同控件时，不调用构造函数但要清零成员
	void ClearWndBase();
	

protected:	// 将变为继承类的保护成员
	HWND m_hWnd;			// 窗口句柄

	TCHAR m_ClassName[128];	// 窗口控件的类名
	long m_atom;			// 窗口控件类的 atom 号，唯一标识一种类，使用基类 BWndBase 的成员
							// CBControl 类继承时，在 SetResID 后即自动用 GetClassName 设置
							// CBForm 类继承时，在 CBForm_DlgProc 的 WM_INITDIALOG 中设置

	STWndProp * m_pPro;		// 窗体或控件附件属性结构体变量的地址
							// 本类的对象不直接保存属性数据，因 CBControl 的对象（带本类作为基类的成员）
							//   是动态创建、动态销毁的。一个控件属性数据保存在一个 STWndProp 空间中，
							//   其地址都由 ms_hashCtrls.ItemLong2 保存。每次创建一个 CBControl 的对象
							//   都从 hWnd 在 ms_hashCtrls 找到属性数据的地址，由本成员指向它
							// 对 CBForm 的对象，不是动态创建的，窗体属性由 CBForm 对象的成员 m_WndProp
							//   直接保存，但也用本成员指向它，以和控件统一处理

protected:
	
	// 获得一个 窗体或控件 的附加属性 STWndProp 空间的地址
	// 对窗体应该返回窗体对象的 m_WndProp 的地址
	// 对控件：若还没有附加属性空间，当 bCreate=false 时返回 NULL，
	//   当 bCreate=true 时新开辟附加属性的空间，返回 新空间地址
	// 对控件：若已有附加属性空间，不会重新开辟新空间，而直接返回原来
	//   空间的地址
	// 对控件：本函数用于从一个 CBControl 对象本身直接获得该对象对应控件的属性空间
	//   如没有 CBControl 对象，而只有控件的 hWnd，应调用 PropertyMemCtrl() 
	//   静态函数获得控件的属性空间
	// 原理是管理 ms_hashCtrls.ItemLong2
	STWndProp * PropMem(bool bCreate=false);

	HFONT GetFontMLF(LOGFONT * lpLf=NULL);	// 将窗口当前所用字体信息获取到 lpLf 所指向的结构，
											//   并返回目前所用字体对象句柄
											// 若 lpLf 为空指针，直接返回字体句柄，不获取字体信息
													
	HFONT SetFontMLF(LOGFONT * lpLf);		// 根据 lpLf 所指向的结构创建一种字体，并设置窗口使用该种字体；
											//   函数返回新字体对象的句柄
	
};
