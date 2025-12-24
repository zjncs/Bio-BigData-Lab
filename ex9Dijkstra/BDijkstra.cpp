//////////////////////////////////////////////////////////////////////
// CBDijkstra 类的实现：求无向图的最短路径
//
//////////////////////////////////////////////////////////////////////

#include "BDijkstra.h"
#include "BReadLinesEx.h"


const long CBDijkstra::mcMaxDistance=999999999;
const int CBDijkstra::mcNodesExpandStep = 50000;

//////////////////////////////////////////////////////////////////////////
// 构造和析构
//////////////////////////////////////////////////////////////////////////

CBDijkstra::CBDijkstra( int ik/*=1*/ )
{
	m_k = ik;	// k=1时仅求最短路径
	
	m_Nodes = NULL;
	m_NodesCount = 0;  m_NodesUbound=-1;
	
	m_indexStart=0;
	ShowMsgIfFail=true;
	memset(ErrDescription, 0, sizeof(ErrDescription));
}

CBDijkstra::~CBDijkstra()
{
	Clear();
}



//////////////////////////////////////////////////////////////////////////
// 公有函数
//////////////////////////////////////////////////////////////////////////


int CBDijkstra::LoadFileData( LPCTSTR szFileName, int ik/*=0*/, pFunDjReadFileCallBack pf /*= NULL*/, long userDataCallBack /*= 0*/ )
{
	CBReadLinesEx dataFile;
	LPTSTR szLine=NULL, delimiter=NULL;
	TCHAR ** s=NULL;  
	int iFlds=0, i=0, j=0;
	bool blRetCallBack=false;

	if (ik>0) m_k = ik;

	if (! dataFile.OpenFile(szFileName)) 
	{
		_tcscpy(ErrDescription, TEXT("Open data file failed!"));  // "打开数据文件失败"
		return 0;  // 打开文件失败
	}
	
	dataFile.TrimControls = true;
	dataFile.TrimSpaces = true;
	dataFile.IgnoreEmpty = true;
	dataFile.ShowMsgIfErrRead = ShowMsgIfFail;

	// ============ 准备数据结构 ============
	// 清除所有已有数据
	if (m_Nodes) Clear();

	// ============ 按行读取文件，读取结点和连接数据 ============
	long ii=0;

	// 开始调用一次回调函数
	blRetCallBack=(*pf)(0, userDataCallBack);
	if (! blRetCallBack) 
	{ _tcscpy(ErrDescription, _TEXT("\nReading file not finished. Break by the user. ")); return 0;}

	// 读取文件
	while (! dataFile.IsEndRead())
	{
		dataFile.GetNextLine(szLine);
		if (dataFile.IsErrOccured()) 
		{
			_tcscpy(ErrDescription, TEXT("Error occurred when reading file. "));  // "读取数据文件出错"
			return 0;
		}
		
		// 如果还未确定各字段分隔符，现在确定
		if (delimiter==NULL)
		{
			if (InStr(szLine, TEXT(" ")))
				delimiter = TEXT(" ");
			else if (InStr(szLine, TEXT("\t")))
				delimiter = TEXT("\t");
			else if (InStr(szLine, TEXT(";")))
				delimiter = TEXT(";");
			else if (InStr(szLine, TEXT(",")))
				delimiter = TEXT(",");
			else
				delimiter = TEXT(" ");	// 默认都是空格分隔
		}
		
		// 分隔 szLine 行的字段
		if (*szLine==0) continue;		// 如果是空串，继续
		iFlds=Split(szLine, s, delimiter);
		if (iFlds<3)
		{
			_tcscpy(ErrDescription, StrAppend(szLine, TEXT("\r\n"), 
				TEXT("Can not find 3 or more fields. Reading file aborted. ")));  // "不能找到3个或以上的字段，数据读取终止。"
			if (ShowMsgIfFail) MsgBox(ErrDescription, 
				TEXT("Dijkstra data reading fail"), mb_OK, mb_IconExclamation);   // "Dijkstra数据读取失败"
			return 0;
		}
		
		// ============ 处理本行字段 ============
		if ( ! AddNodesDist( (long)Val(s[1]) , (long)Val(s[2]), (long)Val(s[3]) ) )
		{	_tcscpy(ErrDescription, StrAppend(szLine, TEXT("\r\n"), 
				TEXT("Can not save NODE "), TEXT(". "), TEXT("\r\n"), ErrDescription));    // "不能添加结点 "
			if (ShowMsgIfFail) MsgBox(ErrDescription, 
				TEXT("Dijkstra inner data structure error"), mb_OK, mb_IconExclamation); // "Dijkstra内部数据结构错误"
			return  0;
		}

		ii++;
		if (pf && ii%5000==0)
		{
			blRetCallBack=(*pf)(dataFile.GetPercent(2), userDataCallBack);
			if (! blRetCallBack) 
			{	
				_tcscpy(ErrDescription, 
				  _TEXT("\nReading file not finished. Break by the user. ")); 
				return 0;	// break the reading file
			}		
		}

	}  // end while (! dataFile.IsEndRead())

	// 返回结点总数
	return m_NodesCount;
}



long CBDijkstra::GetDistance( long idNodeStart, long idNodeEnd, long &rDistance, long *&pIdsInPath, pFunDjCalculatingCallBack pf/*=NULL*/, long userDataCallBack /*= 0*/ )
{
	int iStart = 0, iEnd = 0;
	long *ids = NULL, iUboundIds=-1;		// ids 指向一个数组（下标从1开始用，0浪费），数组逆序保存最短路径各结点 id
	int ctCount=0;							// 最短路径上的结点个数 
	long iIdsExpandStep = 100;				// ids 数组每次扩增的空间个数
	long i = 0, j=0;

	// 从结点 id 获得起始结点 index => iStart
	iStart=m_hashNodesIdxes.Item(idNodeStart,false);
	if (iStart<=0) 
	{
		_tcscpy(ErrDescription, StrAppend(TEXT("Getting path failed: not found starting Node: "), Str(idNodeStart), TEXT(". ") ) );  // "获得路径失败：没有找到起始结点 "
		if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Getting path failed"), mb_OK, mb_IconExclamation);
		return false;
	}
	
	// 从结点 id 获得终止结点 index => iEnd
	iEnd = m_hashNodesIdxes.Item(idNodeEnd, false);
	if (iEnd<=0) 
	{
		_tcscpy(ErrDescription, StrAppend(TEXT("Getting path failed: not found ending Node: "), Str(idNodeEnd), TEXT(". ") ) );   // "获得路径失败：没有找到终止结点 "
		if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Getting path failed"), mb_OK, mb_IconExclamation);
		return false;
	}
	
	// 计算最短路径状态
	if ( m_indexStart<=0 || m_indexStart != iStart )		// 需计算或重新计算最短路径状态，存入数据结构
		if (! Calculate(idNodeStart, pf, userDataCallBack) ) return 0;			// 计算失败

	// 释放 pIdsInPath 的内存（如果有的话）
	if (HM.IsPtrManaged(pIdsInPath))
		HM.Free(pIdsInPath);

	// 从 m_Nodes[iEnd] 回溯最短路径结点
	i = iEnd;
	while (i != iStart && ctCount<m_NodesCount)
	{
		ctCount++;
		if (ctCount>iUboundIds)
			iUboundIds = Redim(ids, iUboundIds+iIdsExpandStep, iUboundIds, true);
		ids[ctCount] = m_Nodes[i].ID;

		// 如果 m_Nodes[i] 没有回溯父结点，返回无路径
		if ( m_Nodes[i].pParentList->Count() == 0 )
		{
			rDistance = mcMaxDistance;	// 返回最大值
			delete []ids;
			pIdsInPath = NULL;
			return -1;					// 返回无路径
		}

		// 将 m_Nodes[i] 的回溯父结点 index => i
		i = m_Nodes[i].pParentList->Item(1);
		if ( i<=0 || i>m_NodesCount )		// 容错
		{
			_tcscpy(ErrDescription, StrAppend(TEXT("Getting path failed: invalid parent of NODE (index="), Str(i) , TEXT("). ") ));   //"获得路径失败：路径中途的回溯结点无效，index="
			if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Getting path failed"), mb_OK, mb_IconExclamation);
			delete []ids;
			pIdsInPath = NULL;
			rDistance = 0;
			return false;
		}
	}	// end while (i != iStart && ctCount<m_NodesCount)

	if (i != iStart)	// 容错
	{
		_tcscpy(ErrDescription, StrAppend(TEXT("Getting path failed: can not trace back to Starting NODE '"), Str(idNodeStart) , TEXT("'. ") ));  //"获得路径失败：在路径中不能回溯到到起始结点 "
		if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Getting path failed"), mb_OK, mb_IconExclamation);
		delete []ids;
		pIdsInPath = NULL;
		rDistance = 0;
		return false;
	}

	// 将路径中的各结点 id，由 ids 逆向存入结果数组 pIdsInPath；
	//  并增加起始结点 id
	// 本函数自行开辟空间并用 HM 管理空间
	pIdsInPath = new long [ctCount+1];	// 下标从0开始使用；+1表示多一个起始结点 id
	HM.AddPtr(pIdsInPath);

	pIdsInPath[0] = idNodeStart;  j=1;	// 手动添加起始结点
	for(i=ctCount; i>0; i--)
		pIdsInPath[j++]=ids[i];

	// 返回
	rDistance = m_Nodes[iEnd].w;
	delete []ids;
	return ctCount+1;
}



void CBDijkstra::Clear()
{
	int i;
	for (i=1; i<=m_NodesCount; i++)
	{
		if (m_Nodes[i].pEdges) 
		{ delete m_Nodes[i].pEdges; m_Nodes[i].pEdges=NULL; }
		
		if (m_Nodes[i].pParentList) 
		{ delete m_Nodes[i].pParentList; m_Nodes[i].pParentList=NULL; }
		
		if ( m_Nodes[i].tag ) 
		{ delete []m_Nodes[i].tag; m_Nodes[i].tag=NULL; }
	}

	if (m_Nodes) { delete []m_Nodes; m_Nodes=NULL; }
	m_NodesCount = 0;  m_NodesUbound=-1;

	m_hashNodesIdxes.Clear();
	m_hashNodesIdxes.AlloMem(mcNodesExpandStep);

	// 开辟第一批 mcNodesExpandStep 个空间
	Redim(m_Nodes, mcNodesExpandStep, -1, false);
	m_NodesUbound=mcNodesExpandStep;

	m_indexStart = 0;					// 当前 m_Nodes[] 中所记录的最短路径状态的起始结点无效
	m_lkNodesTouch.Clear();
	memset(ErrDescription, 0, sizeof(ErrDescription));
}











//////////////////////////////////////////////////////////////////////////
// 私有函数
//////////////////////////////////////////////////////////////////////////





bool CBDijkstra::Calculate(long idNodeStart, pFunDjCalculatingCallBack pf/*=NULL*/, long userDataCallBack/*=0*/)
{
	int ctVisits = 0;			// 已 visited 结点数
	int iStart=0;				// 对应 idNodeStart 的起始结点 index
	int iVertex=0;				// 每次循环时要考察结点的 index（在 m_lkNodesTouch 中 w 最小）
	long distance=0;			// 临时变量
	long i=0, j=0, t=0, ix=0;	// 临时变量
	CBArrLink * pe = NULL;		// 临时变量
	bool blRetCallBack=false;	// 回调函数返回值

	if (m_NodesCount<=0 || m_Nodes==NULL)
	{
		_tcscpy(ErrDescription, TEXT("Calculating path failed: No data nodes exist. ") );	// "计算路径失败：没有任何结点数据。" 
		if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Calculating path failed"), mb_OK, mb_IconExclamation);
		return false;
	}


	// 从结点 id 获得起始结点 index => iStart
	iStart=m_hashNodesIdxes.Item(idNodeStart,false);
	if (iStart<=0) 
	{
		_tcscpy(ErrDescription, StrAppend(TEXT("Calculating path failed: not found Starting NODE '"), Str(idNodeStart), TEXT("'. ") ) );		// "计算路径失败：没有找到起始结点 "
		if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Calculating path failed"), mb_OK, mb_IconExclamation);
		return false;
	}


	// ============ 准备工作 ============
	// 将所有结点的 w 设为无穷大、fVisited设为 false、清除所有结点的回溯父结点
	for (i=1; i<=m_NodesCount; i++)
	{
		m_Nodes[i].w = mcMaxDistance;
		m_Nodes[i].fVisited = false;

		if (m_Nodes[i].pParentList) 
			m_Nodes[i].pParentList->Clear();
		else	// 容错：应该早已在 LoadFileData 时 = new 了
			m_Nodes[i].pParentList = new CBArrLink;
	}
	m_indexStart = 0;					// 当前 m_Nodes[] 中所记录的最短路径状态的起始结点无效
	m_lkNodesTouch.Clear();				// 清除“已涉及结点”为空
	memset(ErrDescription, 0, sizeof(ErrDescription));	// 清除错误信息

	// 设置起始结点的 fVisited
	m_Nodes[iStart].fVisited = true;	// 设置起始结点 visited=true （起始结点不设 w、pParentList）
	ctVisits = 1;						// 已有1个结点 visited
	
	// 考察与起始结点直接相连的所有结点，将这些结点：
	//   都设置其 w 为此相连边的长度、都设其回溯父结点（pParentList）为起始结点
	//   并且将其加入 m_lkNodesTouch 为初始“涉及到”的结点
	pe = m_Nodes[iStart].pEdges;
	t = pe->Count();
	for (i=1; i<=t; i++)
	{
		// 获得与起始结点直接相连的一个结点 的 index、distance => index、distance
		ix = pe->Item(i);  distance = pe->Item2(i);
		// 设置与起始结点直接相连的这一个结点的 w 为 distance
		m_Nodes[ix].w = distance;  
		// 设置与起始结点直接相连的这一个结点的回溯父结点为：起始结点
		m_Nodes[ix].pParentList->Clear();
		m_Nodes[ix].pParentList->Add(iStart);

		// 将 与起始结点直接相连的这一个结点 加入 m_lkNodesTouch
		m_lkNodesTouch.Add (ix);
	}
	
	// ============ 循环 m_lkNodesTouch 中的所有结点（在循环中 m_lkNodesTouch 还会被新增新内容） ============
	int ctCycled=0;

	// 调用一次回调函数
	blRetCallBack=(*pf)(ctCycled, m_lkNodesTouch.Count(), userDataCallBack);
	if (! blRetCallBack) 
	{
		_tcscpy(ErrDescription, 
			_TEXT("\nFinding path not finished. Break by the user. ")); 
		return 0;	// break the calculation
	}

	// 每次从 m_lkNodesTouch 中取一个 w 最小的结点进行操作，并设此结点已 visited
	while (m_lkNodesTouch.Count()>0 && ctVisits<m_NodesCount-1)
	{	// 如果 m_lkNodesTouch.Count()==0 为空，即使 ctVisits<m_NodesCount-1 也结束，
		//	因都是 ∞ 了没必要从 ∞ 中选一个再在 ∞ 基础上去累加距离

		// ==== 取 m_lkNodesTouch 中 w 最小的结点 => iVertex, 所在 m_lkNodesTouch 中的下标 =>j ====
		iVertex=-1;
		j=0; 
		distance=mcMaxDistance;
		t=m_lkNodesTouch.Count();
		for (i=1; i<=t; i++)
		{
			ix = m_lkNodesTouch.Item(i);
			if ( m_Nodes[ix].w < distance ) { j=i; distance=m_Nodes[ix].w; }
		}
		iVertex = m_lkNodesTouch.Item(j);
		if (iVertex<=0)	// 容错
		{
			_tcscpy(ErrDescription, StrAppend(TEXT("Calculating path failed: Can not find a NODE with minimum w in the "), Str(ctVisits+1), TEXT("th cycle. ") ) );     // "计算路径失败：第 "   " 次循环不能找到 w 值最小的结点。"
			if (ShowMsgIfFail) MsgBox(ErrDescription, TEXT("Dijkstra Calculating path failed"), mb_OK, mb_IconExclamation);
			return false;
		}

		// ==== 设置标志 ====
		m_Nodes[iVertex].fVisited = true;	// 设置此结点已 visited
		ctVisits++;							// 已 visited 结点数 +1
		m_lkNodesTouch.Remove(j);			// 从 m_lkNodesTouch 中删除这一结点

		// ==== 内层循环：循环所有与 iVertex 直接相连、且未 visited 的结点 ====
		// 用 ix 保存每次循环要操作的这样的一个结点的 index
		pe = m_Nodes[iVertex].pEdges; 
		t = pe->Count();
		for (i=1; i<=t; i++)
		{
			ix = pe->Item(i);			// 用 ix 保存每次循环要操作的这样的一个结点的 index
			distance = pe->Item2(i);	// 从 iVertex 到 ix 的边权值为 pe->Item2(i)

			if ( m_Nodes[ix].fVisited ) continue; 

			// 更新 m_Nodes[ix].w
			if ( m_Nodes[ix].w >=mcMaxDistance )
			{
				// m_Nodes[ix].w 是由无穷大被更新的
				m_Nodes[ix].w = m_Nodes[iVertex].w + distance;	// 更新 m_Nodes[ix].w
				m_Nodes[ix].pParentList->Clear();				// 更新 m_Nodes[ix] 的回溯父结点为 iVertex
				m_Nodes[ix].pParentList->Add(iVertex);

				// 如果 m_Nodes[ix].w 是由无穷大被更新的，就在 m_lkNodesTouch 中添加 ix
				m_lkNodesTouch.Add(ix);
			}
			else	// if ( m_Nodes[ix].w >=mcMaxValue )
			{
				// 否则只需更新 m_Nodes[ix].w 即可
				if ( m_Nodes[iVertex].w + distance < m_Nodes[ix].w ) 
				{
					m_Nodes[ix].w = m_Nodes[iVertex].w + distance;	// 更新 m_Nodes[ix].w
					m_Nodes[ix].pParentList->Clear();				// 更新 m_Nodes[ix] 的回溯父结点为 iVertex
					m_Nodes[ix].pParentList->Add(iVertex);
				}
				else if ( m_Nodes[iVertex].w + distance == m_Nodes[ix].w ) 
				{
					// 并列最短路径
					m_Nodes[ix].pParentList->Add(iVertex);			// 在 m_Nodes[ix] 的回溯父结点中添加一个结点 iVertex
				}	// if ( m_Nodes[iVertex].w + distance < m_Nodes[ix].w ) 
			}	// if ( m_Nodes[ix].w >=mcMaxValue ) // 是否 m_Nodes[ix].w 是无穷大
		}	// for (i=1; i<=t; i++)
		// ==== 内层循环结束 ====
		// ====================================================================

		ctCycled++;
		if (pf && ctCycled%299==0)
		{
			blRetCallBack=(*pf)(ctCycled, m_lkNodesTouch.Count(), userDataCallBack);
			if (! blRetCallBack) 
			{ 
				_tcscpy(ErrDescription, 
					_TEXT("\nFinding path not finished. Break by the user. ")); 
				return 0;	// break the calculation
			}		
		}
	}	// end of while

	// 完成：调用一次回调函数
	blRetCallBack=(*pf)(ctCycled, 0, userDataCallBack);	// 第2个参数给0，表示完成
	if (! blRetCallBack) 
	{
		_tcscpy(ErrDescription, 
			_TEXT("\nFinding path not finished. Break by the user. ")); 
		return 0;	// break the calculation
	}

	// 释放 pe
	pe = NULL;

	// 设置当前 m_Nodes[] 中所记录的最短路径状态的起始结点为有效值
	m_indexStart = iStart;
	
	// ============ 返回值 ============
	return true;
}






int CBDijkstra::NodeCount()
{
	return m_NodesCount;	
}

int CBDijkstra::NodeIndex( long idNode )
{
	return m_hashNodesIdxes.Item(idNode, false);
}


long CBDijkstra::NodeID( int idxNode )
{
	if (idxNode<1 || idxNode>m_NodesCount)
		return 0;	// for error
	else
		return m_Nodes[idxNode].ID;
}

int CBDijkstra::NodeAdjEdgesCount( int idxNode )
{
	if (idxNode<1 || idxNode>m_NodesCount)
		return 0;	// for error
	else
	{
		if (m_Nodes[idxNode].pEdges==NULL)
			return 0;	// for error
		else
			return m_Nodes[idxNode].pEdges->Count();	
	}
}

long CBDijkstra::NodeOneAdjEdge( int idxNode, int idxEdge, int &idxNodeAdj )
{
	idxNodeAdj = 0;  // set firstly. If error occurred below, idxNodeAdj is still be set to 0

	if (idxNode<1 || idxNode>m_NodesCount)
		return 0;	// for error
	else
	{
		if (m_Nodes[idxNode].pEdges==NULL)
			return 0;	// for error
		else
		{
			if (idxEdge<1 || idxNodeAdj>m_Nodes[idxNode].pEdges->Count())
				return 0;	// for error
			else
			{
				idxNodeAdj = m_Nodes[idxNode].pEdges->Item(idxEdge);
				return m_Nodes[idxNode].pEdges->Item2(idxEdge);
			}
		}
	}	
}

bool CBDijkstra::AddNodesDist( long idNode1, long idNode2, long distance )
{
	long idNode[3]={0}; int indexNode[3]={0};  // 两个结点的 ID、Index
						//（下标从1开始，0浪费不用，两个结点故需3个空间）
	int iFlds=0, i=0, k=0; 

	// 找到结点 idNode[i] 在 m_Nodes[] 数组中的下标 => idxNode[i]
	//   若尚无此结点，先添加此结点，然后获得新下标 => idxNode[i]
	idNode[1] = idNode1; idNode[2] = idNode2;
	for (i=1; i<=2; i++)	// 处理 2 个结点
	{
		if (! m_hashNodesIdxes.IsKeyExist(idNode[i]) )	// 如尚未添加过此结点
		{
			// ========================================
			// 添加新结点 idNode[i] 
			m_NodesCount ++;

			// 准备空间
			if (m_NodesCount>m_NodesUbound)
			{
				// 扩大 m_Nodes 的空间
				Redim(m_Nodes, m_NodesUbound+mcNodesExpandStep, m_NodesUbound, true);
				m_NodesUbound = m_NodesUbound+mcNodesExpandStep; 
			}

			// 在 m_Nodes[m_NodesCount] 中添加新结点 idNode[i]
			m_Nodes[m_NodesCount].fVisited = false;
			m_Nodes[m_NodesCount].ID = idNode[i];
			m_Nodes[m_NodesCount].pEdges = new CBArrLink;
			m_Nodes[m_NodesCount].w = mcMaxDistance;		// 无穷大值
			m_Nodes[m_NodesCount].pParentList = new CBArrLink;	
			m_Nodes[m_NodesCount].tag = NULL;

			// 将新结点下标 和 idNode[i] 的对应关系，记录到哈希表
			if (! m_hashNodesIdxes.Add(m_NodesCount, idNode[i], 0, 0, 0, 0, 0.0, false))
			{
				_tcscpy(ErrDescription, StrAppend( 
					TEXT("Can not add NODE "), Str(idNode[i]), TEXT(". ")));    // "不能添加结点 "
				return false;
			}
			// 添加新结点 idNode[i]完成
			// ========================================

			// 新结点 idNode[i] 的 index => indexNode[i]
			indexNode[i] = m_NodesCount;
		} 
		else // if (! m_hashNodesIdxes.IsKeyExist(idNode[i]) )	// 如尚未添加过此结点
		{
			// 已经添加过此结点，不重复添加，根据 idNode[i] 找到其 index => indexNode[i]
			indexNode[i] = m_hashNodesIdxes.Item(idNode[i], false);
		}	// end if (! m_hashNodesIdxes.IsKeyExist(idNode[i]) )	// 如尚未添加过此结点
	}	// end for (i=1; i<=2; i++) // 处理 2 个结点

	CBArrLink *phs; int idxNode;
	for(i=1; i<=2; i++)		// 处理 2 个结点
	{
		// 对 indexNode[1]：将与 indexNode[2] 相连、及其 distance 信息添加到 m_Nodes[indexNode[1]]
		// 对 indexNode[2]：将与 indexNode[1] 相连、及其 distance 信息添加到 m_Nodes[indexNode[2]]
		phs = m_Nodes[indexNode[i]].pEdges;
		if (i==1) idxNode = indexNode[2]; else idxNode = indexNode[1];

		// 检查结点 indexNode[i] 的“边”数据中是否已记录有了与结点 idxNode 相连的距离
		for (k = 1; k <= phs->Count(); k++)
			if ( phs->Item(k) == idxNode ) break;
		if ( k <= phs->Count() )
		{
			//   已记录了该距离。校验记录的距离是否和 iDis 一致；如一致无事，如不一致，报错返回
			if (phs->Item2(k) != distance)
			{
				_tcscpy(ErrDescription, StrAppend( 
					TEXT("Found confilict duplicate distances between NODE '"), Str(m_Nodes[indexNode[i]].ID), 
					TEXT("' and NODE '"), Str(m_Nodes[idxNode].ID), TEXT("': "), Str(distance), 
					TEXT(". Another distance already set is "), Str(phs->Item2(k)), TEXT(".") ));    
				return false;
			}
			// 到此处说明虽然有重复记录，但已记录的和本次的 iDis 一致，不再添加重复“边”距离，直接下一个即可
		}	// if (k<=phs->Count())
		else
		{
			// 添加新的“边”距离
			phs->Add(idxNode, distance);	// node index, distance

		}	// end if (k<=phs->Count()) -- else
		phs=NULL;
	}

	return true;
}




// void CBDijkstra::PrintLinks()
// {
// 	int i,j;
// 	for (i=1; i<=m_NodesCount; i++)
// 	{
// 		cout<<i<<":"<<m_Nodes[i].ID<<"\t";
// 		for (j=1; j<=m_Nodes[i].pEdges->Count(); j++)
// 			cout<<m_Nodes[i].pEdges->Item(j)<<"("<<m_Nodes[i].pEdges->Item2(j)<<") ";
// 		cout<<endl;
// 	}
// 	cout<<endl<<endl<<"哈希表情况："<<endl;
// 	cout<<m_hashNodesIdxes.Count()<<endl;
// 	for (i=1; i<=m_hashNodesIdxes.Count(); i++)
// 		cout<<m_hashNodesIdxes.IndexToKey(i)<<" "<<m_hashNodesIdxes.Item(i)<<endl;
// }

