#include "resource.h"
#include "BForm.h"
#include "BDijkstra.h"
#include "BReadLinesEx.h" 
#include <vector>
#include <string>
#include <set>
#include <windows.h> 
#include <commdlg.h>

// 核心对象
CBForm form1(ID_form1);
CBDijkstra dj;
bool g_bRealTimeCalc = false;
bool g_bShowLabels = true;
HACCEL g_hAccel = NULL;

// ==================== 前置声明 ====================
void UpdateStatus(LPCTSTR szMsg);
void PerformCalculation(); 
void RefreshCountLabel();
void RefreshNodeInfo();
void TryRealTimeCalc();
void DrawVisualPath(long* idNodes, long count);
bool ValidateNodeInput(const tstring& sNode, long& value, LPCTSTR fieldName);
void ExportData();
void ImportData();

// ==================== 增强的回调函数 ====================
bool FunDjCalculatingCallBack(int iCycled, int iTotalCycleCurrent, long userData)
{
    if (iTotalCycleCurrent > 0)
    {
        int percent = (iCycled * 100) / iTotalCycleCurrent;
        form1.Control(ID_progress).ValueSet(percent);
    }
    
    // 每10个节点更新一次状态，减少UI刷新频率
    if (iCycled % 10 == 0) 
    {
        tstring sMsg = TEXT("正在计算... 已扫描节点: ");
        sMsg += Str(iCycled);
        sMsg += TEXT(" / ");
        sMsg += Str(iTotalCycleCurrent);
        UpdateStatus(sMsg.c_str());
        DoEvents(); 
    }
    return true; 
}

// ==================== 输入验证函数 ====================
bool ValidateNodeInput(const tstring& sNode, long& value, LPCTSTR fieldName)
{
    if (sNode.empty()) {
        tstring msg = fieldName;
        msg += TEXT(" 不能为空！");
        MsgBox(msg.c_str(), TEXT("输入错误"), mb_OK, mb_IconExclamation);
        return false;
    }
    
    value = (long)Val(sNode.c_str());
    if (value == 0) {
        tstring msg = fieldName;
        msg += TEXT(" 必须是非零整数！");
        MsgBox(msg.c_str(), TEXT("输入错误"), mb_OK, mb_IconExclamation);
        return false;
    }
    
    return true;
}

// ==================== 增强的绘图函数 ====================
void DrawVisualPath(long* idNodes, long count)
{
    HWND hDlg = FindWindow(NULL, TEXT("ex9Dijkstra 交互式最短路径分析 [增强版]"));
    if (!hDlg) return;
    
    HWND hCanvas = GetDlgItem(hDlg, ID_picCanvas);
    if (!hCanvas) return;

    HDC hdc = GetDC(hCanvas);
    RECT rect;
    GetClientRect(hCanvas, &rect);
    
    // 渐变背景
    HBRUSH hBrushBg = CreateSolidBrush(RGB(250, 252, 255)); 
    FillRect(hdc, &rect, hBrushBg);
    DeleteObject(hBrushBg);

    if (count <= 0) {
        // 显示提示文字
        SetTextColor(hdc, RGB(150, 150, 150));
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                 DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Microsoft YaHei UI"));
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);
        DrawText(hdc, TEXT("计算后将显示路径图"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(hCanvas, hdc);
        return;
    }

    // 绘制参数
    HPEN hPenLine = CreatePen(PS_SOLID, 2, RGB(0, 120, 215)); 
    HPEN hPenNode = CreatePen(PS_SOLID, 2, RGB(40, 120, 180));  
    HBRUSH hBrushNode = CreateSolidBrush(RGB(255, 255, 255)); 
    HBRUSH hBrushStart = CreateSolidBrush(RGB(76, 175, 80));  // 起点绿色
    HBRUSH hBrushEnd = CreateSolidBrush(RGB(244, 67, 54));    // 终点红色
    
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
    HFONT hFontSmall = CreateFont(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
    
    HGDIOBJ hOldPen = SelectObject(hdc, hPenLine);
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrushNode);
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT); 
    SetTextColor(hdc, RGB(0, 0, 0));

    int w = rect.right;
    int h = rect.bottom;
    int centerY = h / 2;
    int margin = 25; 
    int stepX = (count > 1) ? (w - 2 * margin) / (count - 1) : 0;
    int radius = 15; 

    // 绘制连线和箭头
    SelectObject(hdc, hPenLine);
    for (int i = 0; i < count - 1; i++)
    {
        int x1 = margin + i * stepX;
        int x2 = margin + (i + 1) * stepX;
        
        // 主线
        MoveToEx(hdc, x1 + radius, centerY, NULL);
        LineTo(hdc, x2 - radius, centerY);
        
        // 箭头
        int arrowSize = 6;
        MoveToEx(hdc, x2 - radius, centerY, NULL);
        LineTo(hdc, x2 - radius - arrowSize, centerY - arrowSize/2);
        MoveToEx(hdc, x2 - radius, centerY, NULL);
        LineTo(hdc, x2 - radius - arrowSize, centerY + arrowSize/2);
        
        // 显示步骤编号
        if (g_bShowLabels) {
            SelectObject(hdc, hFontSmall);
            tstring stepText = TEXT("步骤 ");
            stepText += Str(i + 1);
            int textX = (x1 + x2) / 2;
            int textY = centerY - 15;
            SetTextColor(hdc, RGB(100, 100, 100));
            TextOut(hdc, textX - 15, textY, stepText.c_str(), _tcslen(stepText.c_str()));
            SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(0, 0, 0));
        }
    }

    // 绘制节点
    SelectObject(hdc, hPenNode);
    for (int i = 0; i < count; i++)
    {
        int centerX = margin + i * stepX;
        if (count == 1) centerX = w / 2; 
        
        // 根据位置选择颜色
        if (i == 0) {
            SelectObject(hdc, hBrushStart);  // 起点
            SetTextColor(hdc, RGB(255, 255, 255));
        } else if (i == count - 1) {
            SelectObject(hdc, hBrushEnd);    // 终点
            SetTextColor(hdc, RGB(255, 255, 255));
        } else {
            SelectObject(hdc, hBrushNode);   // 中间节点
            SetTextColor(hdc, RGB(0, 0, 0));
        }
        
        // 绘制圆形节点
        Ellipse(hdc, centerX - radius, centerY - radius, 
                centerX + radius, centerY + radius);
        
        // 绘制节点ID
        tstring sText = Str(idNodes[i]);
        SIZE sizeText;
        GetTextExtentPoint32(hdc, sText.c_str(), _tcslen(sText.c_str()), &sizeText);
        TextOut(hdc, centerX - sizeText.cx / 2, centerY - sizeText.cy / 2, 
                sText.c_str(), _tcslen(sText.c_str()));
    }

    // 清理资源
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldFont);
    DeleteObject(hPenLine);
    DeleteObject(hPenNode);
    DeleteObject(hBrushNode);
    DeleteObject(hBrushStart);
    DeleteObject(hBrushEnd);
    DeleteObject(hFont);
    DeleteObject(hFontSmall);
    ReleaseDC(hCanvas, hdc);
}

// ==================== 状态更新函数 ====================
void UpdateStatus(LPCTSTR szMsg) {
    form1.Control(ID_lblStatus).TextSet(szMsg);
}

void RefreshCountLabel() {
    long count = form1.Control(ID_lsvDists).ListCount();
    tstring sInfo = TEXT("共 ");
    sInfo += Str(count);
    sInfo += TEXT(" 条边");
    form1.Control(ID_lblCount).TextSet(sInfo.c_str());
}

void RefreshNodeInfo() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long edgeCount = lsv.ListCount();
    
    // 统计唯一节点数
    std::set<long> uniqueNodes;
    for (long i = 1; i <= edgeCount; i++) {
        long n1 = (long)Val(lsv.ItemText(i, 1));
        long n2 = (long)Val(lsv.ItemText(i, 2));
        uniqueNodes.insert(n1);
        uniqueNodes.insert(n2);
    }
    
    tstring sInfo = TEXT("节点数: ");
    sInfo += Str((long)uniqueNodes.size());
    sInfo += TEXT(" | 边数: ");
    sInfo += Str(edgeCount);
    form1.Control(ID_lblNodeInfo).TextSet(sInfo.c_str());
}

// ==================== 核心计算函数 ====================
void PerformCalculation() {
    long idStart = form1.Control(ID_txtNodeCalc1).TextInt();
    long idEnd   = form1.Control(ID_txtNodeCalc2).TextInt();

    if (idStart == 0 || idEnd == 0) {
        if (!g_bRealTimeCalc) 
            MsgBox(TEXT("请输入有效的起点和终点 ID (非零数字)"), TEXT("提示"), mb_OK, mb_IconExclamation);
        return;
    }
    
    if (idStart == idEnd) {
        MsgBox(TEXT("起点和终点不能相同！"), TEXT("提示"), mb_OK, mb_IconExclamation);
        return;
    }

    UpdateStatus(TEXT("正在构建网络拓扑..."));
    form1.Control(ID_progress).ValueSet(0); 

    dj.Clear(); 
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        UpdateStatus(TEXT("[错误] 没有边数据，请先添加边！"));
        MsgBox(TEXT("请先添加边数据！"), TEXT("提示"), mb_OK, mb_IconWarning);
        return;
    }
    
    // 构建图
    for (long i = 1; i <= count; i++) {
        long n1 = (long)Val(lsv.ItemText(i, 1));   
        long n2 = (long)Val(lsv.ItemText(i, 2));   
        long dist = (long)Val(lsv.ItemText(i, 3)); 
        dj.AddNodesDist(n1, n2, dist);
    }

    long distance = 0;
    long *idNodes = NULL; 
    long ctPath = dj.GetDistance(idStart, idEnd, distance, idNodes, FunDjCalculatingCallBack);

    form1.Control(ID_progress).ValueSet(100); 

    if (ctPath > 0) {
        // 显示距离
        form1.Control(ID_txtDistResu).TextSet(distance);
        
        // 显示路径
        CBControl lst = form1.Control(ID_lstPath);
        lst.ListClear();
        for (int i = 0; i < ctPath; i++) {
            tstring sItem = TEXT("步骤 ");
            sItem += Str(i + 1);
            sItem += TEXT(": 节点 ");
            sItem += Str(idNodes[i]);
            lst.AddItem(sItem.c_str());
        }
        
        tstring sMsg = TEXT("✓ 计算完成！最短距离: ");
        sMsg += Str(distance);
        sMsg += TEXT(" | 经过 ");
        sMsg += Str(ctPath);
        sMsg += TEXT(" 个节点");
        UpdateStatus(sMsg.c_str());
        
        DrawVisualPath(idNodes, ctPath); 
    }
    else if (ctPath < 0) {
        form1.Control(ID_txtDistResu).TextSet(TEXT("不可达"));
        form1.Control(ID_lstPath).ListClear();
        UpdateStatus(TEXT("✗ 计算完成：两点之间无路径。"));
        DrawVisualPath(NULL, 0); 
        
        if (!g_bRealTimeCalc)
            MsgBox(TEXT("从起点到终点不存在路径！\n请检查图的连通性。"), TEXT("提示"), mb_OK, mb_IconInformation);
    }
    else {
        UpdateStatus(TEXT("计算出错或未找到路径。"));
    }
}

void TryRealTimeCalc() {
    g_bRealTimeCalc = (form1.Control(ID_chkRealTime).Value() == 1);
    if (g_bRealTimeCalc) PerformCalculation();
}

// ==================== 数据导入导出 ====================
void ExportData() {
    TCHAR szFile[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = TEXT("文本文件 (*.txt)\0*.txt\0CSV文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = TEXT("导出数据");
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = TEXT("txt");
    
    if (!GetSaveFileName(&ofn)) return;
    
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有数据可导出！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    FILE* fp = _tfopen(szFile, TEXT("w"));
    if (!fp) {
        MsgBox(TEXT("文件创建失败！"), TEXT("错误"), mb_OK, mb_IconError);
        return;
    }
    
    // 写入表头
    _ftprintf(fp, TEXT("起点\t终点\t距离\n"));
    
    for (long i = 1; i <= count; i++) {
        _ftprintf(fp, TEXT("%s\t%s\t%s\n"), 
                  lsv.ItemText(i, 1),
                  lsv.ItemText(i, 2),
                  lsv.ItemText(i, 3));
    }
    
    fclose(fp);
    
    tstring msg = TEXT("成功导出 ");
    msg += Str(count);
    msg += TEXT(" 条数据到文件！");
    MsgBox(msg.c_str(), TEXT("导出成功"), mb_OK, mb_IconInformation);
    UpdateStatus(TEXT("✓ 数据已导出"));
}

void ImportData() {
    TCHAR szFile[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = TEXT("文本文件 (*.txt)\0*.txt\0CSV文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = TEXT("导入数据");
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (!GetOpenFileName(&ofn)) return;
    
    FILE* fp = _tfopen(szFile, TEXT("r"));
    if (!fp) {
        MsgBox(TEXT("文件打开失败！"), TEXT("错误"), mb_OK, mb_IconError);
        return;
    }
    
    CBControl lsv = form1.Control(ID_lsvDists);
    long nAdded = 0;
    TCHAR line[1024];
    
    // 跳过表头
    if (_fgetts(line, 1024, fp)) {}
    
    while (_fgetts(line, 1024, fp)) {
        TCHAR **pFields = NULL;
        long nF = Split(line, pFields, TEXT("\t"));
        if (nF < 3) nF = Split(line, pFields, TEXT(","));
        if (nF < 3) nF = Split(line, pFields, TEXT(" "));
        
        if (nF >= 3) {
            long idx = lsv.AddItem(pFields[1]);
            lsv.ItemTextSet(idx, pFields[2], 2);
            lsv.ItemTextSet(idx, pFields[3], 3);
            nAdded++;
        }
    }
    
    fclose(fp);
    
    if (nAdded > 0) {
        tstring msg = TEXT("成功导入 ");
        msg += Str(nAdded);
        msg += TEXT(" 条数据！");
        MsgBox(msg.c_str(), TEXT("导入成功"), mb_OK, mb_IconInformation);
        RefreshCountLabel();
        RefreshNodeInfo();
        TryRealTimeCalc();
        UpdateStatus(TEXT("✓ 数据已清空"));
    }
}

void cmdCheck_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有数据可检查！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    long nErr = 0;
    long nSelfLoop = 0;
    tstring errMsg;
    
    for (long i = 1; i <= count; i++) {
        long n1 = (long)Val(lsv.ItemText(i, 1));
        long n2 = (long)Val(lsv.ItemText(i, 2));
        long dist = (long)Val(lsv.ItemText(i, 3));
        
        if (dist < 0) {
            nErr++;
        }
        if (n1 == n2) {
            nSelfLoop++;
        }
    }
    
    if (nErr > 0 || nSelfLoop > 0) {
        errMsg = TEXT("发现问题：\n");
        if (nErr > 0) {
            errMsg += TEXT("• 负权边: ");
            errMsg += Str(nErr);
            errMsg += TEXT(" 条\n");
        }
        if (nSelfLoop > 0) {
            errMsg += TEXT("• 自环边: ");
            errMsg += Str(nSelfLoop);
            errMsg += TEXT(" 条\n");
        }
        MsgBox(errMsg.c_str(), TEXT("数据检查结果"), mb_OK, mb_IconWarning);
        UpdateStatus(TEXT("⚠ 数据存在问题"));
    } else {
        MsgBox(TEXT("✓ 数据正常，未发现问题！"), TEXT("数据检查结果"), mb_OK, mb_IconInformation);
        UpdateStatus(TEXT("✓ 数据检查通过"));
    }
}

void cmdUnique_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有数据！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    long removed = 0;
    for (long i = 1; i <= count; i++) {
        for (long j = i + 1; j <= lsv.ListCount(); ) {
            if (lstrcmp(lsv.ItemText(i, 1), lsv.ItemText(j, 1)) == 0 &&
                lstrcmp(lsv.ItemText(i, 2), lsv.ItemText(j, 2)) == 0 &&
                lstrcmp(lsv.ItemText(i, 3), lsv.ItemText(j, 3)) == 0) {
                lsv.RemoveItem(j);
                removed++;
            } else j++;
        }
    }
    
    tstring sMsg;
    if (removed > 0) {
        sMsg = TEXT("✓ 去重完成，删除 ");
        sMsg += Str(removed);
        sMsg += TEXT(" 条重复数据");
        MsgBox(sMsg.c_str(), TEXT("去重结果"), mb_OK, mb_IconInformation);
    } else {
        sMsg = TEXT("未发现重复数据");
        MsgBox(sMsg.c_str(), TEXT("去重结果"), mb_OK, mb_IconInformation);
    }
    
    UpdateStatus(sMsg.c_str());
    RefreshCountLabel();
    RefreshNodeInfo();
}

void cmdDo_Click() { 
    PerformCalculation(); 
}

void chkRealTime_Click() {
    g_bRealTimeCalc = (form1.Control(ID_chkRealTime).Value() == 1);
    if (g_bRealTimeCalc) {
        UpdateStatus(TEXT("✓ 实时计算已启用"));
        PerformCalculation();
    } else {
        UpdateStatus(TEXT("实时计算已关闭"));
    }
}

void chkShowLabels_Click() {
    g_bShowLabels = (form1.Control(ID_chkShowLabels).Value() == 1);
    // 重新绘制当前路径
    CBControl lst = form1.Control(ID_lstPath);
    if (lst.ListCount() > 0) {
        // 有路径时重新绘制
        long count = lst.ListCount();
        long* nodes = new long[count];
        for (long i = 0; i < count; i++) {
            tstring item = lst.ItemText(i + 1);
            // 从 "步骤 X: 节点 Y" 中提取节点ID
            size_t pos = item.find(TEXT("节点 "));
            if (pos != tstring::npos) {
                nodes[i] = (long)Val(item.substr(pos + 3).c_str());
            }
        }
        DrawVisualPath(nodes, count);
        delete[] nodes;
    }
}

void cmdSwapNodes_Click() {
    tstring s1 = form1.Control(ID_txtNodeCalc1).Text();
    tstring s2 = form1.Control(ID_txtNodeCalc2).Text();
    
    form1.Control(ID_txtNodeCalc1).TextSet(s2.c_str());
    form1.Control(ID_txtNodeCalc2).TextSet(s1.c_str());
    
    UpdateStatus(TEXT("✓ 起终点已交换"));
    TryRealTimeCalc();
}

void cmdAutoDetect_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有边数据！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    // 自动选择第一条边的起点和终点
    form1.Control(ID_txtNodeCalc1).TextSet(lsv.ItemText(1, 1));
    form1.Control(ID_txtNodeCalc2).TextSet(lsv.ItemText(1, 2));
    
    UpdateStatus(TEXT("✓ 已自动设置起终点"));
    TryRealTimeCalc();
}

void cmdCopyResult_Click() {
    tstring result = form1.Control(ID_txtDistResu).Text();
    CBControl lst = form1.Control(ID_lstPath);
    
    if (result.empty() || result == TEXT("不可达")) {
        MsgBox(TEXT("没有可复制的结果！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    tstring copyText = TEXT("最短距离: ");
    copyText += result;
    copyText += TEXT("\n路径: ");
    
    for (long i = 1; i <= lst.ListCount(); i++) {
        tstring item = lst.ItemText(i);
        size_t pos = item.find(TEXT("节点 "));
        if (pos != tstring::npos) {
            if (i > 1) copyText += TEXT(" -> ");
            copyText += item.substr(pos + 3);
        }
    }
    
    ClipboardSetText(copyText.c_str());
    UpdateStatus(TEXT("✓ 结果已复制到剪贴板"));
}

void cmdExport_Click() {
    ExportData();
}

void cmdImport_Click() {
    ImportData();
}

// 右键菜单事件
void lsvDists_ClickRight() {
    HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDM_POPUP));
    if (!hMenu) return;
    
    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    POINT pt;
    GetCursorPos(&pt);
    
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, 
                   FindWindow(NULL, TEXT("ex9Dijkstra 交互式最短路径分析 [增强版]")), NULL);
    
    DestroyMenu(hMenu);
}

void mnuCopy_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long selIdx = lsv.ListIndex();
    
    if (selIdx > 0) {
        tstring copyText = lsv.ItemText(selIdx, 1);
        copyText += TEXT("\t");
        copyText += lsv.ItemText(selIdx, 2);
        copyText += TEXT("\t");
        copyText += lsv.ItemText(selIdx, 3);
        ClipboardSetText(copyText.c_str());
        UpdateStatus(TEXT("✓ 已复制选中行"));
    }
}

void mnuPaste_Click() {
    cmdPaste_Click();
}

void mnuDelete_Click() {
    cmdDel_Click();
}

void mnuEdit_Click() {
    cmdEdit_Click();
}

void mnuSelectAll_Click() {
    // ListView不支持多选，这里只是示例
    UpdateStatus(TEXT("ListView单选模式不支持全选"));
}

// ==================== 主函数 ====================
int main() {
    // 注册事件
    form1.EventAdd(0, eForm_Load, form1_Load);
    form1.EventAdd(ID_cmdAdd, eCommandButton_Click, cmdAdd_Click);
    form1.EventAdd(ID_cmdEdit, eCommandButton_Click, cmdEdit_Click);
    form1.EventAdd(ID_cmdDel, eCommandButton_Click, cmdDel_Click);
    form1.EventAdd(ID_cmdClear, eCommandButton_Click, cmdClear_Click);
    form1.EventAdd(ID_cmdPaste, eCommandButton_Click, cmdPaste_Click); 
    form1.EventAdd(ID_cmdCheck, eCommandButton_Click, cmdCheck_Click); 
    form1.EventAdd(ID_cmdUnique, eCommandButton_Click, cmdUnique_Click); 
    form1.EventAdd(ID_cmdExport, eCommandButton_Click, cmdExport_Click);
    form1.EventAdd(ID_cmdImport, eCommandButton_Click, cmdImport_Click);
    form1.EventAdd(ID_cmdDo, eCommandButton_Click, cmdDo_Click);
    form1.EventAdd(ID_chkRealTime, eCommandButton_Click, chkRealTime_Click);
    form1.EventAdd(ID_chkShowLabels, eCommandButton_Click, chkShowLabels_Click);
    form1.EventAdd(ID_cmdSwapNodes, eCommandButton_Click, cmdSwapNodes_Click);
    form1.EventAdd(ID_cmdAutoDetect, eCommandButton_Click, cmdAutoDetect_Click);
    form1.EventAdd(ID_cmdCopyResult, eCommandButton_Click, cmdCopyResult_Click);
    
    // 右键菜单事件
    form1.EventAdd(ID_lsvDists, eListView_ClickRight, lsvDists_ClickRight);
    form1.EventAdd(IDM_COPY, eMenuItem_Click, mnuCopy_Click);
    form1.EventAdd(IDM_PASTE, eMenuItem_Click, mnuPaste_Click);
    form1.EventAdd(IDM_DELETE, eMenuItem_Click, mnuDelete_Click);
    form1.EventAdd(IDM_EDIT, eMenuItem_Click, mnuEdit_Click);
    form1.EventAdd(IDM_SELECT_ALL, eMenuItem_Click, mnuSelectAll_Click);
    
    form1.IconSet(IDI_ICON1);
    form1.Show();
    return 0;
} 数据已导入"));
    } else {
        MsgBox(TEXT("未导入任何数据！\n请检查文件格式。"), TEXT("提示"), mb_OK, mb_IconWarning);
    }
}

// ==================== 事件处理函数 ====================
void form1_Load() {
    CBControl lsv = form1.Control(ID_lsvDists);
    lsv.ListViewAddColumn(TEXT("起点 ID"), 70); 
    lsv.ListViewAddColumn(TEXT("终点 ID"), 70);
    lsv.ListViewAddColumn(TEXT("权重"), 60);
    lsv.ListViewGridLinesSet(true);      
    lsv.ListViewFullRowSelectSet(true);  

    CBControl cbo = form1.Control(ID_cboMode);
    cbo.AddItem(TEXT("单源最短路径 (Dijkstra)")); 
    cbo.AddItem(TEXT("全源路径矩阵 (Floyd)")); 
    cbo.ListIndexSet(0); 

    // 默认勾选显示标签
    form1.Control(ID_chkShowLabels).ValueSet(1);
    g_bShowLabels = true;

    UpdateStatus(TEXT("[系统就绪] 等待数据录入..."));
    RefreshCountLabel();
    RefreshNodeInfo();

    // 调整GroupBox层级
    HWND hDlg = FindWindow(NULL, TEXT("ex9Dijkstra 交互式最短路径分析 [增强版]"));
    if (hDlg) 
    {
        HWND hGrp1 = GetDlgItem(hDlg, ID_grpInput);
        HWND hGrp2 = GetDlgItem(hDlg, ID_grpList);
        HWND hGrp3 = GetDlgItem(hDlg, ID_grpCalc);
        HWND hGrp4 = GetDlgItem(hDlg, ID_grpResult);
        SetWindowPos(hGrp1, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(hGrp2, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(hGrp3, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(hGrp4, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    
    DrawVisualPath(NULL, 0);  // 显示初始提示
}

void cmdAdd_Click() {
    tstring sN1 = form1.Control(ID_txtNode1).Text();
    tstring sN2 = form1.Control(ID_txtNode2).Text();
    tstring sDist = form1.Control(ID_txtDist).Text();

    long n1, n2, dist;
    if (!ValidateNodeInput(sN1, n1, TEXT("起点ID"))) return;
    if (!ValidateNodeInput(sN2, n2, TEXT("终点ID"))) return;
    
    if (sDist.empty()) {
        MsgBox(TEXT("请输入权重！"), TEXT("输入错误"), mb_OK, mb_IconExclamation);
        return;
    }
    
    dist = (long)Val(sDist.c_str());
    if (dist < 0) {
        MsgBox(TEXT("本程序暂不支持负权边！"), TEXT("数据警告"), mb_OK, mb_IconExclamation);
        return;
    }
    
    if (n1 == n2) {
        MsgBox(TEXT("起点和终点不能相同！"), TEXT("输入错误"), mb_OK, mb_IconExclamation);
        return;
    }

    CBControl lsv = form1.Control(ID_lsvDists);
    
    // 添加边
    long idx = lsv.AddItem(sN1.c_str());      
    lsv.ItemTextSet(idx, sN2.c_str(), 2);   
    lsv.ItemTextSet(idx, sDist.c_str(), 3); 
    
    // 如果勾选了双向边，添加反向边
    bool isBidirectional = (form1.Control(ID_chkBidirectional).Value() == 1);
    if (isBidirectional) {
        idx = lsv.AddItem(sN2.c_str());
        lsv.ItemTextSet(idx, sN1.c_str(), 2);
        lsv.ItemTextSet(idx, sDist.c_str(), 3);
    }

    // 智能输入：将终点ID移到起点框
    form1.Control(ID_txtNode1).TextSet(sN2.c_str()); 
    form1.Control(ID_txtNode2).TextSet(TEXT(""));
    form1.Control(ID_txtDist).TextSet(TEXT(""));
    form1.Control(ID_txtNode2).SetFocus(); 

    tstring msg = TEXT("✓ 添加成功");
    if (isBidirectional) msg += TEXT(" (双向)");
    UpdateStatus(msg.c_str());
    RefreshCountLabel();
    RefreshNodeInfo();
    TryRealTimeCalc(); 
}

void cmdEdit_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long selIdx = lsv.ListIndex();
    
    if (selIdx <= 0) {
        MsgBox(TEXT("请先选中要编辑的行！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    // 将选中行数据填入输入框
    form1.Control(ID_txtNode1).TextSet(lsv.ItemText(selIdx, 1));
    form1.Control(ID_txtNode2).TextSet(lsv.ItemText(selIdx, 2));
    form1.Control(ID_txtDist).TextSet(lsv.ItemText(selIdx, 3));
    
    // 删除原行
    lsv.RemoveItem(selIdx);
    
    UpdateStatus(TEXT("请修改后点击【添加边】保存"));
    form1.Control(ID_txtNode1).SetFocus();
    RefreshCountLabel();
    RefreshNodeInfo();
}

void cmdPaste_Click() {
    tstring sClip = ClipboardGetText();
    if (sClip.empty()) {
        MsgBox(TEXT("剪贴板为空！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }

    TCHAR **pLines = NULL;
    long nLines = Split(sClip.c_str(), pLines, TEXT("\n"), 1000); 
    CBControl lsv = form1.Control(ID_lsvDists);
    long nAdded = 0;

    for (long i = 1; i <= nLines; i++) {
        TCHAR **pFields = NULL;
        long nF = Split(pLines[i], pFields, TEXT("\t")); 
        if (nF < 3) nF = Split(pLines[i], pFields, TEXT(" "));  
        if (nF < 3) nF = Split(pLines[i], pFields, TEXT(",")); 
        if (nF >= 3) {
            // 验证数据有效性
            long n1 = (long)Val(pFields[1]);
            long n2 = (long)Val(pFields[2]);
            long dist = (long)Val(pFields[3]);
            
            if (n1 != 0 && n2 != 0 && n1 != n2 && dist >= 0) {
                long idx = lsv.AddItem(pFields[1]); 
                lsv.ItemTextSet(idx, pFields[2], 2); 
                lsv.ItemTextSet(idx, pFields[3], 3); 
                nAdded++;
            }
        }
    }
    
    if (nAdded > 0) {
        tstring sMsg = TEXT("批量导入完成，共导入 ");
        sMsg += Str(nAdded);
        sMsg += TEXT(" 条有效数据。");
        MsgBox(sMsg.c_str(), TEXT("导入报告"), mb_OK, mb_IconInformation);
        
        RefreshCountLabel();
        RefreshNodeInfo();
        TryRealTimeCalc();
        UpdateStatus(TEXT("✓ 批量导入成功"));
    } else {
        MsgBox(TEXT("未导入任何有效数据！\n请检查数据格式：起点 终点 权重"), TEXT("提示"), mb_OK, mb_IconWarning);
    }
}

void cmdDel_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long selIdx = lsv.ListIndex();
    
    if (selIdx > 0) {
        lsv.RemoveItem(selIdx);
        UpdateStatus(TEXT("✓ 已删除选中行"));
    } else {
        MsgBox(TEXT("请先选中要删除的行！"), TEXT("提示"), mb_OK, mb_IconInformation);
    }
    
    RefreshCountLabel();
    RefreshNodeInfo();
    TryRealTimeCalc();
}

void cmdClear_Click() {
    if (MsgBox(TEXT("确定清空所有数据？"), TEXT("确认"), mb_YesNo, mb_IconQuestion) == idYes) {
        form1.Control(ID_lsvDists).ListClear();
        form1.Control(ID_lstPath).ListClear();
        form1.Control(ID_txtDistResu).TextSet(TEXT(""));
        form1.Control(ID_txtNode1).TextSet(TEXT(""));
        form1.Control(ID_txtNode2).TextSet(TEXT(""));
        form1.Control(ID_txtDist).TextSet(TEXT(""));
        form1.Control(ID_txtNodeCalc1).TextSet(TEXT(""));
        form1.Control(ID_txtNodeCalc2).TextSet(TEXT(""));
        DrawVisualPath(NULL, 0);
        dj.Clear();
        RefreshCountLabel();
        RefreshNodeInfo();
        UpdateStatus(TEXT("✓ 数据已清空"));
    }
}

void cmdCheck_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有数据可检查！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    long nErr = 0;
    long nSelfLoop = 0;
    tstring errMsg;
    
    for (long i = 1; i <= count; i++) {
        long n1 = (long)Val(lsv.ItemText(i, 1));
        long n2 = (long)Val(lsv.ItemText(i, 2));
        long dist = (long)Val(lsv.ItemText(i, 3));
        
        if (dist < 0) {
            nErr++;
        }
        if (n1 == n2) {
            nSelfLoop++;
        }
    }
    
    if (nErr > 0 || nSelfLoop > 0) {
        errMsg = TEXT("发现问题：\n");
        if (nErr > 0) {
            errMsg += TEXT("• 负权边: ");
            errMsg += Str(nErr);
            errMsg += TEXT(" 条\n");
        }
        if (nSelfLoop > 0) {
            errMsg += TEXT("• 自环边: ");
            errMsg += Str(nSelfLoop);
            errMsg += TEXT(" 条\n");
        }
        MsgBox(errMsg.c_str(), TEXT("数据检查结果"), mb_OK, mb_IconWarning);
        UpdateStatus(TEXT("⚠ 数据存在问题"));
    } else {
        MsgBox(TEXT("✓ 数据正常，未发现问题！"), TEXT("数据检查结果"), mb_OK, mb_IconInformation);
        UpdateStatus(TEXT("✓ 数据检查通过"));
    }
}

void cmdUnique_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有数据！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    long removed = 0;
    for (long i = 1; i <= count; i++) {
        for (long j = i + 1; j <= lsv.ListCount(); ) {
            if (lstrcmp(lsv.ItemText(i, 1), lsv.ItemText(j, 1)) == 0 &&
                lstrcmp(lsv.ItemText(i, 2), lsv.ItemText(j, 2)) == 0 &&
                lstrcmp(lsv.ItemText(i, 3), lsv.ItemText(j, 3)) == 0) {
                lsv.RemoveItem(j);
                removed++;
            } else j++;
        }
    }
    
    tstring sMsg;
    if (removed > 0) {
        sMsg = TEXT("✓ 去重完成，删除 ");
        sMsg += Str(removed);
        sMsg += TEXT(" 条重复数据");
        MsgBox(sMsg.c_str(), TEXT("去重结果"), mb_OK, mb_IconInformation);
    } else {
        sMsg = TEXT("未发现重复数据");
        MsgBox(sMsg.c_str(), TEXT("去重结果"), mb_OK, mb_IconInformation);
    }
    
    UpdateStatus(sMsg.c_str());
    RefreshCountLabel();
    RefreshNodeInfo();
}

void cmdDo_Click() { 
    PerformCalculation(); 
}

void chkRealTime_Click() {
    g_bRealTimeCalc = (form1.Control(ID_chkRealTime).Value() == 1);
    if (g_bRealTimeCalc) {
        UpdateStatus(TEXT("✓ 实时计算已启用"));
        PerformCalculation();
    } else {
        UpdateStatus(TEXT("实时计算已关闭"));
    }
}

void chkShowLabels_Click() {
    g_bShowLabels = (form1.Control(ID_chkShowLabels).Value() == 1);
    // 重新绘制当前路径
    CBControl lst = form1.Control(ID_lstPath);
    if (lst.ListCount() > 0) {
        long count = lst.ListCount();
        long* nodes = new long[count];
        for (long i = 0; i < count; i++) {
            tstring item = lst.ItemText(i + 1);
            size_t pos = item.find(TEXT("节点 "));
            if (pos != tstring::npos) {
                nodes[i] = (long)Val(item.substr(pos + 3).c_str());
            }
        }
        DrawVisualPath(nodes, count);
        delete[] nodes;
    }
}

void cmdSwapNodes_Click() {
    tstring s1 = form1.Control(ID_txtNodeCalc1).Text();
    tstring s2 = form1.Control(ID_txtNodeCalc2).Text();
    
    form1.Control(ID_txtNodeCalc1).TextSet(s2.c_str());
    form1.Control(ID_txtNodeCalc2).TextSet(s1.c_str());
    
    UpdateStatus(TEXT("✓ 起终点已交换"));
    TryRealTimeCalc();
}

void cmdAutoDetect_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long count = lsv.ListCount();
    
    if (count == 0) {
        MsgBox(TEXT("没有边数据！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    form1.Control(ID_txtNodeCalc1).TextSet(lsv.ItemText(1, 1));
    form1.Control(ID_txtNodeCalc2).TextSet(lsv.ItemText(1, 2));
    
    UpdateStatus(TEXT("✓ 已自动设置起终点"));
    TryRealTimeCalc();
}

void cmdCopyResult_Click() {
    tstring result = form1.Control(ID_txtDistResu).Text();
    CBControl lst = form1.Control(ID_lstPath);
    
    if (result.empty() || result == TEXT("不可达")) {
        MsgBox(TEXT("没有可复制的结果！"), TEXT("提示"), mb_OK, mb_IconInformation);
        return;
    }
    
    tstring copyText = TEXT("最短距离: ");
    copyText += result;
    copyText += TEXT("\n路径: ");
    
    for (long i = 1; i <= lst.ListCount(); i++) {
        tstring item = lst.ItemText(i);
        size_t pos = item.find(TEXT("节点 "));
        if (pos != tstring::npos) {
            if (i > 1) copyText += TEXT(" -> ");
            copyText += item.substr(pos + 3);
        }
    }
    
    ClipboardSetText(copyText.c_str());
    UpdateStatus(TEXT("✓ 结果已复制到剪贴板"));
}

void cmdExport_Click() {
    ExportData();
}

void cmdImport_Click() {
    ImportData();
}

void lsvDists_ClickRight() {
    HMENU hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDM_POPUP));
    if (!hMenu) return;
    
    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    POINT pt;
    GetCursorPos(&pt);
    
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, 
                   FindWindow(NULL, TEXT("ex9Dijkstra 交互式最短路径分析 [增强版]")), NULL);
    
    DestroyMenu(hMenu);
}

void mnuCopy_Click() {
    CBControl lsv = form1.Control(ID_lsvDists);
    long selIdx = lsv.ListIndex();
    
    if (selIdx > 0) {
        tstring copyText = lsv.ItemText(selIdx, 1);
        copyText += TEXT("\t");
        copyText += lsv.ItemText(selIdx, 2);
        copyText += TEXT("\t");
        copyText += lsv.ItemText(selIdx, 3);
        ClipboardSetText(copyText.c_str());
        UpdateStatus(TEXT("✓ 已复制选中行"));
    }
}

void mnuPaste_Click() {
    cmdPaste_Click();
}

void mnuDelete_Click() {
    cmdDel_Click();
}

void mnuEdit_Click() {
    cmdEdit_Click();
}

void mnuSelectAll_Click() {
    UpdateStatus(TEXT("ListView单选模式不支持全选"));
}

// ==================== 主函数 ====================
int main() {
    form1.EventAdd(0, eForm_Load, form1_Load);
    form1.EventAdd(ID_cmdAdd, eCommandButton_Click, cmdAdd_Click);
    form1.EventAdd(ID_cmdEdit, eCommandButton_Click, cmdEdit_Click);
    form1.EventAdd(ID_cmdDel, eCommandButton_Click, cmdDel_Click);
    form1.EventAdd(ID_cmdClear, eCommandButton_Click, cmdClear_Click);
    form1.EventAdd(ID_cmdPaste, eCommandButton_Click, cmdPaste_Click); 
    form1.EventAdd(ID_cmdCheck, eCommandButton_Click, cmdCheck_Click); 
    form1.EventAdd(ID_cmdUnique, eCommandButton_Click, cmdUnique_Click); 
    form1.EventAdd(ID_cmdExport, eCommandButton_Click, cmdExport_Click);
    form1.EventAdd(ID_cmdImport, eCommandButton_Click, cmdImport_Click);
    form1.EventAdd(ID_cmdDo, eCommandButton_Click, cmdDo_Click);
    form1.EventAdd(ID_chkRealTime, eCommandButton_Click, chkRealTime_Click);
    form1.EventAdd(ID_chkShowLabels, eCommandButton_Click, chkShowLabels_Click);
    form1.EventAdd(ID_cmdSwapNodes, eCommandButton_Click, cmdSwapNodes_Click);
    form1.EventAdd(ID_cmdAutoDetect, eCommandButton_Click, cmdAutoDetect_Click);
    form1.EventAdd(ID_cmdCopyResult, eCommandButton_Click, cmdCopyResult_Click);
    
    form1.EventAdd(ID_lsvDists, eListView_ClickRight, lsvDists_ClickRight);
    form1.EventAdd(IDM_COPY, eMenuItem_Click, mnuCopy_Click);
    form1.EventAdd(IDM_PASTE, eMenuItem_Click, mnuPaste_Click);
    form1.EventAdd(IDM_DELETE, eMenuItem_Click, mnuDelete_Click);
    form1.EventAdd(IDM_EDIT, eMenuItem_Click, mnuEdit_Click);
    form1.EventAdd(IDM_SELECT_ALL, eMenuItem_Click, mnuSelectAll_Click);
    
    form1.IconSet(IDI_ICON1);
    form1.Show();
    return 0;
}