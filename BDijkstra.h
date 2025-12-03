//////////////////////////////////////////////////////////////////////
// CBDijkstra类：Dijkstra 算法求无向图的最短路径类
//
// 支持：需要 BWindows、BReadLinesEx 模块的支持（需要 CBHashLK、CBArrLink 类）
//////////////////////////////////////////////////////////////////////


#pragma once

#include "BWindows.h"

//////////////////////////////////////////////////////////////////////////
// About NODE ID and NODE Index:
// -----------------------------
// NODE ID is defined by users, the value can be any, and can be non-continuous. 
// NODE Index is continuous, from 1 to NodeCount. 
// NODE ID is primarily for user use; NODE Index is primarily for the
//   inner data structure use. 
//////////////////////////////////////////////////////////////////////////


// function pointer type pointing to a call back function for CBDijkstra.LoadFileData processing
// the function pointer can be specified in LoadFileData() call
//   if specified, LoadFileData() will call back the function when each several lines 
//   have been read
// parameters:
// fPercentRead:	file content percent (<=1.0) already read
// userData:		User defined data (specified in LoadFileData() call) 
// the call back function should returns true; if returns false, the LoadFileData() 
//   will beak reading file and exit immeadiately
typedef bool (*pFunDjReadFileCallBack)(float fPercentRead, long userData);


// function pointer type pointing to a call back function for CBDijkstra.GetDistance processing
// the function pointer can be specified in GetDistance() call
//   if specified, GetDistance() will call back the function when several cycles 
//   have been processed (Actually called back by Calculate())
// parameters:
// iCycled:	 how many cycles have been processed(incresing)
// iTotalCycleCurrent:  how many cycles left should be processed (may changing)
// userData:		User defined data (specified in GetDistance() call) 
// the call back function should returns true; if returns false, the GetDistance() 
//   will beak reading file and exit immeadiately
typedef bool (*pFunDjCalculatingCallBack)(int iCycled, int iTotalCycleCurrent, long userData);

class CBDijkstra
{
// ================ 类型定义 ================ 
private:
	typedef struct _NodeInfType		// 保存一个结点的信息
	{
		long ID;					// 结点 ID（用户使用ID，数值可以不连续；与结点 index 不同，后者从1开始必须连续）
		CBArrLink *pEdges;			// 指向一个 CBArrLink 的对象，后者保存该结点所连接的所有边(结点 index, 边权值)
									//   pEdges->Item1() 为直接连接的结点 index；
									//	 pEdges->Item2() 为直接连接边的权值
									// 在读取文件时，要判断以支持该结构中的数据不重复
									//  （否则当数据文件中有两条记录：1 2 dist和2 1 dist时该结构的Item1、Item2可有重复）
 
		bool fVisited;				// 算法过程中使用的标志变量：遍历时是否被访问完毕
		long w;						// 该结点上目前为止的“最小路径和值”W
		CBArrLink *pParentList;		// 达成最小路径的上一回溯父结点 index，使用列表以支持并列最短路径

		LPTSTR tag;					// 结点附加数据
	} NodeInfType;


private:
	static const long mcMaxDistance;			// 无穷大值
	static const int mcNodesExpandStep;			// 结点动态空间每次扩大的结点个数

// ================ 数据成员和函数成员 ================
public:
	bool ShowMsgIfFail;				// 失败时是否自动弹出提示
	TCHAR ErrDescription[1024];		// 错误信息

public:
	CBDijkstra(int ik=1);
	~CBDijkstra();
	
	// 读取一个数据文件数据并计算，成功返回结点数，失败返回 0
	// 数据文件为文本文件，没有标题行，每行有3列：“结点1ID  结点2ID  距离值”
	//   分隔符支持空格、Tab、逗号、分号，但要全文件统一使用一种分隔符
	// k=0 时使用原来设置过的本对象目前的 k 值；否则使用本参数给出的新 k 值
	// a user-defined call back function can be specified by pf, which the prototype is :
	//
	//       bool FunReadFileCallBack(float fPercentRead, long userData);
	//
	// if specified, LoadFileData will call this function back when each several lines 
	//   have been read. The function should returns true if the user want to continue. 
	//   If the function returns false, LoadFileData() will stop reading file and exit. 
	int LoadFileData(LPCTSTR szFileName, int ik=0, pFunDjReadFileCallBack pf = NULL, long userDataCallBack = 0);	
	
	// 添加一个“关系”：两个结点的ID、以及它们的距离
	// 成功返回 True，失败返回 False（会设置 ErrDescription）
	//   如需清除本对象中的所有已添加“关系”，可执行 Clear() 方法
	bool AddNodesDist(long idNode1, long idNode2, long distance);

	// 获得一个从 idNodeStart 到 idNodeEnd 的最短路径
	// 函数返回最短路径所经历的结点个数（包含起始、终止结点，>=2），失败返回0，无路径返回-1
	// 参数 pIdsInPath 指向一个数组（下标从0开始用），从此参数返回最短路径的历经各结点的 id，
	//   数组空间由本函数开辟由 HM 管理；
	// 从参数 distance 返回最短路径的“权值和”（无路径时返回无穷大值 mcMaxValue，出错时返回0）
	long GetDistance(long idNodeStart, long idNodeEnd, long &distance, long *&pIdsInPath, pFunDjCalculatingCallBack pf = NULL, long userDataCallBack = 0 );

	// 返回当前已读入的网络中所有结点个数
	int NodeCount();

	// 给出一个Node index（连续，1--NodeCount()），返回用户的该结点的 ID（可以不连续）
	long NodeID(int idxNode);

	// 给出结点ID: idNode，返回该结点在本数据结构中的 index；
	// 若不存在该结点返回 0 
	//（可用于测试某个 ID 的结点在当前已读入的网络中是否存在）
	int NodeIndex(long idNode);	

	// 返回当前已读入的网络中，与结点（ID: idNode）直接相连的边的条数
	int NodeAdjEdgesCount(int idxNode);

	// 返回与结点 idxNode 直接相连的一个结点的相连边的权值
	// 从参数 idxNodeAdj 返回直接相连的这个结点的 index
	// idxEdge 范围：1--NodeAdjEdgesCount(idxNode)
	long NodeOneAdjEdge(int idxNode, int idxEdge, int &idxNodeAdj);

	// 清除数据结构中的所有内容
	void Clear();
	


private:
	int m_k;						// 是否求 k-th 路径（k>1），k=1时仅求最短路径

	NodeInfType *m_Nodes;			// 指向一个 NodeInfType 类型的数组，保存本图的所有结点，以及结点连接的所有边
									//   下标从1开始，0 浪费不用；有效结点空间 m_Nodes[1]～m_Nodes[m_NodesCount]
									// 预开辟的空间为 m_Nodes[0]～m_Nodes[m_NodesUbound]
									// m_Nodes[] 数组下标称结点 index （从1开始必须连续）；而结点 ID 为用户ID可以不连续
									//  ID => index 转换用 m_hashNodesIdxes；index => ID 转换用 m_Nodes[index].ID
	int m_NodesCount,m_NodesUbound;	// 当前结点个数，当前 m_Nodes 的最大下标（m_Nodes指向空间有 m_NodesUbound+1 个空间）
	
	CBHashLK m_hashNodesIdxes;		// 哈希表用于由结点 ID 转换为 index：key = ID，Item = m_Nodes[] 数组下标

	int m_indexStart;				// 已 Calculate 后设置此变量，保存现在所求最短路径的“起始结点 index”
	CBArrLink m_lkNodesTouch;		// 求最短路径过程中“涉及到的结点”列表：保存所有未 visited 且 w 不是 ∞ 的结点的 index
									// 求最短路径时，每次从中取一个 w 最小的结点进行操作，
									//   并设此结点已 visited
private:
	// 给出一个起始结点，计算并在数据结构中保存最短路径状态：
	//   分别记录到达结点 i 的最短路径“权值和” m_Nodes[i].w
	//   并分别标记到达结点 i 的最短路径的回溯父节点 m_Nodes[i].pParentList
	// 之后只要给出一个末尾结点 i，就能获得到此结点的最短路径“权值和”以及最短路径
	bool Calculate(long idNodeStart, pFunDjCalculatingCallBack pf=NULL, long userDataCallBack=0);

};