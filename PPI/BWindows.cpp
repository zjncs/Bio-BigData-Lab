//////////////////////////////////////////////////////////////////////////
// Bwindows.cpp：编写 Windows 程序常用的一些自定义函数和类的实现
//   类包括：
// 		CBHashLK：长整数型 key 的哈希表类
//		CBArrLink： 高效数组链表类
// 		CHeapMemory： 用全局对象维护所有通过 new 分配的内存指针类
//      CBApp 类： 管理应用程序全局信息
//
//   同时包含 CBHeapMemory 类的全局对象 HM 的定义、
//      CBApp 类全局 pApp 指针的定义
//
//////////////////////////////////////////////////////////////////////////

#include "Bwindows.h"
#include <stdio.h>		// 使用 _vsntprintf 等
#include <stdlib.h>		// 需用 atoi、atof 等函数
#include <time.h>

// pApp：全局 App 对象的指针。所指向的对象用于获得本程序的某些程序信息，
//   如 hInstance 等。必须同时包含 BForm 模块才能使用它获得正确信息
// 其所指对象的属性为常量，该指针所指对象的属性不能被改变
// 在全局 App 对象的构造函数中，用成员初始化表给其中的常成员赋值
CBApp *pApp=0;	


// 全局对象HM的定义
// 管理程序中所有以 new 开辟的内存空间的地址，该对象析构时可自动 delete 这些内存
//   Bwindows.h 中有此全局对象的声明，任何模块包含 Bwindows.h 即可使用此全局对象
CBHeapMemory HM; 


// 共用空间供字符串函数容错使用：
//（gEmptyTSTR用于兼容Unicode和Ansi；gEmptySTR仅用于Ansi；gEmptyWSTR 仅用于 Unicode）
// 出错时（如指针为0），容错返回空字符串，就返回此空间的内容
// 此空间在 BWindows.h 中未作声明，其他模块不得使用
TCHAR gEmptyTSTR[1];
char gEmptySTR[1];
WCHAR gEmptyWSTR[1];


//////////////////////////////////////////////////////////////////////////
// 常用自定义函数
//
//////////////////////////////////////////////////////////////////////////


// 弹出消息框
//   如 title 为 NULL，就自动使用应用程序名作为 title
EDlgBoxCmdID MsgBox(LPCTSTR szPrompt, 
					LPCTSTR szTitle,	
					EMsgBoxBtn buttons, 
					EMsgBoxIcon icon, 
					EDlgBoxDefBtn defBtn, 
					bool bTopMost, 
					bool bHelpButton, 
					bool bRightJustified, 
					bool bRightToLeftReading)
{
	UINT uType;
	if (szTitle == NULL) 
	{
		// 如果 title 为 NULL，就获得应用程序名作为 title
		TCHAR fName[2048] = {0};
		TCHAR * slash, * dot;

		// 获得应用程序名
		if (GetModuleFileName(0, fName, sizeof(fName)) == 0)
		{
			// 获得应用程序名失败
			szTitle=TEXT("Message");		// 容错：标题使用 "Message"
		}
		else	// if (GetModuleFileName(0, fName, 2048) == 0)
		{
			// 获得应用程序名成功
			szTitle = fName;	// 先设置为完整应用程序名，下面再提取其中文件名部分
								//   如果提取文件名部分失败，仍使用完整的应用程序名
			// 查找最后一个 '\\'
			slash=fName;
			while(*slash) slash++;
			while(*slash != TCHAR('\\') && slash>=fName) slash--;
			if (slash>=fName)
			{
				// 截取从 slash + 1 开始到最后一个 '.' 之前的部分为文件主名
				szTitle = slash + 1;	// 先设置截取从 '\\' 后到末尾的部分，
										//   下面再查找 '.'，如果查找 '.' 失败，
										//    仍使用从 '\\' 后到末尾的部分

				// 查找 slash + 1 后的最后一个 '.'
				dot = slash+1;
				while(*dot) dot++;
				while(*dot != TCHAR('.') && dot>slash+1) dot--;
				if (*dot == TCHAR('.')) * dot = '\0';		// 查找 '.' 成功，将 '.' 的位置
															//   改为 '\0' 使截断字符串
			}	// end if (slash)
		}	// end if (GetModuleFileName(0, fName, 2048) == 0)
	}	// end if (title == NULL) 
	
	uType = buttons | icon | defBtn;
	uType |= MB_TASKMODAL;				// hWnd == 0 时，添加 MB_TASKMODAL 
										//	以使当前线程所属的顶层窗口都被禁用
	if (bTopMost) 
	{
		// hWnd == 0 时，用 MB_TASKMODAL；否则用 MB_SYSTEMMODAL
		uType |= MB_TOPMOST;
	}

	if (bHelpButton) uType |= MB_HELP;
	if (bRightJustified) uType |= MB_RIGHT;
	if (bRightToLeftReading) uType |= MB_RTLREADING;
	return (EDlgBoxCmdID)MessageBox(0, szPrompt, szTitle, uType);
}

EDlgBoxCmdID MsgBox( char valueChar, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueChar);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( unsigned short int valueInt, /* TCHAR */ LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueInt);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( int valueInt, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueInt);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( long valueLong, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueLong);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( unsigned int valueInt, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueInt);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( unsigned long valueInt, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueInt);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( float valueSng, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueSng);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( double valueDbl, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueDbl);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( long double valueDbl, LPCTSTR szTitle /*= NULL*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPTSTR buff=Str(valueDbl);
	EDlgBoxCmdID ret;
	ret = MsgBox(buff, szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
	HM.Free(buff);	// 可及时释放 buff 的空间
	return ret;
}

EDlgBoxCmdID MsgBox( tstring valueString, tstring valueTitle /*= TEXT("")*/, EMsgBoxBtn buttons /*= mb_OK*/, EMsgBoxIcon icon /*= mb_IconNone*/, EDlgBoxDefBtn defBtn /*= mb_DefButton1*/, bool bTopMost /*= true*/, bool bHelpButton /*= false*/, bool bRightJustified /*= false*/, bool bRightToLeftReading /*= false*/ )
{
	LPCTSTR szTitle;
	if (*(valueTitle.c_str())==0) szTitle=NULL; else szTitle=valueTitle.c_str();
	return MsgBox(valueString.c_str(), szTitle, buttons, icon, defBtn, bTopMost, bHelpButton, bRightJustified, bRightToLeftReading);
}



BOOL MsgBeep( EMsgBeep soundStyle/* = mb_SoundSpeaker*/ )
{
	return MessageBeep(soundStyle);
}

// 将一个整数转换为八进制的字符串，字符串空间自动开辟、由 HM 自动管理
LPTSTR Oct(long number)
{
	return StrPrintf(TEXT("%o"), number);
}

// 将一个整数转换为十六进制的字符串，字符串空间自动开辟、由 HM 自动管理
LPTSTR Hex(long number)
{
	return StrPrintf(TEXT("%X"), number);
}

// 获得当前路径字符串，字符串空间自动开辟、由 HM 自动管理
LPTSTR CurDir()
{
	LPTSTR curDirBuff = new TCHAR[1024];
	HM.AddPtr(curDirBuff);
	GetCurrentDirectory(1024, curDirBuff);
	return curDirBuff;
}

// 获取一个自定义资源的字节数据（空间自动开辟、由 HM 自动管理）
//   rSize 不为0时，将从此参数指向的空间返回资源的字节数
unsigned char * LoadResData( UINT idRes, UINT typeRes, unsigned long * rSize/*=0*/ )
{
	return LoadResData(MAKEINTRESOURCE(idRes), MAKEINTRESOURCE(typeRes), rSize); 
}

unsigned char * LoadResData( UINT idRes, LPCTSTR typeRes, unsigned long * rSize/*=0*/ )
{
	return LoadResData(MAKEINTRESOURCE(idRes), typeRes, rSize); 
}

unsigned char * LoadResData( LPCTSTR idRes, UINT typeRes, unsigned long * rSize/*=0*/ )
{
	return LoadResData(idRes, MAKEINTRESOURCE(typeRes), rSize); 
}

unsigned char * LoadResData( LPCTSTR idRes, LPCTSTR typeRes, unsigned long * rSize/*=0*/ )
{
	HRSRC hRes = FindResource(0, idRes, typeRes);
	if (rSize) *rSize = SizeofResource(0, hRes);
	HGLOBAL hGlob = LoadResource(0, hRes);
	return (unsigned char *)LockResource(hGlob); 
}

//////////////////////////////////////////////////////////////////////////
// 时间 函数
//////////////////////////////////////////////////////////////////////////

// 返回当前系统日期、时间的一个字符串，字符串空间自动开辟、由 HM 自动管理
//   若 lpDblTime 不为0，还将当前系统日期、时间转换为 double 
//     （为1601-1-1以来经历的毫秒数）存入它指向的 double 型变量中
//   若 lpTime 不为0，还将当前系统日期、时间存储到它指向的结构中
LPTSTR Now( double *lpDblTime/*=0*/, SYSTEMTIME *lpTime/*=0*/ )
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	if (lpTime) *lpTime = st;
	if (lpDblTime) *lpDblTime = DateTimeDbl(st);

	return StrPrintf(TEXT("%d-%d-%d %02d:%02d:%02d"),	// %02d：不足2位的数字前加0而非加空格补足2位
		st.wYear, st.wMonth, st.wDay, 
		st.wHour, st.wMinute, st.wSecond);
}

// 设置当前系统日期、时间
BOOL NowSet( SYSTEMTIME stNow )
{
	return SetLocalTime (&stNow);
}

// 将一个日期、时间转换为 double 返回（为1601-1-1以来经历的毫秒数）
double DateTimeDbl( SYSTEMTIME stDatetime )
{
	FILETIME ft;
	SystemTimeToFileTime(&stDatetime, &ft);
	return ((double)ft.dwLowDateTime  
		+ 4294967296.0 * ft.dwHighDateTime)/1e4;
}

// 计算两个日期、时间的时间间隔
// style 为指定时间间隔的单位
double DateDiff( eDataTimeDiffStyle style, 
				 SYSTEMTIME stDatetime1, 
				 SYSTEMTIME stDatetime2 )
{
	double diff=DateTimeDbl(stDatetime2)-DateTimeDbl(stDatetime1);
	switch (style)
	{
	case edtYearDiff:
		diff /= 365;	// 继续向下执行其他 case 的语句
	case edtMonthDiff:
		diff /= 12;		// 继续向下执行其他 case 的语句
	case edtDayDiff:
		diff /= 24;		// 继续向下执行其他 case 的语句
	case edtHourDiff:
		diff /= 60;		// 继续向下执行其他 case 的语句
	case edtMinuteDiff:
		diff /= 60;		// 继续向下执行其他 case 的语句
	case edtSecondDiff:
		diff /= 1000;	// 继续向下执行其他 case 的语句
	case edtMilliseconds:
		break;
	}
	return diff;
}

// 返回从 1970-1-1 00:00 经过的秒数
// 若参数 blClockTicks==True，则返回本进程经过的 clock ticks 数
long TimeClock( bool blClockTicks/*=false */ )
{
	if (blClockTicks)
		return clock();
	else
		return (long)time(NULL);  // time() 函数可能返回 long long 类型数据，这里只返回其低4位
}




//////////////////////////////////////////////////////////////////////////
// 自定义字符串 函数
//////////////////////////////////////////////////////////////////////////


// 以 printf 方式制作一个字符串（字符串空间自动开辟、由 HM 自动管理）
LPTSTR cdecl StrPrintf( LPCTSTR szFormat, ... )
{
	if (szFormat==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	// 开辟保存结果字符串的空间 lpszResult
	TCHAR * lpszResult = new TCHAR [2048];
	HM.AddPtr(lpszResult);			// 将该新分配的内存首地址保存到 HM 以自动管理
	
	va_list pArgList;
	va_start(pArgList, szFormat);
	_vsntprintf(lpszResult, 2047, szFormat, pArgList);
	va_end(pArgList);
	
	return lpszResult;
}

// 取字符串的前 length 个字符组成新字符串，函数返回新字符串的首地址
//  （使用Unicode时1个汉字算1个长度，使用ANSI时1个汉字算2个长度）
// 新字符串空间自动开辟、由 HM 自动管理
// length超过字符串长度时返回整个字符串，length<=0 时返回 指向 "\0" （空串）的指针
LPTSTR Left(LPCTSTR szStringSrc, int length)  // LPCTSTR 就是 const TCHAR *
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	int lenSrc = lstrlen(szStringSrc);
	
	// 将要截取的部分长度 length 限定在 0～字符串最大长度
	if (length < 1) length = 0;
	if (length > lenSrc) length = lenSrc;

	// 开辟保存结果字符串的空间 lpszResult
	TCHAR * lpszResult = new TCHAR [length+1];
	HM.AddPtr(lpszResult);			// 将该新分配的内存指针保存到 HM 以自动管理

	// 拷贝内容
	_tcsncpy(lpszResult, szStringSrc, length);
	*(lpszResult+length)='\0';		// 结果字符串最后添加 '\0'

	return lpszResult;
}



// 取字符串的后 length 个字符组成新字符串，函数返回新字符串的首地址
//  （使用Unicode时1个汉字算1个长度，使用ANSI时1个汉字算2个长度）
// 新字符串空间自动开辟、由 HM 自动管理
// length超过字符串长度时返回整个字符串，length<=0 时返回 指向 "\0" （空串）的指针
LPTSTR Right(LPCTSTR szStringSrc, int length)
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	int lenSrc = lstrlen(szStringSrc);
	
	// 将要截取的部分长度 length 限定在 0～字符串最大长度
	if (length < 1) length = 0;
	if (length > lenSrc) length = lenSrc;
	
	// 开辟保存结果字符串的空间 lpszResult
	TCHAR * lpszResult = new TCHAR [length+1];
	HM.AddPtr(lpszResult);	// 将该新分配的内存指针保存到 HM 以自动管理
	
	// 生成结果字符串内容
	lstrcpy(lpszResult, szStringSrc+lenSrc-length);
	return lpszResult;
}



// 取字符串的从第 startPos 个字符起，长 length 个字符组成的字符串，
//   函数返回新字符串的首地址
//  （第一个字符位置为1，使用Unicode时1个汉字算1个长度，
//   使用ANSI时1个汉字算2个长度）
// 新字符串空间自动开辟、由 HM 自动管理
// startPos+length-1 超过字符串长度时返回整个字符串，length<=0 时
//	 或 startPos<=0 或 startPos>源字符串长度 时返回指向 "\0" （空串）的指针
LPTSTR Mid(LPCTSTR szStringSrc, int startPos, int length)
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	int lenSrc = lstrlen(szStringSrc);
	
	// 将要截取的起始位置 startPos 限定在 1～字符串最大长度+1，
	//	 当 startPos 为“字符串最大长度+1”时，截取到的为 ""
	if (startPos < 1 || startPos>lenSrc) startPos = lenSrc+1;
	
	// 将要截取的部分从 startPos 开始、长度为 length 限定在 
	//	字符串长度范围之内（调整 length）
	if (length < 1) length = 0;
	if (startPos+length-1 > lenSrc) length = lenSrc-startPos+1;
	
	// 开辟保存结果字符串的空间 lpszResult
	TCHAR * lpszResult = new TCHAR [length+1];
	HM.AddPtr(lpszResult);			// 该新分配的内存指针保存在 HM
	
	// 拷贝内容
	_tcsncpy(lpszResult, szStringSrc+startPos-1, length);
	*(lpszResult+length)='\0';		// 结果字符串最后添加 '\0'
	
	return lpszResult;
}


// 删除字符串的前导、尾部和所有空格，返回删除后的新字符串，
//   新字符串空间自动开辟、由 HM 自动管理
// bDelOtherSpace=true 时删除所有空格在内的 isspace() 返回真的
//   所有字符；bDelOtherSpace=false 时只删除空格
LPTSTR LTrim( LPCTSTR szStringSrc, bool bDelOtherSpace/*=false*/ )
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错
	
	// 开辟保存结果字符串的空间 lpszResult，拷贝内容
	TCHAR * lpszResult = new TCHAR [lstrlen(szStringSrc)+1];
	HM.AddPtr(lpszResult);			// 该新分配的内存指针保存在 HM
	
	// 在新字符串中执行删除
	TCHAR *p=(TCHAR *)szStringSrc;
	while (*p && (*p == TEXT(' ') || (bDelOtherSpace && _istspace(*p))) ) p++;	// 指向源字符串的第一个非空格
	_tcscpy(lpszResult, p);	// 从 p 的位置拷贝字符串

	// 返回新字符串
	return lpszResult;
}

LPTSTR RTrim( LPCTSTR szStringSrc, bool bDelOtherSpace/*=false*/ )
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错
	
	// 开辟保存结果字符串的空间 lpszResult，拷贝内容
	TCHAR * lpszResult = new TCHAR [lstrlen(szStringSrc)+1];
	HM.AddPtr(lpszResult);			// 该新分配的内存指针保存在 HM
	_tcscpy(lpszResult, szStringSrc);
	
	// 在新字符串中执行删除
	TCHAR *p=(TCHAR *)lpszResult;
	while (*p) p++; p--;	// 指向最后一个字符
	while (*p==TEXT(' ') || (bDelOtherSpace && _istspace(*p))) p--;
	p++;
	*p=0;

	// 返回新字符串
	return lpszResult;

}

LPTSTR Trim( LPCTSTR szStringSrc, bool bDelOtherSpace/*=false*/ )
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错
	
	// 开辟保存结果字符串的空间 lpszResult，拷贝内容
	TCHAR * lpszResult = new TCHAR [lstrlen(szStringSrc)+1];
	HM.AddPtr(lpszResult);			// 该新分配的内存指针保存在 HM
	
	// 在新字符串中执行删除前导空格
	TCHAR *p=(TCHAR *)szStringSrc;
	while (*p && (*p==TEXT(' ') || (bDelOtherSpace && _istspace(*p))) ) p++;	// 指向源字符串的第一个非空格
	_tcscpy(lpszResult, p);	// 从 p 的位置拷贝字符串
	
	// 在新字符串中执行删除尾部空格
	p=(TCHAR *)lpszResult;
	while (*p) p++; p--;	// 指向最后一个字符
	while (*p==TEXT(' ') || (bDelOtherSpace && _istspace(*p))) p--;
	p++;
	*p=0;
	
	// 返回新字符串
	return lpszResult;
}


// 在 szSrc 中，从第 start 个字符开始（第一个字符位置为1），
//	  查找字符串 szFind 的第一次出现的位置（第一个字符位置为1），
//    找到返回值>0，没找到返回0
//  说明：本函数未调用任何库函数（strlen也未调用），提高了运行效率
int InStr(int			start, 
		  LPCTSTR		szSrc, 
		  LPCTSTR		szFind, 
	eBStrCompareMethod	compare/*=bcmTextCompare*/)
{
	TCHAR * pSrcCompPos = (TCHAR *)(szSrc + start - 1);	// 源字符串中开始比较的位置（地址）

	if ( szSrc==0 || szFind==0 ) return 0;
	
	// 要被查找的字符串为空串（szSrc 指向 '\0'），返回 0
	if (*szSrc == 0) return 0;							
	// 要查找的内容为空串（szFind 指向 '\0'），返回 start
	if (*szFind == 0) return start;							

	// 通过移动指针 pSrcComp 到源字符串的末尾，扫描源字符串
	while (*pSrcCompPos)
	{
		// ------------------------------------------------------------------------
		// 比较源字符串从 pSrcCompPos 开始的一段内容，是否与 stringFind 字符串相同
		// ------------------------------------------------------------------------
		TCHAR *p1, *p2;	
		p1 = pSrcCompPos;		// 源字符串从 pSrcCompPos 开始逐字符扫描
		p2 = (TCHAR *)szFind;	// 查找字符串从 szFind 开始逐字符扫描

		// 如果 源字符串 和 查找字符串 都未到末尾，就继续逐个字符比较
		while (*p1 && *p2)
		{
			// 获得要比较的两个字符，=> c1、c2
			TCHAR c1= *p1, c2= *p2;		// 通过将 *p1、*p2 内容存入变量，避免以后反复进行 * 运算，提高运行效率
			if (compare == bcmTextCompare)
			{
				// 若不区分大小写比较，现在将 c1、c2 统一为大写
				if (c1>='a' && c1<='z') c1-=32;
				if (c2>='a' && c2<='z') c2-=32;
			}

			// 如果 c1、c2 不相等，就跳出内层 while
			if (c1 != c2) break;

			// 比较下一个字符
			p1++;
			p2++;
		}	// end while (*p1 && *p2)

		// 跳出内层 while 循环分三种情况：
		//	 (1) 查找字符串 到达末尾，此时不论 源字符串 是否到达末尾：表示已经找到
		//	 (2) 源字符串 到达末尾，但查找字符串没有到达末尾：表示没有找到
		//	 (3) 源字符串 和 目标字符串 都没有到达末尾，说明是由 break 跳出的：表示没有找到
		// 只有在第 (1) 种情况(已经找到)时函数返回，其它两种情况都继续从源字符串的下一个位置开始查找
		if (*p2 == 0)
		{
			// 第 (1) 种情况：查找字符串 到达末尾
			// 函数返回，任务完成
			return pSrcCompPos - szSrc + 1;
		}


		// ------------------------------------------------------------------------
		// 在源字符串中向后移动一个比较位置
		// ------------------------------------------------------------------------
		pSrcCompPos++;
	}	// end while (*pSrcCompPos)

	// 没有找到
	return 0;
}


int InStr(LPCTSTR szSrc,								// InStr 的重载版
		  LPCTSTR szFind,
		  eBStrCompareMethod compare/*=bcmBinaryCompare*/)
{
	return InStr(1, szSrc, szFind, compare);
}


// 在 szSrc 中，从第 start 个字符开始（第一个字符位置为1）到末尾的部分，
//	  查找字符串 szFind 的倒数第一次出现的位置（第一个字符位置为1），
//    找到返回值>0，没找到返回0
//  说明：本函数未调用任何库函数（strlen也未调用），提高了运行效率
int InStrRev(LPCTSTR		szSrc, 
			 LPCTSTR		szFind, 
			 int			start/*=1*/,
			 eBStrCompareMethod	compare/*=bcmTextCompare*/)
{
	TCHAR * pSrcCompStartPos = (TCHAR *)(szSrc + start - 1);	// 源字符串中开始比较的位置（地址）
	TCHAR * pSrcCompPos = pSrcCompStartPos; 
	
	if ( szSrc==0 || szFind==0 ) return 0;
	
	// 要被查找的字符串为空串（szSrc 指向 '\0'），返回 0
	if (*szSrc == 0) return 0;							
	// 要查找的内容为空串（szFind 指向 '\0'），返回 start
	if (*szFind == 0) return start;	
	
	// 移动指针 pSrcComp 到最后一个字符
	while (*pSrcCompPos) pSrcCompPos++; pSrcCompPos--;

	// 通过向前移动指针 pSrcComp 到源字符串的 pSrcCompStartPos，扫描源字符串
	while (pSrcCompPos>=pSrcCompStartPos)
	{
		// ------------------------------------------------------------------------
		// 比较源字符串从 pSrcCompPos 开始的一段内容，是否与 stringFind 字符串相同
		// ------------------------------------------------------------------------
		TCHAR *p1, *p2;	
		p1 = pSrcCompPos;		// 源字符串从 pSrcCompPos 开始逐字符扫描
		p2 = (TCHAR *)szFind;	// 查找字符串从 szFind 开始逐字符扫描
		
		// 如果 源字符串 和 查找字符串 都未到末尾，就继续逐个字符比较
		while (*p1 && *p2)
		{
			// 获得要比较的两个字符，=> c1、c2
			TCHAR c1= *p1, c2= *p2;		// 通过将 *p1、*p2 内容存入变量，避免以后反复进行 * 运算，提高运行效率
			if (compare == bcmTextCompare)
			{
				// 若不区分大小写比较，现在将 c1、c2 统一为大写
				if (c1>='a' && c1<='z') c1-=32;
				if (c2>='a' && c2<='z') c2-=32;
			}
			
			// 如果 c1、c2 不相等，就跳出内层 while
			if (c1 != c2) break;
			
			// 比较下一个字符
			p1++;
			p2++;
		}	// end while (*p1 && *p2)
		
		// 跳出内层 while 循环分三种情况：
		//	 (1) 查找字符串 到达末尾，此时不论 源字符串 是否到达末尾：表示已经找到
		//	 (2) 源字符串 到达末尾，但查找字符串没有到达末尾：表示没有找到
		//	 (3) 源字符串 和 目标字符串 都没有到达末尾，说明是由 break 跳出的：表示没有找到
		// 只有在第 (1) 种情况(已经找到)时函数返回，其它两种情况都继续从源字符串的下一个位置开始查找
		if (*p2 == 0)
		{
			// 第 (1) 种情况：查找字符串 到达末尾
			// 函数返回，任务完成
			return pSrcCompPos - szSrc + 1;
		}
		
		
		// ------------------------------------------------------------------------
		// 在源字符串中向前移动一个比较位置
		// ------------------------------------------------------------------------
		pSrcCompPos--;
	}	// end while (*pSrcCompPos)
	
	// 没有找到
	return 0;
}


// 按分隔字符串 delimiters ，分割一个字符串，生成若干子字符串
//	 各 子字符串 的地址由 ptrStrings[] 数组返回，函数返回子字符串的个数
//   ptrStrings[] 数组下标从1开始，最大到函数返回值
// delimiters 为分隔符字符串，省略或为 "" 时使用：空格
// limit 限制返回子字符串的最大个数，为 -1 时不限制，将返回所有字符串
//   子字符串内存及 ptrStrings[] 数组内存都自动分配、自动管理
// iPtrArrExpandPer 为各字段的指针数组 每次动态扩大的大小，应估计字段
//   个数来设置以提高效率
//
// 连续调用 Split，第二次及以后调用并传递上次的 ptrStrings 参数时，
//   本函数将自动 free 前一次的 ptrStrings 参数的指针数组的空间，
//   及指针数组的各元素所指空间
//
// 用法举例：================================
// 	TCHAR ** s;
// 	int n,i;
// 	n=Split(TEXT("a,bc, d,efg, 123"), s, TEXT(","));
// 	for (i=1;i<=n;i++)
// 		MsgBox(s[i]);
// ================================================================
int Split(LPCTSTR		stringSrc, 
		  TCHAR		** &ptrStrings,						// 一个字符指针数组，数组每个元素为一个子字符串的首地址，数组将在本函数中被分配空间，下标 [0]～[函数返回值]，[0]空间不用
		  LPCTSTR		delimiters/*=NULL*/,			// 分隔符字符串，省略或为""时使用 空格
		  int		    limit/*=-1*/,					// limit, 限制返回的子字符串总数，–1表示返回所有的子字符串
	eBStrCompareMethod  compare/*=bcmBinaryCompare*/,	// 发现“分隔字符串”的字符串比较是否区分大小写
		  int iPtrArrExpandPer/*=200*/)					// ptrStrings[] 每次扩大的大小
{
	void **ptrArr=0;
	int iArrUbound=0, iSubstrCount=0;
	LPTSTR sep=NULL;

	if (delimiters==NULL ) 
	{
		// 使用空格
		sep = new TCHAR [2];
		sep[0]=' '; sep[1]='\0';
	}
	else if(*delimiters=='\0')
	{
		// 使用空格
		sep = new TCHAR [2];
		sep[0]=' '; sep[1]='\0';
	}
	else
	{
		// 拷贝 delimiters 到 sep
		sep = new TCHAR [lstrlen(delimiters)+1];
		_tcscpy(sep, delimiters);
	}
	
	if (stringSrc == NULL) { ptrStrings = NULL; return 0; 	}
	
	// 如果 ptrStrings 已经被 HM 保存，释放原来的空间
	if ( HM.IsPtrManaged(ptrStrings) )
	{
		void * pStrLast = NULL;	// ptrStrings 的附加数据，即一个连续字符串空间，指针数组各元素都指其中间的某个位置
		pStrLast = (void *)HM.UserData(ptrStrings);
		HM.Free(pStrLast);		// 释放连续字符串空间
		HM.Free(ptrStrings);	// 释放指针数组本身的空间
	}

	// 拷贝源字符串到一个临时空间 => stringBak; stringBak 的空间也记录在 HM 里，
	//   稍后也记录在返回值 ptrStrings 的 HM 的 UserData 里
	//   使既可由 HM 在程序结束自动 free，也可由本函数 free（见上段程序）
	LPTSTR stringBak = new TCHAR [ lstrlen(stringSrc)+1 ];
	HM.AddPtr(stringBak);
	_tcscpy(stringBak, stringSrc);

	// 定义指针数组的初始大小
	iArrUbound = iPtrArrExpandPer;
	Redim(ptrArr, iArrUbound);

	// 在 stringSrc 字符串中查找下一个 sep
	int pos=1, pos2=0;
	pos2 = InStr(pos, stringSrc, sep, compare);
	while (pos2)
	{
		// 将从 pos 到 pos2 的子字符串，存入 ptrArr[]
		// 方法是 ptrArr[] 存 stringBak 的 pos 位置的地址
		// 并将 stringBak 的 pos2 位置设为 '\0'
		iSubstrCount++;
		if (iSubstrCount > iArrUbound)
		{
			// 重定义指针数组 ptrArr 的初始大小
			Redim(ptrArr, iArrUbound+iPtrArrExpandPer, iArrUbound, true);
			iArrUbound = iArrUbound+iPtrArrExpandPer;
		}
		ptrArr[iSubstrCount] = stringBak+pos-1;		// ptrArr[] 存 stringBak 的 pos 位置的地址
		*(stringBak+pos2-1) = TEXT('\0');					// 将 stringBak 的 pos2 位置设为 '\0'
		if (limit>0 && iSubstrCount >= limit) goto FinishSub;

		// pos 向后移动，查找下一个 sep
		pos = pos2 + lstrlen(sep);
		pos2 = InStr(pos, stringSrc, sep, compare);
	}

	// 最后一部分
	pos2 = lstrlen(stringSrc) + 1;
	// 将从 pos 到 pos2 的子字符串，存入 ptrArr[]
	iSubstrCount++;
	if (iSubstrCount > iArrUbound)
	{
		// 重定义指针数组 ptrArr 的初始大小
		Redim(ptrArr, iArrUbound+iPtrArrExpandPer, iArrUbound, true);
		iArrUbound = iArrUbound+iPtrArrExpandPer;
	}
	ptrArr[iSubstrCount] = stringBak+pos-1;		// ptrArr[] 存 stringBak 的 pos 位置的地址
	*(stringBak+pos2-1) = TEXT('\0');					// 将 stringBak 的 pos2 位置设为 '\0'

FinishSub:
	if (iSubstrCount)
	{
		// 重定义指针数组 ptrArr 到所需大小
		Redim(ptrArr, iSubstrCount, iArrUbound, true);

		// 该新分配的内存指针保存在 mHM
		// 同时设置附加数据为指针 stringBak，使下次调用 Split 
		//   并传递同样 ptrArr 时可free本次的 stringBak
		HM.AddPtr(ptrArr, true, (long)stringBak);	
		
		// 返回值：设置参数 ptrStrings 的返回值
		ptrStrings = (TCHAR **)ptrArr;
	}
	else
	{
		// 无子字符串
		Erase(ptrArr);
		ptrStrings = 0;
	}


	if (sep) { delete []sep; sep=NULL; }

	// 返回子字符串的个数
	return iSubstrCount;
}


// 以 delimiter 连接多个字符串，返回连接好的字符串
// 多个字符串的地址由数组 stringSrcArray[] 给出，本函数将连接
//   数组中从下标 arrayIndexStart 到 arrayIndexEnd 的字符串
// delimiter 为 0 时，默认以 "\0" 连接字符串；否则以字符串 delimiter 连接
// bTailDoubleNull 若为 true，则在结果字符串的最后再加一个'\0'（即最后有两个'\0'）
// 结果字符串的内存自动分配、由 HM 自动管理
LPTSTR Join(TCHAR * stringSrcArray[], 
		   const int    arrayIndexEnd, 
		   LPCTSTR delimiter/*=0*/, 
		   const int    arrayIndexStart/*=1*/, 
		   const bool   bTailDoubleNull/*=false*/)
{
	if (stringSrcArray==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	// 获得结果字符串的总长度 => lenResult
	int lenResult = 0;
	int lenDelimiter;
	int i;

	if (delimiter)
		lenDelimiter = lstrlen(delimiter);
	else
		lenDelimiter = 1;					// 字符串中间添加 '\0'（一个字符）

	for(i=arrayIndexStart; i<=arrayIndexEnd; i++)
	{
		if (stringSrcArray[i]==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错
		lenResult += lstrlen(stringSrcArray[i]);
		lenResult += lenDelimiter;
	}
	lenResult -= lenDelimiter;				// 最后一个字符串后面不加 delimiter
	if (bTailDoubleNull) lenResult += 1;	// 如果最后要两个'\0'，再加一个长度

	// 分配结果字符串的内存空间
	TCHAR * stringResult = new TCHAR[lenResult + 1];	// +1为最后的 '\0' 空间（双'\0'时指最后一个'\0'）
	HM.AddPtr(stringResult);						// 该新分配的内存指针保存在 HM

	// 连接字符串
	TCHAR *p = stringResult;
	for(i=arrayIndexStart; i<=arrayIndexEnd; i++)
	{
		// 连接 stringSrcArray[i]
		lstrcpy(p, stringSrcArray[i]);
		p += lstrlen(stringSrcArray[i]);

		// 连接 连接字符串
		if (i<arrayIndexEnd)	// 不在最后一个字符串后连接 delimiter
		{
			if (delimiter)
			{
				// 以字符串 delimiter 作为连接字符
				lstrcpy(p, delimiter);
				p += lstrlen(delimiter);
			}
			else
			{
				// 以 '\0' 作为连接字符
				*p = '\0';
				p++;
			}
		}
	}

	// 添加结尾的 '\0'
	if (bTailDoubleNull) {*p = '\0'; p++;}
	*p = '\0';

	// 返回结果字符串指针
	return stringResult;
}


// 连接字符串，生成连接后的长字符串
// 返回连接好的字符串的首地址，自动分配结果字符串的空间、由 HM 自动管理
// 每次调用可最多连接9个字符串
LPTSTR StrAppend( LPCTSTR str1/*=0*/, 
				  LPCTSTR str2/*=0*/, 
			  	  LPCTSTR str3/*=0*/, 
				  LPCTSTR str4/*=0*/, 
				  LPCTSTR str5/*=0*/, 
				  LPCTSTR str6/*=0*/, 
				  LPCTSTR str7/*=0*/,
				  LPCTSTR str8/*=0*/,
				  LPCTSTR str9/*=0*/ )
{
	// 求结果字符串的总长度 => resultStrLen
	int resultStrLen=0;
	if (str1) resultStrLen+=_tcslen(str1);
	if (str2) resultStrLen+=_tcslen(str2);
	if (str3) resultStrLen+=_tcslen(str3);
	if (str4) resultStrLen+=_tcslen(str4);
	if (str5) resultStrLen+=_tcslen(str5);
	if (str6) resultStrLen+=_tcslen(str6);
	if (str7) resultStrLen+=_tcslen(str7);
	if (str8) resultStrLen+=_tcslen(str8);
	if (str9) resultStrLen+=_tcslen(str9);
	
	// 开辟结果字符串的空间
	TCHAR * resultStr = new TCHAR[resultStrLen+1];
	HM.AddPtr(resultStr);
	memset(resultStr, 0, resultStrLen+1);
	
	// 拷贝连接字符串
	if (str1) _tcscat(resultStr, str1);
	if (str2) _tcscat(resultStr, str2);
	if (str3) _tcscat(resultStr, str3);
	if (str4) _tcscat(resultStr, str4);
	if (str5) _tcscat(resultStr, str5);
	if (str6) _tcscat(resultStr, str6);
	if (str7) _tcscat(resultStr, str7);
	if (str8) _tcscat(resultStr, str8);
	if (str9) _tcscat(resultStr, str9);
	
	// 最后赋值 '\0'
	*(resultStr + resultStrLen)='\0';
	
	// 返回
	return resultStr;
}



// 将 ANSI 或 UTF8 字符串转换为 Unicode，返回结果字符串首地址
//   参数 bToUTF8orANSI 为 false 时转换 ANSI，为 true 时转换 UTF8
//   结果字符串的内存自动分配、由 HM 自动管理
LPWSTR StrConvUnicode(const char * szAnsi, bool bFromUTF8orANSI /*=false*/)   // LPWSTR 就是 unsigned short int *
{
	if (szAnsi==0) {gEmptyWSTR[0]=0; return gEmptyWSTR;}  // 容错

	UINT codePage;
	WCHAR * wszResult=0;  
	int wLen=0; 
	
	if (bFromUTF8orANSI) codePage=CP_UTF8; else codePage=CP_ACP;

	// 获得结果字符串所需字符个数，参数 -1 使函数自动计算 szAnsi 的长度
	wLen = MultiByteToWideChar(codePage, 0, szAnsi, -1, NULL, 0);
	// 分配结果字符串的空间
	wszResult = new WCHAR [wLen+1];
	HM.AddPtr(wszResult);
	// 转换
	MultiByteToWideChar(codePage, 0, szAnsi, -1, wszResult, wLen);
	wszResult[wLen]='\0';

	return wszResult; 
}


// 将 Unicode 字符串转换为 ANSI 或 UTF8，返回结果字符串首地址
//   参数 bToUTF8orANSI 为 false 时转换为 ANSI，为 true 时转换为 UTF8
//   结果字符串的内存自动分配、由 HM 自动管理
char * StrConvFromUnicode(LPCWSTR szUnicode, bool bToUTF8orANSI /*=false*/ )
{
	if (szUnicode==0) {gEmptySTR[0]=0; return gEmptySTR;}  // 容错

	UINT codePage;
	char * szResult=0;
	int aLenBytes=0;

	if (bToUTF8orANSI) codePage=CP_UTF8; else codePage=CP_ACP;

	// 获得结果字符串所需字符个数，参数 -1 使函数自动计算 szUnicode 的长度
	aLenBytes=WideCharToMultiByte(codePage, 0, szUnicode, -1, NULL, 0, NULL, NULL);
	// 分配结果字符串的空间
	szResult = new char [aLenBytes];
	HM.AddPtr(szResult);
	// 转换：用 aLenBytes 因函数需要的是字节数非字符数
	WideCharToMultiByte(codePage, 0, szUnicode, -1, szResult, aLenBytes, NULL, NULL);  

	return szResult;
}

// 将字符串转换为 double 型数值
double Val( LPCWSTR stringVal )
{
	if (stringVal==0) {return 0;}  // 容错

	// 将 stringVal 转换为 ANSI 格式的字符串 => szResult
	char * szResult = NULL;
	int aLenBytes=0;
	double dblResult = 0.0;

	// 获得结果字符串所需字符个数，参数 -1 使函数自动计算 szUnicode 的长度
	aLenBytes=WideCharToMultiByte(CP_ACP, 0, stringVal, -1, NULL, 0, NULL, NULL);
	// 分配结果字符串的空间
	szResult = new char [aLenBytes];
	// 转换：用 aLenBytes 因函数需要的是字节数非字符数
	WideCharToMultiByte(CP_ACP, 0, stringVal, -1, szResult, aLenBytes, NULL, NULL);  
	
	dblResult = atof(szResult);
	delete [] szResult;

	return dblResult;
}

double Val( LPCSTR stringVal )
{
	if (stringVal==0) {return 0;}  // 容错
	return atof(stringVal);
}


// 将各种类型数据转换为字符串
// 返回字符串首地址，字符串空间自动开辟、由 HM 自动管理
LPTSTR Str(char character)
{
	LPTSTR buff=new TCHAR [10];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%c"), character);
	return buff;
}

LPTSTR Str(unsigned short int number)	// TCHAR
{
	LPTSTR buff=new TCHAR [20];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
#ifdef UNICODE
	*buff=number;		// 按字符串的方式输出一个字符
	*(buff+1)='\0';		// 须用 TEXT 宏赋值字符，例TCHAR tch=TEXT('汉');
#else
	_stprintf(buff, TEXT("%u"), number);
#endif
	return buff;
}

LPTSTR Str(int number)
{
	LPTSTR buff=new TCHAR [20];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%d"), number);
	return buff;
}

LPTSTR Str(long number)
{
	LPTSTR buff=new TCHAR [20];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%ld"), number);
	return buff;
}

LPTSTR Str(unsigned int number)
{
	LPTSTR buff=new TCHAR [20];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%u"), number);
	return buff;
}

LPTSTR Str(unsigned long number)
{
	LPTSTR buff=new TCHAR [20];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%lu"), number);
	return buff;
}

LPTSTR Str(float number)
{
	LPTSTR buff=new TCHAR [40];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%.7g"), number);
	return buff;
}

LPTSTR Str(double number)
{
	LPTSTR buff=new TCHAR [40];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%.15g"), number);
	return buff;
}

LPTSTR Str(long double number)
{
	LPTSTR buff=new TCHAR [40];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_stprintf(buff, TEXT("%.15g"), number);
	return buff;
}

LPTSTR Str( LPCTSTR sText )	// 原样拷贝后返回
{
	if (sText==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	LPTSTR buff =0;
	buff = new TCHAR [lstrlen(sText)+1];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_tcscpy(buff, sText); 

	return buff;
}



// 将字符串中的字母全部转换为大写（UCase）、小写（LCase）
//   返回转换后的新字符串，新字符串空间自动开辟、由 HM 自动管理
LPTSTR LCase( LPCTSTR szStringSrc )
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	// 拷贝一份字符串到 buff
	LPTSTR buff =0;
	buff = new TCHAR [lstrlen(szStringSrc)+1];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_tcscpy(buff, szStringSrc); 
	
	// 转换 buff 中的字符串
	CharLower(buff);

	// 返回新字符串（buff）
	return buff;
}

LPTSTR UCase( LPCTSTR szStringSrc )
{
	if (szStringSrc==0) {gEmptyTSTR[0]=0; return gEmptyTSTR;}  // 容错

	// 拷贝一份字符串到 buff
	LPTSTR buff =0;
	buff = new TCHAR [lstrlen(szStringSrc)+1];
	HM.AddPtr(buff);	// 用 HM 管理动态空间
	_tcscpy(buff, szStringSrc); 
	
	// 转换 buff 中的字符串
	CharUpper(buff);
	
	// 返回新字符串（buff）
	return buff;
}


// 替换字符串，返回替换后的字符串
// 替换后的字符串空间自动分配、由 HM 自动管理
LPTSTR Replace( LPCTSTR szStringSrc, 
			    LPCTSTR szFind, 
				LPCTSTR szReplaceWith, 
				int start/*=1*/, 
				int countLimit/*=-1*/, 
				eBStrCompareMethod compare/*=bcmBinaryCompare*/ )
{
	// ======================== 准备工作 ========================
	// 若源字符串指针为0，或源字符串为 ""，返回 ""
	if (szStringSrc == 0) { gEmptyTSTR[0]=0; return gEmptyTSTR; } 
	if (*szStringSrc == 0) { gEmptyTSTR[0]=0; return gEmptyTSTR;} 
	
	// 若要替换为的子字符串指针为空，设置要替换为的子字符串为空串
	if (szReplaceWith == 0) { gEmptyTSTR[0]=0; szReplaceWith=gEmptyTSTR; }

	// 拷贝一份字符串到 buff
	//   buff 长度至少为 源字符串长度 + 一个 szReplaceWith 的长度
	LPTSTR buff = 0;
	int iFindLen = lstrlen(szFind);
	int iReplLen = lstrlen(szReplaceWith);
	int buffLen = lstrlen(szStringSrc) + iReplLen +1;
	buff = new TCHAR [buffLen];
	// 最后再用 HM.AddPtr(buff); 用 HM 管理动态空间，因可能还要扩大空间
	_tcscpy(buff, szStringSrc); 

	// 若要查找的子字符串指针为空，或子字符串为 ""，返回源字符串
	if (szFind == 0) return buff;
	if (*szFind == 0) return buff;

	// 设置将来若 buff 空间不足时，扩大空间时的扩大步长 => ibuffLenExpandPer
	int ibuffLenExpandPer = 200;	
	if (iReplLen > ibuffLenExpandPer) ibuffLenExpandPer=iReplLen;
	
	// ======================== 查找和替换 ========================
	// 将源字符串中的字符逐一拷贝到 buff；
	// 但若遇到 szFind，不拷贝 szFind 而拷贝 szReplaceWith
	TCHAR *p=(TCHAR *)(szStringSrc+start-1), *p1, *p2, *p3;
	TCHAR *buffWrite=buff;
	int iReplacedCount=0; 
	while (*p)
	{
		// 在源字符串的 p 位置查找是否与 szFind 相等
		p1=p; p2=(TCHAR *)szFind;
		while (*p1 && *p2)
		{
			// 获得要比较的两个字符，=> c1、c2
			TCHAR c1= *p1, c2= *p2;		// 通过将 *p1、*p2 内容存入变量，避免以后反复进行 * 运算，提高运行效率
			if (compare == bcmTextCompare)
			{
				// 若不区分大小写比较，现在将 c1、c2 统一为大写
				if (c1>='a' && c1<='z') c1-=32;
				if (c2>='a' && c2<='z') c2-=32;
			}
			
			// 如果 c1、c2 不相等，就跳出内层 while
			if (c1 != c2) break;
			
			// 比较下一个字符
			p1++;
			p2++;
		}	// end while (*p1 && *p2)

		// 跳出内层 while 循环分三种情况：
		//	 (1) 查找字符串 到达末尾，此时不论 源字符串 是否到达末尾：表示已经找到
		//	 (2) 源字符串 到达末尾，但查找字符串没有到达末尾：表示没有找到
		//	 (3) 源字符串 和 目标字符串 都没有到达末尾，说明是由 break 跳出的：表示没有找到
		// 只有在第 (1) 种情况(已经找到)时需要替换，其它两种情况都直接拷贝源字符串的 *p 
		if (*p2 == 0)
		{
			// 第 (1) 种情况：执行替换，
			//   即拷贝 szReplaceWith 而不拷贝源字符串，
			//   并调整 p 的位置越过一个 szFind 的长度
			p3 = (TCHAR *)szReplaceWith;
			while (*p3)
			{  *buffWrite = *p3; buffWrite++; p3++;  }
			p += (iFindLen-1);	// -1 是因为最外层 while 最后还要执行 p++
			
			// 计数替换次数和判断替换限制条件
			iReplacedCount++;
			if (countLimit>0 && iReplacedCount>=countLimit) break; // 跳出最外层 while
		}
		else
		{
			// 第 (2)、(3) 种情况：直接拷贝源字符串
			*buffWrite = *p; buffWrite++;
		}

		// buff 的剩余长度至少要多出一个 szReplaceWith 的长度
		if ( buffLen - (buffWrite-buff+1) < iReplLen )
		{
			// 需要扩大 buff
			// buff 的长度至少要多出一个 szReplaceWith 的长度
			int buffSizeLast = buffLen * sizeof(TCHAR);
			buffLen += ibuffLenExpandPer;			// 新空间大小（字符个数）
			TCHAR *buff2=new TCHAR [buffLen];		// 开辟新空间
			HM.CopyMem(buff2, buff, buffSizeLast);	// 将老空间内容拷贝到新空间
			buffWrite = buff2 + (buffWrite-buff);	// 使 buffWrite 指向新空间的相同位置
			delete []buff;	// 删除老空间
			buff = buff2;		// 最后会执行 HM.AddPtr(buff); 管理新空间
		}

		// 源字符串指针 p 指向下一字符
		p++;
	}	// end while (*p)

	*buffWrite = '\0';

	// ======================== 返回结果 ========================
	HM.AddPtr(buff);
	return buff;
}	




//////////////////////////////////////////////////////////////////////////
// 剪贴板 函数
//////////////////////////////////////////////////////////////////////////

LPTSTR ClipboardGetText()
{
	if (! OpenClipboard(NULL)) 	// OpenClipboard 失败
	{
		// 返回 ""
		gEmptyTSTR[0] = TEXT('\0');
		return gEmptyTSTR;
	}

#ifdef UNICODE
	HANDLE hHandle = GetClipboardData(CF_UNICODETEXT);
#else
	HANDLE hHandle = GetClipboardData(CF_TEXT);
#endif
	
    if (hHandle == NULL) 
	{
		// 返回 ""
		gEmptyTSTR[0] = TEXT('\0');
		CloseClipboard();
		return gEmptyTSTR;
	}
	else
	{
        LPTSTR szGlobal = (LPTSTR)GlobalLock(hHandle);
		if (szGlobal == NULL )
		{
			// 返回 ""
			gEmptyTSTR[0] = TEXT('\0');
			GlobalUnlock(hHandle);
			CloseClipboard();
			return gEmptyTSTR;
		}
		else
		{
			int iLen = _tcslen(szGlobal);
			LPTSTR szText = new TCHAR [iLen + 1];
			HM.AddPtr(szText, true);
			_tcscpy(szText, szGlobal);
			GlobalUnlock(hHandle);
			CloseClipboard();
			return szText;
		}
    }
}

void ClipboardSetText( LPCTSTR szText )
{
	if (! OpenClipboard(NULL)) return;		// OpenClipboard 失败
	EmptyClipboard();

	HANDLE hHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (_tcslen(szText) + 1) * sizeof(TCHAR));
	LPTSTR szGlobal = (LPTSTR)GlobalLock(hHandle);
	_tcscpy(szGlobal, szText);
	GlobalUnlock(hHandle);
	
#ifdef UNICODE
	SetClipboardData (CF_UNICODETEXT, hHandle);
#else
    SetClipboardData (CF_TEXT, hHandle);
#endif

	CloseClipboard();
}

void ClipboardSetText( tstring stringText )
{
	ClipboardSetText(stringText.c_str());
}


void ClipboardSetBmp( HBITMAP hBmp )
{
	if (! OpenClipboard(NULL)) return;		// OpenClipboard 失败
	EmptyClipboard();
	SetClipboardData (CF_BITMAP, hBmp);
	CloseClipboard();
}

HBITMAP ClipboardGetBmp()
{
	HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	CloseClipboard();
	return hBitmap;
}



void ClipboardClear()
{
	if (! OpenClipboard(NULL)) return;		// OpenClipboard 失败
	EmptyClipboard();
	CloseClipboard();
}



//////////////////////////////////////////////////////////////////////////
// 自定义 动态数组 函数
//////////////////////////////////////////////////////////////////////////

// template <typename REDIMTYPE>
// int Redim( REDIMTYPE *  &arr, int uboundCurrent, int toUBound, bool preserve=false )	// template 函数定义要在头文件中




//////////////////////////////////////////////////////////////////////////
// 文件处理 函数
// 所有文件位置都从 0 开始
//////////////////////////////////////////////////////////////////////////


// 全局哈希表，保存所有打开的文件。Key=句柄，字符串数据为文件名
CBHashLK mEFOpenedFiles;	


HANDLE EFOpen( LPCTSTR szFileName, 
			   eEFOpenStyle openStyle /*= EF_OpStyle_Binary*/, 
			   bool bShowMsgIfFail /*= true*/, 
			   LPCTSTR szFailInfo /*= TEXT("无法打开文件。")*/, 
			   eEFShareStyle shareStyle /*= EF_ShareCanReadWrite*/, 
			   bool bDeleteOnClose /*= false*/ )
{
	DWORD iAccess=0, iDisposition=0, iAttr=0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	switch(openStyle)
	{
	case EF_OpStyle_Append:
		iAccess = GENERIC_WRITE;
		iDisposition = OPEN_EXISTING;
		iAttr = FILE_FLAG_SEQUENTIAL_SCAN;   // Indicates that the file is to be accessed sequentially from beginning to end. The system can use this as a hint to optimize file caching. If an application moves the file pointer for random access, optimum caching may not occur; however, correct operation is still guaranteed.
		break;
	case EF_OpStyle_Binary:
        iAccess = GENERIC_READ | GENERIC_WRITE;
        iDisposition = OPEN_ALWAYS;
        iAttr = FILE_FLAG_RANDOM_ACCESS;     // Indicates that the file is accessed randomly. The system can use this as a hint to optimize file caching.
		break;
	case EF_OpStyle_Input:
        iAccess = GENERIC_READ;
        iDisposition = OPEN_EXISTING;
        iAttr = FILE_FLAG_SEQUENTIAL_SCAN;
		break;
	case EF_OpStyle_Output:
        iAccess = GENERIC_WRITE;
        iDisposition = CREATE_ALWAYS;
        iAttr = FILE_FLAG_SEQUENTIAL_SCAN;
		break;
	default:
        goto errH;
		break;
	}

	if (bDeleteOnClose) iAttr |= FILE_FLAG_DELETE_ON_CLOSE;
	hFile = CreateFile(szFileName, iAccess, (DWORD)shareStyle, 0, iDisposition, iAttr, 0);
	if (hFile == INVALID_HANDLE_VALUE) goto errH;

    // 如果是 EF_OpStyle_Append 模式，将文件读写指针移动到末尾（位置从0开始，
	// 有效位置为从0-文件长度-1，以下为移动到文件长度的位置即指向下次写入的位置）
	if (openStyle == EF_OpStyle_Append) SetFilePointer(hFile, 0, 0, FILE_END);

	// 将文件打开信息存入哈希表
	mEFOpenedFiles.Add(0, (long)hFile, 0, 0, szFileName);

	return hFile;

errH:
	int sizeErr = (lstrlen(szFileName) + lstrlen(szFailInfo) + 2);
	TCHAR * szErr = new TCHAR [sizeErr+1];
	_tcscpy(szErr, szFileName);
	_tcscat(szErr, TEXT("\r\n"));
	_tcscat(szErr, szFailInfo);
	if (bShowMsgIfFail) MsgBox(szErr);
	delete []szErr;

	//sFailAddiInfo = strTemp '从 sFailAddiInfo 可返回失败信息
        
    return INVALID_HANDLE_VALUE;
}

HANDLE EFOpen( tstring stringFileName, eEFOpenStyle openStyle /*= EF_OpStyle_Binary*/, bool bShowMsgIfFail /*= true*/, tstring stringFailInfo /*= TEXT("无 ù蚩募?)*/, eEFShareStyle shareStyle /*= EF_ShareCanReadWrite*/, bool bDeleteOnClose /*= false*/ )
{
	return EFOpen(stringFileName.c_str(), openStyle, bShowMsgIfFail, stringFailInfo.c_str(), shareStyle, bDeleteOnClose);
}


void EFClose( HANDLE hFile, bool bCloseAll/* = false*/ )
{
	if (bCloseAll)
	{
		// 关闭所有文件
		int i; 
		for (i=1; i<=mEFOpenedFiles.Count(); i++)
			CloseHandle((void *)mEFOpenedFiles.IndexToKey(i, false));
		mEFOpenedFiles.Clear();
	}
	else
	{
		// 仅关闭文件 hFile
		CloseHandle(hFile);

		// 将文件打开信息从哈希表中删除
		mEFOpenedFiles.Remove((long)hFile, false);
	}
}


LONGLONG EFLOF( HANDLE hFile )
{
	LARGE_INTEGER li = {0,0};
	li.LowPart = GetFileSize(hFile, (LPDWORD)&li.HighPart);
	return li.QuadPart;
}


LONGLONG EFSeekGet( HANDLE hFile )
{
	LARGE_INTEGER li={0,0};
	li.LowPart = SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_CURRENT);
	if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)	// 调用出错
		li.QuadPart = -1;	
	return li.QuadPart; 
}


int EFSeekSet( HANDLE hFile, 
			   LONGLONG llReadPos, 
			   int iShowResume /*= 1*/, 
			   LPCTSTR szFailInfo /*= TEXT("无法移动文件读写指针。")*/ )
{
	LARGE_INTEGER li;
	bool blSuccess = false;
	EDlgBoxCmdID ms = idOk;

	li.QuadPart = llReadPos;
	do
    {
		li.LowPart = SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
		if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)	// 调用出错
		{
			blSuccess = false;
			ms = _EFRetryBox(hFile, iShowResume, szFailInfo, TEXT("Error") );
		}
		else	// if (li.LowPart == 0xFFFFFFFF && GetLa ...
		{
			blSuccess = true;
		}		// end if (li.LowPart == 0xFFFFFFFF && GetLa ...
	}
	while (!blSuccess && ms == idRetry );
	
	if (blSuccess)
		return 1;
	else
		{	if (ms == idIgnore) return -2; else return -1;  }
}


bool EFSeekSetEnd( HANDLE hFile )
{
	LARGE_INTEGER li={0,0};
	li.LowPart = SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_END);
	if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)	// 调用出错
		return false;
	else
		return true;
}


bool EFEOF( HANDLE hFile )
{
	// 获得文件当前读写位置
	LARGE_INTEGER li={0,0};
	li.LowPart = SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_CURRENT);
	if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)	// 调用出错
		return false;

	// 获得文件长度
	LONGLONG fileLen = EFLOF(hFile);

	// 判断当前读写位置是否为文件末尾
	return (li.QuadPart>=fileLen);
}


bool EFBOF( HANDLE hFile )
{
	// 获得文件当前读写位置
	LARGE_INTEGER li={0,0};
	li.LowPart = SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_CURRENT);
	if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)	// 调用出错
		return false;
	return (li.QuadPart == 0);
}


LONG EFGetBytes( HANDLE hFile, 
				 LONGLONG llReadPos, 
				 char * const pBuff, 
				 LONG iBuffMax /*= 131072*/, 
				 int iShowResume /*= 1*/ ,
				 LPCTSTR szFailInfo /*= TEXT("无法读取文件。")*/)
{
	LONG iBytesRead=0;	// 真实读取了多少字节
	int r=0;
	EDlgBoxCmdID ms = idOk;

	// 移动文件读写指针
	if (llReadPos >= 0)  // llReadPos<0 时不移动读写指针，为从当前读写位置开始读
	{
		r = EFSeekSet(hFile, llReadPos, iShowResume, szFailInfo);  // 此函数若出错会弹出重试对话框
		if (r < 0)  // 移动读写指针失败
			{ if (r == -2)  return -2; else return -1; }
	}
	
	// 读取数据
	ms = idOk;
	do 
	{
		r = ReadFile(hFile, pBuff, iBuffMax, (LPDWORD)&iBytesRead, 0);  // 出错 ReadFile 返回 0，成功返回 非0
		int ii=GetLastError();
		if (r==0)
			ms = _EFRetryBox(hFile, iShowResume, szFailInfo, TEXT("Read Error") );
		else
			break;
	} while ( ms==idRetry );
	
	// 返回函数值为实际读取的字节数
	return iBytesRead;
}


LONG EFPutBytes( HANDLE hFile, 
				 LONGLONG llWritePos, 
				 char * const pBuff, 
				 LONG iBuffLen /*= 131072*/, 
				 int iShowResume /*= 1*/, 
				 LPCTSTR szFailInfo /*= TEXT("无法读取文件。")*/)
{
	LONG iBytesWritten=0;	// 真实写入了多少字节
	int r=0;
	EDlgBoxCmdID ms = idOk;
	
	// 移动文件读写指针
	if (llWritePos >= 0)  // llWritePos<0 时不移动读写指针，为从当前读写位置开始写
	{
		r = EFSeekSet(hFile, llWritePos, iShowResume, szFailInfo);  // 此函数若出错会弹出重试对话框
		if (r < 0)  // 移动读写指针失败
			{ if (r == -2)  return -2; else return -1; }
	}

	// 写入数据
	ms = idOk;
	do 
	{
		r = WriteFile(hFile, pBuff, iBuffLen, (LPDWORD)&iBytesWritten, 0);  // 出错 ReadFile 返回 0，成功返回 非0
		if (r==0)
			ms = _EFRetryBox(hFile, iShowResume, szFailInfo, TEXT("Write Error") );
		else
			break;
	} while ( ms==idRetry );

	// 返回函数值为实际写入的字节数
	return iBytesWritten;
}


BOOL EFFlushFile( HANDLE hFile )
{
	return FlushFileBuffers(hFile);
}


BOOL EFSetEndOfFile( HANDLE hFile, LONGLONG llPosEnd )
{
	if (llPosEnd >=0)  // 设置新的文件读写位置
        if (EFSeekSet(hFile, llPosEnd, 0) <0 ) return false; // 设置新位置时若出错不弹出重试对话框
    return SetEndOfFile(hFile);
}



EDlgBoxCmdID _EFRetryBox( HANDLE hFile, int iShowResume, LPCTSTR szFailInfo, LPCTSTR szTitle )
{
	if (iShowResume==0) return idOk;
	
	int sizeErr = (lstrlen(szFailInfo) + 50);	// 错误信息字符串总长度
	
	// 从哈希表中获取 hFile 的文件名字符串 => szFile
	LPTSTR szFile = mEFOpenedFiles.ItemStr((long)hFile, false);
	if (szFile) sizeErr += lstrlen(szFile); 
	
	// 制作文件读写位置字符串 => szPos
	TCHAR szPos[30];						// 表示文件当前位置的字符串
	_stprintf(szPos, TEXT("%.15g"), (double)EFSeekGet(hFile) );
	
	// 构造提示信息 => szErr
	TCHAR * szErr = new TCHAR [sizeErr+1];
	*szErr = 0;		// 将 szErr 的空间准备为空串
	if (szFile) { _tcscat(szErr, szFile); _tcscat(szErr, TEXT("\r\n")); }
	_tcscat(szErr, szFailInfo); _tcscat(szErr, TEXT("\r\n"));
	_tcscat(szErr, TEXT("position:")); 	_tcscat(szErr, szPos);
	
	// 显示提示框
	if (iShowResume==1)
		return MsgBox(szErr, szTitle, mb_RetryCancel, mb_IconError);
	else if (iShowResume==2)
		return MsgBox(szErr, TEXT("Error"), mb_AbortRetryIgnore, mb_IconError);
	else
		return idOk;	// 出错或未弹出提示框，都返回 idOK
	
	delete []szErr;
}


LONG EFPrint( HANDLE hFile, 
			  LPCTSTR szText, 
			  eEFLineFeed styleLineFeed /*= EF_LineSeed_CrLf*/, 
			  LONGLONG llWritePos /*= -1*/, 
			  int iShowResume /*= 1*/, 
			  LPCTSTR szFailInfo /*= TEXT("无法向文件中写入字符串。")*/ )
{
	// 使用 char 一律写入 Ansi 格式的文本
	char * buff = 0; 
	int iLenBytes=0;

	if (szText==0 || *szText==0) 
	{
		// 无字符串或字符串为空串，都打印空串	
		iLenBytes = 1;
		buff = new char [iLenBytes + 2]; 
	}
	else
	{
		#ifdef UNICODE
			// 获得结果字符串所需字符个数，参数 -1 使函数自动计算 szText 的长度
			// iLenBytes 为包含最后的 \0 的总共字节数
			iLenBytes = WideCharToMultiByte(CP_ACP, 0, szText, -1, NULL, 0, NULL, NULL);
			// 分配结果字符串的空间
			buff = new char [iLenBytes + 2];  // 最多比文本长度+\0的长度多2个字节：\0(\r) \n \0
			// 转换为 Ansi 的字符串：用 aLenBytes 因函数需要的是字节数非字符数
			WideCharToMultiByte(CP_ACP, 0, szText, -1, buff, iLenBytes, NULL, NULL);  
		#else
			iLenBytes = lstrlen(szText) + 1;
			buff = new char [ iLenBytes + 2 ];  // 最多比文本长度+\0的长度多2个字节：\0(\r) \n \0
			strcpy(buff, szText);
		#endif
	}
    
    switch (styleLineFeed)
	{
	case EF_LineSeed_Lf:
		buff[iLenBytes-1] = 10;	// \n
		buff[iLenBytes] = 0;	// \0
		iLenBytes += 1;
		break;
	case EF_LineSeed_Cr:
		buff[iLenBytes-1] = 13;	// \r
		buff[iLenBytes] = 0;	// \0
		iLenBytes += 1;
		break;
	case EF_LineSeed_CrLf:
		buff[iLenBytes-1] = 13;	// \n
		buff[iLenBytes] = 10;	// \0
		buff[iLenBytes+1] = 0;	// \0
		iLenBytes += 2;
		break;
	case EF_LineSeed_None:
		break;
	}
	
    // iLenBytes-1 去掉最后的 \0 字节
	LONG ret = EFPutBytes(hFile, llWritePos, buff, iLenBytes-1, iShowResume, szFailInfo);
	delete []buff;
	return ret;
}

LONG EFPrint( HANDLE hFile, tstring stringText, eEFLineFeed styleLineFeed /*= EF_LineSeed_CrLf*/, LONGLONG llWritePos /*= -1*/, int iShowResume /*= 1*/, tstring stringFailInfo /*= TEXT("无 ㄏ蛭募行慈胱??)*/ )
{
	return EFPrint(hFile, stringText.c_str(), styleLineFeed, llWritePos, iShowResume, stringFailInfo.c_str());
}

tstring StrS( char character )
{
	tstring sRet;
	TCHAR buff[10];
	_stprintf(buff, TEXT("%c"), character);
	sRet=buff;
	return sRet;
}

tstring StrS( unsigned short int number )
{
	tstring sRet;
	TCHAR buff[20];
#ifdef UNICODE
	*buff=number;		// 按字符串的方式输出一个字符
	*(buff+1)='\0';		// 须用 TEXT 宏赋值字符，例TCHAR tch=TEXT('汉');
#else
	_stprintf(buff, TEXT("%u"), number);
#endif
	sRet=buff;
	return sRet;
}

tstring StrS( int number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%d"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( long number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%ld"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( unsigned int number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%u"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( unsigned long number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%lu"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( float number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.7g"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( double number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( long double number )
{
	tstring sRet;
	TCHAR buff[20];
	_stprintf(buff, TEXT("%.15g"), number);
	sRet=buff;
	return sRet;
}

tstring StrS( LPCTSTR sText )
{
	tstring sRet = sText;
	return sRet;
}





//////////////////////////////////////////////////////////////////////////
// CBHashLK 类的实现: 长整型键值的哈希表
//
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Static 常量定值
//////////////////////////////////////////////////////////////////////
const int   CBHashLK::mcIniMemSize = 7;					// 初始 mem[] 的大小
const int   CBHashLK::mcMaxItemCount = 100000000;		// 最多元素个数（可扩大此值到 long 表示的范围之内）
const float CBHashLK::mcExpandMaxPort = 0.75;			// 已有元素个数大于 0.75*memCount 时就扩大 mem[] 的空间
const int   CBHashLK::mcExpandCountThres = 10000;		// 扩大 mem[] 空间时，若 memCount 小于此值则每次扩大到 memCount*2；若 memCount 大于此值则每次扩大到 Count+Count/2
const int   CBHashLK::mcExpandCountThresMax = 10000000;	// 扩大 mem[] 空间时，若 memCount 已大于此值，则每次不再扩大到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBHashLK::mcExpandBigPer = 1000000;			// 扩大 mem[] 空间时，若 memCount 已大于 mcExpandCountThresMax，则每次不再扩大到到 Count+Count/2，而只扩大到 Count+mcExpandBigPer
const int   CBHashLK::mcExpandMem2Per = 10;				// 每次扩大 mem2[] 的大小
const int   CBHashLK::mcSeqMax = 5;						// 顺序检索最大值


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CBHashLK::CBHashLK(int memSize/*=0*/)
{
	mArrTable=0; mArrTableCount = -1;	// 无缓存数组

	memUsedCount = 0; 
	mem2 = 0;
	memCount2 = 0;
    memUsedCount2 = 0;
	
	if (memSize)
	{
		// 初始定义 memSize 个的 mem[] 空间，无 mem2[] 的空间
		RedimArrMemType(mem, memSize);
		memCount = memSize;
	}
	else
	{
		// 初始定义 mcIniMemSize 个的 mem[] 空间，无 mem2[] 的空间
		RedimArrMemType(mem, mcIniMemSize);
		memCount = mcIniMemSize;
	}
}

CBHashLK::~CBHashLK()
{
	Clear();
	// Clear() 函数中，重新开辟了初始大小的 mem[] 空间，再将其删除即可
	if (mem) delete[] mem;
	memCount = 0;
}



//////////////////////////////////////////////////////////////////////
// 公有方法
//////////////////////////////////////////////////////////////////////

void CBHashLK::AlloMem(int memSize )
{
	/*
	程序初始化时只定义了 mcIniMemSize 大小的 l_Mem[]，以后随使用随自动扩 \
	  大；但若事先知道有多大，可以先用本函数定义足够大以免以后不断 \
	  自动扩大费时；注意这时要比预用的元素个数多定义一些，否则分配空间 \
	  时若空间冲突本类还会自动扩大
	此函数也可用于截断 l_Mem[] 后面没有使用的空间
      注：memSize <= memUsedCount 时，拒绝重新定义，以确保数据不会丢失 
	*/

	if (memSize <= memUsedCount || memSize > mcMaxItemCount) return;
	int iPreMemCount;
	iPreMemCount = memCount;

	// ReDim Preserve mem(1 To memSize)
	RedimArrMemType(mem, memSize, memCount, true);
	memCount = memSize;

	if (iPreMemCount <= memCount) ReLocaMem(iPreMemCount); else ReLocaMem(memCount);

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;

	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}

bool CBHashLK::Add( DataType data, KeyType key/*=0*/, DataLongType dataLong/*=0*/, DataLong2Type dataLong2/*=0*/, LPCTSTR dataStr/*=NULL*/, LPCTSTR dataStr2/*=NULL*/, double dataDouble/*=0.0*/, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	
	// 哈希表中的数据个数最多不能超过 mcMaxItemCount
	if (memUsedCount + memUsedCount2 >= mcMaxItemCount)
    {		
		if (raiseErrorIfNotHas)  throw (unsigned char)7;	// 超出内存
		return false;
	}
	
	// 当前哈希表中不能有相同的“键”存在
	if (IsKeyExist(key))
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)5;	// 无效的过程调用或参数
		return false;
	}
	
	// 通过调用 AlloMemIndex 函数获得一个可用空间的下标：idx
	idx = AlloMemIndex(key);
	
	if (idx > 0)
	{
		// 获得的下标值为正数时，使用 mem[] 数组的空间
		mem[idx].Data = data;
		mem[idx].DataLong = dataLong;
		mem[idx].DataLong2 = dataLong2;
		mem[idx].DataDouble = dataDouble;
		mem[idx].Key = key;

		SaveItemString(&(mem[idx].DataStr), dataStr);
		SaveItemString(&(mem[idx].DataStr2), dataStr2);

		mem[idx].Used = true;

		memUsedCount = memUsedCount + 1;
	}
	else if (idx < 0)
	{
		// 获得的下标值为负数时，使用 mem2[] 数组的空间，_
		// 空间下标为 idx 的绝对值
		mem2[-idx].Data = data;
		mem2[-idx].DataLong = dataLong;
		mem2[-idx].DataLong2 = dataLong2;
		mem2[-idx].DataDouble = dataDouble;
		mem2[-idx].Key = key;

		SaveItemString(&(mem2[-idx].DataStr), dataStr);
		SaveItemString(&(mem2[-idx].DataStr2), dataStr2);

		mem2[-idx].Used = true;

		memUsedCount2 = memUsedCount2 + 1;
	}
	else // idx == 0
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)9;	// 下标越界：无法分配新数据空间
	}
	
	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;

	// 函数返回成功
    return true;
}


CBHashLK::DataType CBHashLK::Item( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].Data;
	else if (idx < 0)
		return mem2[-idx].Data;
	else
		return 0;
}


CBHashLK::DataLongType CBHashLK::ItemLong( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataLong;
	else if (idx < 0)
		return mem2[-idx].DataLong;
	else
		return 0;
}


CBHashLK::DataLong2Type CBHashLK::ItemLong2( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataLong2;
	else if (idx < 0)
		return mem2[-idx].DataLong2;
	else
		return 0;
}

double CBHashLK::ItemDouble( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataDouble;
	else if (idx < 0)
		return mem2[-idx].DataDouble;
	else
		return 0;

}


LPTSTR CBHashLK::ItemStr( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataStr;
	else if (idx < 0)
		return mem2[-idx].DataStr;
	else
		return 0;

}

LPTSTR CBHashLK::ItemStr2( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int idx;
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx > 0)
		return mem[idx].DataStr2;
	else if (idx < 0)
		return mem2[-idx].DataStr2;
	else
		return 0;

}


// 判断一个 Key 是否在当前集合中存在
bool CBHashLK::IsKeyExist( KeyType key )
{
	int idx;
	idx = GetMemIndexFromKey(key, false);
    return (idx != 0);
}

bool CBHashLK::Remove( KeyType key, bool raiseErrorIfNotHas/*=True*/ )
{
	int idx;
	
	// 调用 GetMemIndexFromKey 函数获得“键”为 Key 的数据所在空间的下标
	idx = GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (idx == 0)
		return false;
	else if (idx > 0)
	{
		// 哈希表中“键”为 Key 的数据在 mem[] 数组中，下标为 idx
		mem[idx].Used = false;
		mem[idx].Key = 0;
		SaveItemString(&(mem[idx].DataStr), 0);  // 第二个参数为0，删除 mem[idx].DataStr 指向的空间，并让 mem[idx].DataStr=0;
		SaveItemString(&(mem[idx].DataStr2), 0); 

		memUsedCount = memUsedCount - 1;
	}
	else
	{
		// idx<0 表示：哈希表中“键”为 Key 的数据在 mem2[] 数组中 \
		// 下标为 idx 的绝对值
		// 删除下标为“-idx”的元素

		SaveItemString(&(mem[-idx].DataStr), 0); // 第二个参数为0，删除 mem[-idx].DataStr 指向的空间，并让 mem[-idx].DataStr=0;
		SaveItemString(&(mem[-idx].DataStr2), 0); 

		for(int i=-idx; i<= - 1; i++)
			mem2[i] = mem2[i+1];
		mem2[memUsedCount2].DataStr=0;		// 直接设置为0，因此空间已经传递给上一元素
		mem2[memUsedCount2].DataStr2=0;		// 直接设置为0，因此空间已经传递给上一元素
		memUsedCount2 = memUsedCount2 - 1;
	}

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;

	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;

	// 函数返回成功
	return true;
}

void CBHashLK::StartTraversal()
{
	// 开始用 NextXXX ... 方法遍历
    mTravIdxCurr = 1;
}


CBHashLK::DataType CBHashLK::NextItem( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 Data
    // 若 bRetNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].Data;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].Data;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

CBHashLK::DataLongType CBHashLK::NextItemLong( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataLong;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataLong;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}


CBHashLK::DataLong2Type CBHashLK::NextItemLong2( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataLong2;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataLong2;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

double CBHashLK::NextItemDouble( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataDouble;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataDouble;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

LPTSTR CBHashLK::NextItemStr( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataStr;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataStr;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

LPTSTR CBHashLK::NextItemStr2( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 DataLong
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].DataStr2;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].DataStr2;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

CBHashLK::KeyType CBHashLK::NextKey( bool &bRetNotValid )
{
	// 调用 StartTraversal 后，用此函数遍历 String
    // 若 bRetEndNotValid 返回 True，表此次遍历已结束（此时函数返回值也无效）
	int idx;
	idx = TraversalGetNextIdx();
	if (idx > 0)
	{
		bRetNotValid = false;
		return mem[idx].Key;
	}
	else if (idx < 0)
	{
		bRetNotValid = false;
		return mem2[-idx].Key;
	}
	else
	{
		bRetNotValid = true;
		return 0;
	}
}

// 清除所有元素，重定义 mcIniMemSize 个存储空间
void CBHashLK::Clear( void )
{
	// 清除	按 Index 访问各元素的缓存数组
	if (mArrTable) {delete []mArrTable; mArrTable=0;}
	mArrTableCount=-1;

	// 删除 mem[] 和 mem2[] 中的每个元素的 字符串数据 指向的空间
	int i;
	for (i=1; i<=memCount; i++)
	{
		if (mem[i].DataStr) {delete [] mem[i].DataStr; mem[i].DataStr=0; }
		if (mem[i].DataStr2) {delete [] mem[i].DataStr2; mem[i].DataStr2=0; }
	}
	for (i=1; i<=memCount2; i++)
	{
		if (mem2[i].DataStr) {delete [] mem2[i].DataStr; mem2[i].DataStr=0; }
		if (mem2[i].DataStr2) {delete [] mem2[i].DataStr2; mem2[i].DataStr2=0; }
	}

	// 删除 mem[] 和 mem2[] 的空间
	delete [] mem; mem=0;
	delete [] mem2; mem2=0;
	memCount = 0; memUsedCount = 0;
	memCount2 = 0; memUsedCount2 = 0;

	// 重新开辟空间
	RedimArrMemType(mem, mcIniMemSize, memCount, false);
	memCount = mcIniMemSize;

	mTravIdxCurr = 0;
}


// 返回共有元素个数
int CBHashLK::Count( void )
{
	return memUsedCount + memUsedCount2;
}




//////////////////////////////////////////////////////////////////////
// 私有方法
//////////////////////////////////////////////////////////////////////

void CBHashLK::ReLocaMem( int preMemCountTo )
{
	/*
	重新分配 mem[], mem2[] 的各元素的空间，mem2[] 的某些元素可能被 \
	重新移动到 mem
	将修改 memUsedCount,memUsedCount2, memCount2, mem2[] 的值
	preMemCountTo 只考虑 mem[1 to preMemCountTo]，preMemCountTo 以后的元素被认为 \
	未用，不考虑；但无论如何都考虑 mem2[] 中的所有元素
	*/
	
	// 将 mem[] 中的已使用元素和 mem2[] 中的所有元素先放入 memUsed[] 中， \
	// 把 memUsed[] 定义为足够大，实际 memUsed[] 只使用了 lngUsedCount 个元素
	MemType * memUsed;
	RedimArrMemType(memUsed, preMemCountTo + memUsedCount2);

	int iUsedCount=0;		
	int i;

	// 将 mem[] 中已使用的元素存入 memUsed[]
	for (i=1; i<=preMemCountTo; i++)
		if (mem[i].Used)
		{
			iUsedCount = iUsedCount + 1;
			memUsed[iUsedCount] = mem[i];
		}

	// 将 mem2[] 中的所有元素存入 memUsed[]
	for (i=1; i<=memUsedCount2; i++)
	{
		iUsedCount = iUsedCount + 1;
		memUsed[iUsedCount] = mem2[i];
	}


	/*
	此时 memUsed[1 To lngUsedCount] 中为所有 mem[] \
	中的已使用元素 和 mem2[] 中的所有元素
	*/

	// 清空 mem，也清空了所有 Used 域
	RedimArrMemType(mem, memCount, memCount, false); 
	memUsedCount=0;  // memUsedCount 置0，后面随移动随+1

	// 清空 mem2，也清空了所有 Used 域
	RedimArrMemType(mem2, -1, memCount2, false);
	memCount2 = 0;
	memUsedCount2 = 0; 

	// 逐个把 memUsed[1 To lngUsedCount] 中的元素按新数组大小映射下标存入 mem[]
	int idx;
	for (i=1; i<=iUsedCount; i++)
	{
		idx = AlloMemIndex(memUsed[i].Key, false);
		if (idx > 0)
		{
			mem[idx] = memUsed[i];
			mem[idx].Used = 1;
			memUsedCount = memUsedCount + 1;
		}
		else
		{
			mem2[-idx] = memUsed[i];
			mem2[-idx].Used = 1;
			memUsedCount2 = memUsedCount2 + 1;
		}
	}

	// 删除临时空间 memUsed
	delete [] memUsed; memUsed=0;

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}


// 重定义 mem[] 数组大小，扩大 mem[] 的空间
void CBHashLK::ExpandMem( void )
{
	int iCount, iPreMemCount;

	// 计算哈希表中共有数据总数
	iCount = memUsedCount + memUsedCount2;

	// 取“共有数据总数”和“当前 mem[] 的空间总数”两者的较大值
	if (iCount < memCount) iCount = memCount;

	// 保存扩增空间之前的、原来的 mem[] 的空间总数
	iPreMemCount = memCount;

	
	if (iCount<1) iCount=1;		// 避免 iCount 为0时，无法扩大空间
	if (iCount < mcExpandCountThres)
	{
		// 如果数据总数“比较少”，就扩增空间为原来的2倍
		iCount = iCount * 2;

	}
	else if (iCount < mcExpandCountThresMax)
	{
		// 如果数据总数已经“有点多”，就扩增空间为原来的1.5倍
		iCount = iCount * 3 / 2;
	}
	else
	{
		// 如果数据总数“很多”，就扩增 mcExpandBigPer 个空间
		iCount = iCount + mcExpandBigPer;
	}

	// 重定义数组大小
	// ReDim Preserve mem(1 To lngCount)
	RedimArrMemType(mem, iCount, memCount, true);
	memCount = iCount;

	// 按新数组大小，重新安排其中所有数据的新位置，参数中要传递
	// 扩增空间之前的、原来的 mem[] 的空间总数
	ReLocaMem(iPreMemCount);

	// 哈希表遍历的指针重置
	mTravIdxCurr = 0;
	
	// 按 Index 访问各元素的缓存数组重置
    mArrTableCount = -1;
}

int CBHashLK::AlloSeqIdx( int fromIndex, int toIndex )
{
    /*
	 找 mem[] 中一个没使用的空间，从 fromIndex 开始， \
		到 toIndex 结束
	 返回 mem[] 的一个没使用元素的下标，没找到返回 0
	*/
	int i;
	if (fromIndex <= 0)  fromIndex = 1;
	if (toIndex > memCount) toIndex = memCount;
	
	for (i=fromIndex; i<=toIndex; i++)
		if (! mem[i].Used) return i; 

	return 0;
}

// 遍历哈希表，将数据存入 mArrTable()，设置 mArrTableCount 为数据个数（返回成功或失败）
bool CBHashLK::RefreshArrTable()
{
	int iCount;
	int i,j;
	
	// 计算哈希表中共有数据总数
	iCount = memUsedCount + memUsedCount2;
	
	mArrTableCount=iCount;
	if (mArrTableCount<=0) return false;
	
	if (mArrTable) {delete []mArrTable; mArrTable=0;}
	mArrTable=new int [iCount+1];	// 使数组下标从1开始
	
	j=1;
	for (i=1; i<=memCount; i++)
	{
		if (mem[i].Used)
		{
			if (j > iCount) return false;
			
			mArrTable[j] = i;		// 将 哈希表的本数据所在的 mem[] 的下标 i 存入 mArrTable[j]
			mem[i].Index = j;		// 在 哈希表的本数据 的 Index 成员中记录 mArrTable 的下标 j

			j=j+1;
		}
	}
	
	for (i=1; i<=memUsedCount2; i++)
	{
		if (mem2[i].Used)
		{
			if (j > iCount) return false;
			
			mArrTable[j] = -i;		// 将 哈希表的本数据所在的 mem2[] 的下标 i （取负）存入 mArrTable[j]
			mem[i].Index = j;		// 在 哈希表的本数据 的 Index 成员中记录 mArrTable 的下标 j
			
			j=j+1;
		}
	}
	
	return true;
}




int CBHashLK::AlloMemIndex( KeyType key, bool CanExpandMem/*=true */ )
{
	/* 
	  根据 Key 分配一个 mem[] 中的未用存储空间，返回 mem[] 数组下标
	  如果 Key 是负值，则转换为正数计算它的存储空间
		返回负值表不能在 mem[] 中找到空间：返回值的绝对值为 mem2[] 的 \
		下一个可用下标空间（mem2[]自动Redim），以存入 mem2[]
	  本函数确保返回一个可使用的空间，最差情况返回 mem2[] 中的空间
	  另：本函数不修改 memUsedCount2 的值，但 redim mem2[]
		CanExpandMem=true 时，允许本函数自动扩大 mem[]，否则不会自动扩大
		
		方法：
		1. 先用 Key Mod memCount + 1，此 Index -> idxMod
		2. 若上面的元素已经使用，则看Key是否 < cMaxNumForSquare (sqr(2^31)=46340) \
		若 <，则平方 Key，然后 mod memCount + 1； \
		若 >=，则用按位和移位运算，后相加 key 的各个部分，然后 mod memCount + 1
		无论哪种情况，此步 Index -> idxSq
		3. 用 memCount-idxMod+1 -> idxModRev
		4. 用 memCount-idxSq+1 -> idxSqRev
		5. 若上面找到的 Index 都被使用了，则看 Count 是否 > \
		mcExpandMaxPort*Count，若是，若 CanExpandMem=true， \
		则扩大 mem[] 的存储空间，然后递归本过程，重复 1-4 步
		6. 用 idxMod+1,+2,...,+mcSeqMax；用 idxMod-1,-2,...,-mcSeqMax
		7. 再没有，返回负值，绝对值为 mem2[] 的下一个可用空间，以存入 mem2[]
	*/


	const int cMaxNumForSquare = 46340;
	
	int idxMod=0, idxSq=0;
    int idxModRev=0, idxSqRev=0;
    int iCount=0;
    int keyToCalc=key; // 计算用 Key，永远为>0的数

	keyToCalc = key;
	if (keyToCalc < 0) keyToCalc = 0 - keyToCalc;	// 如果 Key 是负值，则转换为正数计算它的存储空间
	iCount = memUsedCount + memUsedCount2;
    
	if (memCount) 
	{		
		// 1: 先用 Key Mod memCount + 1，此 Index -> idxMod
		idxMod = keyToCalc % memCount + 1;
		if (! mem[idxMod].Used) return idxMod;
    
		// 2: 用 平方Key 后再除法取余，此 Index -> idxSq
		if (keyToCalc <= cMaxNumForSquare)
		{
			idxSq = (keyToCalc * keyToCalc) % memCount + 1;
		}
		else
		{
			int kBitSum=0;
			kBitSum = (keyToCalc & 0xFFFF0000)>>16;
			kBitSum += (keyToCalc & 0xFF00)>>8;
			kBitSum += (keyToCalc & 0xF0)>>4;
			kBitSum += (keyToCalc & 0xF);
			idxSq = kBitSum % memCount + 1;
		}
		if (! mem[idxSq].Used) return idxSq;

		// 3: 尝试倒数第 idxMod 个空间 -> idxModRev
		idxModRev = memCount - idxMod + 1; 
		if (! mem[idxModRev].Used) return idxModRev;
    
		// 4: 尝试倒数第 idxSq 个空间 -> idxSqRev
		idxSqRev = memCount - idxSq + 1;
		if (! mem[idxSqRev].Used) return idxSqRev;
	}
    
    // 5: 如果空间使用百分比超过阈值，就扩大 mem[] 的 空间
    if (CanExpandMem && iCount > mcExpandMaxPort * memCount)
	{
		ExpandMem();  // 扩大 mem[] 的空间
		return AlloMemIndex(key, CanExpandMem); // 递归，重复1-4步
	}
        
    
    int lngRetIdx;
    
    // 6: 从 idxMod 开始向前、向后线性搜索 mcSeqMax 个空间
    int idxMdSta, idxMdEnd; 
    idxMdSta = idxMod - mcSeqMax; idxMdEnd = idxMod + mcSeqMax;
    lngRetIdx = AlloSeqIdx(idxMdSta, idxMod - 1); 
    if (lngRetIdx > 0) return lngRetIdx;
    lngRetIdx = AlloSeqIdx(idxMod + 1, idxMdEnd);
    if (lngRetIdx > 0) return lngRetIdx; 
    
    // 8: 返回负值，绝对值为 mem2[] 的下一个元素，以存入 mem2[]
    if (memUsedCount2 + 1 > memCount2)
    {    
        // ReDim Preserve mem2(1 To mcExpandMem2Per)
		RedimArrMemType(mem2, memCount2 + mcExpandMem2Per, memCount2, true);
		memCount2 = memCount2 + mcExpandMem2Per;
	}

    return -(memUsedCount2 + 1);
}


int CBHashLK::FindSeqIdx( KeyType key, int fromIndex, int toIndex )
{
	
    // 找 mem[] 中键为Key的元素下标，从 fromIndex 开始， \
	//	到 toIndex 结束
	//	返回 mem[] 的找到键的下标（>0），没找到返回 0

	int i;
	if (fromIndex < 1) fromIndex = 1;
	if (toIndex > memCount) toIndex = memCount;
	
	for (i=fromIndex; i<=toIndex; i++)
		if ((mem[i].Used) && mem[i].Key == key )
			return i;
	
	return 0;
}




int CBHashLK::TraversalGetNextIdx( void )
{
	// 用 NextXXX 方法遍历时，返回下一个（Next）的 mem[]下标（返回值>0）， \
	// 或 mem2[] 的下标（返回值<0），或已遍历结束（返回值=0）
	
	int iRetIdx;
	
	if (mTravIdxCurr > memCount ||
		-mTravIdxCurr > memCount2 ||
		mTravIdxCurr == 0) return 0;
	
	if (mTravIdxCurr > 0)
	{
		//////////// 在 mem[] 中找 ////////////
		while (! mem[mTravIdxCurr].Used)
		{
			mTravIdxCurr = mTravIdxCurr + 1;
			if (mTravIdxCurr > memCount) break;
		}
		
		if (mTravIdxCurr > memCount)
		{
			//// 已遍历结束，看若 mem2[] 中还有数据继续遍历 mem2[] ////
			if (memCount2 > 0)
			{
				// 设置下次遍历 mem2[] 中数据的下标的负数
				mTravIdxCurr = -1;
				// 执行下面的 if mTravIdxCurr < 0 Then 
			}
			else
			{
				// 返回结束
				iRetIdx = 0;
				return iRetIdx;
			}
		}
		else
		{
			//// 返回 mTravIdxCurr ////
			iRetIdx = mTravIdxCurr;
			// 调整下次遍历指针 指向下一个位置（或是 mem[] 的下一个， \
			// 或是 mem2[] 的起始）
			mTravIdxCurr = mTravIdxCurr + 1;
			if (mTravIdxCurr > memCount) if (memCount2 > 0) mTravIdxCurr = -1;
			return iRetIdx;
		}
	}
	
	if (mTravIdxCurr < 0)
	{
		//////////// 在 mem2[] 中找 ////////////
		while (! mem2[-mTravIdxCurr].Used)
		{	
			mTravIdxCurr = mTravIdxCurr - 1;
			if (-mTravIdxCurr > memCount2) break;
		}
		
		if (-mTravIdxCurr > memCount2)
		{
			//// 已遍历结束 ////
			// 返回结束
			iRetIdx = 0; 
		}
		else
		{
			// 返回负值的 mTravIdxCurr
			iRetIdx = mTravIdxCurr;
			// 调整 mTravIdxCurr 的指针
			mTravIdxCurr = mTravIdxCurr - 1;
		}
		return iRetIdx;
	}
	
	return 0;
}




// 重定义 一个 MemType 类型的数组（如可以是 mem[] 或 mem2[]）的大小，新定义空间自动清零
// arr：为数组指针，可传递：mem 或 mem2，本函数将修改此指针的指向
// toUBound：为要重定义后数组的上界，定义为：[0] to [toUBound]，为 -1 时不开辟空间，可用于删除原
//	 空间，并 arr 会被设为0
// uboundCurrent：为重定义前数组的上界 [0] to [uboundCurrent]，为 -1 表示尚未开辟过空间为第一次调用
// preserve：保留数组原始数据否则不保留
// 返回新空间上标，即 toUBound
int CBHashLK::RedimArrMemType( MemType * &arr, int toUBound/*=-1*/, int uboundCurrent/*=-1*/, bool preserve/*=false*/ )
{
	// 开辟新空间：[0] to [toUBound]
	if (toUBound >= 0)
	{
		MemType * ptrNew = new MemType [toUBound + 1];		// +1 为使可用下标最大到 toUBound
		// 新空间清零
		memset(ptrNew, 0, sizeof(MemType)*(toUBound + 1));
		
		// 将原有空间内容拷贝到新空间
		if (preserve && arr!=0 && uboundCurrent>=0)
		{
			int ctToCpy;										// 保留原有数据，需要拷贝内存的 MemType 元素个数
			ctToCpy = uboundCurrent;
			if (uboundCurrent>toUBound) ctToCpy = toUBound;		// 取 uboundCurrent 和 toUBound 的最小值
			ctToCpy = ctToCpy + 1;								// 必须 +1，因为 uboundCurrent 和 toUBound 都是数组上界
			memcpy(ptrNew, arr, sizeof(MemType)*ctToCpy); 
		}

		// 删除原有空间
		if (arr!=0 && uboundCurrent>=0) delete [] arr;

		// 指针指向新空间
		arr = ptrNew;
		return toUBound;
	}
	else		// if (toUBound < 0)，不开辟空间，删除原有空间
	{
		if(arr!=0 && uboundCurrent>=0) delete [] arr;
		arr = 0;
		return 0;
	}
	
}


int CBHashLK::GetMemIndexFromKey( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	const int cMaxNumForSquare = 46340;  // sqrt(2^31)=46340
    
    int idxMod=0, idxSq=0;
    int idxModRev=0, idxSqRev=0;
    int keyToCalc=key; // 计算用 Key，永远为>=0的数
    if (keyToCalc < 0) keyToCalc = 0 - keyToCalc;

	if (memCount)
	{
		// 1: 先用 Key Mod memCount + 1，此 Index -> idxMod
		idxMod = keyToCalc % memCount + 1;
		if (mem[idxMod].Used && mem[idxMod].Key == key) 
			return idxMod;
    
		// 2: 用 平方Key后再除法取余，此 Index -> idxSq
		if (keyToCalc <= cMaxNumForSquare)
		{
			idxSq = (keyToCalc * keyToCalc) % memCount + 1;
		}
		else
		{
			int kBitSum=0;
			kBitSum = (keyToCalc & 0xFFFF0000)>>16;
			kBitSum += (keyToCalc & 0xFF00)>>8;
			kBitSum += (keyToCalc & 0xF0)>>4;
			kBitSum += (keyToCalc & 0xF);
			idxSq = kBitSum % memCount + 1;
		}
		if (mem[idxSq].Used && mem[idxSq].Key == key)
			return idxSq;
    
		// 3: 尝试倒数第 idxMod 个空间 -> idxModRev
		idxModRev = memCount - idxMod + 1;
		if (mem[idxModRev].Used && mem[idxModRev].Key == key)
		   return idxModRev;
    
		// 4: 尝试倒数第 idxSq 个空间 -> idxSqRev
		idxSqRev = memCount - idxSq + 1;
		if (mem[idxSqRev].Used && mem[idxSqRev].Key == key)
		   return idxSqRev;
	}

    int lngRetIdx=0;
    
    // 6: 从 idxMod 开始向前、向后线性搜索 mcSeqMax 个空间
    int idxMdSta, idxMdEnd;
    idxMdSta = idxMod - mcSeqMax; idxMdEnd = idxMod + mcSeqMax;
    lngRetIdx = FindSeqIdx(key, idxMdSta, idxMod - 1);
    if (lngRetIdx > 0)  return  lngRetIdx;
    lngRetIdx = FindSeqIdx(key, idxMod + 1, idxMdEnd);
    if (lngRetIdx > 0)  return  lngRetIdx;
    
    // 7: 再查看 mem2[] 中的元素有没有
    for (int i=1; i<=memUsedCount2; i++)
        if (mem2[i].Used && mem2[i].Key == key) return -i;
	
	if (raiseErrorIfNotHas) throw (unsigned char)5;	// 无效的过程调用或参数
	return 0;
}

// 从 index 获得数据在 mem[] 中的下标（返回值>0）或在 mem2[] 中的下标（返回值<0），出错返回 0
int CBHashLK::GetMemIndexFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	if (mArrTableCount != memUsedCount + memUsedCount2) RefreshArrTable(); // 刷新数组缓冲
	if (index<1 || index>mArrTableCount)
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)5;	// 无效的过程调用或参数
		return 0;
	}
	
	int idx=mArrTable[index];
	if (idx==0) 
	{
		if (raiseErrorIfNotHas)  throw (unsigned char)7;	// 超出内存（mArrTable[index]意外为0）
		return 0;
	}
	else
		return idx;
}


// 用 new 开辟新字符串空间，把 ptrNewString 指向的字符串拷贝到新空间；
//   ptrSaveTo 是一个保存字符串地址的指针变量的地址，其指向的指针变量将保存
//   “用 new 开辟的新字符串空间的地址”，即让 “*ptrSaveTo = 新空间地址”
// 兼有释放“*ptrSaveTo”所指向的空间的功能
//   如 ptrSaveTo 参数可被传递 &(mem[i].key) 即指针的指针；ptrNewString 可被传递新的 key
//   以完成“mem[i].key=key”的操作，本函数修改 mem[i].key 的内容：
//   先删除它旧指向的空间，再让它指向新空间
// 如果 key 为空指针，仅释放“*ptrSaveTo”所指向的空间
void CBHashLK::SaveItemString( TCHAR ** ptrSaveTo, LPCTSTR ptrNewString )
{
	// 注意 ptrSaveTo 是个二级指针
	if (ptrSaveTo==0) return;  // 没有保存的位置
	
	// 如果 ptrSaveTo 指向的指针变量不为“空指针”，表示要保存之处已正
	//   保存着一个以前开辟的空间地址，应先删除以前开辟的空间
	if (*ptrSaveTo != 0) {delete [] (*ptrSaveTo); *ptrSaveTo=0; }
	
	if (ptrNewString)
	{
		// 开辟新空间，保存 ptrNewString 这个字符串到新空间
		TCHAR * p = new TCHAR [lstrlen(ptrNewString)+1];
		lstrcpy(p, ptrNewString);
		
		// 使 *ptrSaveTo 指向新空间
		*ptrSaveTo = p;
	}
}



// ---------------- 以 Index 返回数据属性(包括Key，但Key为只读) ----------------
// 注：随着数据增删，Index 可能会变化。某数据的 Index 并不与数据一一对应

CBHashLK::DataType CBHashLK::ItemFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Data;
	else if (ii < 0)
		return mem2[-ii].Data;
	else
		return 0;
}

CBHashLK::DataLong2Type CBHashLK::ItemLong2FromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataLong2;
	else if (ii < 0)
		return mem2[-ii].DataLong2;
	else
		return 0;
}

CBHashLK::DataLongType CBHashLK::ItemLongFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataLong;
	else if (ii < 0)
		return mem2[-ii].DataLong;
	else	
		return 0;
}

double CBHashLK::ItemDoubleFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataDouble;
	else if (ii < 0)
		return mem2[-ii].DataDouble;
	else
		return 0;
}

LPTSTR CBHashLK::ItemStrFromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataStr;
	else if (ii < 0)
		return mem2[-ii].DataStr;
	else			
		return 0;
}

LPTSTR CBHashLK::ItemStr2FromIndex( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].DataStr2;
	else if (ii < 0)
		return mem2[-ii].DataStr2;
	else			
		return 0;	
}

CBHashLK::KeyType CBHashLK::IndexToKey( int index, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Key;
	else if (ii < 0)
		return mem2[-ii].Key;
	else			
		return 0;	
}


int CBHashLK::KeyToIndex( KeyType key, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	if (mArrTableCount != memUsedCount + memUsedCount2) RefreshArrTable(); // 刷新数组缓冲
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii > 0)
		return mem[ii].Index;
	else if (ii < 0)
		return mem2[-ii].Index;
	else
		return 0;
}


// ---------------- 以 Key 设置数据属性 ----------------

bool CBHashLK::ItemSet( KeyType key, DataType vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].Data = vNewValue;
	else if (ii < 0)
		mem2[-ii].Data = vNewValue;
	return true;
}

bool CBHashLK::ItemLongSet( KeyType key, DataLongType vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong = vNewValue;
	return true;	
}


bool CBHashLK::ItemLong2Set( KeyType key, DataLong2Type vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong2 = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong2 = vNewValue;
	return true;	
}

bool CBHashLK::ItemDoubleSet( KeyType key, double vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataDouble = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataDouble = vNewValue;
	return true;	
}

bool CBHashLK::ItemStrSet( KeyType key, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr), vNewValue);
	return true;	
}

bool CBHashLK::ItemStr2Set( KeyType key, LPCTSTR vNewValue,bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromKey(key, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr2), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr2), vNewValue);
	return true;	
}

// ---------------- 以 Index 设置数据属性(Key为只读不能设置Key) ----------------
// 注：随着数据增删，Index 可能会变化。某数据的 Index 并不与数据一一对应

bool CBHashLK::ItemFromIndexSet( int index, DataType vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].Data = vNewValue;
	else if (ii < 0)
		mem2[-ii].Data = vNewValue;
	return true;
}

bool CBHashLK::ItemLongFromIndexSet( int index, DataLongType vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong = vNewValue;
	return true;
}

bool CBHashLK::ItemLong2FromIndexSet( int index, DataLong2Type vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataLong2 = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataLong2 = vNewValue;
	return true;	
}

bool CBHashLK::ItemDoubleFromIndexSet( int index, double vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		mem[ii].DataDouble = vNewValue;
	else if (ii < 0)
		mem2[-ii].DataDouble = vNewValue;
	return true;	
}

bool CBHashLK::ItemStrFromIndexSet( int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr), vNewValue);
	return true;	
}

bool CBHashLK::ItemStr2FromIndexSet( int index, LPCTSTR vNewValue, bool raiseErrorIfNotHas/*=true*/ )
{
	int ii;
	ii=GetMemIndexFromIndex(index, raiseErrorIfNotHas);
	if (ii == 0)
		return false;
	else if (ii > 0)
		SaveItemString(&(mem[ii].DataStr2), vNewValue);
	else if (ii < 0)
		SaveItemString(&(mem2[-ii].DataStr2), vNewValue);
	return true;
}









//////////////////////////////////////////////////////////////////////
// CBArrLink 类的实现：高效数组链表类
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Static 常量定值
//////////////////////////////////////////////////////////////////////
const int   CBArrLink::mcItemsPerArr = 512;		// 每个数组的元素个数的默认值（此值不得为0），可通过构造函数改变
const int	CBArrLink::mcppArrInit = 128;  		// 保存各数组地址的指针数组初始空间大小（此值不得小于2）
const int	CBArrLink::mcppArrExpPer = 128;  	// 保存各数组地址的指针数组每次扩增的空间（此值不得小于1）


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBArrLink::CBArrLink(int iItemsPerArr/*=0*/)
{
	// 成员变量定初值

	// 每个数组的元素个数的默认值
	if (iItemsPerArr>0)
		m_ItemsPerArr = iItemsPerArr;	
	else
		m_ItemsPerArr = mcItemsPerArr;	

	ppArr = 0; 
	m_ppArrUbound = 0;
	m_ppArrUsedCount = 0;
	m_DataCount = 0;
	m_ptrArrRet = 0;

	// 分配初始空间
	Init();
}

CBArrLink::~CBArrLink()
{
	Dispose();
}


//////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////

int CBArrLink::Add( int data1, int data2/*=0*/ )
{
	m_DataCount++; 
	int index = m_DataCount;
	int i = (index-1) / m_ItemsPerArr + 1, j = (index-1) % m_ItemsPerArr;
		// 新数据将被保存在 ppArr[i][j]
	
	// 如果已有的 数据数组 空间都已用完，就开辟新的 数据数组
	if (i>m_ppArrUsedCount)
	{
		m_ppArrUsedCount++; 
				// 新数据数组 的首地址将被保存到 ppArr[ppArrUsedCount]

		// 如果 ppArr[ppArrUsedCount] 本身的空间尚不存在，就开辟 ppArr[ppArrUsedCount] 的空间
		if ( m_ppArrUsedCount>m_ppArrUbound )
		{
			// 指针数组 ppArr[] 开辟新空间
			int sizeNew = m_ppArrUbound+1+mcppArrExpPer;
						// 注意“+1”，ppArr下标有效范围 [0]～[ppArrUbound]，ppArr 已有空间个数 ppArrUbound+1
			MemType ** ppArrTemp = new MemType * [sizeNew];				// 新指针数组
			memset( ppArrTemp, 0, sizeof(MemType *) * sizeNew );		// 清零新指针数组中的各地址
			memcpy( ppArrTemp, ppArr, sizeof(MemType *) * (m_ppArrUbound+1));	// 拷贝原指针数组中的数据
						// 注意“+1”，ppArr下标有效范围 [0]～[ppArrUbound]，ppArr 已有空间个数 ppArrUbound+1

			delete []ppArr;				// 删除原指针数组
			ppArr = ppArrTemp;			// 将 ppArr 更改为指向新指针数组地址
			m_ppArrUbound = sizeNew-1;	// 指针数组空间已被扩大
										// 注意“-1”，ppArr下标有效范围 [0]～[sizeNew-1]

			// 此时应 ppArrUsedCount<=ppArrUbound ，以下语句为容错
			if ( m_ppArrUsedCount>m_ppArrUbound ) return 0;
		}
		
		// 开辟 新 数据数组，地址 => ppArr[ppArrUsedCount]
		ppArr[m_ppArrUsedCount] = new MemType [m_ItemsPerArr];

		// 此时应 i<=ppArrUsedCount，以下语句为容错
		if (i>m_ppArrUsedCount) return 0;

	}	// end if (i>ppArrUsedCount)

	// 保存新数据
	ppArr[i][j].DataInt  = data1;
	ppArr[i][j].DataInt2 = data2;

	return m_DataCount;
}


int CBArrLink::Remove( int index, bool bRaiseErrIfErr/*=false*/ )
{
	int i = (index-1) / m_ItemsPerArr + 1, j = (index-1) % m_ItemsPerArr;
	if (index<=m_DataCount && i>0 && i<=m_ppArrUsedCount && j>=0) // 注意 j 可以 ==0
	{
		// 用最后一个数据覆盖 ppArr[i][j]
		int iEnd = (m_DataCount-1) / m_ItemsPerArr + 1, 
			jEnd = (m_DataCount-1) % m_ItemsPerArr;
		ppArr[i][j] = ppArr[iEnd][jEnd];

		// 删除最后一个数据
		if (jEnd==0)
		{
			// 删除最后一批“数据数组”
			delete []ppArr[iEnd];  ppArr[iEnd]=0;
			m_ppArrUsedCount = iEnd-1;
		}
		m_DataCount--;
	}
	else // if (index<=m_DataCount && i>0 && ...
	{
		if (bRaiseErrIfErr) throw (unsigned char)9;			// 数组索引超出范围
	}    // end if (index<=m_DataCount && i>0 && ...
	
	return m_DataCount;		// 返回目前的数据总个数
}

int CBArrLink::Item( int index, bool bRaiseErrIfErr/*=false*/ )
{
	int i = (index-1) / m_ItemsPerArr + 1, j = (index-1) % m_ItemsPerArr;
 	if (index<=m_DataCount && i>0 && i<=m_ppArrUsedCount && j>=0) // 注意 j 可以 ==0
		return ppArr[i][j].DataInt; 
	else
	{
		if (bRaiseErrIfErr) throw (unsigned char)9;			// 数组索引超出范围
		return -2147483647-1;
	} 
}


int CBArrLink::Item2( int index, bool bRaiseErrIfErr/*=false*/ )
{
	int i = (index-1) / m_ItemsPerArr + 1, j = (index-1) % m_ItemsPerArr;
	if (index<=m_DataCount && i>0 && i<=m_ppArrUsedCount && j>=0) // 注意 j 可以 ==0
		return ppArr[i][j].DataInt2; 
	else
	{
		if (bRaiseErrIfErr) throw (unsigned char)9;			// 数组索引超出范围
		return -2147483647-1;
	} 
}


int CBArrLink::Count()
{
	return m_DataCount;
}


void * CBArrLink::GetItemsArr()
{
	if (m_ptrArrRet) { delete []m_ptrArrRet; m_ptrArrRet=0; }

	// 准备临时空间的一个数组 用于保存所有数据，首地址 => pArr
	MemType * pArr = new MemType [m_DataCount+1];
	memset( pArr, 0, sizeof(MemType)*(m_DataCount+1) );	// 清零数组空间

	// 拷贝所有数据 => 临时空间
	MemType *p = pArr+1;	// 临时空间即将要拷贝到的位置，开始位置除去 [0] 的空间 
	int i;

	// 拷贝整批的数据数组，不写 <=
	for (i=1; i<m_ppArrUsedCount; i++ )	
	{
		memcpy(p, ppArr[i], sizeof(MemType) * m_ItemsPerArr);
		p += m_ItemsPerArr;
	}
	
	// 拷贝最后一批的数据数组
	memcpy(p, ppArr[i], sizeof(MemType) * (m_DataCount % m_ItemsPerArr) );
	p += (m_DataCount % m_ItemsPerArr);

	// 返回值
	m_ptrArrRet = (void *)pArr;
	return m_ptrArrRet;
}

void CBArrLink::Clear()
{
	Dispose();
	Init();
}


//////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////

void CBArrLink::Init()
{
	if (ppArr != 0) Dispose();		// 若尚有数据，先清空所有数据

	// 为 保存各数组地址的指针数组 开辟初始空间
	ppArr = new MemType * [mcppArrInit];
	memset( ppArr, 0, sizeof(MemType *)*mcppArrInit );	// 清零指针数组中的各地址
	m_ppArrUbound = mcppArrInit-1;	// 注意“-1”，ppArr下标有效范围 [0]～[mcppArrInit-1]
	m_ppArrUsedCount = 0;
	
	// 开辟第1个数据数组的空间，地址 => ppArr[1]
	ppArr[1] = new MemType [m_ItemsPerArr];
	m_ppArrUsedCount = 1;		// 有效指针数组元素为：ppArr[1] ～ ppArr[1]

	// 尚无数据，数据总个数为 0
	m_DataCount = 0;
}

void CBArrLink::Dispose()
{
	if (ppArr==0) return;	// 已经 Delete
	
	int i;

	// 删除返回数组的空间
	if (m_ptrArrRet) delete []m_ptrArrRet;
	m_ptrArrRet = 0;

	// 删除所有数据数组中的元素
	// 各数据数组 的首地址为：ppArr[1] ～ ppArr[ppArrUsedCount]
	for (i=1; i<=m_ppArrUsedCount; i++)
	{
		if (ppArr[i]) delete [] ppArr[i];
		ppArr[i] = 0;
	}
	m_ppArrUsedCount = 0;
	m_DataCount = 0;
	
	// 删除指针数组本身的空间
	delete [] ppArr; ppArr=0;
	m_ppArrUbound = 0;	
}


//////////////////////////////////////////////////////////////////////
// CHeapMemory 类的实现：用全局对象维护所有通过 new 分配的内存指针
//
//////////////////////////////////////////////////////////////////////



CBHeapMemory::CBHeapMemory(int initSize/*=0*/)
{
	// 开辟初始空间
	if (initSize) initSize=1000;
	memHash.AlloMem(initSize);
}

CBHeapMemory::~CBHeapMemory()
{
	Dispose();
}


int CBHeapMemory::AddPtr( void *ptrNew, bool bArrayNew/*=true*/, long iUserData/*=0*/ )
{
	memHash.Add((long)ptrNew, (long)ptrNew, bArrayNew, iUserData, 0, 0, 0, false);
	return memHash.Count();
}


// 用 new 分配 size 个字节的空间，并自动清0
void * CBHeapMemory::Alloc( int size )
{
	char * ptr=new char[size];
	memset(ptr, 0, size);
	AddPtr(ptr, true);
	return (void *)ptr;
}

// 释放 ptr 所指向的一段内存空间
//   ptr 必须是由本对象所管理的空间，否则本函数不会释放
void CBHeapMemory::Free( void *ptr )
{
	if ( ptr && memHash.IsKeyExist((long)ptr) )
	{
		if ( memHash.Item((long)ptr,false) == (long)ptr )	// 校验，若 !=，不 delete
		{
			if (memHash.ItemLong((long)ptr,false) )
				delete []ptr;
			else
				delete ptr;
		}	
		memHash.Remove((long)ptr, false);
	}
}


bool CBHeapMemory::IsPtrManaged( void *ptr )
{
	return memHash.IsKeyExist((long)ptr);
}

int CBHeapMemory::CountPtrs()
{
	return memHash.Count();
}

void * CBHeapMemory::PtrEach( int index, bool * ptrbArrayNew/*=0*/ )
{
	if ( ptrbArrayNew )
	{
		if ( memHash.ItemLongFromIndex(index,false) )
			*ptrbArrayNew=true; 
		else
			*ptrbArrayNew=false;
	}
	return (void *)memHash.ItemFromIndex(index, false);
}


long CBHeapMemory::UserData( void *ptr )
{
	return memHash.ItemLong2((long)ptr, false);
}

void CBHeapMemory::Dispose()
{
	// 删除对象中记录的所有空间
	int i=0; 
	void * ptr;
	for (i=1; i<=memHash.Count(); i++)
	{
		ptr = (void *)memHash.ItemFromIndex(i,false);
		if ( ptr )
		{
			if ( memHash.ItemLongFromIndex(i,false) )
				delete [] ptr; 
			else
				delete ptr;  
			memHash.ItemFromIndexSet(i, 0, false);
		}
	}
	
	// 清空 memHash
	memHash.Clear();
}


// 清零一块内存空间（实际是调用 memset (本类已 include <memory.h>) ，
// 为使主调程序不必再 include <memory.h>，本类也提供了这个功能接口）
void CBHeapMemory::ZeroMem( void * ptr, unsigned int length )
{
	memset(ptr, 0, length);
}


// 内存拷贝（实际是调用 memcpy (本类已 include <memory.h>) ，
// 为使主调程序不必再 include <memory.h>，本类也提供了这个功能接口）
void CBHeapMemory::CopyMem( void * dest, void * source, unsigned int length )
{
	memcpy(dest, source, length);	
}








//////////////////////////////////////////////////////////////////////
// CBApp 类的实现：管理应用程序全局信息
//
//////////////////////////////////////////////////////////////////////

CBApp::CBApp( HINSTANCE hInst, 
			 HINSTANCE hPrevInst, 
			 char * lpCmdLine, 
			 int nShowCmd ): 
hInstance(hInst), 
CmdShow(nShowCmd)
{
	m_CursorGlobalIdx = 0;
	m_hCursorGlobal = NULL;

	// ==== 获得命令行字符串并处理 ====
	LPTSTR szCommand = GetCommandLine();
	// 拷贝 命令行字符串 => m_szCommandFields
	m_szCommand = new TCHAR [ _tcslen(szCommand) + 2];
	_tcscpy(m_szCommand, pApp->Command());
	// 将 m_szCommandFields 中字段分隔处的空格改为 \0；最后写两个连续的 \0
	m_iCommandArgs = 0;
	TCHAR *p = m_szCommand;	// p 指向第1个字符
	while (*p)
	{	
		while (TEXT(' ') == *p && *p) p++;		// 跳过本字段开头的空格（如果有）。条件 && *p 限制不超过 \0
		if (TEXT('\"') == *p)	// 如果第1个字符是 "，就找到下一个 " 或'\0'
		{
			p++;								// 跳过第1个 " 字符
			while (TEXT('\"') != *p && *p) p++;	// 再找下一处定界 " 字符。条件 && *p 限制不超过 \0
			if ( TEXT('\"') == *p ) p++;		// 跳过下一处定界符 "
		}
		// 如果第1个字符不是 "（或是" 但已找到配对 "，并指向配对"的下一个字符）：找下一个空格或'\0'
		while (TEXT(' ') != *p && *p) p++;	// 找下一处空格，注意 ' '内是空格
		if ( TEXT('\0') == *p ) break;		// 超过整个命令行字符串的末尾 \0，跳出 while
		*p = TEXT('\0'); p++;				// 当前位置（空格）设置为 \0
		if (*p) m_iCommandArgs++;			// p所指内容未超过整个字符串时，参数个数+1

	}	// end of while (*p)
	p++;  *p = TEXT('\0');						//最后写两个连续的 \0
}


CBApp::~CBApp()
{
	ClearImageObjs();	// 释放所有记录的图像对象
	delete []m_szCommand;
	m_szCommand = NULL;
}


int CBApp::CommandArgs()
{
	return m_iCommandArgs;
}

LPTSTR CBApp::Command(int indexField/* = -1*/, bool fOmitHeadTailQuotes/* = true*/)
{
	if (indexField<0)
		return GetCommandLine();
	else
	{
		if (NULL==m_szCommand) 
		{ *m_szCommandArgBuff = TEXT('\0'); return m_szCommandArgBuff; }

		LPTSTR p = m_szCommand;
		int idxFieldFound = 0;
		while (indexField > idxFieldFound)
		{
			while (*p) p++;					//找下一个 \0
			idxFieldFound++;
			p++;							// p目前指向 \0，再下一个进入下一字段
			if (TEXT('\0') == *p) break;	// 发现两个连续的 \0，结束
		}

		if (indexField <= idxFieldFound) 
		{
			// p指向所需字段的首地址，返回 p 即可
			while (TEXT(' ') == *p && *p) p++;		// 跳过本字段开头的空格（如果有）。条件 && *p 限制不超过 \0
			if (! fOmitHeadTailQuotes)
				// 结果用 p 所指的 m_szCommand 内的空间
				return p;
			else
			{
				// 自动去除开头和结尾的引号（结果用 m_szCommandArgBuff[] 的空间）
				// 跳过开头的引号（如果有），开头引号个数 => ctHeadQuotes
				int ctHeadQuotes=0;
				while (TEXT('\"') == *p && *p) { p++; ctHeadQuotes++;}	
				// 跳过开头的引号后的内容拷贝入 m_szCommandArgBuff
				_tcscpy(m_szCommandArgBuff, p);
				// 去除 m_szCommandArgBuff 内的连续末尾引号
				p = m_szCommandArgBuff + _tcslen(m_szCommandArgBuff);
				p--;						// 使 p 指向最后一个字符
				while (TEXT('\"') == *p)	// 最多去除 ctHeadQuotes 个末尾引号
				{ p--; ctHeadQuotes--; if (ctHeadQuotes<=0) break; }
				p++;						// 使 p 指向最后一个引号
				*p = TEXT('\0');			// 将该引号位置设置为 \0，截断字符串
				return m_szCommandArgBuff;
			}
		}
		else
		{ *m_szCommandArgBuff = TEXT('\0'); return m_szCommandArgBuff; }
	}
}

// 获得应用程序当前运行的路径
LPTSTR CBApp::Path()
{
	if ( GetModuleFileName(0, m_szPath, sizeof(m_szPath)) )
	{
		// 找到最后的 \\，下一位置截断
		TCHAR *p = m_szPath;
		while (*p) p++; 
		while (*p != '\\') p--;
		p++; *p='\0';
	}
	else
	{
		// 获得应用程序名失败，返回 ""
		m_szPath[0] = 0;
	}
	return m_szPath;
}

int CBApp::ScreenWidth() const
{
	return GetSystemMetrics(SM_CXSCREEN);
}

int CBApp::ScreenHeight() const
{
	return GetSystemMetrics(SM_CYSCREEN);
}



long CBApp::MousePointerGlobal()
{
	return m_CursorGlobalIdx;
}

HCURSOR CBApp::MousePointerGlobalHCursor()
{
	return m_hCursorGlobal;
}

void CBApp::MousePointerGlobalSet( EStandardCursor cursor )
{
	MousePointerGlobalSet((long)cursor, 0);
}

void CBApp::MousePointerGlobalSet( long idResCursor, LPCTSTR typeRes/*=0*/ )
{
	m_CursorGlobalIdx = idResCursor;
	
	if (m_CursorGlobalIdx)
	{
		// 加载光标，句柄存入 ms_hCursorGlobal
		// =============================================================
		// LoadCursor 函数即使重复被调用，也不会重复加载资源；系统会判断
		//   如果对应光标已经加载，LoadCursor 直接返回句柄
		// =============================================================
		if ( m_CursorGlobalIdx > gc_IDStandCursorIDBase)
		{
			// 标准光标
			// ms_CursorGlobalIdx-gc_IDStandCursorIDBase 才是标准光标的ID号
			m_hCursorGlobal = 
				LoadCursor(NULL, 
				MAKEINTRESOURCE(m_CursorGlobalIdx-gc_IDStandCursorIDBase));
		}
		else
		{
			// 资源光标
			// ms_CursorGlobalIdx 就是资源 ID
			if (typeRes==0)
			{
				// 加载 Cursor 类型的资源
				m_hCursorGlobal =
					LoadCursor(pApp->hInstance, MAKEINTRESOURCE(m_CursorGlobalIdx));
			}
			else
			{
				// 加载自定义类型的资源（typeRes 类型的资源）
				unsigned long size=0; 
				unsigned char * p= LoadResData(m_CursorGlobalIdx, typeRes, &size);
				m_hCursorGlobal = (HCURSOR)CreateIconFromResource(p, size, 0, 0x00030000);
			}

			// 记录该光标句柄，以便程序退出前自动删除
			AddImageObjHandle((HANDLE)m_hCursorGlobal, eImgCursor);
		}
	}	
	else	// if (ms_CursorGlobalIdx)
	{
		// 不特殊设置光标，使用默认：设置 ms_hCursorGlobal 为 0
		m_hCursorGlobal = 0;
	}		// end if (ms_CursorGlobalIdx)
	
	// 先向本程序当前前台窗口发送 WM_SETCURSOR，前台窗口处理 WM_SETCURSOR 以使光标立即生效
	SendMessage(GetActiveWindow(), WM_SETCURSOR, (WPARAM)GetActiveWindow(), 0);
	// 在本程序的所有窗口（包括子窗口）的、不断接收到的 WM_SETCURSOR 消息中会改变鼠标光标	
}


bool CBApp::AddImageObjHandle( HANDLE hObject, EImgObjType typeImage )
{
	return m_hashImageObjs.Add((long)hObject, (long)hObject, 
		(long)typeImage, 0, 0, 0, 0, false);
}

void CBApp::ClearImageObjs()
{
	int i;
	for (i=1; i<=m_hashImageObjs.Count(); i++)
	{
		switch(m_hashImageObjs.ItemLongFromIndex(i,false))
		{
		case IMAGE_ICON:
			DestroyIcon((HICON)m_hashImageObjs.ItemFromIndex(i, false));
			break;
		case IMAGE_CURSOR:
			DestroyCursor((HCURSOR)m_hashImageObjs.ItemFromIndex(i, false));
			break;
		default:	// 包含 IMAGE_BITMAP
			DeleteObject((HGDIOBJ)m_hashImageObjs.ItemFromIndex(i, false));
			break;
		}
	}
	
	// 清空 gHashImageObjs 中的内容
	m_hashImageObjs.Clear();
}




