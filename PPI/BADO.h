//////////////////////////////////////////////////////////////////////////
// ADO 数据库操作通用模块
//
// 在主程序中，若还要包含其他头文件，应先包含其他头文件如 "BWindows.h"，
//   最后再包含本头文件
//////////////////////////////////////////////////////////////////////////

#pragma once

#import "msado15.dll" no_namespace rename("EOF","EOFado") 

#include <string>
typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;
using std::string;
using std::wstring;

class CBAdoConnection	// 封装 _ConnectionPtr 的 ADO Connection 的对象
{
private:
	// 静态成员：本类对象已创建个数（所有本类对象共用）：
	//   构造函数中，该值+1，若 +1 后 ==1，则执行 CoInitialize(NULL);
	//   析构函数中，该值-1，若 -1 后 ==0，则执行 CoUninitialize();
	static long ms_ConnObjCounts;

public:
	CBAdoConnection();
	~CBAdoConnection();

	// 建立ADO连接，成功返回true，失败返回false
	// 打开 szDabaBaseName 数据库文件
	// 如果数据库设置了密码，请指定 szDBPassword，否则把它设置为 NULL 表示没有密码
	// SQLServerUID 是只对 SQLServer 有用 uid=
	bool Open(	LPCTSTR szDabaBaseName,
				LPCTSTR szDBPassword = NULL,
				bool	blShowMsgBoxIfFail = true, 
				bool	blSQLServer = false, 
				LPCTSTR szSQLServerUID = NULL, 
				LPCTSTR szSQLServerName = NULL,
				long	iTimeOut = 20);

	bool Execute(LPCTSTR szSQL);
	long State();
	void Close();				// 关闭对象
	_ConnectionPtr &ConnPtr();	// 返回 m_pConn
	LPCTSTR ErrorLastStr();		// 返回上次错误信息字符串
private:
	_ConnectionPtr m_pConn;
	TCHAR m_ErrStr[1024];		// 错误信息的缓冲区
};

class CBAdoRecordset
{
public:
	CBAdoRecordset();
	~CBAdoRecordset();

	// 使用全局 ADOConn 打开 RecordSet
	bool Open(LPCTSTR szSQL, bool blShowMsgBoxIfFail=true);

	// 使用参数指定的 conn 打开 RecordSet
	bool Open(LPCTSTR szSQL, CBAdoConnection &conn, bool blShowMsgBoxIfFail=true);
	bool Open(tstring &sSQL, CBAdoConnection &conn, bool blShowMsgBoxIfFail=true);

	long State();

	bool EOFRs();
	bool MoveNext();
	bool MovePrevious();
	bool MoveFirst();
	bool MoveLast();

	// 获得当前记录的一个字段值
	LPTSTR GetField(LPCTSTR szFieldName);
	LPTSTR GetField(long index);

	// 设置当前记录的一个字段值
	bool SetField(LPCTSTR szFieldName, LPCTSTR szNewValue);
	bool SetField(long index, LPCTSTR szNewValue);

	bool SetField(LPCTSTR szFieldName, int iNewValue);
	bool SetField(long index, int iNewValue);

	bool SetField(LPCTSTR szFieldName, float fNewValue);
	bool SetField(long index, float fNewValue);

	bool SetField(LPCTSTR szFieldName, double dNewValue);
	bool SetField(long index, double dNewValue);


	// 新增一条记录
	bool AddNew();

	// 更新记录集
	bool Update();

	void Close();
	_RecordsetPtr & RsPtr();	// 返回 m_pRst
	LPCTSTR ErrorLastStr();		// 返回上次错误信息字符串

private:
	_RecordsetPtr m_pRst;	
	TCHAR m_ErrStr[1024];		// 错误信息的缓冲区
};

// 定义全局对象（这个一定要有，它还占用一个 CBADOConnection 类的对象的个数
//   即使 CBADOConnection::ms_ConnObjCounts>0，
//   当 CBADOConnection::ms_ConnObjCounts=0 时，将执行 CoUninitialize();
extern CBAdoConnection ADOConn;

