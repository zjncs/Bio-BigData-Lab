//////////////////////////////////////////////////////////////////////
// CBReadLinesEx 类：一行一行地读文件 类 （支持超过4G的大文件）
//
// 用法示例：=================================
// CBReadLinesEx file(TEXT("C:\\abc.txt"));
// LPTSTR szLine;
// 
// // file.TrimSpaces=true;		// 删除空格、Tab 等空白字符，这里可不设置，但默认为 false
// // file.TrimControls=true;	// 删除其他控制字符（0x01 -- 0x08, 0x0e -- 0x1F, 0x127），这里可不设置，但默认为 false
// // file.IgnoreEmpty=true;	// 这里可不设置，但默认为 false（注意最后一行仍可能返回空行）
// while ( !file.IsEndRead() )
// {
// 	file.GetNextLine(szLine);
// 	if (file.IsErrOccured()) break;
// 	cout<<szLine<<endl;
// }
// ============================================
// Support: 需要 BWindows 模块的支持（使用其中的读写文件函数 EFxxx()）

//////////////////////////////////////////////////////////////////////

#pragma once

#include "BWindows.h"

#define mc_RL_BufLen 131072				// 每个缓冲区字节长度默认值（每次读取的字节数）

class CBReadLinesEx
{
private:
	static const TCHAR mcszFailInfoDefault[];	// 默认读取出错字符串信息
	static const TCHAR mcszFileNotFoundInfoDefault[];	// 默认文件不存在字符串信息

private:
	typedef struct _RLBuffType		// 一个缓冲区 的数据类型
	{	
		LONGLONG llStartPosAbso;	// 该缓冲区在文件中的绝对位置（从0开始～m_llFileLength-1）
		LONG iBufLen;				// 缓冲区总长
		LONG iPtrInBuf;				// 缓冲区内部指针（从0开始～iBufLen-1）
		LONG iNextLineStaInBuf;		// 下一行内容开始位置(从此处算到下一个cr/lf为下一行)（从0开始～iBufLen-1）
		int iIgnoreFirstLf;			// 是否忽略本缓冲区的开头 vblf：=0不忽略；=1开头是0或lf或lf+0都忽略；=2开头是lf或lf+0都忽略；=3开头是0则忽略
		char bufBytes[mc_RL_BufLen];// 缓冲区字节内容；下标从0开始 bufBytes[0] ～ bufBytes[mc_RL_BufLen-1]
	} RLBuffType;

	typedef struct _RLLastBuffType	// 缓冲区剩余的字节的数据类型
	{
		char * bytsLeft;			// 如果多批缓冲（多批mc_RL_BufLen）还未找到 \r\n，则此空间会被无限延长
		LONG sizeBytsLeft;
	} RLLastBuffType;

public:  // 属性
	bool TrimSpaces;		// 是否自动去除每行空格字符（空格、Tab 等空白字符都被删除）
	bool TrimControls;		// 是否自动去除每行控制字符（0x01 -- 0x08, 0x0e -- 0x1F, 0x127）
	bool AutoOpen;			// 是否设置 FileName 属性时自动打开文件，默认为true(类初始化时设为true)
	bool AutoClose;			// 是否 读取行读完文件或出错时 自动关闭文件，默认为true(类初始化时设为true)
	bool IgnoreEmpty;		// 是否自动忽略空行（注意：如果是最后一行仍可能返回空行）
	bool ShowMsgIfErrRead;	// 读取失败是否自动提示
	bool AsUnicode;			// 是否源文件使用 Unicode 格式；除可通过属性设置外，在 OpenFile 时可自动设置此值
	bool AsUTF8;			// 是否源文件使用 UTF8 格式；除可通过属性设置外，在 OpenFile 时可自动设置此值

	// 行的结束标志:0=未设。13,10 or 2573(vbcrlf) ；
	// -1:unknown(此时再次调用GetNextLine后看EndLineSignLast获得)；
	// -2:未知，读到文件末尾，文件末尾无换行符
	int iEndLineSign;		

	// 上一行的结束标志 0=未设
	int iEndLineSignLast;	

	LPTSTR FailInfoRead;		// 读取出错时弹出的提示信息，如不设置此项，将弹出 mcszFailInfoDefault
								// （ShowMsgIfErrRead 为 True 时才会弹出）
	LPTSTR FailInfoFileNotFound;// 文件未找到时弹出的提示信息，如不设置此项，将弹出 mcszFileNotFoundInfoDefault
								// （ShowMsgIfErrRead 为 True 时才会弹出）

public:  // 方法
	// 构造函数：szFile 为 NULL 时，暂不设置文件名
	//   否则，用 szFile 作为文件名并重设本对象内
	//   部的 m_szFileName，并自动 OpenFile
	CBReadLinesEx(LPCTSTR szFile=NULL);
	~CBReadLinesEx();

	// 初始化
	void Init();	

	// 读取文件的下一行文本，支持 \r\n、\n、\r 的多种分行符
	// 将修改参数 szLine 的值，为所读取的此行文本的首地址
	// 这行字符串空间由本类自动开辟，指针未由 HM 管理，而是
	//   保存到 m_szOneLine 中；本类的一个对象只能
	//   保存最近一行的字符串，因下次 GetNextLine 时，上一行的
	//   字符串空间就被释放；最后一次一行的字符串空间由析构函数释放
	// 返回 1 表示正常读取了
	//   返回 -1 也表示正常，但读完了文件
	//   返回 0 表示出错或非法
	//     1. 一般出错返回 0，并设置 IsErrOccured=True
	//     2. 如果上次读完了文件，则允许再额外调用一次 GetNextLine (返回 0 并
	//        不提示出错，IsErrOccured 仍为 false，此算非法)；如果再调用就出错了
	//        (函数仍返回0，但 IsErrOccured 为 true 此算出错)
	//     3. IgnoreEmpty=True 时自动忽略空行，如果从当前一直读到文件结束
	//        都是空行，则都忽略，并返回0（此时 IsErrOccured=false，此算非法）
	// 只有要设置 IsErrOccured=true 才会在 ShowMsgIfErrRead=true 时给出出错提示
	int GetNextLine(LPTSTR &szLine);

	// 读取文件，返回文件中所有内容组成的一个字符串（以 \r\n 分隔）
	// 空间由本函数自动开辟，并保存到 HM 中管理（本对象被析构也不会回收）
	LPTSTR GetFileContent();

    // 将文件指针移动到 llToPos 位置，再调用 GetNextLine 时继续读取
	//   本函数将修改 lIsEndRead=false
	void SeekFile(LONGLONG llToPos);

	// 返回当前读取完的文件的进度
	//   iDotNum 保留几位小数，<0 或 >7 为不保留小数
	float GetPercent(int iDotNum = 2);

	// 打开文件：
	// szFileName 为 NULL 时，使用本对象内部的 m_szFileName 作为文件名
	//   否则，用 szFileName 作为文件名并重设本对象内部的 m_szFileName
	// 此函数不是必须由主调程序调用的，当 AutoOpen 属性为 true 时，
	//   主调程序构造函数中给参数，或调用 FileNameSet 时本类也将自动调用本函数
	bool OpenFile(LPCTSTR szFileName= NULL);
	
	// 关闭文件：当 AutoClose 属性为 true 时，本类将自动调用本函数
	void CloseFile();

	// 将文件读取指针指向文件开头，若文件已关闭，则重新打开文件
	void SetReadPtrToStart();		

	// 返回或设置属性值的方法
	void FileNameSet(LPCTSTR szFile);		// 设置要读取的文件的文件名，
											// 当 AutoOpen 属性为 true 时，会自动 OpenFile
	LPTSTR FileName();		// 获得文件名
	HANDLE hFile();			// 获得文件打开的句柄
	int Status();			// 获得状态：-1=已关闭;1=已打开;2=已经开始读取;0=未设
	bool IsEndRead();		// 获得读完状态：=true表示或者读完文件或者出错，即不能再继续读了，主程序应退出读取
	bool IsErrOccured();	// 获得是否发生了错误状态：是否上次 GetNextLine 发生了一个错误

private:
	RLBuffType m_buff;			// 一个缓冲区
	RLLastBuffType m_LastBuff;	// 缓冲区剩余的字节

	LPTSTR m_szOneLine;			// 保存一行字符串的空间地址（Ansi版保存Ansi字符串，Unicode 版保存Unicode字符串）
								//   每次 GetNextLine 中的 BytsToString 都 Delete 上一次的空间；
								//  析构时再 Delete 其所指向的空间
	LPTSTR BytsToString(const char * pByts,		// 将一个字节数组按 Ansi 或 Unicode 版转换为字符串返回，
						LONG sizeByts);			//   如 TrimSpaces 还将做删除空格和空白字符处理
												//   如 TrimControls 还将做删除其他控制字符处理（0x01 -- 0x08, 0x0e -- 0x1F, 0x127）
												// 空间自动开辟，返回字符串空间首地址，且将空间首地址保存到 m_szOneLine											
												// 每次执行该函数时，都 Delete 上一次的字符串空间
																

	bool m_bOneEndRead;			// 是否在关闭文件后还允许再调用一次 GetNextLine
	LONGLONG m_llFileLength;	// 文件总长度（字节数）

	TCHAR m_szFileName[2048];	// 文件名
	HANDLE m_hFile;				// 文件打开句柄
	int m_iStatus;				// 状态：-1=已关闭;1=已打开;2=已经开始读取;0=未设
	bool m_bIsEndRead;			// =true表示或者读完文件或者出错，即不能再继续读了，主程序应退出读取
	bool m_bErrOccured;			// 是否上次 GetNextLine 发生了一个错误
	TCHAR mEmptyStr[2];			// 空串缓冲区，返回字符串的函数出错时返回此数组的地址作为返回空串
};

// Dim af_strBuf As String
// Dim af_bytsBuf() As Byte '此仅为GetNextLine函数用，为了不每次调用GetNextLine时候都重新定义，故将之做为全局的了，其实应是局部的
// Dim j As Long