#include "resource.h"
#include "BForm.h"
#include "BDijkstra.h"
#include "BReadLinesEx.h"
#include <vector>
#include <string>
#include <algorithm> 
#include <windows.h> 

// ============================================================================
// 全局变量定义
// ============================================================================
CBForm form1(ID_form1);           // 主窗口对象
CBForm formMatrix(ID_formMatrix); // 邻接矩阵显示窗口对象
CBDijkstra dj;                    // Dijkstra 算法核心处理对象

// ============================================================================
// 辅助函数声明
// ============================================================================
void UpdateStatus(LPCTSTR szMsg);
void RefreshCountLabel();
void DrawVisualPath(long* idNodes, long count);
void CopyDists();
void CalculateMatrix(); 

// ----------------------------------------------------------------------------
// 进度回调函数
// 在算法计算过程中被调用，用于更新进度条并响应系统消息。
// ----------------------------------------------------------------------------
bool FunDjCalculatingCallBack(int iCycled, int iTotalCycleCurrent, long userData) {
    if (iTotalCycleCurrent > 0) {
        int percent = (iCycled * 100) / iTotalCycleCurrent;
        form1.Control(ID_progress).ValueSet(percent); // 更新 UI 进度条
    }
    
    // 防止界面假死
    // 在繁重的计算循环中，每隔 50 次循环处理一次 Windows 消息队列
    // 否则程序会显示“未响应”
    if (iCycled % 50 == 0) {
        DoEvents(); 
    }
    return true;
}

// ----------------------------------------------------------------------------
// 哑回调函数
// 用于矩阵计算时的轻量级回调。
// 这里的空实现是为了让算法库有东西可调，防止库内部出现空指针异常(NULL Pointer Exception)。
// 同时保留 DoEvents 以保持界面在长时间计算矩阵时的响应能力。
// ----------------------------------------------------------------------------
bool FunDummyCallBack(int iCycled, int iTotalCycleCurrent, long userData) {
    if (iCycled % 100 == 0) DoEvents(); 
    return true;
}

// ============================================================================
// 核心逻辑实现
// ============================================================================

// 更新底部状态栏文字
void UpdateStatus(LPCTSTR szMsg) {
    form1.Control(ID_lblStatus).TextSet(szMsg);
}

// 刷新显示的边(Edge)数量统计
void RefreshCountLabel() {
    long count = form1.Control(ID_lsvDists).ListCount();
    tstring sInfo = TEXT("Total: ");
    sInfo += Str(count);
    sInfo += TEXT(" edges");
    form1.Control(ID_lblCount).TextSet(sInfo.c_str());
}

// ----------------------------------------------------------------------------
// GDI 绘图函数：可视化路径
// 参数: idNodes (路径节点ID数组), count (节点数量)
// 原理: 获取 Picture Control 的句柄和设备上下文(DC)，使用 GDI 绘制点和线。
// ----------------------------------------------------------------------------
void DrawVisualPath(long* idNodes, long count) {
    // 1. 获取画布句柄
    HWND hDlg = FindWindow(NULL, TEXT("ex9Dijkstra Shortest Path Analysis"));
    if (!hDlg) return;
    
    HWND hCanvas = GetDlgItem(hDlg, ID_picCanvas);
    if (!hCanvas) return;

    HDC hdc = GetDC(hCanvas); // 获取设备上下文
    if (!hdc) return;

    RECT rect;
    GetClientRect(hCanvas, &rect);
    
    // 2. 清空画布 (用白色填充背景)
    HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &rect, hBrushBg);
    DeleteObject(hBrushBg);

    if (count <= 0) {
        ReleaseDC(hCanvas, hdc);
        return;
    }

    // 3. 创建 GDI 对象 (画笔 Pen, 画刷 Brush, 字体 Font)
    HPEN hPenLine = CreatePen(PS_SOLID, 2, RGB(0, 120, 215)); // 蓝色连线
    HPEN hPenNode = CreatePen(PS_SOLID, 2, RGB(50, 50, 50));  // 深灰节点边框
    HBRUSH hBrushNode = CreateSolidBrush(RGB(240, 240, 240)); // 浅灰节点填充
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
    
    // 选择对象入 DC 并保存旧对象
    HGDIOBJ hOldPen = SelectObject(hdc, hPenLine);
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrushNode);
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT); // 文字背景透明

    // 4. 计算布局坐标
    int w = rect.right;
    int h = rect.bottom;
    int centerY = h / 2;
    int margin = 30;
    // 根据节点数量计算水平间距
    int stepX = (count > 1) ? (w - 2 * margin) / (count - 1) : 0;
    int radius = 13;

    // 5. 第一轮循环：绘制连线 (确保线在圆球的下层)
    SelectObject(hdc, hPenLine);
    for (int i = 0; i < count - 1; i++) {
        int x1 = margin + i * stepX;
        int x2 = margin + (i + 1) * stepX;
        MoveToEx(hdc, x1 + radius, centerY, NULL); // 移动到起点右边缘
        LineTo(hdc, x2 - radius, centerY);         // 画线到终点左边缘
        
        // 绘制简单的箭头指示方向
        LineTo(hdc, x2 - radius - 5, centerY - 3);
        MoveToEx(hdc, x2 - radius, centerY, NULL);
        LineTo(hdc, x2 - radius - 5, centerY + 3);
    }

    // 6. 第二轮循环：绘制节点圆圈和文字
    SelectObject(hdc, hPenNode);
    SelectObject(hdc, hBrushNode);
    for (int i = 0; i < count; i++) {
        int centerX = margin + i * stepX;
        if (count == 1) centerX = w / 2;
        // 绘制圆形
        Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
        
        // 绘制节点 ID 文字 (居中对齐)
        tstring sText = Str(idNodes[i]);
        SIZE sizeText;
        GetTextExtentPoint32(hdc, sText.c_str(), _tcslen(sText.c_str()), &sizeText);
        TextOut(hdc, centerX - sizeText.cx / 2, centerY - sizeText.cy / 2, sText.c_str(), _tcslen(sText.c_str()));
    }

    // 7. 资源清理 (GDI 编程必须步骤，防止内存泄漏)
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldFont);
    DeleteObject(hPenLine);
    DeleteObject(hPenNode);
    DeleteObject(hBrushNode);
    DeleteObject(hFont);
    ReleaseDC(hCanvas, hdc);
}

// 复制当前列表中的距离数据到剪贴板
void CopyDists() {
    CBControl lsv = form1.Control(ID_lsvDists);
    tstring sBuff = TEXT("");
    long nLines = lsv.ListCount();
    for (int i = 1; i <= nLines; i++) {
        sBuff += lsv.ItemText(i, 1);
        sBuff += TEXT("\t");
        sBuff += lsv.ItemText(i, 2);
        sBuff += TEXT("\t");
        sBuff += lsv.ItemText(i, 3);
        sBuff += TEXT("\r\n");
    }
    ClipboardSetText(sBuff.c_str());
    UpdateStatus(TEXT("Copied to clipboard."));
}

// ----------------------------------------------------------------------------
// 执行单次最短路径计算
// 流程：输入校验 -> 构建图数据 -> 调用算法 -> 更新 UI 结果
// ----------------------------------------------------------------------------
void PerformCalculation() {
    long idStart = form1.Control(ID_txtNodeCalc1).TextInt();
    long idEnd   = form1.Control(ID_txtNodeCalc2).TextInt();

    // 输入合法性检查
    if (idStart == 0 || idEnd == 0) {
        MsgBox(TEXT("Please enter valid Start/End Node IDs"), TEXT("Tip"), mb_OK, mb_IconExclamation);
        DrawVisualPath(NULL, 0); 
        return; 
    }

    UpdateStatus(TEXT("Building network..."));
    form1.Control(ID_progress).ValueSet(0); 
    dj.Clear(); // 清空旧的图数据
    
    // 从 ListView 读取边数据并加载到算法对象中
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();

    if (count == 0) {
        UpdateStatus(TEXT("List is empty!"));
        return;
    }

    for (long i = 1; i <= count; i++) {
        long n1 = (long)Val(lsv.ItemText(i, 1)); 
        long n2 = (long)Val(lsv.ItemText(i, 2)); 
        long dist = (long)Val(lsv.ItemText(i, 3)); 
        dj.AddNodesDist(n1, n2, dist); // 添加边：节点1 -> 节点2，权重 dist
    }

    // 起点终点相同的情况
    if (idStart == idEnd) {
            form1.Control(ID_txtDistResu).TextSet(0);
            DrawVisualPath(NULL, 0);
            UpdateStatus(TEXT("Start == End"));
            return;
    }

    // 调用 Dijkstra 算法
    long distance = 0;
    long *idNodes = NULL;
    // GetDistance 返回路径节点数，idNodes 指针指向路径数组
    long ctPath = dj.GetDistance(idStart, idEnd, distance, idNodes, FunDjCalculatingCallBack);
    form1.Control(ID_progress).ValueSet(100);

    // 处理结果
    if (ctPath > 0) {
        form1.Control(ID_txtDistResu).TextSet(distance);
        CBControl lst = form1.Control(ID_lstPath);
        lst.ListClear();
        // 显示路径步骤
        for (int i = 0; i < ctPath; i++) {
            tstring sItem = TEXT("Step ");
            sItem += Str(i + 1);
            sItem += TEXT(": Node ");
            sItem += Str(idNodes[i]);
            lst.AddItem(sItem.c_str());
        }
        tstring sMsg = TEXT("Done! Distance: ");
        sMsg += Str(distance);
        UpdateStatus(sMsg.c_str());
        
        // 绘制可视化路径
        DrawVisualPath(idNodes, ctPath);
    } else if (ctPath < 0) {
        form1.Control(ID_txtDistResu).TextSet(TEXT("Unreachable"));
        form1.Control(ID_lstPath).ListClear();
        UpdateStatus(TEXT("No path found."));
        DrawVisualPath(NULL, 0);
    } else {
        UpdateStatus(TEXT("Error."));
    }
}

// ============================================================================
// 矩阵计算逻辑 
// 作用：计算图中所有节点对之间的最短路径，生成 O(N^2) 的矩阵
// ============================================================================
void CalculateMatrix() {
    // 1. 数据准备
    dj.Clear();
    CBControl lsvData = form1.Control(ID_lsvDists);
    long dataCount = lsvData.ListCount();
    
    std::vector<long> nodes; 
    
    // 提取所有不重复的节点 ID
    for (long i = 1; i <= dataCount; i++) {
        long n1 = (long)Val(lsvData.ItemText(i, 1));
        long n2 = (long)Val(lsvData.ItemText(i, 2));
        long dist = (long)Val(lsvData.ItemText(i, 3));
        dj.AddNodesDist(n1, n2, dist);
        nodes.push_back(n1);
        nodes.push_back(n2);
    }

    // 排序并去重
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

    // 2. 界面初始化
    CBControl lsvMatrix = formMatrix.Control(ID_lsvMatrix);
    lsvMatrix.ListClear();
    
    // 清空旧的列 (使用 Windows API 直接操作 ListView 句柄)
    HWND hDlgMatrix = FindWindow(NULL, TEXT("Global Shortest Path Matrix"));
    if (hDlgMatrix) {
        HWND hLsv = GetDlgItem(hDlgMatrix, ID_lsvMatrix);
        if (hLsv) {
             while (SendMessage(hLsv, LVM_DELETECOLUMN, 0, 0)) {} 
        }
    }

    // 添加动态表头
    lsvMatrix.ListViewAddColumn(TEXT("From \\ To"), 80);
    for (size_t i = 0; i < nodes.size(); i++) {
        lsvMatrix.ListViewAddColumn(Str(nodes[i]), 50);
    }
    
    lsvMatrix.ListViewGridLinesSet(true);
    lsvMatrix.ListViewFullRowSelectSet(true);

    // 3. 矩阵计算循环 (双重循环)
    for (size_t i = 0; i < nodes.size(); i++) {
        long startNode = nodes[i];
        long idx = lsvMatrix.AddItem(Str(startNode));
        
        for (size_t j = 0; j < nodes.size(); j++) {
            long endNode = nodes[j];
            tstring sResult = TEXT("-");

            if (startNode == endNode) {
                sResult = TEXT("0");
            } else {
                long distance = 0;
                long* idPath = NULL;
                
                // ★★★ 关键修复：传入 FunDummyCallBack 而不是 NULL ★★★
                // 防止算法内部直接调用空指针导致崩溃
                long ct = dj.GetDistance(startNode, endNode, distance, idPath, FunDummyCallBack);
                
                if (ct > 0) sResult = Str(distance);
                else if (ct < 0) sResult = TEXT("Inf"); // 不可达
            }
            lsvMatrix.ItemTextSet(idx, sResult.c_str(), j + 2);
        }
        DoEvents(); // 防止界面卡死
    }
}

// ----------------------------------------------------------------------------
// 事件处理函数 (Event Handlers)
// ----------------------------------------------------------------------------

void cmdOpenMatrix_Click() {
    formMatrix.Show();
    CalculateMatrix();
}

void btnRefreshMatrix_Click() {
    CalculateMatrix();
}

void form1_Load() {
    // 初始化列表视图的列
    CBControl lsv = form1.Control(ID_lsvDists);
    lsv.ListViewAddColumn(TEXT("Node 1"), 60);
    lsv.ListViewAddColumn(TEXT("Node 2"), 60);
    lsv.ListViewAddColumn(TEXT("Dist"), 60);
    lsv.ListViewGridLinesSet(true);
    lsv.ListViewFullRowSelectSet(true);

    // 窗口层级调整 (Z-Order)
    HWND hDlg = FindWindow(NULL, TEXT("ex9Dijkstra Shortest Path Analysis"));
    if (hDlg) {
        // 将部分控件置于底部，防止遮挡画布
        SetWindowPos(GetDlgItem(hDlg, ID_grpInput), HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        SetWindowPos(GetDlgItem(hDlg, ID_grpList), HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        SetWindowPos(GetDlgItem(hDlg, ID_grpCalc), HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        SetWindowPos(GetDlgItem(hDlg, ID_grpResult), HWND_BOTTOM, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
    }
    UpdateStatus(TEXT("Ready."));
    RefreshCountLabel();
}

// 添加新的边数据
void cmdAdd_Click() {
    tstring sN1 = form1.Control(ID_txtNode1).Text();
    tstring sN2 = form1.Control(ID_txtNode2).Text();
    tstring sDist = form1.Control(ID_txtDist).Text();
    if (Val(sN1.c_str()) == 0 || Val(sN2.c_str()) == 0 || sDist == TEXT("")) return;

    CBControl lsv = form1.Control(ID_lsvDists);
    long idx = lsv.AddItem(sN1.c_str());
    lsv.ItemTextSet(idx, sN2.c_str(), 2);
    lsv.ItemTextSet(idx, sDist.c_str(), 3);

    // 输入焦点自动跳转，方便连续输入
    form1.Control(ID_txtNode1).TextSet(sN2.c_str());
    form1.Control(ID_txtNode2).TextSet(TEXT(""));
    form1.Control(ID_txtDist).TextSet(TEXT(""));
    form1.Control(ID_txtNode2).SetFocus();
    RefreshCountLabel();
}

// 粘贴剪贴板数据并解析
void cmdPaste_Click() {
    tstring sClip = ClipboardGetText();
    if (sClip == TEXT("")) return;

    TCHAR **pLines = NULL;
    // 拆分行
    long nLines = Split(sClip.c_str(), pLines, TEXT("\n"), 10000);
    CBControl lsv = form1.Control(ID_lsvDists);
    long nAdded = 0;

    for (long i = 1; i <= nLines; i++) {
        TCHAR **pFields = NULL;
        // 尝试不同的分隔符 (Tab, 空格, 逗号)
        long nF = Split(pLines[i], pFields, TEXT("\t"));
        if (nF < 3) nF = Split(pLines[i], pFields, TEXT(" "));
        if (nF < 3) nF = Split(pLines[i], pFields, TEXT(","));
        if (nF >= 3) {
            long idx = lsv.AddItem(pFields[1]);
            lsv.ItemTextSet(idx, pFields[2], 2);
            lsv.ItemTextSet(idx, pFields[3], 3);
            nAdded++;
        }
    }
    if (nAdded > 0) {
        UpdateStatus(TEXT("Import Success."));
        RefreshCountLabel();
    }
}

void cmdDel_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    if (lsv.ListIndex() > 0) { lsv.RemoveItem(lsv.ListIndex()); RefreshCountLabel(); }
}

void cmdClear_Click() {
    if (MsgBox(TEXT("Clear All?"), TEXT("Confirm"), mb_YesNo, mb_IconQuestion) == idYes) {
        form1.Control(ID_lsvDists).ListClear();
        form1.Control(ID_lstPath).ListClear();
        form1.Control(ID_txtDistResu).TextSet(TEXT(""));
        DrawVisualPath(NULL, 0); // 清空画布
        dj.Clear();
        RefreshCountLabel();
    }
}

// 检查是否有负权边 (Dijkstra 不支持负权)
void cmdCheck_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long nErr = 0;
    for (long i = 1; i <= lsv.ListCount(); i++) if (Val(lsv.ItemText(i, 3)) < 0) nErr++;
    if (nErr > 0) MsgBox(TEXT("Negative weights found!"), TEXT("Warning"));
    else MsgBox(TEXT("Data OK."), TEXT("Result"));
}

// 去除重复的边
void cmdUnique_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long removed = 0;
    for (long i = 1; i <= lsv.ListCount(); i++) {
        for (long j = i + 1; j <= lsv.ListCount(); ) {
            // 比较三列内容是否完全一致
            if (lstrcmp(lsv.ItemText(i, 1), lsv.ItemText(j, 1)) == 0 &&
                lstrcmp(lsv.ItemText(i, 2), lsv.ItemText(j, 2)) == 0 &&
                lstrcmp(lsv.ItemText(i, 3), lsv.ItemText(j, 3)) == 0) {
                lsv.RemoveItem(j); removed++;
            } else j++;
        }
    }
    RefreshCountLabel();
}

// 快捷键处理 (Ctrl+C, Ctrl+V)
void lsvDists_KeyUp(int keyCode, int shift, int pbCancel) {
    if (keyCode == 67 && shift == 2) CopyDists(); 
    else if (keyCode == 86 && shift == 2) cmdPaste_Click(); 
}

void lsvDists_ClickRight(int indexItem, int indexSubItem, int x, int y) {
    form1.PopupMenu(ID_mnuPop, x + form1.Control(ID_lsvDists).Left(), y + form1.Control(ID_lsvDists).Top());
}

void form1_MenuClick(int menuID, int bIsFromAcce, int bIsFromSysMenu) {
    if (menuID == ID_mnuPopCopy) CopyDists();
    if (menuID == ID_mnuPopPaste) cmdPaste_Click();
}

void txtDist_KeyPress(int keyAscii, int pbCancel) {
    if (keyAscii == 13) cmdAdd_Click(); // 回车键添加
}

void cmdDo_Click() { PerformCalculation(); }

// ============================================================================
// 程序入口 (Main)
// ============================================================================
int main() {
    // 绑定事件 (Event Binding)
    form1.EventAdd(0, eForm_Load, form1_Load);
    form1.EventAdd(ID_cmdAdd, eCommandButton_Click, cmdAdd_Click);
    form1.EventAdd(ID_cmdDel, eCommandButton_Click, cmdDel_Click);
    form1.EventAdd(ID_cmdClear, eCommandButton_Click, cmdClear_Click);
    form1.EventAdd(ID_cmdPaste, eCommandButton_Click, cmdPaste_Click);
    form1.EventAdd(ID_cmdCheck, eCommandButton_Click, cmdCheck_Click);
    form1.EventAdd(ID_cmdUnique, eCommandButton_Click, cmdUnique_Click);
    form1.EventAdd(ID_cmdDo, eCommandButton_Click, cmdDo_Click);
    
    form1.EventAdd(ID_cmdOpenMatrix, eCommandButton_Click, cmdOpenMatrix_Click);
    formMatrix.EventAdd(ID_btnRefreshMatrix, eCommandButton_Click, btnRefreshMatrix_Click);

    form1.EventAdd(ID_lsvDists, eKeyUp, lsvDists_KeyUp);
    form1.EventAdd(ID_lsvDists, eListView_ClickRight, lsvDists_ClickRight);
    form1.EventAdd(0, eMenu_Click, form1_MenuClick);
    form1.EventAdd(ID_txtDist, eKeyPress, txtDist_KeyPress);

    form1.IconSet(IDI_ICON1);
    form1.Show();
    return 0;
}