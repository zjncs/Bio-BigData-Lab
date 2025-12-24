//////////////////////////////////////////////////////////////////////
// BForm.cpp: 一些全局函数（如 DoEvents 函数） 和 WinMain 函数的定义，
//            CBForm 类的实现
//   
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "BForm.h"
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")	// 使工程引入 comctl32.lib 库


//////////////////////////////////////////////////////////////////////////
// 全局函数 和 WinMain 函数
//////////////////////////////////////////////////////////////////////////

  
// 封装 WinMain 函数，用户仍可将 main() 作为程序的入口
int main();
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char * lpCmdLine, int nShowCmd )
{
	// 初始化通用控件库
	InitCommonControls();  // 如果任意公有函数中，调用 SHFileOperation 似乎也能达到同样的效果（执行SHFileOperation(0);即可）

	// 初始化 rich edit
	HMODULE hRichDll = LoadLibrary(TEXT("Msftedit.dll"));		// rich edit 5.0  请修改资源rc文件的对应控件类名为 RICHEDIT50W
	//HMODULE hRichDll = LoadLibrary(TEXT("riched20.dll"));		// rich edit 2.0, 3.0 

	// 定义 CBApp 对象，将有关程序信息保存其中。该对象虽为 WinMain 的局部变量
	//   但程序运行全程都是存在的，因为 WinMain 函数结束，程序就结束了
	// 使全局指针变量 pApp 指向 app，则全局程序中可使用 (*pApp) 访问此变量
	CBApp app(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	pApp=&app;
	
	// 调用 main 函数，main 函数由用户在自己的模块中自己定义。这样程序的
	//   入口仍是 main 函数
	int retMain=main();

	// 进入本线程的消息循环，这是在用户 main() 函数执行结束后的事
	// 注意用户 main() 函数结束，整个程序并未结束
	MessageLoop(0);		// 参数 0 表示主消息循环，GetMessage 收到 0 才结束

	// 卸载 rich edit
	FreeLibrary(hRichDll); hRichDll=0;

	// WinMain 返回，返回值为用户 main() 函数的返回值
	return retMain;
}


// 消息循环函数
// bExitStyle=0时：为主消息循环，无消息、且无窗体时便 return
// bExitStyle>1时：实际是 +1 后的 ms_iModalLevel 的值
//   用于模态对话框显示后的消息循环（每显示一层模态对话框新进一层
//   新的 MessageLoop。若 ms_iModalLevel<iExitStyle，或无窗体时 return
// bExitStyle=-1时：用于 DoEvents()，
//   当前线程无消息（PeekMessage 返回0）时就 return
// 无论 bExitStyle 为多少，在 GetMessage 收到 0 都会 return 
//   并在 return 前再次 PostQuitMessage(0); 以将退出消息传播
//   到前面各层的 MessageLoop 使前面各层的 MessageLoop 都能退出
static void MessageLoop(int iExitStyle/*=0*/)
{
	// 进入本线程的消息循环：将获得本线程中的所有消息，然后将消息
	//   派发到它所属的各自的窗口过程中
	// 显示模态窗体时，再次调用本函数，并设置 iExitStyle 为 ms_iModalLevel（>0）
	//   当模态窗体隐藏或卸载时，退出新进入调用的本函数，返回上层调用的本函数 
	//   最低一层是 WinMain 所调用的本函数

	MSG msg;
	int iret=-1;		// 不能初始化为0，因 while 后有条件判断，iret==0 表示收到 WM_QUIT 消息

	while (1) 
	{
		if (iExitStyle>0)		// 用于模态窗体：ms_iModalLevel<iExitStyle，或无窗体时 return
		{
			if ( CBForm::ms_iModalLevel<iExitStyle ||  
				CBForm::ms_hashWnd.Count()==0 ) break;
		}
		else if (iExitStyle<0)	// 用于 DoEvents：当前线程无消息就 return
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)==0) break;
		}
		else					// 主消息循环：无消息且无窗体时便 return
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)==0  &&  
				CBForm::ms_hashWnd.Count()==0 ) break;
		}
		
		iret=GetMessage(&msg, NULL, 0, 0);	// 获得本线程所有窗口的消息

		// GetMessage出错会返回 -1，故 
		// while (GetMessage( lpMsg, hWnd, 0, 0)) ... 是不可取的
		// 返回 0 表示本线程收到退出信号 WM_QUIT 消息，退出消息循环
		if (iret==-1 || iret==0) break;	

		// 处理加速键
		// 用 TranslateAccelerator 转换和处理对应消息（如果有加速键，加速键句柄为 iret）
		// 如已转换处理，不要再 IsDialogMessage、TranslateMessage、DispatchMessage
		// 不能转换为加速键的消息 TranslateAccelerator 不处理，继续
		if (CBForm::ms_hAccelCurrUsed && CBForm::ms_hWndActiForm) 
		{
			if (TranslateAccelerator(CBForm::ms_hWndActiForm, CBForm::ms_hAccelCurrUsed, &msg)) continue;
		}
		
		// 处理对话框消息（如按 Tab 跳转控件焦点等）
		// 用 IsDialogMessage 转换和处理对应消息
		// 如该条消息已被 IsDialogMessage 处理，不要再 TranslateMessage、DispatchMessage
		// 非对话框的消息 IsDialogMessage 不处理，再 TranslateMessage、DispatchMessage
		
		if (msg.message == WM_KEYDOWN && (msg.wParam==13 || msg.wParam==27))
		{
			// 按下回车 或 按下 ESC 不交由 IsDialogMessage 处理
			//   否则 按下回车 或 按下 ESC 会被 IsDialogMessage 转换为 WM_COMMAND 消息
			//   （wParam==1 和 2），使窗体和其他控件不能正常接收 回车、ESC
			// 消息转换和派发
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (!IsDialogMessage(CBForm::ms_hWndActiForm, &msg))	
			{
				// 消息转换和派发
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}	// end of while (1)

	// 如果是由于收到程序退出消息 而退出的 while 循环（iret==0）
	//   再次 PostQuitMessage(0); 以将退出消息传播
	//   到前面各层的 MessageLoop 使前面各层的 MessageLoop 都能退出
	if (iret==0) PostQuitMessage(0);
}


void DoEvents()
{
	MessageLoop(-1);
}


extern void End( int nExitCode/*=0*/ )
{
	PostQuitMessage(nExitCode);
}



//////////////////////////////////////////////////////////////////////////
// CBForm 类的实现
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// 定义 CBForm 类的 友元函数


// 公用窗口过程：所有本类对象（窗体）都用此函数作为窗口过程
// CBForm 类的友元函数
static BOOL WINAPI CBForm_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// =======================================================================
	// 本函数中不得 PostQuitMessage
	// 因为在一个对话框中 PostQuitMessage 后，整个程序就都关闭了，而不是只有
	//   这一个对话框被关闭
	// =======================================================================
	switch(uMsg)
	{
	case WM_INITDIALOG:
		// ===================================================================
		// lparam 应为一个对象的地址，将此地址与 hwnd 关联的关系存入哈希表
		if (lParam)
		{
			// =================================================================
			// ============ 有新窗体创建：将其信息记录到 ms_hashWnd ============
			// =================================================================

			// 容错：若哈希表中已存在键为 hwnd 的项目，则先删除它，再用新的内容覆盖
			if (CBForm::ms_hashWnd.IsKeyExist((long)hWnd)) 
				CBForm::ms_hashWnd.Remove((long)hWnd,false); 
			
			// 向哈希表中添加新项（Key=hwnd，Data=对象地址，
			//   ItemLong=加速键句柄，ItemLong2=受模态对话框影响的 Enabled 状态）
			CBForm::ms_hashWnd.Add(lParam, (long)hWnd, 0, 0);
			
			// 设置对象中的 m_hWnd 成员为 窗口句柄
			((CBForm *)lParam)->m_hWnd = hWnd;

			// 设置对象中的 类名成员
			memset( ((CBForm *)lParam)->m_ClassName, 0, sizeof( ((CBForm *)lParam)->m_ClassName));
			((CBForm *)lParam)->m_atom = GetClassLong(hWnd, GCW_ATOM);
			GetClassName(hWnd, ((CBForm *)lParam)->m_ClassName, 
				sizeof( ((CBForm *)lParam)->m_ClassName ) / sizeof(TCHAR)-1 );
			
			// 将所有子窗口控件子类处理：参数 lParam 设为本窗体的句柄 m_hWnd
			EnumChildWindows( hWnd, EnumChildProcSubClass, (LPARAM)hWnd );

			// 触发 Form_Load 事件
			((CBForm *)lParam)->EventsGenerator(WM_INITDIALOG, wParam, lParam);
		}

		return 1; // 返回 True, Windows 会自动将输入焦点放到第一个有 WS_TABSTOP 的控件上

		break;

	default:
		
		// ===================================================================
		// 调用各自对象的 EventsGenerator，后者处理这些消息，必要时生成事件
		//   本函数返回 EventsGenerator 的返回值
		// ===================================================================

		CBForm *pForm;
		pForm=0;
		if (CBForm::ms_hashWnd.IsKeyExist((long)hWnd))
		{
			pForm=(CBForm *)CBForm::ms_hashWnd.Item((long)hWnd, false);
			if (pForm) 
			{ 
				return pForm->EventsGenerator(uMsg, wParam, lParam);
			}
		}
	}

	return 0;
}


// 公用窗口过程：所有本类对象（窗体）中的子窗口控件，都用此函数作为窗口过程，并所有子窗口控件都被子类处理
// CBForm 类的友元函数
static int CALLBACK CBForm_ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	CBForm *pForm = 0; 
	long r; 

	// 所有消息送到 对应父窗体的.EventsGeneratorCtrl
	if (CBWndBase::ms_hashCtrls.IsKeyExist((long)hWnd))
	{
		// ItemLong = 所位于窗体的 hWnd，可以此为 key 到 ms_hashWnd 中获得窗体
		//   CBForm 对象的地址
		long lHWndForm=CBWndBase::ms_hashCtrls.ItemLong((long)hWnd, false);
		pForm = (CBForm *)CBForm::ms_hashWnd.Item(lHWndForm, false);
		if (pForm) 
		{ 
			r = pForm->EventsGeneratorCtrl(hWnd, uMsg, wParam, lParam);
			if (r != gc_APICEventsGenDefautRet) return r;
		}
	}

	// 从 CBForm::ms_hashCtrls 中获得本窗口的默认窗口程序的地址，并调用默认窗口程序
	return CallWindowProc ((WNDPROC)(CBWndBase::ms_hashCtrls.Item((long)hWnd,false)), 
		hWnd, uMsg, wParam, lParam);
}


// 枚举子窗口控件、子类处理所有子窗口控件的回调函数：
// lParam 为控件所属窗体的句柄时（非0），表示加载窗体时的子类处理；
// lParam 为 0 时，表示卸载窗体时的恢复子类处理
// CBForm 类的友元函数
static BOOL CALLBACK EnumChildProcSubClass(HWND hWnd, LPARAM lParam)
{
	if (lParam)
	{
		// ======== 窗体加载时（WM_INITDIALOG）调用的 ========
		// 处理本窗口的子窗口
		EnumChildWindows(hWnd, EnumChildProcSubClass, lParam);

		// 设置所有子窗口具有 WS_CLIPSIBLINGS 风格
		// 否则控件彼此覆盖时，后方控件重绘时会擦掉前方控件
		SetWindowLong( hWnd, GWL_STYLE,
			GetWindowLong(hWnd, GWL_STYLE) | WS_CLIPSIBLINGS ) ;

		// 不同类型的控件分类调整
		TCHAR strClassName[128];
		GetClassName(hWnd, strClassName, sizeof(strClassName)/sizeof(TCHAR)-1);
		
		if (_tcscmp(strClassName, TEXT("Static"))==0)
		{
			// 如果是 Static 控件，都添加 SS_NOTIFY 风格，否则不但某些事件不能响应
			//   控件也不能响应 Mouse_Move、MouseDown 等
			SetWindowLong( hWnd, GWL_STYLE,
				GetWindowLong(hWnd, GWL_STYLE) | SS_NOTIFY ) ;
		}
		else if (_tcscmp(strClassName, TEXT("ComboBox"))==0)
		{
			// 修正 ComboBox 若高度太小不能拉下下拉框的问题
			// 获得类名字符串 => strClassName 
			// (HWND)lParam 是父窗体句柄
			CBControl ctrl(hWnd);		// 借 CBControl 对象修改 ComboBox 高度；lParam 为父窗体句柄
			int heightReq = SendMessage(hWnd, CB_GETITEMHEIGHT, 0, 0) *10; 
			if ( ctrl.Height() < heightReq ) ctrl.HeightSet(heightReq);
		}
		else if (_tcsstr(strClassName, TEXT("RICHEDIT")))
		{
			// 设置 Rich Edit 允许最大文本长度 &H7FFFFFFE、允许所有通知消息
			SendMessage(hWnd, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);		// 设置允许的文本长度为 &H7FFFFFFE
			SendMessage(hWnd, EM_SETEVENTMASK, 0, 0xFFFFFFFF);		// 设置所有通知消息有效
		}

		// 将所有子窗口控件子类处理（Control_Proc），并将它们的原窗口程序地址
		//   存入 CBForm 类的静态成员 哈希表 ms_hashCtrls
		// 将 lParam （即父窗体的句柄）存入哈希表元素的 ItemLong
		if (! CBWndBase::ms_hashCtrls.IsKeyExist((long)hWnd))
		{
			// ItemLong2 设为0，表示没有设置过附加属性；只有设置附加属性时
			//   再动态开辟 STWndProp 的空间，ItemLong2 才为空间地址（不为0）
			CBWndBase::ms_hashCtrls.Add((long)GetWindowLong(hWnd,GWL_WNDPROC), (long)hWnd,
				(long)lParam, 0, 0, 0, 0, false);
			SetWindowLong(hWnd, GWL_WNDPROC, (long)CBForm_ControlProc);
		}

		
		// 将所有子窗口控件的 ID 及句柄、父窗口句柄、窗体句柄对应关系
		// 存入哈希表 ms_hashCtrlResIDs
		// Key = 控件资源ID；Item = 控件本身的 hWnd
		// ItemLong = 直接所属父窗口的 hWnd；ItemLong2 = 所位于窗体的 hWnd
		int idCtrl = GetDlgCtrlID(hWnd);
		if (idCtrl  &&  ! CBWndBase::ms_hashCtrlResIDs.IsKeyExist(idCtrl))
			CBWndBase::ms_hashCtrlResIDs.Add((long)hWnd, idCtrl, 
			    (long)GetParent(hWnd), (long)lParam, 0, 0, 0.0, false);
		

		// 返回非0值，以继续枚举同层其他子窗口控件
		return 1;
	}
	else  // if (lParam) else： (lParam == 0)
	{
		// ======== 窗体卸载时（WM_DESTROY）调用的 ========
		// 清理所有子窗口控件的相关信息
		//   即 CBForm 类的静态成员 哈希表 CBWndBase::ms_hashCtrls 中的相关项目

		if ( CBWndBase::ms_hashCtrls.IsKeyExist((long)hWnd) )
		{
			// 删除附加属性的空间（如果设置过附加属性的话）
			STWndProp * pPro = CBWndBase::PropertyMemCtrl(hWnd, false); 
			if (pPro)
			{
				// 释放附加属性中的相关资源
				if (pPro->hBrushBack) 
				{ DeleteObject(pPro->hBrushBack); pPro->hBrushBack = NULL; }
				
				// 如原先光标 属于资源光标，删除原先的光标
				// 不必删除光标对象，系统会自动删除
				// pPro->hCursor
				// if (pPro->cursorIdx > 0 && pPro->cursorIdx < gc_IDStandCursorIDBase) 
				//	DestroyCursor(pPro->hCursor);
				pPro->hCursor = NULL; pPro->hCursor2 = NULL;
				pPro->cursorIdx = 0;  pPro->cursorIdx2 = 0;

				
				if (pPro->hFont) 
				{ DeleteObject(pPro->hFont); pPro->hFont = NULL; }
				
				if (pPro->hBmpDisp)
				{  DeleteObject(pPro->hBmpDisp); pPro->hBmpDisp = NULL; }

				// 将 rcPicture 的各成员设为 -1 表示不截取图片的一部分显示，而是要显示图片的全部
				SetRect(&pPro->rcPictureClip, -1, -1, -1, -1);

				if (pPro->tagString)	// pPro->tagString 未由 HM 管理
				{ delete [](pPro->tagString); pPro->tagString = NULL; 	}

				// 删除附加属性的空间
				delete pPro;
			}	// end if (pPro)

			// 恢复子类处理（原窗口程序地址位于 ms_hashCtrls.Item 中）
			SetWindowLong(hWnd, GWL_WNDPROC, 
				CBWndBase::ms_hashCtrls.Item((long)hWnd, false));
			
			// 删除 ms_hashCtrls 和 ms_hashCtrlResIDs 中的对应项目
			CBWndBase::ms_hashCtrls.Remove((long)hWnd, false);
			if (CBWndBase::ms_hashCtrlResIDs.Item((long)GetDlgCtrlID(hWnd),false) == (long)hWnd)
				CBWndBase::ms_hashCtrlResIDs.Remove((long)GetDlgCtrlID(hWnd), false);
		}	// end if ( CBWndBase::ms_hashCtrls.IsKeyExist((long)hWnd) )

		
		// 处理本窗口的子窗口
		EnumChildWindows(hWnd, EnumChildProcSubClass, lParam);
		
		// 返回非0值，以继续枚举同层其他子窗口控件
		return 1;
	}   // end if (lParam)-else  // (lParam == 0)
}


//////////////////////////////////////////////////////////////////////////
// 定义类中的 static 成员和 static 函数

CBHashLK CBForm::ms_hashWnd;


// 模态对话框 的层次
int CBForm::ms_iModalLevel = 0;


// 加速键句柄和加速键要发送到的目标窗口
HACCEL CBForm::ms_hAccelCurrUsed=NULL;  
HWND CBForm::ms_hWndActiForm=NULL;


int CBForm::FormsCount()
{
	return ms_hashWnd.Count();
}

CBForm * CBForm::FormsObj( int index )
{
	return (CBForm *)ms_hashWnd.ItemFromIndex(index, false);
}



//////////////////////////////////////////////////////////////////////////
// 构造和析构

// 构造函数
CBForm::CBForm( unsigned short int idResDialog /*=0*/):CBWndBase(NULL, &m_WndProp)
{
	mResDlgID = idResDialog;

	// 当前没有系统托盘
	m_NIData.cbSize = 0;

	// 初始化附加属性
	m_WndProp.cursorIdx = 0;		// 窗体当前的鼠标光标索引号，0 表示使用系统默认；
	m_WndProp.cursorIdx2 = 0;		// .cursorIdx 是副本
	m_WndProp.hCursor = NULL;		// 窗体当前的鼠标光标句柄（在 cursorIdx 非 0 时才有效）
	m_WndProp.hCursor2 = NULL;		// .hCursor 的副本

	m_WndProp.backColor = -1;		// 窗口背景色，-1 表示使用默认色（不修改窗口类，而是通过响应 WM_ERASEBKGND 实现）
	m_WndProp.hBrushBack = NULL;	// 窗口背景填充的画刷

	m_WndProp.foreColor = -1;		// 窗体或控件 前景色，-1 表示使用默认色
	m_WndProp.bBackStyleTransparent = false;		// 控件背景是否透明（仅对Static等某些控件有效）
	m_WndProp.hFont = NULL;			// 所用字体（为0表示用系统字体；否则表示用自定义字体，最后需 DeleteObject ）
	SetRect(&m_WndProp.rcTextOutClip, 0, 0, 0, 0);	// PrintText 输出文本时的输出位置（.left、.top）和输出范围
	m_WndProp.iTextOutStartLeft = 0;// 输出文本的范围的原始左边界，供 PrintText 换行时确定新行左边界
	m_WndProp.iTextOutFlags = 0;	// 输出文本的选项

	m_WndProp.stretch = false;		// 窗体被设置图片时使用，是否自动拉伸图片大小以适应控件
	m_WndProp.hBmpDisp=NULL;		// （不作为属性使用）要显示的位图图片。stretch=true 时，此为原始未缩放的位图句柄
	SetRect(&m_WndProp.rcPictureClip, -1, -1, -1, -1);	// 设为-1 表示不截取图片的一部分显示
	m_WndProp.iTitleBarBehav = 0;	// 是否具有标题栏行为（=0不具有；=1 左键拖动将移动窗体）
	
	m_WndProp.fDisableContextMenu = false;	// 是否禁用右键菜单（即是否取消 WM_CONTEXTMENU 消息）

	m_WndProp.tagString = NULL;	// 附加字符串数据
	m_WndProp.iTag1 = 0;		// 附加整数数据1
	m_WndProp.iTag2 = 0;		// 附加整数数据2
	m_WndProp.dTag = 0.0;		// 附加 double 型数据	



	m_ModalShown = false;	// 是否正以模态对话框显示的标志

	m_EdgeWidth=0;			// >0 时表示要在 WM_PAINT 事件中自动绘制窗体边框，此为边框宽度（支持圆角矩形边框）
	m_EdgeColor=0;			// m_EdgeWidth==0时，m_EdgeColor无效。m_EdgeWidth >0 时表示要在 WM_PAINT 事件中自动绘制窗体边框的颜色（支持圆角矩形边框）

	m_iBorderAutoResizable=0;	// （对无边框窗体）是否允许处理拖动窗体四周改变窗体大小（允许则>0，值为可改变大小的边框范围）

	m_RoundRect = 0;		// 窗体非圆角矩形窗体
	m_hRgnRoundRect = NULL;	

	// 类公有成员赋初值
	KeyPreview = false;		// 是否优先由窗体处理所有控件的键盘按键消息
}

// 析构函数
CBForm::~CBForm()
{
	UnLoad();
	ClearResource();
}


//////////////////////////////////////////////////////////////////////////
// 公有成员函数

HWND CBForm::hWnd()
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载
	return m_hWnd;
}


HMENU CBForm::hMenu()
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载
	return GetMenu(m_hWnd);
}


void CBForm::hMenuSet( HMENU hMenuNew )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载

	SetMenu(m_hWnd, hMenuNew); 
	if (hMenuNew)
	{	
		ConstructMenuIDItems(GetMenu(m_hWnd), true);
		ConstructMenuIDItems(GetSystemMenu(m_hWnd, 0), false);	// 系统菜单
	}
	else
	{
		ConstructMenuIDItems(NULL, true);
		ConstructMenuIDItems(GetSystemMenu(m_hWnd, 0), false);	// 系统菜单
	}
}


HACCEL CBForm::hAccel()
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载
	return (HACCEL)ms_hashWnd.ItemLong((long)m_hWnd, false);
}


void CBForm::hAccelSet( HACCEL hAccelNew )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载，然后才能设置加速键
	
	ms_hashWnd.ItemLongSet((long)m_hWnd, (long)hAccelNew, false);	// 记录到 ms_hashWnd 的 ItemLong
	// 如果现在前台窗体正是本对象的窗体，就设置 CBForm::hWndAccel、CBForm::hAccel
	if (m_hWnd==GetActiveWindow())
	{
		CBForm::ms_hWndActiForm = m_hWnd; 
		CBForm::ms_hAccelCurrUsed = hAccelNew; 
	}
}


bool CBForm::Show( int modal/*=0*/, HWND hwndOwner/*=NULL*/ )
{
	// 由于模态方式显示窗体，不能自写消息循环，导致不能处理加速键
	//   在本类中，所有窗体以非模态方式显示
	// 对于“模态”的实现，是本函数通过将所有顶层窗体全部 
	//   Enabled=False 实现的

	HWND hwnd;
	HWND hWndActiLast = GetActiveWindow(); 

	if (0==m_hWnd) 
	{	
		// 若尚未加载对话框，现在加载
		// 所有窗体以非模态方式 CreateDialogParam 加载
		hwnd=Load(hwndOwner);   
		if (hwnd==0) return false;
	}
	else 
	{	
		// 对话框已经加载，在显示时可能会改变其 Owner
		// 通过以下语句改变窗体的 Owner （不是Parent）
		//   MSDN 并没有介绍改变 Owner 的方法，应该是用  
		//   SetWindowLong 以 -8 设置 Owner
		if (hwndOwner !=0 && hwndOwner != GetWindow(m_hWnd, GW_OWNER)) 
			SetWindowLong(m_hWnd, -8, (long)hwndOwner); 
	}

	if (modal)
	{
		// ======== 显示模态对话框 ========

		// 在显示一个模态对话框时：
		//   若其他某窗口目前为 Disabled 且 ms_hashWnd.ItemLong2 值为0，则维持该窗口的此值不变；
		//   否则，其他某窗口目前为 Enabled，或者该窗口此值 >0，都会将该窗口的此值 +1
		// 在隐藏一个模态对话框时：
		//   如某窗口此值为0，则不做任何操作；否则将此值-1，如-1后为0，则恢复为 Enabled 状态

		int i; 
		BOOL ena; 
		long level=0; 
		HWND hwndEach=0;
		for (i=1; i<=ms_hashWnd.Count(); i++)
		{
			hwndEach=(HWND)ms_hashWnd.IndexToKey(i);
			if (hwndEach==m_hWnd) 
			{
				// 本模态窗体，不做；但设置 ItemLong2 为 0
				ms_hashWnd.ItemLong2FromIndexSet(i, (long)0, false);
				continue;	
			}

			
			// 根据窗体 i 的当前 enabled 状态和 ItemLong2（level） 做出判断
			ena = IsWindowEnabled(hwndEach);
			level = ms_hashWnd.ItemLong2FromIndex(i, false); 
			if (ena || level>0)	
			{	
				// 记录将来要把本窗口恢复到Enabled
				ms_hashWnd.ItemLong2FromIndexSet(i, level+1, false);	
			}
			// else 若窗口原为 Disabled，且  ItemLong2（level） 为0
			//   则仍维持此值为0（不变）

			// 将窗体 i 设置为 Diabled
			EnableWindow(hwndEach, 0);
		}


		// 设置本对象的显示模态标志
		m_ModalShown = true;
	
		// 模态层次 ++
		if (ms_iModalLevel<0) ms_iModalLevel=0;		// 容错
		ms_iModalLevel++;

		// 显示“模态”窗体
		ShowWindow(m_hWnd, SW_NORMAL);
		
		// 本函数不要立即返回
		// 再次调用 MessageLoop，并设置参数为 1
		// 直到“模态”窗体被隐藏或关闭后下面的 MessageLoop 函数才返回
		MessageLoop(ms_iModalLevel); 
	}
	else
	{
		// ======== 显示非模态对话框 ========
		
		if ( ms_iModalLevel>0 )	
		{
			// 如果已有模态对话框正在显示，原则上不允许再显示
			//   非模态对话框。但本类允许，只要将新对话框设为 Disabled 即可
			// 新对话框的 ItemLong2 值应为 ms_iModalLevel
			ms_hashWnd.ItemLong2Set((long)m_hWnd, ms_iModalLevel, false);
			EnableWindow(m_hWnd, 0);

			// 显示新窗口（但不要将新窗口设为前台）
			ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);	
			SetActiveWindow(hWndActiLast);	// 恢复原来前台窗口仍为前台
		}
		else
		{
			ShowWindow(m_hWnd, SW_NORMAL);	
		}
		
		// 以非模态显示，设置标志
		m_ModalShown = false;
	}

	return true;
}


long CBForm::Hide()
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载
	return ShowWindow(m_hWnd, SW_HIDE);
}


// 加载对话框但并不显示出来（只能以非模态方式加载）
// 在需要对话框被加载后才能调用的一些函数如 SetAccelerator 中，可自动调用本函数
HWND CBForm::Load( HWND hwndOwner/*=NULL*/, bool fRaiseErrIfFail/*=true*/ )
{
	// 一律以“非模态”方式加载窗体：调用 CreateDialogParam
	// 将 dwInitParam 参数设为本对象地址即this，
	//   此将作为 WM_INITDIALOG 的 lParam 参数传给 CBForm_DlgProc
	HWND hwnd;
	hwnd=CreateDialogParam(pApp->hInstance, MAKEINTRESOURCE(mResDlgID), hwndOwner, 
	  CBForm_DlgProc, (long)this);	// 立即返回，返回窗口句柄；在窗口过程处理 WM_INITDIALOG 时会设置 m_hWnd

	// 如果没有 WS_VISIBLE 样式此时窗体不会自动显示
	
	if (!hwnd)
	{
		if (fRaiseErrIfFail) 
		{
			MsgBox(  StrAppend( TEXT("CBForm::Load() failed. Form ID="), Str((int)mResDlgID), 
				TEXT("\r\n"), TEXT("GetLastError="), Str(GetLastError())), 
				TEXT("Debug Error from CBForm::Load()"), mb_OK, mb_IconError);
			throw (unsigned char)5;	// 无效的过程调用或参数
		}
	}
	else
	{
		// 初始化菜单
		ConstructMenuIDItems(GetMenu(hwnd),true);	// 如无菜单即参数为0，将清除菜单哈希表记录
		ConstructMenuIDItems(GetSystemMenu(hwnd, 0), false);	// 系统菜单
	}

	return hwnd;
}



// 结束对话框
// 接收到 WM_CLOSE 消息时可自动调用本函数，也可直接由用户调用
void CBForm::UnLoad()
{
	if (m_NIData.cbSize) SysTrayRemove();	// 如有系统托盘，现在删除
	SendMessage(m_hWnd, WM_CLOSE, 0, 0);
}


// 设置和返回最小化(=1)、最大化(=2)、还原状态(=0)
int CBForm::WindowState()
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(m_hWnd, &wp);
	
	switch(wp.showCmd)
	{
	case SW_SHOWNORMAL: case SW_RESTORE: case SW_SHOW: 
	case SW_SHOWMINNOACTIVE: case SW_SHOWNA: case SW_SHOWNOACTIVATE:
		return 0;
		break;
	case SW_SHOWMINIMIZED: case SW_MINIMIZE:
		return 1;
		break;
	case SW_SHOWMAXIMIZED:
		return 2;
		break;
	case SW_HIDE:
	default:
		return -1;
		break;
	}
}

void CBForm::WindowStateSet( int iState )
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(m_hWnd, &wp);

	switch(iState)
	{
	case 0:
		wp.showCmd = SW_SHOWNORMAL;
		break;
	case 1:
		wp.showCmd = SW_SHOWMINIMIZED;
		break;
	case 2:
		wp.showCmd = SW_SHOWMAXIMIZED;
		break;
	}
	
	SetWindowPlacement(m_hWnd, &wp);
}



// 设置本窗口将处理加速键，资源 ID 为 idResAcce
// 若取消加速键，将参数设为 0 即可
void CBForm::SetAccelerator( unsigned short int idResAcce )
{
	HACCEL hAcc=NULL;
	if (idResAcce) hAcc=LoadAccelerators(pApp->hInstance, MAKEINTRESOURCE(idResAcce)); 
	hAccelSet(hAcc);	// 调用公有方法 hAccelSet 设置
}

// 重新设置本窗口的菜单
void CBForm::SetMenuMain( unsigned short idResMenu )
{
	HMENU hMenu=NULL;
	if (idResMenu)
	{
		hMenu=LoadMenu(pApp->hInstance, MAKEINTRESOURCE(idResMenu));
		hMenuSet(hMenu);	// 调用公有方法 hMenuSet 设置
	}
	else
		hMenuSet(NULL);		// 调用公有方法 hMenuSet 设置
}


// 返回一个控件（CBControl 对象），之后可对该控件进行操作（调用 CBControl 类的属性方法）
CBControl CBForm::Control( unsigned short int idResControl, bool fRaiseErrIfFail/*=true*/ )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载

	if (m_Control.SetResID(idResControl))
	{	
		return m_Control;
	}
	else
	{
		if (fRaiseErrIfFail) 
		{
			MsgBox(  StrAppend( TEXT("CBForm::Control() Failed. \r\nControl not found. ID="), Str((int)idResControl)), 
				TEXT("Debug Error from CBForm::Control()"), mb_OK, mb_IconError);
			throw (unsigned char)5;	// 无效的过程调用或参数
		}
		m_Control.SetResID( (unsigned short int)0 );
		return m_Control;
	}
}


// 返回一个菜单项（CBMenuItem 对象），之后可对该菜单项进行操作（调用 CBMenuItem 类的属性方法）
CBMenuItem CBForm::Menu( UINT idResMenuItem )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载

	// 根据 idMenuItem，在哈希表中查找其所对应的父菜单的 句柄 -> hMenu
	HMENU hMenu;
	hMenu = (HMENU)m_hashMenuIDs.Item(idResMenuItem, false);

	if (hMenu)
	{
		m_MenuItem.SetFromResID(hMenu, idResMenuItem, m_hWnd, &m_hashMenuIDs); 
		return m_MenuItem;
	}
	else
		return 0;
}

CBMenuItem CBForm::Menu( int pos1, int pos2, int pos3/*=0*/, int pos4/*=0*/, int pos5/*=0*/, int pos6/*=0*/, int pos7/*=0*/ )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载

	HMENU hMenu;

	if (pos1>0)
	{
		// 获得主菜单
		hMenu = GetMenu(m_hWnd);

		if (pos2)
		{
			hMenu = GetSubMenu(hMenu, pos1 - 1);		//-1变为从0开始编号，获得顶层菜单的句柄：例如若pos1==1，hMenuSub为整个“文件”菜单的句柄
		}
		else
		{
			m_MenuItem.SetFromPos(hMenu, pos1, m_hWnd, &m_hashMenuIDs);	// 返回顶级菜单的一个菜单项（位置参数从1开始），例如若pos1==1，返回“文件”菜单项
			return m_MenuItem;
		}
	}
	else if (pos1<0)
	{
		// 获得系统菜单
		hMenu = GetSystemMenu(m_hWnd, 0);
		if (pos2==0)
		{
			// 返回系统菜单的“父菜单项”
			//  （它引出的子菜单句柄为 GetSystemMenu，即它的子菜单的第一项是【还原】）
			m_MenuItem.SetFromPos(0, 1, m_hWnd, &m_hashMenuIDs);		// 1-1 == 0
			return m_MenuItem; 
		}
	}
	else	// pos1 == 0
	{
		// 获得顶级菜单
		hMenu = GetMenu(m_hWnd);
		if (pos2==0)
		{
			// 返回顶层菜单的“父菜单项”
			//  （它引出的子菜单句柄为 GetMenu，即它的子菜单的第一项是【文件】）
			m_MenuItem.SetFromPos(0, 2, m_hWnd, &m_hashMenuIDs);
			return m_MenuItem; 
		}
	}
	

	if (pos3)
	{
		hMenu = GetSubMenu(hMenu, pos2 - 1);		//-1变为从0开始编号
	}
	else
	{
		m_MenuItem.SetFromPos(hMenu, pos2, m_hWnd, &m_hashMenuIDs);	// 返回一级菜单的一个菜单项（位置参数从1开始），例如若pos1==1、pos2==2，返回“文件”-“打开”的菜单项
		return m_MenuItem;	
	}

	if (pos4)
	{
		hMenu = GetSubMenu(hMenu, pos3 - 1);		//-1变为从0开始编号
	}
	else
	{
		m_MenuItem.SetFromPos(hMenu, pos3, m_hWnd, &m_hashMenuIDs);	// 位置参数从1开始
		return m_MenuItem;	
	}

	if (pos5)
	{
		hMenu = GetSubMenu(hMenu, pos4 - 1);		//-1变为从0开始编号
	}
	else
	{
		m_MenuItem.SetFromPos(hMenu, pos4, m_hWnd, &m_hashMenuIDs);	// 位置参数从1开始
		return m_MenuItem;	
	}

	if (pos6)
	{
		hMenu = GetSubMenu(hMenu, pos5 - 1);		//-1变为从0开始编号
	}
	else
	{
		m_MenuItem.SetFromPos(hMenu, pos5, m_hWnd, &m_hashMenuIDs);	// 位置参数从1开始
		return m_MenuItem;	
	}

	if (pos7)
	{
		hMenu = GetSubMenu(hMenu, pos6 - 1);		//-1变为从0开始编号
	}
	else
	{
		m_MenuItem.SetFromPos(hMenu, pos6, m_hWnd, &m_hashMenuIDs);	// 位置参数从1开始
		return m_MenuItem;	
	}

	m_MenuItem.SetFromPos(hMenu, pos7, m_hWnd, &m_hashMenuIDs);		// 位置参数从1开始
	return m_MenuItem;	

}


CBMenuItem CBForm::Menu( ESysMenu idSysMenu )
{
	if (m_hWnd==NULL) Load(); // 如果窗口还未加载，现在加载
	
	return this->Menu((UINT)idSysMenu);
}

void CBForm::MenuSysRestore() const
{
	GetSystemMenu(m_hWnd, 1);
}


BOOL CBForm::PopupMenu( UINT idResMenu, int x, int y, bool bAllowRightClick/*=true*/ )
{
	HMENU hMenuPop;
	BOOL ret;
	UINT flags=0;
	POINT pt; 
	hMenuPop = LoadMenu(pApp->hInstance, MAKEINTRESOURCE(idResMenu)); 
	if (bAllowRightClick) flags = flags | TPM_RIGHTBUTTON;
	pt.x=x; pt.y=y; 
	ClientToScreen(m_hWnd, &pt);
	ret = TrackPopupMenu(GetSubMenu(hMenuPop, 0), flags, pt.x, pt.y, 0, m_hWnd, NULL);
		// TrackPopupMenu 只能弹出 popup 式菜单，LoadMenu 得到的不是 popup 式菜单，
		//   应用 GetSubMenu 取其子菜单（这里只取第0项的子菜单）
	DestroyMenu(hMenuPop);
	return ret;
}


void CBForm::IconSet(EStandardIcon icon)
{
	if (m_hWnd==NULL) Load();					// 如果窗口还未加载，现在加载
	HICON hIco = LoadIcon(0, (LPCTSTR)icon);	// 获得的是shared icon，不要 DestroyIcon
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIco);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIco);
}

void CBForm::IconSet(unsigned short iconRes)
{
	if (m_hWnd==NULL) Load();		// 如果窗口还未加载，现在加载
	HICON hIco = LoadIcon(pApp->hInstance, (LPCTSTR)((DWORD)iconRes));	// 获得的是shared icon，不要 DestroyIcon
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIco);
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIco);
}

#ifndef WS_EX_NOACTIVATE
	#define WS_EX_NOACTIVATE        0x08000000L
#endif
bool CBForm::NoActivated()
{
	return ((StyleEx() & WS_EX_NOACTIVATE) != 0);
}

void CBForm::NoActivatedSet( bool newValue )
{
	StyleExSet(WS_EX_NOACTIVATE, newValue?1:-1);
}







// =================== 事件函数关联 ===================



// 设置一个窗体的或子窗口控件的事件（多个重载版本）
// ptrFunHandler 为一个事件处理函数的地址
// 如果是窗体事件，idResControl 参数应设为0
// 如果是控件事件，idResControl 参数应设为 控件的资源ID
// 如果是菜单（包括系统菜单）、加速键事件，第2个参数为 eMenu_Click，将忽略 idResControl 参数
void CBForm::EventAdd( unsigned short int idResControl, 
					  ECBFormCtrlEventsVoid eventType, 
					  ONEventVoid ptrFunHandler )
{
	EventAddPrivate(idResControl, (long)eventType, (long)ptrFunHandler, 0);
}

void CBForm::EventAdd( unsigned short int idResControl, 
					   ECBFormCtrlEventsI eventType, 
					   ONEventI ptrFunHandler )
{
	EventAddPrivate(idResControl, (long)eventType, (long)ptrFunHandler, 1);
}

void CBForm::EventAdd( unsigned short int idResControl, 
					  ECBFormCtrlEventsII eventType, 
					  ONEventII ptrFunHandler )
{
	EventAddPrivate(idResControl, (long)eventType, (long)ptrFunHandler, 2);
}

void CBForm::EventAdd( unsigned short int idResControl, 
					  ECBFormCtrlEventsIII eventType, 
					  ONEventIII ptrFunHandler )
{

	if (eventType == eMenu_Click)
	{
		// ============ 菜单（包括系统菜单）或加速键的事件 ============
		// 忽略 idResControl
		// 将事件处理函数地址存入 m_hashEventNotify.Item(c_CBMenuClickEventKey)，函数需 3 个 int 型的参数
		if (m_hashEventNotify.IsKeyExist(c_CBMenuClickEventKey)) 
			m_hashEventNotify.Remove(c_CBMenuClickEventKey);
		m_hashEventNotify.Add( (long)ptrFunHandler, c_CBMenuClickEventKey, 3, 0, 0, 0, 0, false); 
	}
	else
	{
		// ============ 窗体或控件的事件 ============
		EventAddPrivate(idResControl, (long)eventType, (long)ptrFunHandler, 3);
	}

}

void CBForm::EventAdd( unsigned short int idResControl, 
					  ECBFormCtrlEventsIIII eventType, 
					  ONEventIIII ptrFunHandler )
{
	EventAddPrivate(idResControl, (long)eventType, (long)ptrFunHandler, 4);
}

// 设置一个窗体的或子窗口控件的事件（多个重载版本的共用函数）
void CBForm::EventAddPrivate(unsigned short int idResControl, long eventType, long ptrFunHandler, int nParamsIntCount )
{
	//////////////////////////////////////////////////////////////////////////
	if (eventType & c_CBNotifyEventBase )
	{   
		// ============ 以通知消息产生的控件事件 ============
		// 将事件函数地址存入 m_hashEventNotify
		// key 的高 2 位为 WM_COMMAND 消息或 WM_NOTIFY 消息的通知码
		//  （通知码就是 eventType 的枚举值），低2位为控件ID
		long key = (long)MAKELONG(idResControl, eventType); 
		if (m_hashEventNotify.IsKeyExist(key))
			m_hashEventNotify.Remove(key);
		m_hashEventNotify.Add(ptrFunHandler, key, nParamsIntCount,0,0,0,0,false);
		
		// 调整必要的风格，否则若不是此风格则不能响应该事件
		if (m_hWnd)
		{ 
			// 为子窗口控件添加 BS_NOTIFY、SS_NOTIFY 或 LBS_NOTIFY 风格
			HWND hwndCtrl = GetDlgItem(m_hWnd, (int)idResControl);
			long style=GetWindowLong(hwndCtrl, GWL_STYLE);
			CBControl ctrl;
			ctrl.SetResID(idResControl);
			if ( ctrl.IsClassName(TEXT("Button")) )
				SetWindowLong(hwndCtrl, GWL_STYLE, style | BS_NOTIFY);
			else if ( ctrl.IsClassName(TEXT("ListBox")) )
				SetWindowLong(hwndCtrl, GWL_STYLE, style | LBS_NOTIFY);
			// Static 控件加 SS_NOTIFY 已在 EnumChildProcSubClass 中完成
		}

	}
	//////////////////////////////////////////////////////////////////////////
	else	// if (eventType & c_CBNotifyEventBase ) else
	{
		// ============ 以窗口消息产生的事件 ============
		//  （非通知消息的 WM_XXX，包括向窗体和向子窗口控件发送 
		//   WM_XXX 产生的，包括窗体事件和控件事件）
		// 将事件函数地址存入 m_hashEventMessage
		// key 的低 2 位为消息 WM_XXX；
		//       高 2 位为 0 （对窗体事件）或 控件资源ID（对控件事件）
		long key = (long)MAKELONG(eventType, idResControl);
		
		// key=WM_COMMAND消息的wParam（高位为通知码，低位为控件ID），值=用户函数地址
		if ( m_hashEventMessage.IsKeyExist(key) )
			m_hashEventMessage.Remove(key, false);
		m_hashEventMessage.Add(ptrFunHandler, key, nParamsIntCount,0,0,0,0,false);

		// 调整必要的风格，否则若不是此风格则不能响应该事件
		if (m_hWnd)
		{ 
			// Static 控件加 SS_NOTIFY 已在 EnumChildProcSubClass 中完成

			// WM_DROPFILES 消息
			if (eventType == eFilesDrop)	
			{
				if (idResControl==0)
				{
					// 为窗体添加 接收 WM_DROPFILES 消息
					DragAcceptFiles(m_hWnd, 1);
				}
				else
				{ 
					// 为子窗口控件添加 接收 WM_DROPFILES 消息
					HWND hwndCtrl = GetDlgItem(m_hWnd, (int)idResControl);
					DragAcceptFiles(hwndCtrl, 1);
				}
			}
		}	// end if (m_hWnd)
	}	// end if (eventType & c_CBNotifyEventBase ) - else
	//////////////////////////////////////////////////////////////////////////
}


int CBForm::IDRaisingEvent()
{
	return m_idRaisingEvent;
}




void CBForm::RestoreModalWinEnabled()
{
	// 在显示一个模态对话框时：
	//   若其他某窗口目前为 Disabled 且此值为0，则维持该窗口的此值不变；
	//   否则，其他某窗口目前为 Enabled，或者该窗口此值 >0，都会将该窗口的此值 +1
	// 在隐藏一个模态对话框时：
	//   如某窗口此值为0，则不做任何操作；否则将此值-1，如-1后为0，则恢复为 Enabled 状态

	int i;  long level=0; 
	for (i=1; i<=ms_hashWnd.Count(); i++)
	{
		level = ms_hashWnd.ItemLong2FromIndex(i, false);
		if (level>0)
		{
			level--;
	
			if (level<=0)
			{
				// 恢复窗口 Enabled
				EnableWindow((HWND)ms_hashWnd.IndexToKey(i), 1);
				level = 0;		// 容错：此时 level 就应为0，
			}
			
			// 将层次记录到 ItemLong2
			ms_hashWnd.ItemLong2FromIndexSet(i, level, false);
		}
		// else level==0 不做任何操作
	}

	// 恢复 Owner 窗口为前台
	SetActiveWindow (GetWindow(m_hWnd, GW_OWNER));

	// 清除标志变量
	m_ModalShown=NULL;

	// 模态层次减1
	ms_iModalLevel--; 
	if (ms_iModalLevel<0) ms_iModalLevel=0;		// 容错
}


void CBForm::ConstructMenuIDItems( HMENU hMenuParent, bool bIsTop/*=false*/ )
{
	int i, iCount;
	UINT idItem;
	
	if (bIsTop) m_hashMenuIDs.Clear();	// 清除已有记录
	if (0==hMenuParent) return;   

	// 获得 hMenuParent 下的菜单项总数
	iCount = GetMenuItemCount(hMenuParent); 
	if (iCount == -1) return;		// GetMenuItemCount 调用失败，或无菜单项
	
	// 逐个处理 hMenuParent 下的所有菜单项
	for(i=0; i<=iCount-1; i++)
	{
		idItem = GetMenuItemID(hMenuParent, i);
		if (idItem == 0xFFFFFFFF)
		{
			// hMenu 下的第 i 项有级联菜单
			// 递归处理此级联菜单下的所有子菜单项
			ConstructMenuIDItems(GetSubMenu(hMenuParent, i));
		}
		else
		{
			// hMenu 下的第 i 项已获得菜单项 ID，保存此 ID 到哈希表
			// 哈希表 key = 菜单项的资源id，data = 该菜单的父菜单的句柄
			m_hashMenuIDs.Add((int)hMenuParent, idItem, 0, 0, 0, 0, 0, false);
		}
		
	}
}




void CBForm::ClearResource()
{
	// 释放附加属性资源
	// 不必删除光标对象，系统会自动删除
	// m_WndProp.hCursor
	// if (m_WndProp.cursorIdx) MousePointerSet(0);  // 其中会 DestroyCursor
	m_WndProp.hCursor = NULL; m_WndProp.hCursor2 = NULL;
	m_WndProp.cursorIdx = 0;  m_WndProp.cursorIdx2 = 0;

	m_WndProp.backColor = -1;
	if (m_WndProp.hBrushBack) 
	{ DeleteObject(m_WndProp.hBrushBack); m_WndProp.hBrushBack = NULL; }

	m_WndProp.foreColor = -1;
	m_WndProp.bBackStyleTransparent = false;
	if (m_WndProp.hFont) 
	{ DeleteObject(m_WndProp.hFont); m_WndProp.hFont = NULL; }

	SetRect(&m_WndProp.rcTextOutClip, 0, 0, 0, 0);
	m_WndProp.iTextOutStartLeft = 0;
	m_WndProp.iTextOutFlags = 0;

	m_WndProp.stretch = false;
	if (m_WndProp.hBmpDisp) { DeleteObject((HGDIOBJ)m_WndProp.hBmpDisp); m_WndProp.hBmpDisp=NULL; }
	SetRect(&m_WndProp.rcPictureClip, -1, -1, -1, -1);
	m_WndProp.iTitleBarBehav = 0;

	if (m_WndProp.tagString)	// m_WndProp.tagString 未由 HM 管理 
	{ delete [](m_WndProp.tagString); m_WndProp.tagString = NULL; 	}
	m_WndProp.iTag1 = 0;
	m_WndProp.iTag2 = 0;
	m_WndProp.dTag = 0.0;	


	// 释放窗体
	if (m_hWnd)
	{
		//==============================================================
		// 在此处不能再调用 DestroyWindow 或发送 WM_DESTROY 消息
		// 因收到 WM_DESTROY 消息时会调用本函数，否则将形成死递归

		// 恢复子窗口控件的子类处理：lParam 设为 0 表示卸载窗口时恢复子类处理子窗口控件
		EnumChildWindows(m_hWnd, EnumChildProcSubClass, 0);

		// 如果前台窗口刚好是本窗口，取消加速键记录和前台窗口记录
		if (CBForm::ms_hWndActiForm == m_hWnd)
		{
			CBForm::ms_hWndActiForm = NULL;
			CBForm::ms_hAccelCurrUsed = NULL;
		}
		
		// 从哈希表中删除该对话框的记录
		ms_hashWnd.Remove((long)m_hWnd, false);
		
		// 如果该窗口为模态，恢复所有窗口 Enabled
		if (m_ModalShown) RestoreModalWinEnabled();
		
		// 清除成员
		m_hashMenuIDs.Clear();
		
		m_hWnd = NULL;
		// m_hashEventNotify.Clear();
		// m_hashEventMessage.Clear();
	}
}








long CBForm::RoundRectForm()
{
	return m_RoundRect;
}


int CBForm::EdgeWidth()
{
	return m_EdgeWidth;
}

void CBForm::EdgeWidthSet( int newEdgeWidth )
{
	m_EdgeWidth = newEdgeWidth;
	InvalidateRect(m_hWnd, NULL, true);	// 使设置生效（在 WM_PAINT 中将绘制边框
}

COLORREF CBForm::EdgeColor()
{
	return m_EdgeColor;
}

void CBForm::EdgeColorSet( COLORREF newEdgeColor )
{
	m_EdgeColor = newEdgeColor;
	InvalidateRect(m_hWnd, NULL, true);	// 使设置生效（在 WM_PAINT 中将绘制边框）
}


void CBForm::RoundRectFormSet(long newRoundCorner)
{
	if (m_hWnd==NULL) Load();					// 如果窗口还未加载，现在加载

	m_RoundRect = newRoundCorner;

	if ( m_RoundRect )
	{
		// 设置圆角窗体
		HRGN hrgnOld = m_hRgnRoundRect;
		
		if (m_RoundRect < 65536)
			m_hRgnRoundRect = CreateRoundRectRgn(0, 0, Width()+1, Height()+1, m_RoundRect, m_RoundRect);
		else
			m_hRgnRoundRect = CreateRoundRectRgn(0, 0, Width()+1, Height()+1, LOWORD(m_RoundRect), HIWORD(m_RoundRect));
		SetWindowRgn(m_hWnd, m_hRgnRoundRect, TRUE);
		
		if (hrgnOld) DeleteObject(hrgnOld);
	}
	else
	{
		// m_RoundRect == 0，不使用圆角窗体
		if (m_hRgnRoundRect)
		{
			// 已经设过圆角窗体，才恢复
			SetWindowRgn(m_hWnd, NULL, TRUE);
			DeleteObject(m_hRgnRoundRect);
			m_hRgnRoundRect = NULL;
		}
		// else
		//     没有设过圆角窗体，不必恢复
	}
}

void CBForm::MoveToScreenCenter( int width/*=-1*/, int height/*=-1*/, bool bHorizontalCenter/*=true*/, bool bVerticalCenter/*=true*/ )
{
	if (m_hWnd==NULL) Load();					// 如果窗口还未加载，现在加载

	RECT rect;
	GetWindowRect(m_hWnd, &rect);

	// 将窗体应该的宽度、高度 => width, height
	if (width<=0) width = rect.right - rect.left;
	if (height<=0) height = rect.bottom - rect.top;

	// 将窗体应该所在屏幕位置的左上角坐标 => rect.left, rect.top
	if (bHorizontalCenter)
		rect.left = ( GetSystemMetrics(SM_CXSCREEN) - width ) / 2;
	if (bVerticalCenter)
		rect.top = ( GetSystemMetrics(SM_CYSCREEN) - height ) / 2;
	
	// 一次性调整窗体的位置、大小
	MoveWindow(m_hWnd, rect.left, rect.top, width, height, 1);
}



// （对无边框窗体）是否允许处理拖动窗体四周改变窗体大小（允许则>0，值为可改变大小的边框范围）
int CBForm::BorderAutoResizable()
{
	return m_iBorderAutoResizable;
}

void CBForm::BorderAutoResizableSet( int iNewValue/*=1*/ )
{
	m_iBorderAutoResizable = iNewValue;
}








long CBForm::EventsGenerator( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	switch(uMsg)
	{
	case WM_COMMAND:	// 子窗口控件通知消息、或菜单消息、或加速键消息
		if (lParam)
		{
			// ======== 子窗口控件通知消息 ========
			// wparam 高位为通知码，低位为控件ID；整个作为“键”
			// 在 m_hashEventNotify 中查找有无键为 wparam 的项，如果有
			//   表示这个事件需要处理，调用用户函数处理事件
			// 用户函数的地址在 m_hashEventNotify 的对应项的 Data 中保存
			if (m_hashEventNotify.IsKeyExist((long)wParam))
			{
				m_idRaisingEvent = LOWORD(wParam);

				ONEventVoid ptrFunc0 = NULL; 
				ONEventI ptrFunc1 = NULL;
				ONEventII ptrFunc2 = NULL;
				ONEventIII ptrFunc3 = NULL;
				ONEventIIII ptrFunc4 = NULL;
				switch(m_hashEventNotify.ItemLong((long)wParam, false))
				{
				case 0:		// 0 个参数
					ptrFunc0 =
						(ONEventVoid)m_hashEventNotify.Item((long)wParam, false);
					if (ptrFunc0) (*ptrFunc0)();								// 加 if 容错判断
					break;
				case 1:		// 1 个 int 型参数
					ptrFunc1 = 
						(ONEventI)m_hashEventNotify.Item((long)wParam, false);
					if (ptrFunc1) (*ptrFunc1)((int)wParam);						// 加 if 容错判断
					break;
				case 2:		// 2 个 int 型参数
					ptrFunc2 = 
						(ONEventII)m_hashEventNotify.Item((long)wParam, false);
					if (ptrFunc2) (*ptrFunc2)((int)wParam, (int)lParam);		// 加 if 容错判断
					break;
				case 3:		// 3 个 int 型参数
					ptrFunc3 = 
						(ONEventIII)m_hashEventNotify.Item((long)wParam, false);
					if (ptrFunc3) (*ptrFunc3)((int)LOWORD(wParam), (int)HIWORD(wParam), 
						lParam);												// 加 if 容错判断
					break;
				case 4:		// 4 个 int 型参数
					ptrFunc4 = 
						(ONEventIIII)m_hashEventNotify.Item((long)wParam, false);
					if (ptrFunc4) (*ptrFunc4)((int)LOWORD(wParam), (int)HIWORD(wParam), 
						(int)LOWORD(lParam), (int)HIWORD(lParam));				// 加 if 容错判断
					break;

				}	// end of switch
			}	// end of if (m_hashEventNotify.IsKeyExist((long)wparam))
		}	// end of if (lparam)
		else	// if (lparam) else：lparam==NULL
		{  
			// ======== 菜单消息（系统菜单不在此处处理）或加速键消息 ========
			ONEventIII ptrMenuClick;
			ptrMenuClick = (ONEventIII)m_hashEventNotify.Item(c_CBMenuClickEventKey, false);
			if (ptrMenuClick)
			{
				// 参数1为被单击的菜单资源ID；
				// 参数2为是直接选择的菜单(=0)，还是从加速键执行的(=1)
				// 参数3=0 表示是从普通菜单选择的，=1表示是从系统菜单选择的
				(*ptrMenuClick)((int)LOWORD(wParam), (int)(HIWORD(wParam)), 0);
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_NOTIFY:
		{
			long key = MAKELPARAM(((NMHDR *)lParam)->idFrom, ((NMHDR *)lParam)->code);
			if (m_hashEventNotify.IsKeyExist(key))
			{
				m_idRaisingEvent =((NMHDR *)lParam)->idFrom;

				ONEventVoid ptrFunc0 = NULL; 
				ONEventI ptrFunc1 = NULL;
				ONEventII ptrFunc2 = NULL;
				ONEventIII ptrFunc3 = NULL;
				ONEventIIII ptrFunc4 = NULL;
				switch(m_hashEventNotify.ItemLong(key, false))
				{
				case 0:		// 0 个参数
					ptrFunc0 = (ONEventVoid)m_hashEventNotify.Item(key, false);
					break;
				case 1:		// 1 个 int 型参数
					ptrFunc1 = (ONEventI)m_hashEventNotify.Item(key, false);
					break;
				case 2:		// 2 个 int 型参数
					ptrFunc2 = (ONEventII)m_hashEventNotify.Item(key, false);
					break;
				case 3:		// 3 个 int 型参数
					ptrFunc3 = (ONEventIII)m_hashEventNotify.Item(key, false);
					break;
				case 4:		// 4 个 int 型参数
					ptrFunc4 = (ONEventIIII)m_hashEventNotify.Item(key, false);
					break;					
				}	// end of switch(m_hashEventNotify.ItemLong(key, false))

				switch (((NMHDR *)lParam)->code)
				{
				case NM_CLICK: case NM_RCLICK:				
				case NM_DBLCLK: case NM_RDBLCLK:
					{
						NMLISTVIEW * pnm = (NMLISTVIEW *)lParam;
						if (ptrFunc4) (*ptrFunc4)(pnm->iItem+1, pnm->iSubItem+1, 
							pnm->ptAction.x, pnm->ptAction.y);  // +1转换为下标从1开始
					}
					break;
				default:
					switch(m_hashEventNotify.ItemLong(key, false))
					{
					case 0:		// 0 个参数
						if (ptrFunc0) (*ptrFunc0)();								// 加 if 容错判断
						break;
					case 1:		// 1 个 int 型参数
						if (ptrFunc1) (*ptrFunc1)((int)wParam);						// 加 if 容错判断
						break;
					case 2:		// 2 个 int 型参数
						if (ptrFunc2) (*ptrFunc2)((int)wParam, (int)lParam);		// 加 if 容错判断
						break;
					case 3:		// 3 个 int 型参数
						if (ptrFunc3) (*ptrFunc3)((int)LOWORD(wParam), (int)HIWORD(wParam), 
							lParam);												// 加 if 容错判断
						break;
					case 4:		// 4 个 int 型参数
						if (ptrFunc4) (*ptrFunc4)((int)LOWORD(wParam), (int)HIWORD(wParam), 
							(int)LOWORD(lParam), (int)HIWORD(lParam));				// 加 if 容错判断
						break;					
					}	// end of switch(m_hashEventNotify.ItemLong(key, false))
				}  // end of switch (((NMHDR *)lParam)->code)
			}
		}	// end of case WM_NOTIFY

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_DESTROY:
		// 必须有 WM_DESTROY 处理过程。因父窗体被关闭时，其子窗体的自动关闭过程
		//   不会收到 WM_CLOSE 消息，只会收到 WM_DESTROY 消息

		// 调用 Form_UnLoad 事件函数（如果有的话）
		{
			ONEventVoid ptrFuncForm0 = (ONEventVoid)m_hashEventMessage.Item((long)uMsg, false);
			if (ptrFuncForm0) (*ptrFuncForm0)();		// 加 if 容错判断
		}
		
		// 在 ClearResource 中，将枚举子窗口，恢复所有子窗口控件的子类处理
		ClearResource();
		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_CLOSE:
		
		// 调用 Form_QueryLoad 事件函数（如果有的话）
		{
			int bCancel = 0;
			ONEventI ptrFuncForm0 = (ONEventI)m_hashEventMessage.Item((long)uMsg, false);
			if (ptrFuncForm0) 
			{
				(*ptrFuncForm0)((int)(&bCancel));		// 加 if 容错判断
				if (bCancel) return bCancel;			// 取消 WM_CLOSE 消息，不卸载窗体
			}
		}

		// 如果前面没有取消（bCancel没有为非0值），继续卸载对话框
		// 不能用 SendMessage(m_hWnd, WM_DESTROY, 0, 0); 
		DestroyWindow(m_hWnd);
		
		// 此时，应该在 CBForm_DlgProc 中收到了 WM_DESTROY 消息，并执行了 ClearResource()
		//   而使 m_hWnd 为 0。如果此时 m_hWnd 仍不为 0，为容错，下面再次执行 ClearResource()
		if (m_hWnd) ClearResource(); 

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_ACTIVATE:
		
		// 调用 eForm_ActiDeactivate 事件函数（失活时）（如果有的话）
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			ONEventI ptrFuncForm0 = (ONEventI)m_hashEventMessage.Item((long)uMsg, false);
			if (ptrFuncForm0) (*ptrFuncForm0)(0);		// 加 if 容错判断
		}

		// 切换当前前台窗体
		ms_hWndActiForm=m_hWnd;

		// 切换加速键
		if (LOWORD(wParam)==WA_ACTIVE || LOWORD(wParam)== WA_CLICKACTIVE)
		{
			ms_hAccelCurrUsed=(HACCEL)ms_hashWnd.ItemLong((long)m_hWnd, false); 
		}

		// 调用 eForm_ActiDeactivate 事件函数（激活时）（如果有的话）
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			ONEventI ptrFuncForm0 = (ONEventI)m_hashEventMessage.Item((long)uMsg, false);
			if (ptrFuncForm0) (*ptrFuncForm0)(1);		// 加 if 容错判断
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_SHOWWINDOW:
		// 如果窗口正被隐藏，而且该窗口为模态，恢复所有窗口 Enabled
		if (wParam==0)
			if (m_ModalShown) RestoreModalWinEnabled();
		
		break;		// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理
		
	case WM_SETCURSOR:
		if (pApp->MousePointerGlobal())
		{
			// ======== 使用全局鼠标光标 ========
			if (pApp->MousePointerGlobalHCursor())
			{
				SetCursor(pApp->MousePointerGlobalHCursor());
				return 1;	// return 1; 不再走 switch 外的默认生成事件的处理，1表示也不让默认窗口程序处理了
			}
		}
		else if (m_WndProp.cursorIdx)  
		{
			// ======== 使用为本窗体所设置的鼠标光标 ========
			if (m_WndProp.hCursor)
			{					
				SetCursor(m_WndProp.hCursor);
				return 1;	// return 1; 不再走 switch 外的默认生成事件的处理，1表示也不让默认窗口程序处理了
			}
		}
		
		// 其他使用默认，即窗口类的鼠标光标（默认窗口程序自动处理）
		break;		// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理
		
	case WM_ERASEBKGND:
		// 设置窗口背景色
		if (m_WndProp.backColor != -1 && m_WndProp.hBrushBack)	// -1 表示采用系统默认颜色，不需干预
		{
			RECT rectBK;
			GetClientRect(m_hWnd, &rectBK);
			FillRect((HDC)wParam, &rectBK, m_WndProp.hBrushBack);
			return 1;	// 如应用程序自行擦出背景，必须返回 1
		}

		break;  // 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理

	case WM_CTLCOLORSTATIC: 
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLOREDIT:
		// 设置子窗口控件背景色和前景色（这些消息是发送到父窗口的，不是发送到控件）
		{
			// 调用 CBWndBase 的静态函数 PropertyMemCtrl，以控件的句柄（lParam）获得该控件的属性空间
			STWndProp *pPro = CBWndBase::PropertyMemCtrl((HWND)lParam, false);

			if (pPro)
			{
				// ======== 设置前景色 ========
				if (pPro->foreColor != -1) 
				{
					SetTextColor((HDC)wParam, pPro->foreColor);

					if (pPro->backColor == -1)
					{
						// 如果没有同时设置背景色，为使前景色生效
						// 这里把背景色设置为系统颜色
						pPro->backColor = GetSysColor(COLOR_BTNFACE);
						if (pPro->hBrushBack) DeleteObject(pPro->hBrushBack);
						pPro->hBrushBack = CreateSolidBrush(pPro->backColor);	
						// pProp->backColor 已 != -1，下面的 if 段会返回背景色画刷
					}
				}

				// ======== 设置背景色 ========
				if (pPro->bBackStyleTransparent)
				{
					// 背景透明
					SetBkMode((HDC)wParam, TRANSPARENT); 
					//hDC->SetTextColor(RGB(0,0,0)); 
					return (long)GetStockObject(HOLLOW_BRUSH); 
				}	// if (pProp->bBackStyleTransparent)
				else
				{
					// 背景不透明
					if (pPro->backColor != -1)
					{
						// 用户指定了背景色
						// 将文字背景改为不透明，返回画刷
						SetBkColor((HDC)wParam, pPro->backColor);
						SetBkMode((HDC)wParam, OPAQUE);
						if (pPro->hBrushBack) return (long)(pPro->hBrushBack);
					}
					// else
						// 用户没有指定背景色，执行 break; 出 switch 后默认处理生成事件
				}	// end if (pPro->bBackStyleTransparent) - els

			}  // end if (pPro)
		}  // end {

		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理

	// ==================================================================
	//	特殊事件的处理
	case WM_PAINT:
		// 如果 m_WndProp.hBmpDisp 不为0，说明指定了要在窗体上显示的位图图片
		// 如果 m_EdgeWidth>0 ，说明指定了要在窗体上绘制边框
		// 这两种情况都 BeginPaint 
		if (m_WndProp.hBmpDisp || m_EdgeWidth>0)	
		{
			PAINTSTRUCT ps;
 			BeginPaint(m_hWnd, &ps);
			
			// 调用 CBWndBase 基类中的 RefreshPicture 显示位图（如 
			//   m_WndProp.hBmpDisp 为 0 以下函数会自动跳过）
			RefreshPicture(ps.hdc, m_EdgeWidth, m_EdgeWidth);

			// 绘制窗体边框
			if (m_EdgeWidth>0)
			{
				int iEdgeWidth;		// 扩大2倍的边框宽度；不使用PS_INSIDEFRAME，否则圆角矩形容易有漏的边
				if (m_EdgeWidth==1) iEdgeWidth = m_EdgeWidth; else iEdgeWidth=m_EdgeWidth*2-1;
				HPEN hPen = CreatePen(PS_SOLID, iEdgeWidth, m_EdgeColor);
				HPEN hPenOld = (HPEN)SelectObject(ps.hdc, hPen);
				HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH); 
				HBRUSH hBrushOld = (HBRUSH)SelectObject(ps.hdc, hBrush);

				if ( m_RoundRect )
				{
					// 使用圆角窗体
					if (m_RoundRect < 65536)
						RoundRect(ps.hdc, 0, 0, Width(), Height(), m_RoundRect, m_RoundRect);
					else
						RoundRect(ps.hdc, 0, 0, Width(), Height(), LOWORD(m_RoundRect), HIWORD(m_RoundRect));
				}
				else
				{
					// m_RoundRect == 0，不使用圆角窗体
					Rectangle(ps.hdc, 0, 0, Width(), Height());
				}

				SelectObject(ps.hdc, hBrushOld);
				SelectObject(ps.hdc, hPenOld);
				DeleteObject(hPen);
			}	// end if (m_EdgeWidth>0)

			EndPaint(m_hWnd, &ps);
		}	// end if (m_hBmpBack || m_EdgeWidth>0)

		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理

	case WM_SIZE: 
		if ( m_RoundRect ) RoundRectFormSet(m_RoundRect);		// 如果是圆角矩形，大小改变后需重新设置窗体的圆角区域
		if ( m_WndProp.hBmpDisp || m_EdgeWidth>0 ) InvalidateRect(m_hWnd, NULL, FALSE);
		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理

	case WM_LBUTTONDOWN: 	case WM_RBUTTONDOWN:	case WM_MBUTTONDOWN:
	case WM_LBUTTONUP:		case WM_RBUTTONUP:		case WM_MBUTTONUP:
	case WM_LBUTTONDBLCLK:	case WM_MBUTTONDBLCLK:	case WM_RBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		// 鼠标左、右、中键按下、抬起、双击、鼠标移动
		{
			long key = 0;  int button = 0;
			if (uMsg == 0x200)				// 鼠标移动
			{
				key = WM_MOUSEMOVE;
				if ( wParam & MK_LBUTTON ) button = button | 1;
				if ( wParam & MK_RBUTTON ) button = button | 2;
				if ( wParam & MK_MBUTTON ) button = button | 4;

				//////////////////////////////////////////////////////////////////////////
				// 模拟 在标题栏上拖动 移动窗体位置行为、拖动边框改变窗体大小行为
				if ( m_WndProp.iTitleBarBehav || m_iBorderAutoResizable )
				{
					static int iCursorIndexLast = m_WndProp.cursorIdx;  // 记录上次鼠标指针，不让每次 MouseMove 都变化

					// 获得鼠标位置是位于边框以及上、下、左、右边框位置(非0)，还是位于窗体内部(0) => posi
					// 鼠标指针应该的形状（系统指针形状） => m_CursorIdx
					// 原先鼠标指针形状在 m_CursorIdx2 中保存，如鼠标未位于边框，可用 m_CursorIdx2 恢复指针形状
					const int c_iBorderRange = 8;
					POINT pt; WPARAM posi=0; int cursor=0; LPARAM lparamPt=0;
					GetCursorPos(&pt);  lparamPt=(LPARAM)(*(LPARAM *)&pt);
					ScreenToClient(m_hWnd, &pt);
					if (pt.y<c_iBorderRange)
					{
						if (pt.x<c_iBorderRange) { posi=HTTOPLEFT; m_WndProp.cursorIdx=IDC_SizeNWSE; }
						else if(pt.x>Width()-c_iBorderRange) { posi=HTTOPRIGHT; m_WndProp.cursorIdx=IDC_SizeNESW; }
						else { posi=HTTOP; m_WndProp.cursorIdx=IDC_SizeNS; }
					}
					else if(pt.y>Height()-c_iBorderRange)
					{
						if (pt.x<c_iBorderRange) { posi=HTBOTTOMLEFT; m_WndProp.cursorIdx=IDC_SizeNESW; }
						else if(pt.x>Width()-c_iBorderRange) { posi=HTBOTTOMRIGHT; m_WndProp.cursorIdx=IDC_SizeNWSE; }
						else { posi=HTBOTTOM; m_WndProp.cursorIdx=IDC_SizeNS; }
					}
					else
					{
						if (pt.x<c_iBorderRange) { posi=HTLEFT; m_WndProp.cursorIdx=IDC_SizeWE; }
						else if(pt.x>Width()-c_iBorderRange) { posi=HTRIGHT; m_WndProp.cursorIdx=IDC_SizeWE; }
						else
						{ posi=0; m_WndProp.cursorIdx=m_WndProp.cursorIdx2; }	// 鼠标位于窗体内部区域（可以移动窗体位置）
					}
					
					if (m_iBorderAutoResizable && iCursorIndexLast!=m_WndProp.cursorIdx)
					{
						// 设置鼠标指针形状为 m_CursorIdx 的形状
						if (m_WndProp.cursorIdx)
						{
							if (m_WndProp.cursorIdx == m_WndProp.cursorIdx2)
								m_WndProp.hCursor = m_WndProp.hCursor2;		// 使用副本的鼠标形状
							else
								m_WndProp.hCursor = LoadCursor(NULL, 
								  MAKEINTRESOURCE(m_WndProp.cursorIdx - gc_IDStandCursorIDBase));
						}
						// 向本窗口发送 WM_SETCURSOR，以使光标立即生效
						//    在本窗口不断接收到的 WM_SETCURSOR 消息中会改变鼠标光标
						SendMessage(m_hWnd, WM_SETCURSOR, (WPARAM)m_hWnd, 0);

						iCursorIndexLast = m_WndProp.cursorIdx;
					}

					if (button==1)
					{
						// 根据 posi 设置窗体位置、大小
						ReleaseCapture();

						// 模拟改变窗体大小
						if (posi && m_iBorderAutoResizable) 
							SendMessage(m_hWnd, WM_NCLBUTTONDOWN, posi, lparamPt);	

						// 模拟移动窗体位置
						if (posi==0 && m_WndProp.iTitleBarBehav)
							SendMessage (m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, *((LPARAM *)(&pt)) );	
					}
				}  // if ( m_iTitleBarBehav || m_iBorderAutoResizable )
			}
			else if ((uMsg-0x200) % 3 == 1)	// 按下：0x201、0x204、0x207，(uMsg-0x200) % 3 == 1
			{
				key = WM_LBUTTONDOWN;		// 统一到 WM_LBUTTONDOWN 而不分为3个枚举值
				button = 1 << (((uMsg-0x0200)-1)/3); 
			}
			else if ((uMsg-0x200) % 3 == 2)	// 抬起：0x202、0x205、0x208，(uMsg-0x200) % 3 == 2
			{
				key = WM_LBUTTONUP;			// 统一到 WM_LBUTTONUP 而不分为3个枚举值
				button = 1 << (((uMsg-0x0200)-2)/3);
			}
			else if ((uMsg-0x200) % 3 == 0)	// 双击：0x203、0x206、0x209，(uMsg-0x200) % 3 == 0
			{
				key = WM_LBUTTONDBLCLK;		// 统一到 WM_LBUTTONDBLCLK 而不分为3个枚举值
				button = 1 << ((uMsg-0x0203)/3);
			}
			else
				return 0;	// 容错。不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

			if (m_hashEventMessage.IsKeyExist(key))
			{
				ONEventIIII ptrFuncForm4 = NULL;
				ptrFuncForm4 = 
					(ONEventIIII)m_hashEventMessage.Item(key, false);
				if (ptrFuncForm4)
				{
					POINT pt; UINT altKey=0;  
					GetCursorPos(&pt);  ScreenToClient(m_hWnd, &pt);
					if ( GetKeyState(VK_MENU)<0 ) altKey=4;		// Shift=1; Ctrl=2; Alt=4

					(*ptrFuncForm4)(button, 
						altKey | ( ( (wParam & (MK_CONTROL | MK_SHIFT))) /4 ), 
						pt.x, pt.y);
					
					return 0;
				}	// if (ptrFuncForm4)
			}	// if (m_hashEventMessage.IsKeyExist(key)
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_CHAR: 
		if (m_hashEventMessage.IsKeyExist(WM_CHAR))
		{
			ONEventII ptrFuncForm2 = NULL;
			int bCancel = 0;
			ptrFuncForm2 = 
				(ONEventII)m_hashEventMessage.Item(WM_CHAR, false);
			if (ptrFuncForm2)
			{
				(*ptrFuncForm2)((int)wParam, (int)(&bCancel)); 
				if (bCancel) return 1; else return 0;
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_KEYDOWN: case WM_KEYUP: 		//   shift=1,2,4 分别表示同时 Shift、Ctrl、Alt 键被按下
	case WM_SYSKEYDOWN: case WM_SYSKEYUP:
		{
			long key = uMsg;
			if (uMsg>=0x0104) key = (uMsg-0x4);	// 将WM_SYS消息归并到WM_消息
			if (m_hashEventMessage.IsKeyExist(key))
			{
				int iShift = 0;
				int bCancel = 0;
				if (GetKeyState(VK_SHIFT)<0 ) iShift |= 1;
				if (GetKeyState(VK_CONTROL)<0 ) iShift |= 2;
				if (GetKeyState(VK_MENU)<0 ) iShift |= 4;
				
				ONEventIII ptrFuncForm3 = NULL;
				ptrFuncForm3 = 
					(ONEventIII)m_hashEventMessage.Item(key, false);
				if (ptrFuncForm3)
				{
					(*ptrFuncForm3)((int)wParam, iShift, (int)(&bCancel)); 
					if (bCancel) return 1; else return 0;
				}
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_SYSCOMMAND:
		{
			ONEventIII ptrMenuClick;
			ptrMenuClick = (ONEventIII)m_hashEventNotify.Item(c_CBMenuClickEventKey, false);
			if (ptrMenuClick)
			{
				// 参数1为被单击的菜单的ID：SC_CLOSE、SC_MAXIMIZE 等
				// 参数2为是直接选择的菜单(=0)，还是从加速键执行的(=1)
				// 参数3=0 表示是从普通菜单选择的，=1表示是从系统菜单选择的
				(*ptrMenuClick)((int)wParam, 0, 1);

				return 0;
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_MENUSELECT:
		if (m_hashEventMessage.IsKeyExist((long)WM_MENUSELECT))
		{
			ONEventI ptrFuncForm1 = NULL;
			
			ptrFuncForm1 = 
				(ONEventI)m_hashEventMessage.Item((long)WM_MENUSELECT, false);
			if (ptrFuncForm1)		// 加 if 容错判断
			{
				if (lParam)
				{
					// 使用模块级 m_MenuItemSel 成员绑定到所选菜单
					if (HIWORD(wParam) & MF_POPUP)
						// 所选菜单项将引出一个弹出式菜单：LOWORD(wParam) 是索引号
						m_MenuItemSel.SetFromPos((HMENU)lParam, LOWORD(wParam)+1, m_hWnd, &m_hashMenuIDs);	//+1 转换为位置号从1开始
					else
						// 所选菜单项是普通命令项：LOWORD(wParam) 是 ResID
						m_MenuItemSel.SetFromResID((HMENU)lParam, LOWORD(wParam), m_hWnd, &m_hashMenuIDs);
					
					(*ptrFuncForm1)((int)&m_MenuItemSel);					
				}
				else
				{
					(*ptrFuncForm1)(NULL);
				}

				return 0;
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_DROPFILES:
		if (m_hashEventMessage.IsKeyExist((long)WM_DROPFILES))
		{
			ONEventIIII ptrFuncForm4 = NULL;
			
			ptrFuncForm4 = 
				(ONEventIIII)m_hashEventMessage.Item((long)WM_DROPFILES, false);
			if (ptrFuncForm4)													// 加 if 容错判断
			{
				// 获得拖动文件的总个数
				UINT count = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
				if (count==0) return 0; 	// 容错。不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

				// 开辟保存字符串指针的空间
				unsigned long * ptrArr = new unsigned long [count+1]; // 不适用 [0] 的空间
				HM.AddPtr(ptrArr);
				HM.ZeroMem(ptrArr, sizeof(unsigned long)*(count+1));

				// 开辟各自字符串的空间，并获得各文件名
				UINT i=0; UINT sizeEach=0; TCHAR * pstr;
				for (i=0; i<count; i++)
				{
					sizeEach = DragQueryFile((HDROP)wParam, i, NULL, 0);
					pstr = new TCHAR [sizeEach+1];
					HM.AddPtr(pstr);
					HM.ZeroMem(pstr, sizeof(TCHAR)*sizeEach);
					ptrArr[i+1] = (unsigned long)pstr;
					DragQueryFile((HDROP)wParam, i, pstr, sizeEach+1);
				}

				// 获得拖放文件时鼠标所在位置
				POINT ptDrag;
				DragQueryPoint((HDROP)wParam, &ptDrag); 
					
				// 释放句柄 hDrop
				DragFinish((HDROP)wParam);

				// 生成事件
				(*ptrFuncForm4)((int)ptrArr, count, (int)ptDrag.x, (int)ptDrag.y);	
			}
		}

		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_USER_NOTIFYICON:
		if (m_hashEventMessage.IsKeyExist((long)WM_USER_NOTIFYICON))
		{
			ONEventIII ptrFuncForm3 = NULL;

			ptrFuncForm3 = 
				(ONEventIII)m_hashEventMessage.Item((long)WM_USER_NOTIFYICON, false);
			if (ptrFuncForm3)													// 加 if 容错判断
			{	
				int action=0, button=0;
				if ((int)lParam == 0x200)	// 鼠标移动
				{ 
					action=0; button=0; 
				}	
				else						// 鼠标按下、抬起、双击
				{
					action = ((int)lParam-0x200) % 3;
					button = 1 << ((((int)lParam-0x0200)-action)/3);
					if (action==0) action=3;
				}

				// 生成事件
				(*ptrFuncForm3)(button, action, (int)wParam);	
			}
		}
		return 0;   // 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理
	case WM_CONTEXTMENU:
		// 禁用右键菜单（即是否取消 WM_CONTEXTMENU 消息）
		{
			// 调用 CBWndBase 的静态函数 PropertyMemCtrl，以控件的句柄（lParam）获得该控件的属性空间
			STWndProp *pPro = CBWndBase::PropertyMemCtrl((HWND)lParam, false);
			if (pPro)
			{			
				if (pPro->fDisableContextMenu) return 0;  // 取消 WM_CONTEXTMENU 消息
			}
		}
		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理
	}	// end switch
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////


	// 生成事件的默认处理：按窗体事件处理
	if (m_hashEventMessage.IsKeyExist((long)uMsg))
	{
		ONEventVoid ptrFuncForm0 = NULL;
		ONEventI ptrFuncForm1 = NULL;
		ONEventII ptrFuncForm2 = NULL;
		ONEventIII ptrFuncForm3 = NULL;
		ONEventIIII ptrFuncForm4 = NULL;
		switch(m_hashEventMessage.ItemLong((long)uMsg, false))
		{
		case 0:		// 0 个参数
			ptrFuncForm0 =
				(ONEventVoid)m_hashEventMessage.Item((long)(uMsg), false);
			if (ptrFuncForm0) (*ptrFuncForm0)();							// 加 if 容错判断
			break;
		case 1:		// 1 个 int 型参数
			ptrFuncForm1 = 
				(ONEventI)m_hashEventMessage.Item((long)(uMsg), false);
			if (ptrFuncForm1) (*ptrFuncForm1)((int)wParam);					// 加 if 容错判断
			break;
		case 2:		// 2 个 int 型参数
			ptrFuncForm2 = 
				(ONEventII)m_hashEventMessage.Item((long)(uMsg), false);
			if (ptrFuncForm2) (*ptrFuncForm2)((int)wParam, (int)lParam);	// 加 if 容错判断
			break;
		case 3:		// 3 个 int 型参数
			ptrFuncForm3 = 
				(ONEventIII)m_hashEventMessage.Item((long)(uMsg), false);
			if (ptrFuncForm3) (*ptrFuncForm3)((int)LOWORD(wParam), (int)HIWORD(wParam),  
				(int)lParam);												// 加 if 容错判断
			break;
		case 4:		// 4 个 int 型参数
			ptrFuncForm4 = 
				(ONEventIIII)m_hashEventMessage.Item((long)(uMsg), false);
			if (ptrFuncForm4) (*ptrFuncForm4)((int)LOWORD(wParam), (int)HIWORD(wParam), 
				(int)LOWORD(lParam), (int)HIWORD(lParam));					// 加 if 容错判断
			break;
		}
	}	// if (m_hashEventMessage.IsKeyExist((long)uMsg))

	return 0;
}



long CBForm::EventsGeneratorCtrl( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	long key = 0;											// 用于检索哈希表的 key
	int idCtrl = GetDlgCtrlID(hWnd);						// 来自控件的控件 ID
	if (idCtrl == 0 || idCtrl == -1) return gc_APICEventsGenDefautRet;		// 无法获得控件 ID，或控件无 ID，不进行事件处理，立即返回
	
	// 即将检索哈希表 m_hashEventMessage，键为：低 2 位为消息（WM_XXX），高 2 位为控件 ID
	key = MAKELONG(uMsg, idCtrl);

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	switch (uMsg)
	{
	case WM_COMMAND: case WM_NOTIFY:

		// 可能属于容器控件内部的控件向容器控件发送通知消息
		// 调用 EventsGenerator 统一处理

		EventsGenerator(uMsg, wParam, lParam);

		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理


	// ==================================================================
	//	特殊事件的处理
	case WM_LBUTTONDOWN: 	case WM_RBUTTONDOWN:	case WM_MBUTTONDOWN:
	case WM_LBUTTONUP:		case WM_RBUTTONUP:		case WM_MBUTTONUP:
	case WM_LBUTTONDBLCLK:	case WM_MBUTTONDBLCLK:	case WM_RBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		// 鼠标左、右、中键按下、抬起、双击、鼠标移动
		{
			//////////////////////////////////////////////////////////////////////////
			// 图片按钮的处理
			STWndProp * pPro = CBWndBase::PropertyMemCtrl(hWnd, false);
			if (pPro)
			{
				if (pPro->ePicBtn)
				{
					CBControl ctrl;
					ctrl.SetResID(idCtrl);
					if ( ctrl.hWnd()==hWnd ) // 若 idCtrl 无效则 ctrl.hWnd() 会为0；此也为再次检验，也为容错处理
					{
						POINT pt;  RECT rc;  
						GetCursorPos(&pt);  ScreenToClient(hWnd, &pt);
						GetWindowRect(hWnd, &rc);	// rect.right - rect.left 为控件 width	
													// rect.bottom - rect.top 为控件 height
						if (uMsg==WM_MOUSEMOVE)
						{	
							if (pt.x > 0 && pt.y > 0 && 
							  pt.x <= rc.right-rc.left && pt.y <= rc.bottom-rc.top)
							{
								if (pPro->iPicBtnCheckedState)
									ctrl.PicButtonStateSet(2, 1);	// 两态按钮的按下状态鼠标移动到按钮内部：显示按下状态
								else
									ctrl.PicButtonStateSet(1, 1);	// 普通按钮或两态按钮的抬起状态鼠标移动到按钮内部：显示高亮状态
							}
							else
							{
								if (pPro->iPicBtnCheckedState)
									ctrl.PicButtonStateSet(2, -1);	// 两态按钮的按下状态鼠标移动到按钮之外：显示按下状态
								else
									ctrl.PicButtonStateSet(0, -1);	// 普通按钮或两态按钮的抬起状态鼠标移动到按钮之外：显示常规状态
							}
						}
						else if (uMsg==WM_LBUTTONDOWN || uMsg==WM_RBUTTONDOWN || uMsg==WM_MBUTTONDOWN)
						{
							ctrl.PicButtonStateSet(2);			// 按下鼠标键：显示按下状态
						}	
						else if (uMsg==WM_LBUTTONUP || uMsg==WM_RBUTTONUP || uMsg==WM_MBUTTONUP)
						{
							
							if (pPro->iPicBtnStateStyle>0)		// 如果是两态，变换两态
							{
								if (pPro->iPicBtnCheckedState==0) 
									pPro->iPicBtnCheckedState=1;
								else
									pPro->iPicBtnCheckedState=0;
							}

							if (pPro->iPicBtnCheckedState)
								ctrl.PicButtonStateSet(2);			// 两态按钮的按下状态抬起鼠标键：显示按下状态
							else
								ctrl.PicButtonStateSet(1);			// 普通按钮或两态按钮的抬起状态抬起鼠标键：显示高亮状态
							// 不能 ReleaseCapture，只要鼠标还在控件上，就要Capture，否则鼠标移开时就不能恢复状态了
						}	
					}	// end if ( ctrl.hWnd()==hWnd ) 
				}	// end if (pPro->ePicBtn)
			}	// end if (pPro)


			//////////////////////////////////////////////////////////////////////////
			// 生成通用事件
			long key = 0;  int button = 0;
			if (uMsg == 0x200)				// 鼠标移动
			{
				key = MAKELONG(WM_MOUSEMOVE, idCtrl);
				if ( wParam & MK_LBUTTON ) button = button | 1;
				if ( wParam & MK_MBUTTON ) button = button | 4;
				if ( wParam & MK_RBUTTON ) button = button | 2;

				// 模拟 在标题栏上拖动 移动窗体位置行为
				if (pPro)
				{
					if (pPro->iTitleBarBehav & button)
					{
						POINT pt;
						GetCursorPos(&pt);
						ReleaseCapture();
						SendMessage (m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, *((LPARAM *)(&pt)) );
					}
				}

			}
			else if ((uMsg-0x200) % 3 == 1)				// 按下：0x201、0x204、0x207，(uMsg-0x200) % 3 == 1
			{
				key = MAKELONG(WM_LBUTTONDOWN, idCtrl);	// 统一到 WM_LBUTTONDOWN 而不分为3个枚举值
				button = 1 << ((uMsg-0x0200-1)/3); 
			}
			else if ((uMsg-0x200) % 3 == 2)				// 抬起：0x202、0x205、0x208，(uMsg-0x200) % 3 == 2
			{
				key = MAKELONG(WM_LBUTTONUP, idCtrl);	// 统一到 WM_LBUTTONUP 而不分为3个枚举值
				button = 1 << ((uMsg-0x0200-2)/3); 
			}
			else if ((uMsg-0x200) % 3 == 0)				// 双击：0x203、0x206、0x209，(uMsg-0x200) % 3 == 0
			{
				key = MAKELONG(WM_LBUTTONDBLCLK,idCtrl);// 统一到 WM_LBUTTONDBLCLK 而不分为3个枚举值
				button = 1 << ((uMsg-0x0200-3)/3);
			}
			else
				return gc_APICEventsGenDefautRet;	// 容错。不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理
			
			if (m_hashEventMessage.IsKeyExist(key))
			{
				ONEventIIII ptrFuncForm4 = NULL;
				ptrFuncForm4 = 
					(ONEventIIII)m_hashEventMessage.Item(key, false);
				if (ptrFuncForm4)			// 加 if 容错判断
				{
					POINT pt; UINT altKey=0;  
					GetCursorPos(&pt);  ScreenToClient(hWnd, &pt);
					if (GetKeyState(VK_MENU)<0) altKey=4;		// Shift=1; Ctrl=2; Alt=4
					
					m_idRaisingEvent = idCtrl;
					(*ptrFuncForm4)(button, 
						altKey | ( ( (wParam & (MK_CONTROL | MK_SHIFT))) /4 ), 
						pt.x, pt.y);
					
				}	// if (ptrFuncForm4)
			}	// if (m_hashEventMessage.IsKeyExist(key)
		}

		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_CHAR: 
		// 如果优先由窗体处理键盘事件，先触发窗体的键盘事件
		if (KeyPreview) if (EventsGenerator(uMsg, wParam, lParam)) return 1; 

		// 触发控件的键盘事件
		if (m_hashEventMessage.IsKeyExist(key))
		{
			ONEventII ptrFuncForm2 = NULL;
			int bCancel = 0;
			ptrFuncForm2 = 
				(ONEventII)m_hashEventMessage.Item(key, false);
			if (ptrFuncForm2)
			{
				m_idRaisingEvent = idCtrl;
				(*ptrFuncForm2)((int)wParam, (int)(&bCancel)); 
				if (bCancel) return 1;
			}
		}

		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理
		
	case WM_KEYDOWN: case WM_KEYUP: 		//   shift=1,2,4 分别表示同时 Shift、Ctrl、Alt 键被按下
	case WM_SYSKEYDOWN: case WM_SYSKEYUP:
		// 如果优先由窗体处理键盘事件，先触发窗体的键盘事件
		if (KeyPreview) EventsGenerator(uMsg, wParam, lParam); 

		// 触发控件的键盘事件
		if (uMsg>=0x0104) key = MAKELONG((uMsg-0x4), idCtrl);	// 将WM_SYS消息归并到WM_消息
		if (m_hashEventMessage.IsKeyExist(key))
		{
			int iShift = 0;
			int bCancel = 0;
			if (GetKeyState(VK_SHIFT)<0 ) iShift |= 1;
			if (GetKeyState(VK_CONTROL)<0 ) iShift |= 2;
			if (GetKeyState(VK_MENU)<0 ) iShift |= 4;
			
			ONEventIII ptrFuncForm3 = NULL;
			ptrFuncForm3 = 
				(ONEventIII)m_hashEventMessage.Item(key, false);
			if (ptrFuncForm3)
			{
				m_idRaisingEvent = idCtrl;
				(*ptrFuncForm3)((int)wParam, iShift, (int)(&bCancel)); 
				if (bCancel) return 1;
			}
		}

		// 由于在 MessageLoop 中屏蔽了 IsDialogMessage 对回车的处理
		//   使在按钮上按下回车时不能触发按钮被按下的事件
		//   这里修补：
		// 如果是按钮控件，且回车被按下，就模拟按钮被单击
		if (wParam == VK_RETURN && uMsg==WM_KEYUP) 
		{
			TCHAR sClassName[255];		
			// 获得类名字符串 => m_ClassName
			GetClassName(hWnd, sClassName, sizeof(sClassName)/sizeof(TCHAR)-1);
			if (_tcscmp(sClassName, TEXT("Button"))==0)
				SendMessage(hWnd, BM_CLICK, 0, 0);
		}

		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_DROPFILES:
		
		if (m_hashEventMessage.IsKeyExist(key))
		{
			ONEventIIII ptrFuncForm4 = NULL;
			
			ptrFuncForm4 = 
				(ONEventIIII)m_hashEventMessage.Item(key, false);
			if (ptrFuncForm4)													// 加 if 容错判断
			{
				// 获得拖动文件的总个数
				UINT count = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
				if (count==0) return gc_APICEventsGenDefautRet;	// 容错。不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理
				
				// 开辟保存字符串指针的空间
				unsigned long * ptrArr = new unsigned long [count+1]; // 不适用 [0] 的空间
				HM.AddPtr(ptrArr);
				HM.ZeroMem(ptrArr, sizeof(unsigned long)*(count+1));
				
				// 开辟各自字符串的空间，并获得各文件名
				UINT i=0; UINT sizeEach=0; TCHAR * pstr;
				for (i=0; i<count; i++)
				{
					sizeEach = DragQueryFile((HDROP)wParam, i, NULL, 0);
					pstr = new TCHAR [sizeEach+1];
					HM.AddPtr(pstr);
					HM.ZeroMem(pstr, sizeof(TCHAR)*sizeEach);
					ptrArr[i+1] = (unsigned long)pstr;
					DragQueryFile((HDROP)wParam, i, pstr, sizeEach+1);
				}
				
				// 获得拖放文件时鼠标所在位置
				POINT ptDrag;
				DragQueryPoint((HDROP)wParam, &ptDrag); 
				
				// 释放句柄 hDrop
				DragFinish((HDROP)wParam);
				
				// 生成事件
				m_idRaisingEvent = idCtrl;
				(*ptrFuncForm4)((int)ptrArr, count, (int)ptDrag.x, (int)ptDrag.y);					
			}
		}
		
		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	case WM_PAINT:
		if ( ms_hashCtrls.IsKeyExist((long)hWnd) )		// 如果该控件被本程序管理
		{
			CBControl ctrl;
			if (ctrl.SetResID(idCtrl))		// 且，如果成功按照 idCtrl 获得了控件(idCtrl 可能无效导致失败)
			{
				STWndProp * pPro = CBWndBase::PropertyMemCtrl(hWnd, false);
				bool fPicStatic = false;		// 是否属于 Static 控件且要在上面显示位图的情况
				
				if (pPro)
					if (pPro->hBmpDisp) 
						if (ctrl.IsClassName(TEXT("Static"))) fPicStatic=true;
				
				if ( fPicStatic )
				{
					//////////////////////////////////////////////////////////////////////////
					// 属于 Static 控件且要在上面显示位图的情况

					PAINTSTRUCT ps;
					BeginPaint(hWnd, &ps);
					
					// 调用 CBWndBase 基类中的 RefreshPicture 显示位图（如 
					//   pPro->hBmpDisp 为 0 以下函数会自动跳过）
					ctrl.RefreshPicture(ps.hdc, 0, 0);

					EndPaint(hWnd, &ps);

					// 不调用默认窗口程序、生成事件、返回
					if (m_hashEventMessage.IsKeyExist(key))	// 再生成事件，由事件子程序绘图
					{
						ONEventVoid ptrFunc0 = 
							(ONEventVoid)m_hashEventMessage.Item(key, false);
						if (ptrFunc0) { m_idRaisingEvent = idCtrl; (*ptrFunc0)(); }
					}
					return 0;								// 不再交给默认窗口程序处理
				}
				else	// else - if ( fPicStatic )
				{
					//////////////////////////////////////////////////////////////////////////
					// 不属于 Static 控件，或属于 Static 控件但不是要在上面显示位图的情况

					// 调用默认窗口程序（先完成默认绘图）、然后生成事件、返回
					CallWindowProc ((WNDPROC)(CBWndBase::ms_hashCtrls.Item((long)hWnd,false)), 
						hWnd, uMsg, wParam, lParam);		// 调用默认窗口程序，先完成默认绘图
					if (m_hashEventMessage.IsKeyExist(key))	// 再生成事件，由事件子程序绘图
					{
						ONEventVoid ptrFunc0 = 
							(ONEventVoid)m_hashEventMessage.Item(key, false);
						if (ptrFunc0) { m_idRaisingEvent = idCtrl; (*ptrFunc0)(); }
					}
					return 0;								// 不再交给默认窗口程序处理
				}	// end if ( fPicStatic )
			}	// end if (ctrl.SetResID(idCtrl))
		}	// end if ( ms_hashCtrls.IsKeyExist((long)hWnd) )  // 如果该控件被本程序管理

		// 如果该控件不被本程序管理，或者，未成功按照 idCtrl 获得了控件(idCtrl 可能无效导致失败)
		return gc_APICEventsGenDefautRet;	// 不再走 switch 外的生成事件的默认处理；如 break; 则走那个默认处理

	// ==================================================================
	case WM_SETCURSOR:
		// 设置鼠标光标：不用 SetCursor 设置的，Windows 将自动显示窗口类的光标
		if (pApp->MousePointerGlobal())
		{
			// ======== 使用全局鼠标光标 ms_hCursorGlobal ========
			if (pApp->MousePointerGlobalHCursor())
			{
				SetCursor(pApp->MousePointerGlobalHCursor());
				return 1;	// return 1; 表示不让默认窗口程序处理了
			}
		}
		else
		{
			// 判断是否为本子窗口控件设置了鼠标光标
			STWndProp * pPro = CBWndBase::PropertyMemCtrl(hWnd, false); 
			if (pPro)
			{
				if (pPro->cursorIdx)
				{
					// ======== 使用为本子窗口控件设置的鼠标光标 ========
					if (pPro->hCursor)
					{
						SetCursor(pPro->hCursor);
						return 1;	// return 1; 表示不让默认窗口程序处理了
					}
				}
			}
			// ======== 其他使用默认，即窗口类的鼠标光标（默认窗口程序自动处理） ========
		}
		
		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理
	case WM_CONTEXTMENU:
		// 禁用右键菜单（即是否取消 WM_CONTEXTMENU 消息）
		{
			// 调用 CBWndBase 的静态函数 PropertyMemCtrl，以控件的句柄（lParam）获得该控件的属性空间
			STWndProp * pPro = CBWndBase::PropertyMemCtrl(hWnd, false); 
			if (pPro)
			{			
				if (pPro->fDisableContextMenu) return 0;  // 取消 WM_CONTEXTMENU 消息
			}
		}
		break;	// 再走 switch 外的生成事件的默认处理；如 return 0; 则不再走那个默认处理
	} // end of switch (uMsg)
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	// 生成事件的默认处理
	if (m_hashEventMessage.IsKeyExist(key))
	{
		ONEventVoid ptrFunc0 = NULL; 
		ONEventI ptrFunc1 = NULL;
		ONEventII ptrFunc2 = NULL;
		ONEventIII ptrFunc3 = NULL;
		ONEventIIII ptrFunc4 = NULL;

		m_idRaisingEvent = idCtrl;

		switch(m_hashEventMessage.ItemLong(key, false))
		{
		case 0:		// 0 个参数
			ptrFunc0 =
				(ONEventVoid)m_hashEventMessage.Item(key, false);
			if (ptrFunc0) (*ptrFunc0)();								// 加 if 容错判断
			break;
		case 1:		// 1 个 int 型参数
			ptrFunc1 = 
				(ONEventI)m_hashEventMessage.Item(key, false);
			if (ptrFunc1) (*ptrFunc1)((int)wParam);						// 加 if 容错判断
			break;
		case 2:		// 2 个 int 型参数
			ptrFunc2 = 
				(ONEventII)m_hashEventMessage.Item(key, false);
			if (ptrFunc2) (*ptrFunc2)((int)wParam, (int)lParam);		// 加 if 容错判断
			break;
		case 3:		// 3 个 int 型参数
			ptrFunc3 = 
				(ONEventIII)m_hashEventMessage.Item(key, false);
			if (ptrFunc3) (*ptrFunc3)((int)LOWORD(wParam), (int)HIWORD(wParam), 
				lParam);												// 加 if 容错判断
			break;
		case 4:		// 4 个 int 型参数
			ptrFunc4 = 
				(ONEventIIII)m_hashEventMessage.Item(key, false);
			if (ptrFunc4) (*ptrFunc4)((int)LOWORD(wParam), (int)HIWORD(wParam), 
				(int)LOWORD(lParam), (int)HIWORD(lParam));				// 加 if 容错判断
			break;
			
		}	// end of switch(m_hashEventMessage.ItemLong(key, false))
	}	// end of if (m_hashEventMessage.IsKeyExist(key)

	return gc_APICEventsGenDefautRet;
}

bool CBForm::SysTrayAdd( LPCTSTR szTooltip, unsigned short idIconRes/*=0*/, UINT idSysTray /*= 0*/ )
{
	// m_NIData.cbSize !=0 表示已经设置了系统托盘
	// 一个窗体只能被设置一个系统托盘图标，如果已设置，则返回
	if (m_NIData.cbSize) return false;

	// 设置图标
	memset(&m_NIData, 0, sizeof(m_NIData));			// 结构体所有成员清0
	m_NIData.cbSize = sizeof(NOTIFYICONDATA);		// 结构体长度，!=0 表示已设系统托盘
	m_NIData.hWnd = m_hWnd;
	m_NIData.uCallbackMessage = WM_USER_NOTIFYICON;	// 要关联的消息
	_tcscpy(m_NIData.szTip, szTooltip);				// 提示信息文本
	m_NIData.uID = idSysTray;						// 主调函数规定的 ID 值

	// 设置图标 => m_NIData.hIcon
	if (idIconRes)
	{
		// 使用资源 ID 为 idIconRes 的图标
		m_NIData.hIcon = LoadIcon(pApp->hInstance, MAKEINTRESOURCE(idIconRes));
	}
	else
	{
		// 使用本窗体的图标
		m_NIData.hIcon = (HICON)SendMessage(m_hWnd, WM_GETICON, ICON_SMALL, 0);
		// 如果本窗体无图标，使用 IDI_APPLICATION
		if (m_NIData.hIcon == NULL) m_NIData.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}

	// 设置 hIcon、uCallbackMessage、szTip 成员有效
	m_NIData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// 调用API函数创建系统托盘，如 Shell_NotifyIcon 返回非0表示成功；否则表示失败
	return (Shell_NotifyIcon(NIM_ADD, &m_NIData) != 0);
}

void CBForm::SysTrayRemove()
{
	if(m_NIData.cbSize)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_NIData);
		m_NIData.cbSize=0;	// 此成员 ==0 标志无系统托盘
	}
}

bool CBForm::SysTraySetIcon( unsigned short idIconRes/*=0*/ )
{
	if( ! m_NIData.cbSize )		// 没有创建托盘图标时，可自动创建
		if ( ! SysTrayAdd(TEXT(""))) return false;	// 创建失败

	// 设置图标 => m_NIData.hIcon
	if (idIconRes)
	{
		// 使用资源 ID 为 idIconRes 的图标
		m_NIData.hIcon = LoadIcon(pApp->hInstance, MAKEINTRESOURCE(idIconRes));
	}
	else
	{
		// 使用本窗体的图标
		m_NIData.hIcon = (HICON)SendMessage(m_hWnd, WM_GETICON, ICON_SMALL, 0);
		// 如果本窗体无图标，使用 IDI_APPLICATION
		if (m_NIData.hIcon == NULL) m_NIData.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}

	// 设置 hIcon 成员有效
	m_NIData.uFlags = NIF_ICON;

	// 调用API函数创建系统托盘，如 Shell_NotifyIcon 返回非0表示成功；否则表示失败
	return (Shell_NotifyIcon(NIM_MODIFY, &m_NIData) != 0);
}

LPTSTR CBForm::SysTrayToolTip()
{
	// 若当前无托盘，返回 ""（先设置 m_NIData.szTip 为 ""，后返回）
	if( ! m_NIData.cbSize ) *m_NIData.szTip = 0;
	return m_NIData.szTip;
}

bool CBForm::SysTrayToolTipSet( tstring sTooltip )
{
	return SysTrayToolTipSet(sTooltip.c_str());
}

bool CBForm::SysTrayToolTipSet( LPCTSTR szTooltip )
{
	if( ! m_NIData.cbSize )		// 没有创建托盘图标时，可自动创建
		return SysTrayAdd(szTooltip);
	
	memset(m_NIData.szTip, 0, sizeof(m_NIData.szTip));
	_tcsncpy(m_NIData.szTip, szTooltip, 127);	// 若文本太长，只拷贝前127个字符
	
	// 设置 szTip 成员有效
	m_NIData.uFlags = NIF_TIP;

	// 调用API函数创建系统托盘，如 Shell_NotifyIcon 返回非0表示成功；否则表示失败
	return (Shell_NotifyIcon(NIM_MODIFY, &m_NIData) != 0);
}

BOOL CBForm::SysTrayPopupMenu( UINT idResMenu, int x/*=-1*/, int y/*=-1*/, bool bAllowRightClick/*=true*/ )
{
	if (x<0 || y<0)
	{
		// 使用鼠标指针当前位置
		POINT pt;
		GetCursorPos(&pt);
		if (x<0) x = pt.x;
		if (y<0) y = pt.y;
	}
		
	HMENU hMenuPop = LoadMenu(pApp->hInstance, MAKEINTRESOURCE(idResMenu));
	UINT flags = TPM_LEFTALIGN | TPM_BOTTOMALIGN;
	if (bAllowRightClick) flags = flags | TPM_RIGHTBUTTON;

	SetForegroundWindow(m_hWnd);
	// TrackPopupMenu 只能弹出 popup 式菜单，LoadMenu 得到的不是 popup 式菜单，
	//   应用 GetSubMenu 取其子菜单（这里只取第0项的子菜单）
	BOOL ret = TrackPopupMenu(GetSubMenu(hMenuPop, 0), flags, x, y, 0, m_hWnd, NULL);

	//有借有还
	DestroyMenu(hMenuPop);
	return ret;
}











