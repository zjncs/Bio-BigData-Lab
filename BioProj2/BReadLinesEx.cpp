//////////////////////////////////////////////////////////////////////
// CBReadLinesEx 类的实现：一行一行地读文件 类 （支持超过4G的大文件）
// 
//////////////////////////////////////////////////////////////////////

#include "BReadLinesEx.h"


const TCHAR CBReadLinesEx::mcszFailInfoDefault[]=TEXT("软件无法读取指定的文件。\r\n请确保磁盘可用并且文件可以访问。");
const TCHAR CBReadLinesEx::mcszFileNotFoundInfoDefault[]=TEXT("文件不存在，或不是一个合法的文件名。");

//////////////////////////////////////////////////////////////////////
// 构造和析构
//////////////////////////////////////////////////////////////////////

CBReadLinesEx::CBReadLinesEx(LPCTSTR szFile/*=NULL*/)
{   
	// 赋初值
	TrimSpaces = false;
	TrimControls = false;
	AutoOpen = true;
	AutoClose = true;
	IgnoreEmpty = false;
	ShowMsgIfErrRead = true;
	AsUnicode = false;
	AsUTF8 = false;
	
	memset(m_szFileName, 0, sizeof(m_szFileName));
	m_hFile = INVALID_HANDLE_VALUE;
	m_iStatus = 0;

	m_szOneLine = NULL;
	m_LastBuff.sizeBytsLeft = 0;
	m_LastBuff.bytsLeft = NULL;

	Init();

	// 如果给出了 szFile 参数，自动打开文件
	if (szFile) FileNameSet(szFile);
}


CBReadLinesEx::~CBReadLinesEx()
{
	CloseFile();

	if (m_szOneLine) { delete []m_szOneLine; m_szOneLine=NULL; }
	if (m_LastBuff.bytsLeft) { delete [] m_LastBuff.bytsLeft; m_LastBuff.bytsLeft = NULL; }
}



//////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////

void CBReadLinesEx::Init()
{
	if (m_LastBuff.bytsLeft) { delete [] m_LastBuff.bytsLeft; m_LastBuff.bytsLeft = NULL; }
	m_LastBuff.sizeBytsLeft = 0;
	
	memset(m_buff.bufBytes, 0, sizeof(m_buff.bufBytes));
	m_buff.iBufLen = 0;
	m_buff.iNextLineStaInBuf = 0;
	m_buff.iPtrInBuf = -1;			// 此作为标志，=-1表示下次运行 GetNextLine 要重新读取新的缓冲区
	m_buff.iIgnoreFirstLf = 0;		// 初始化标志：当前缓冲区不需要忽略第一个字节（若是\n）
									//   否则不重新读取，仍使用当前缓冲区和 .iPtrInBuf 指针
	m_buff.llStartPosAbso = 0;		// 当前缓冲区的起始处所在的文件位置
	
	m_szOneLine=NULL;				// 保存一行字符串内容的空间地址
	m_llFileLength = 0;

	m_bOneEndRead = true;			// 设置标志：关闭后再调用一次 GetNextLine 不出错
	m_bIsEndRead = false;
	m_bErrOccured = false;
	mEmptyStr[0]=0; mEmptyStr[1]=0;
	FailInfoRead = NULL;				// 读取出错时弹出的提示信息，不设置此项，将弹出 mcszFailInfoDefault
	FailInfoFileNotFound = NULL;		// 文件不存在时弹出的提示信息，不设置此项，将弹出 mcszFileNotFoundInfoDefault

	iEndLineSign = 0;
	iEndLineSignLast = 0;
}


int CBReadLinesEx::GetNextLine( LPTSTR &szLine )
{
    // 设置 反映分行符的 iEndLineSign 和 iEndLineSignLast 标志变量
    iEndLineSignLast = iEndLineSign;// 将上一行的分行符更新为当前行的分行符
    iEndLineSign = 0;				// 将当前行的分行符先设为0，在后面程序读完本行后再具体设置
 
	// 设置反映错误的标志变量
    m_bErrOccured = false;			// 表示尚未发生错误；如后续程序中发生了错误再改为 true
 
	// 判断和设置状态
    if (m_iStatus == 0)				// 当前状态非法，尚未打开文件，无法读取
		goto errExit;				
	else if (m_iStatus < 0)			// m_iStatus<0：表示此时文件尚未被打开，或者被强制关闭，
	{								//   或者已经读完文件被自动关闭，总之是不能再继续读取文件了
		if (m_bOneEndRead)			// 若文件已读取完毕，允许再额外地调用一次GetNextLine方法
		{							// 现在允许额外调用一次
			m_bOneEndRead = false;	// 设置标志为 false，不允许再额外调用
			return 0;				// 不出错，但返回0。 此时 m_bErrOccured 仍为 false
        }
		else
			goto errExit;				// 不允许额外调用了，出错
	}

	// ==================================================================
	// 正常读取的情况：此时 m_iStatus 要么为1要么为2，即要么文件已经打开，
	//   要么已经进入读取状态了，总之读取下一行是没有问题的
	m_iStatus = 2;					// 设置为2表示已经进入读取状态

    // //////////////// 读取文件，以找到“一行”的内容 ////////////////
    // 缓冲区逐渐沿文件前进，直到缓冲区起始位置超过文件总长读完文件
    while ( m_buff.llStartPosAbso <= m_llFileLength-1 )
	{
        // ============ （1）根据需要读取文件的下一个缓冲区内容 ============
        // 若 .iPtrInBuf == -1 表示要读取下一个缓冲区，否则不读取下一个，仍使用
        //   当前缓冲区和 .iPtrInBuf 指针
        if ( m_buff.iPtrInBuf < 0 ) 
		{
            // ----从 .llStartPosAbso 开始读取一些字节存入缓冲区 .bufBytes[)
            m_buff.iBufLen = EFGetBytes(m_hFile, m_buff.llStartPosAbso, m_buff.bufBytes, mc_RL_BufLen, 0); 
            if (m_buff.iBufLen < 0) goto errExit;	// 读取出错
            
            // ----初始化缓冲区指针
            m_buff.iPtrInBuf = 0;

            // 看是否需要忽略第一个 \n
			// .iIgnoreFirstLf==1开头是0或lf或lf+0都忽略；==2开头是lf或lf+0都忽略；
			//   ==3开头是0则忽略
            if ( m_buff.iIgnoreFirstLf==1 )   // 开头是0或lf或lf+0都忽略
			{
				if (m_buff.iPtrInBuf < m_buff.iBufLen)
					if (m_buff.bufBytes[m_buff.iPtrInBuf]==0) m_buff.iPtrInBuf++;

				m_buff.iIgnoreFirstLf = 2;  // 进入下面 =2 分支
			}  // ==1 分支结束

			if ( m_buff.iIgnoreFirstLf==2 ) // 开头是lf或lf+0都忽略
			{
				if (m_buff.iPtrInBuf < m_buff.iBufLen)
				{
					if (m_buff.bufBytes[m_buff.iPtrInBuf]==10) 
					{
						m_buff.iPtrInBuf++;
						iEndLineSignLast = 2573;	// 上次的分行符为 \r\n
						m_buff.iIgnoreFirstLf = 3;  // 进入下面 =3 分支
					}
					else // if (m_buff.bufBytes(m_buff.iPtrInBuf)==10) 
					{
						// 第1个字节不是 Lf，而因为要忽略第1个 Lf
						//   说明上一行最后是 Cr，故设置上一行分行符为 Cr
						iEndLineSignLast = 13;
					}   // if (m_buff.bufBytes(m_buff.iPtrInBuf)==10) 
				}  // if (m_buff.iPtrInBuf < m_buff.iBufLen)
			}  // ==2 分支结束


			if ( m_buff.iIgnoreFirstLf==3 ) // 开头是0则忽略
			{
				if (m_buff.iPtrInBuf < m_buff.iBufLen)
					if (m_buff.bufBytes[m_buff.iPtrInBuf]==0) m_buff.iPtrInBuf++;
			}

            m_buff.iIgnoreFirstLf = 0; // 恢复标志，不忽略第一个 Lf(\n)
            
            // 初始化下一行起始位置 iNextLineStaInBuf （下一行内容包含该字节）
            m_buff.iNextLineStaInBuf = m_buff.iPtrInBuf;
        } // end if ( m_buff.iPtrInBuf < 0 )
        
        // ============ （2）逐个扫描缓冲区中的字节，查找分行符 ============
        // 扫描缓冲区中的字节，直到找到 \r或\n 或扫描完缓冲区
        for ( ; m_buff.iPtrInBuf < m_buff.iBufLen; m_buff.iPtrInBuf++ )
            if (m_buff.bufBytes[m_buff.iPtrInBuf] == 13 ||
             m_buff.bufBytes[m_buff.iPtrInBuf] == 10 ) break;

		//////////////////////////////////////////////////////////////////////////
        // 退出 for 后，判断是否找到了分行符 \r或\n
        if (m_buff.iPtrInBuf < m_buff.iBufLen)    // 是否在 .bufBytes[] 中找到了 \r或\n
        {

			//////////////////////////////////////////////////////////////////////////
			// ============ （3）找到一个分行符 \r或\n ============
            // 本行内容为：从 .iNextLineStaInBuf 到位置：af_Buff.iPtrInBuf - 1

			// 开辟动态空间，保存一行内容的字节（pByts 从 [0] 开始用 ～ [sizeByts-1]）；
			// 最后将此中的字节按 Ansi 或 Unicode 转换（拷贝）到 m_szOneLine（通过函数 BytsToString 完成），
			//   将来 szLine = m_szOneLine 并返回 szLine
			// 本分支结束前，delete [] pByts;
			char * pByts = NULL;  LONG sizeByts = 0; 

            // ---- 设置本行换行符 ----
            iEndLineSign = m_buff.bufBytes[m_buff.iPtrInBuf];
            
            // ---- 生成要返回的本行字符串的字节数据 => pByts ----
			sizeByts = m_buff.iPtrInBuf - m_buff.iNextLineStaInBuf + m_LastBuff.sizeBytsLeft + 1;	// +1 为预留 \0
            if ( sizeByts < 2  )
			{
                // .iPtrInBuf == .iNextLineStaInBuf 时，例如 .iPtrInBuf
                //   == .iNextLineStaInBuf == 0 时，即开始就是 \r/\n
                pByts = new char [2];  sizeByts = 2;	// 空串只需1个字节，这里多开辟了1个字节
				*pByts = 0;	// 准备空字符串的字节数据
			}
            else
			{
                // -- 将要返回的字符串的所有字节是上次剩余的字节 和 本次 m_buff.bufBytes[] 中 到
                //   .PtrInBuff 位置的字节(不含 .PtrInBuff 位置)。将这些字节全部存入 pByts[] 数组 --
				pByts = new char [sizeByts];

                // 先将上次剩余的字节 leftBytes 存入 pByts 的开始
				memcpy(pByts, m_LastBuff.bytsLeft, m_LastBuff.sizeBytsLeft);
                
                // 再将本次 [.iNextLineStaInBuf, .iPtrInBuf) 范围的字节 接着存入 pByts 
                //   但不包含 .iPtrInBuf 位置，因为 .iPtrInBuf 是 \r或\n
                memcpy(pByts + m_LastBuff.sizeBytsLeft, 
				   m_buff.bufBytes + m_buff.iNextLineStaInBuf, 
				   m_buff.iPtrInBuf - m_buff.iNextLineStaInBuf);
                
				// 在 pByts[] 中添加末尾 \0
				pByts [sizeByts-1] = 0;	// 也是 pByts [m_LastBuff.sizeBytsLeft + m_buff.iPtrInBuf - m_buff.iNextLineStaInBuf] = 0; 

                // -- 清除上次剩余的字节缓冲区 LeftBytes --
				if (m_LastBuff.bytsLeft) { delete []m_LastBuff.bytsLeft; m_LastBuff.bytsLeft = NULL; }
                m_LastBuff.sizeBytsLeft = 0; 
            }	// end if ( m_buff.iPtrInBuf - m_buff.iNextLineStaInBuf + m_LastBuff.sizeBytsLeft < 1  )

			// ---- 设置数据结构各指针 ----

			// 如果是 vbLf，若下一个字节若是 0 则直接跳过（Unicode格式） ----
            if (m_buff.bufBytes[m_buff.iPtrInBuf] == 10)
			{
				if (m_buff.iPtrInBuf + 1 < m_buff.iBufLen)
				{
					if (m_buff.bufBytes[m_buff.iPtrInBuf+1] == 0) m_buff.iPtrInBuf++;
				}
				else
				{
					m_buff.iIgnoreFirstLf = 3;  // =3开头是0则忽略
				}
            }

			// 判断是否是连续的 \r+\n，若是，跳过下一个 \n
            if (m_buff.bufBytes[m_buff.iPtrInBuf] == 13)  
			{
                if ( m_buff.iPtrInBuf + 1 >= m_buff.iBufLen )
				{
                    // 如果下一个字节已经超过这个缓冲区，则无法判断下一个字节
                    //   是否是 \n，这里只设置标志，以后判断是否 \n 并决定跳过
                    m_buff.iIgnoreFirstLf = 1;
                    iEndLineSign = -1;
				}
                else
				{
                    // 下一个字节没超过这个缓冲区，下一个字节若是 \n 则直接跳过
                    if (m_buff.bufBytes[m_buff.iPtrInBuf + 1] == 10 ) 
                    {
						m_buff.iPtrInBuf ++ ;
                        iEndLineSign = 2573;
                    }
					else if (m_buff.bufBytes[m_buff.iPtrInBuf + 1] == 0 ) 
					{
						// 下一个字节若是 0 则直接跳过（Unicode格式）
						m_buff.iPtrInBuf ++; 
						// 并继续寻找后面的 \n（10）
						if (m_buff.iPtrInBuf + 1 >= m_buff.iBufLen )
						{
							m_buff.iIgnoreFirstLf = 2;
							iEndLineSign = -1;
						}
						else  // if (m_buff.iPtrInBuf + 1 >= m_buff.iBufLen )
						{
							// 在 0 后找 \n（10）//////////////////////////////////////
							if (m_buff.bufBytes[m_buff.iPtrInBuf + 1] == 10 )
							{
								m_buff.iPtrInBuf++;
								iEndLineSign = 2573; 
								// 并继续寻找后面的 0
								if (m_buff.iPtrInBuf + 1 >= m_buff.iBufLen )
								{
									m_buff.iIgnoreFirstLf = 3;
									iEndLineSign = 2573;
								}
								else
								{
									if (m_buff.bufBytes[m_buff.iPtrInBuf + 1] == 0 )
									{
										m_buff.iPtrInBuf++;
										iEndLineSign = 2573; 
									}
								}
							}
							/////////////////////////////////////////////////////////////

						}	  // if (m_buff.iPtrInBuf + 1 >= m_buff.iBufLen )
					}

                }
            }	// if (.bufBytes[.iPtrInBuf] == 13)
            
            // 设置当前缓冲区内部的下一行的起始位置（注：这里还未使 .iPtrInBuf + 1）
            m_buff.iNextLineStaInBuf = m_buff.iPtrInBuf + 1;  // 下一行字符包括这个字节
            
            // ---- 判断是否已经读完文件，若已读完文件，设置各标志，并 CloseFile()
			//   .iPtrInBuf 要 + 1 参与判断，因为本次循环后 .iPtrInBuf 要 +1，现在还未 +1
            //   是否读完文件的标志存到 lIsEndRead，出 if 后据此决定返回值
            if (m_buff.iPtrInBuf + 1 >= m_buff.iBufLen &&
               m_buff.llStartPosAbso + m_buff.iBufLen >= m_llFileLength )
			{
                // 已经读完文件
                m_bIsEndRead = true;
                if (AutoClose) CloseFile();
			}
            else
			{
                // 还未读完文件，再判断是否文件只剩1-3个字节；若只剩1-3个字节并且
                //   剩下的正好是 \n，并且下次要忽略掉 \n，则仍是已经读完文件

				if (m_llFileLength - m_buff.iBufLen - m_buff.llStartPosAbso >=1 &&
					m_llFileLength - m_buff.iBufLen - m_buff.llStartPosAbso <=3 &&
					m_buff.iIgnoreFirstLf > 0)
				{
					// 读取文件中的最后一个字节，只测试一下
					char tByt[mc_RL_BufLen]; LONG tRet;

                    tRet = EFGetBytes(m_hFile, m_buff.llStartPosAbso + m_buff.iBufLen, tByt, mc_RL_BufLen, 0);
                    if (tRet < 0)  goto errExit;   // 出错处理


                    switch (m_buff.iIgnoreFirstLf) 
					{
                    case 1:   // 开头是0或lf或lf+0都忽略
						switch (tRet)
						{
						case 1:
							if (tByt[0] == 0)  {m_bIsEndRead = true; iEndLineSign = 13; }
							if (tByt[0] == 10) {m_bIsEndRead = true; iEndLineSign = 2573; }
							break;
						case 2:
							if (tByt[0] == 0 && tByt[1] == 10) {m_bIsEndRead = true; iEndLineSign = 2573; }
							break;
						case 3:
							if (tByt[0] == 0 && tByt[1] == 10 && tByt[2] == 0) {m_bIsEndRead = true; iEndLineSign = 2573;}
							break;
						}
						break;
					case 2:   // 开头是lf或lf+0都忽略
                        switch (tRet)
                        {
						case 1:
                            if (tByt[0] == 10 ) {m_bIsEndRead = true; iEndLineSign = 2573;}
							break;
                        case 2:
                            if (tByt[0] == 10 && tByt[1] == 0 ) {m_bIsEndRead = true; iEndLineSign = 2573;}
							break;
						}
						break;
					case 3:   // 开头是0则忽略
                        switch (tRet)
						{
                        case 1:
                            if (tByt[0] == 0 ) {m_bIsEndRead = true; iEndLineSign = 2573;}
							break;
						}
						break;
                    } // end switch
                    
                    // 已经读完文件
                    if (m_bIsEndRead)  if (AutoClose) CloseFile();

				}
			}

            m_buff.iPtrInBuf ++ ;
            
			// ---- 将本行字符串的字节数据 pByts 按 Ansi 或 Unicode 转换
			//   为字符串 => szLine 并返回、delete []pByts、退出函数 ---- 
			szLine = BytsToString(pByts, sizeByts);
			delete []pByts; pByts=NULL;

            // 但还要判断是否忽略空行，如果是空行就不退出而继续 Loop
			if (m_bIsEndRead || !(IgnoreEmpty && *szLine == 0) )
			{
                // 已经读完文件；或者不忽略空行，或者忽略空行但不是空行，一定退出函数
                if ( m_bIsEndRead && IgnoreEmpty && *szLine == 0  )
                    return 0;	// 这属于读完文件，且需要忽略空行，且最后一行为空行的，返回0
                else if ( m_bIsEndRead )
					return -1;	// 读完文件，不需要忽略空行或最后不是空行，但读完了文件 返回-1
				else
					return 1;	// 没有读完文件，返回 1
			}
			// else 继续 Loop

			//
			// ============ 结束（3）找到一个分行符 \r或\n ============
			//////////////////////////////////////////////////////////////////////////


        }		// if (m_buff.iPtrInBuf < m_buff.iBufLen)    // 是否在 .bufBytes[] 中找到了 \r或\n  
        else	// if (m_buff.iPtrInBuf < m_buff.iBufLen)    // 是否在 .bufBytes[] 中找到了 \r或\n  
		{

			//////////////////////////////////////////////////////////////////////////
            // ============ （4）没有找到分行符“\r或\n”的处理 ============
            // 设置标志，=-1 表示下次要重新读取新的缓冲区， _
            //   否则将不重新读取，仍使用当前缓冲区和 .iPtrInBuf 指针
            m_buff.iPtrInBuf = -1;
            
            // ==== 看缓冲区中是否还有剩余未处理的字节，若有，
            //    将剩余的存入 m_LastBuff.bytsLeft[] ====
            if (m_buff.iNextLineStaInBuf < m_buff.iBufLen)  
			{
				// 重新定义 m_LastBuff.bytsLeft[] 数组（-1的含义是将个数转换为 ubound）
				Redim(m_LastBuff.bytsLeft, m_LastBuff.sizeBytsLeft + m_buff.iBufLen - m_buff.iNextLineStaInBuf - 1, 
				  m_LastBuff.sizeBytsLeft - 1, true);
                memcpy( m_LastBuff.bytsLeft + m_LastBuff.sizeBytsLeft, 
						m_buff.bufBytes + m_buff.iNextLineStaInBuf, 
						m_buff.iBufLen - m_buff.iNextLineStaInBuf);
                m_LastBuff.sizeBytsLeft +=  m_buff.iBufLen - m_buff.iNextLineStaInBuf;
            }
            
            // ==== 准备继续读下一个缓冲区 ====
            m_buff.llStartPosAbso +=  m_buff.iBufLen;

			//
			// ============ 结束（4）没有找到分行符“\r或\n”的处理 ============
			//////////////////////////////////////////////////////////////////////////
        } // end if (m_buff.iPtrInBuf < m_buff.iBufLen)    // 是否在 .bufBytes[] 中找到了 \r或\n  

    }	// while (m_buff.llStartPosAbso <= m_llFileLength) 缓冲区逐渐沿文件前进，直到缓冲区起始位置超过文件总长读完文件
    
    
    // //////////// 全部读完文件，看还有无剩余的字节 ////////////
    if ( m_LastBuff.sizeBytsLeft )
    {
		// 有剩余的字节，将 m_LastBuff.iLeftBLen 个的 m_LastBuff.leftBytes[] 
		//   按 Ansi 或 Unicode 转换为字符串，由 szLine 返回
		szLine = BytsToString(m_LastBuff.bytsLeft, m_LastBuff.sizeBytsLeft);
		delete []m_LastBuff.bytsLeft; m_LastBuff.bytsLeft = NULL;
		m_LastBuff.sizeBytsLeft = 0;

        if (AutoClose) CloseFile();
        iEndLineSign = -2;		// 读到文件末尾，末尾最后一行无标识符
        m_bIsEndRead = true;

		// 此时读完文件，必须返回
        if ( IgnoreEmpty && *szLine == 0 )  
            return 0;
        else
            return -1;
    }	// end if ( m_LastBuff.iLeftBLen )
	
	// 运行到此处是什么都没有读取到
	if (m_llFileLength==0)
	{
		// 文件字节为0，返回空串，并置读完文件标志
		iEndLineSign = -2; 
		
		// 已经读完文件
		m_bIsEndRead = true;
		if (AutoClose) CloseFile();
		
		// 返回空串
		char * pByts=new char [1];
		*pByts = 0;  
		szLine = BytsToString(pByts, 1);
		delete []pByts; pByts=NULL;

		// 已经读完文件；或者不忽略空行，或者忽略空行但不是空行，一定退出函数
		if ( IgnoreEmpty )
			return 0;	// 这属于读完文件，且需要忽略空行，且最后一行为空行的，返回0
		else
			return -1;	// 读完文件，不需要忽略空行或最后不是空行，但读完了文件 返回-1
	}
	
errExit:
    if (ShowMsgIfErrRead) 
	{
		if (FailInfoRead)
			MsgBox(FailInfoRead, NULL, mb_OK, mb_IconError);
		else
			MsgBox(mcszFailInfoDefault, NULL, mb_OK, mb_IconError);
    }
    m_bErrOccured = true;
    iEndLineSign = 0;
    // 为一般错误，不设置 lIsEndRead = True
    if (AutoClose) CloseFile();

	return 0;
}

LPTSTR CBReadLinesEx::GetFileContent()
{
	if (! OpenFile()) // 打开文件失败，返回空串
		{ 	mEmptyStr[0]=0; mEmptyStr[1]=0; return mEmptyStr; }
	
	LPTSTR szLine;
	TCHAR * szResult = NULL; 
	LONG iUbound=1023, iUboundNew=0, lenUsed = 0;
	Redim(szResult, iUbound, -1, false);
	*szResult=0;

	// 读取第一行，第一行特殊处理，因不加 \r\n
	if (! m_bIsEndRead )
	{
		GetNextLine(szLine);
		if (m_bErrOccured)
		{
			if ( iUbound - lenUsed < lstrlen(szLine)+2 )		// iUbound - lenUsed+1-1（-1表示预留\0）；+2 表示 \r\n
			{
				iUboundNew = iUbound+lstrlen(szLine)+2 + 1000; 
				Redim(szResult, iUboundNew, iUbound, true);
				iUbound = iUboundNew; 
			}
			_tcscpy(szResult, szLine);
			lenUsed += lstrlen(szLine);
		}
	}
	
	// 读取第二行到最末行
	while (!m_bIsEndRead && !m_bErrOccured )
	{
		GetNextLine(szLine);
		if (m_bErrOccured) break;

		if ( iUbound - lenUsed < lstrlen(szLine)+2 )		// iUbound - lenUsed+1-1（-1表示预留\0）；+2 表示 \r\n
		{
			iUboundNew = iUbound+lstrlen(szLine)+2 + 1000;
			Redim(szResult, iUboundNew, iUbound, true);
			iUbound = iUboundNew; 
		}
		_tcscat(szResult, TEXT("\r\n"));
		_tcscat(szResult, szLine);
		lenUsed += lstrlen(szLine)+2;
	}
	CloseFile();

	HM.AddPtr(szResult);
	return szResult;
}

void CBReadLinesEx::SeekFile( LONGLONG llToPos )
{

	if (m_iStatus <= 0)  OpenFile();	// 重新打开文件
	m_buff.llStartPosAbso = llToPos;
	memset(m_buff.bufBytes, 0, sizeof(m_buff.bufBytes));
	m_buff.iBufLen = 0;
	m_buff.iIgnoreFirstLf = 0;
	m_buff.iPtrInBuf = -1;  // 此作为标志，=-1表示下次运行 GetNextLine 要重新读取新的缓冲区
							// 否则不重新读取，仍使用当前缓冲区和 .iPtrInBuf 指针
	m_buff.iNextLineStaInBuf = 1;

	m_LastBuff.sizeBytsLeft = 0;
	if (m_LastBuff.bytsLeft) {delete []m_LastBuff.bytsLeft; m_LastBuff.bytsLeft=NULL;}
	
	m_bIsEndRead = false;
	m_bOneEndRead = true;
}

float CBReadLinesEx::GetPercent( int idotNum /*= 2*/ )
{
    // dotNum保留几位小数，<0或>7为不保留小数
	float fPerc = 0;
	if (m_llFileLength > 0)
	{
		if (m_buff.iPtrInBuf < 0)
			fPerc = (float)(m_buff.llStartPosAbso / m_llFileLength);
		else
			fPerc = (float)(m_buff.llStartPosAbso + m_buff.iPtrInBuf) / m_llFileLength;
	}
	if (idotNum>=0 || idotNum<=7)
	{
		float fWeight = 1;  int i;
		for (i=1; i<=idotNum; i++) fWeight *= 10;
		fPerc = (int)(fPerc * fWeight + 0.5) / fWeight;
	}
    return fPerc;
}


bool CBReadLinesEx::OpenFile(LPCTSTR szFileName /*= NULL*/)
{
    if ( m_hFile != INVALID_HANDLE_VALUE )  CloseFile();   // 如果已打开了文件，则先关闭它
	
	// 如果给定了参数 szFileName，就重新设置文件名 m_szFileName
	if (szFileName) 
		if (lstrlen(szFileName) < sizeof(m_szFileName)/sizeof(TCHAR))
			_tcscpy(m_szFileName, szFileName);

	// 如果文件不存在，就返回 false
	bool fRet = true;
	TCHAR * p=m_szFileName;  
	if (m_szFileName == 0) 
		fRet = false;	// 无文件名
	else if (*m_szFileName == 0) 
		fRet = false;	// 文件名字符串为 ""
	else
	{
		while (*p) p++; p--;
		if (*p == TEXT('\\') || *p == TEXT(':') ) { fRet=false;	goto finishFindFile; } // 以 : 或 \\ 结尾
		WIN32_FIND_DATA fd;
		HANDLE hr=FindFirstFile(m_szFileName, &fd);
		if (hr==INVALID_HANDLE_VALUE) { fRet=false; goto finishFindFile; }
		FindClose(hr);	// 关闭查找句柄
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { fRet=false; goto finishFindFile; } // 为文件夹
		
		m_hFile = EFOpen(m_szFileName, EF_OpStyle_Input);
		if (m_hFile == INVALID_HANDLE_VALUE )  
		{
			CloseFile();
			fRet=false;
			goto finishFindFile;
		}
	}
finishFindFile:
	if (! fRet)
	{
		if (ShowMsgIfErrRead) 
		{
			if (FailInfoFileNotFound)
				MsgBox(StrAppend(m_szFileName, TEXT("\r\n"), FailInfoFileNotFound), NULL, mb_OK, mb_IconError);
			else
				MsgBox(StrAppend(m_szFileName, TEXT("\r\n"), mcszFileNotFoundInfoDefault), NULL, mb_OK, mb_IconError);
		}
		return false;
	}

	
	Init();								// 初始化操作
	m_iStatus = 1;						// 表示文件已打开
	m_llFileLength = EFLOF(m_hFile);	// 设置文件总大小

    // 检查是否为 UTF8 或 Unicode
	char byt3[3];
	if (EFGetBytes(m_hFile, 0, byt3, 3, 0)==3) 
	{
		if ((unsigned char)byt3[0]==0xEF && (unsigned char)byt3[1]==0xBB && (unsigned char)byt3[2]==0xBF)
		{
			AsUTF8 = true;
			m_buff.llStartPosAbso = 3;  // 当前缓冲区的起始处所在的文件位置跳过前3个字节
		}
		else if ((unsigned char)byt3[0]==0xFF && (unsigned char)byt3[1]==0xFE)
		{
			AsUnicode = true;
			m_buff.llStartPosAbso = 2; // 当前缓冲区的起始处所在的文件位置跳过前2个字节
		}
	}

	return true;
}

void CBReadLinesEx::CloseFile()
{
    if (m_hFile != INVALID_HANDLE_VALUE) 
	{
		EFClose(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	m_iStatus = -1; // 表示文件已关闭
    // 不Init，防止读取行后自动关闭文件时状态变量被初始化；在OpenFile时会Init
}


void CBReadLinesEx::SetReadPtrToStart()
{
    // 将文件读取指针指向文件开头，若文件已关闭，则重新打开文件
	if (m_hFile == INVALID_HANDLE_VALUE ||  m_iStatus <= 0)
	{
		OpenFile();
	}
	else
	{
		Init();
		m_llFileLength = EFLOF(m_hFile);
	}
}




//////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////


LPTSTR CBReadLinesEx::BytsToString( const char * pByts, LONG sizeByts )
{
	// 转换结果 => m_szOneLine，最后返回 m_szOneLine
	// 每次 BytsToString 都删除上次一行字符串的空间：m_szOneLine
	if (m_szOneLine) { delete []m_szOneLine; m_szOneLine=NULL; }
	
	// 将 pByts 中的内容延长2个字节 0，保证最后有两个 0 => pByts2
	//（Unicode需要两个0，其他需要一个0，这里统一为两个0）
	int wlen = 0;
	char *pByts2 = new char [sizeByts+2];
	memcpy(pByts2, pByts, sizeByts+2);
	pByts2[sizeByts] = TEXT('\0');
	pByts2[sizeByts+1] = TEXT('\0');

	// 处理 pByts2 中的内容
	if (AsUTF8)
	{
		// pByts2 中的内容是 UTF8 格式，把 pByts2 中的内容转换为 Unicode 格式
		
		// 先转换为 Unicode => wszTemp
		// 获得结果字符串所需空间大小（字符单位）=> wlen，参数 -1 使函数自动计算 pByts2 的长度
		wlen = MultiByteToWideChar(CP_UTF8, 0, pByts2, -1, NULL, 0);
		WCHAR * wszTemp = new WCHAR [wlen];
		MultiByteToWideChar(CP_UTF8, 0, pByts2, -1, wszTemp, wlen);

		// 再将 wszTemp 中的内容倒回 pByts2	
		delete []pByts2;
		pByts2 = new char [wlen*sizeof(WCHAR)];
		memcpy(pByts2, wszTemp, wlen*sizeof(WCHAR));

		// 删除临时空间
		delete []wszTemp; wszTemp = NULL; 
	}

	if (AsUnicode || AsUTF8)
	{
		// pByts2 中的内容都是 Unicode 格式
		#if UNICODE
			// Unicode 版程序直接用 pByts2，存入 => m_szOneLine
			wlen = _tcslen((TCHAR*)pByts2) + 1;
			m_szOneLine = new WCHAR [wlen];
			memcpy(m_szOneLine, pByts2, wlen*sizeof(WCHAR));
		#else
			// ANSI 版程序将 pByts2 转换为 ANSI，存入 => m_szOneLine

			// 获得结果字符串所需空间大小（字节单位），参数 -1 使函数自动计算 pByts2 的长度
			wlen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pByts2, -1, NULL, 0, NULL, NULL);
			m_szOneLine = new char [wlen];
			WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pByts2, -1, m_szOneLine, wlen, NULL, NULL);  
		#endif
	}
	else 
	{
		// pByts2 中的内容都是 ANSI 格式
		#if UNICODE
			// 如是 UNICODE 版的程序，将 pByts2 中的内容转换为 Unicode 字符串存入=> m_szOneLine
			// 获得结果字符串所需空间大小（字符单位）=> wlen，参数 -1 使函数自动计算 pByts 的长度
			wlen = MultiByteToWideChar(CP_ACP, 0, pByts2, -1, NULL, 0);
			m_szOneLine = new WCHAR [wlen];
			MultiByteToWideChar(CP_ACP, 0, pByts2, -1, m_szOneLine, wlen);
		#else
			// 如是 ANSI 版的程序，pByts2 中的内容就是 ANSI，直接存入=> m_szOneLine
			wlen = _tcslen((TCHAR*)pByts2) + 1;
			m_szOneLine = new TCHAR [wlen];
			memcpy(m_szOneLine, pByts2, wlen*sizeof(TCHAR));
		#endif	
	}

	delete []pByts2; pByts2=NULL;
	
	if (TrimSpaces || TrimControls)
	{
		TCHAR * szOneLineTemp = new TCHAR [lstrlen(m_szOneLine)+1];
		// 删除前导空格
		TCHAR *p = (TCHAR *)m_szOneLine;
		while (*p && 
		   ( (TrimSpaces && (*p==TEXT(' ') || _istspace(*p))) ||
		     (TrimControls && ((*p>=0x01 && *p<=0x08) || (*p>=0x0e && *p<=0x1F) || (*p==0x127) )) 
		   )
		) 	p++;	// 指向源字符串的第一个非空格
		_tcscpy(szOneLineTemp, p);	// 从 p 的位置拷贝字符串
		
		// 删除尾部空格
		p=(TCHAR *)szOneLineTemp;
		while (*p) p++; p--;	// 指向最后一个字符
		while (*p && 
		   ( (TrimSpaces && (*p==TEXT(' ') || _istspace(*p))) ||
		     (TrimControls && ((*p>=0x01 && *p<=0x08) || (*p>=0x0e && *p<=0x1F) || (*p==0x127) )) 
		   )
		) p--;
		p++;
		*p=0;
		
		_tcscpy(m_szOneLine, szOneLineTemp);
		
		delete []szOneLineTemp;
	}

	return m_szOneLine;
}



//////////////////////////////////////////////////////////////////////
// 返回或设置属性值的方法
//////////////////////////////////////////////////////////////////////

void CBReadLinesEx::FileNameSet( LPCTSTR szFile )
{
	if (m_hFile != INVALID_HANDLE_VALUE) 
		{ EFClose (m_hFile);m_hFile=INVALID_HANDLE_VALUE;}
	
	if (lstrlen(szFile) < sizeof(m_szFileName)/sizeof(TCHAR))
		_tcscpy(m_szFileName, szFile);
    if (AutoOpen) OpenFile();	// 自动打开文件
}

LPTSTR CBReadLinesEx::FileName()
{
	return m_szFileName;
}

HANDLE CBReadLinesEx::hFile()
{
	return m_hFile;
}

int CBReadLinesEx::Status()
{
	return m_iStatus;
}

bool CBReadLinesEx::IsEndRead()
{
	return m_bIsEndRead;
}

bool CBReadLinesEx::IsErrOccured()
{
	return m_bErrOccured;
}

