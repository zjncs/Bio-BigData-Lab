#include "resource.h"
#include "BForm.h"
#include <tchar.h>
#include <cstdio>

// ——按PPT：全局窗体对象，使用对话框资源ID——
CBForm form1(ID_form1);

// ——工具：结果格式化（去尾零/孤点），VS2010兼容写法——
static tstring FormatNumber(double v) {
    TCHAR buf[128] = {0};
#if defined(_MSC_VER)
    _stprintf_s(buf, TEXT("%.12f"), v);
#else
    _stprintf(buf, TEXT("%.12f"), v);
#endif
    tstring s = buf;
    // 去尾 0
    while (!s.empty()) {
        TCHAR last = s[s.size() - 1];
        if (last == TEXT('0')) s.erase(s.end() - 1);
        else break;
    }
    // 去尾 .
    if (!s.empty() && s[s.size() - 1] == TEXT('.')) s.erase(s.end() - 1);
    if (s == TEXT("-0")) s = TEXT("0");
    if (s.empty()) s = TEXT("0");
    return s;
}

// ——历史区：最多保留3条，写到只读多行 Edit（ID_txtHistory）——
static tstring g_hist1, g_hist2, g_hist3; // 最新在 g_hist1
static void HistoryPush(const tstring& line) {
    // 下移
    g_hist3 = g_hist2;
    g_hist2 = g_hist1;
    g_hist1 = line;

#ifdef ID_txtHistory
    tstring all;
    if (!g_hist1.empty()) { all += g_hist1; }
    if (!g_hist2.empty()) { if (!all.empty()) all += TEXT("\r\n"); all += g_hist2; }
    if (!g_hist3.empty()) { if (!all.empty()) all += TEXT("\r\n"); all += g_hist3; }
    form1.Control(ID_txtHistory).TextSet(all.c_str());
#endif
}

// ——读取两个输入（按 PPT：TextVal），允许为 0，不做 TextGet/Focus/SelAll —— 
static bool TryReadAB(double& a, double& b) {
    a = form1.Control(ID_txtData1).TextVal();
    b = form1.Control(ID_txtData2).TextVal();
    // 若需要更严格校验，可在此加 MsgBox 提示；此处保持简洁与兼容。
    return true;
}

// ——一次计算链：结果显示 + 回填数据1 + 清空数据2 + 写历史——
static void ChainResult(double a, double b, double result, TCHAR opSymbol) {
    const tstring sa = FormatNumber(a);
    const tstring sb = FormatNumber(b);
    const tstring sr = FormatNumber(result);

    // 显示与回填
    form1.Control(ID_txtResult).TextSet(sr.c_str());
    form1.Control(ID_txtData1).TextSet(sr.c_str());
    form1.Control(ID_txtData2).TextSet(TEXT(""));

    // 写历史：例如 "12.5 × 2.5 = 31.25"
    tstring line = sa;
    line += TEXT(" ");
    TCHAR bufOp[4] = {0}; bufOp[0] = opSymbol; bufOp[1] = 0;
    line += bufOp;
    line += TEXT(" ");
    line += sb;
    line += TEXT(" = ");
    line += sr;

    HistoryPush(line);
}

// ——四则运算（保持 PPT 事件/API 风格）——
void cmdJia_Click() {
    double a,b; if (!TryReadAB(a,b)) return;
    ChainResult(a, b, a + b, TEXT('+'));
}
void cmdJian_Click() {
    double a,b; if (!TryReadAB(a,b)) return;
    ChainResult(a, b, a - b, TEXT('-'));
}
void cmdCheng_Click() {
    double a,b; if (!TryReadAB(a,b)) return;
    // 乘号用 'x' 或 '×' 都行；为了兼容宽窄字符，这里用乘号 '×'
    ChainResult(a, b, a * b, TEXT('×'));
}
void cmdChu_Click() {
    double a,b; if (!TryReadAB(a,b)) return;
    if (b == 0.0) {
        MsgBox(TEXT("除数不能为0。"), TEXT("出错啦"), mb_OK, mb_IconExclamation);
        form1.Control(ID_txtResult).TextSet(TEXT(""));
        return;
    }
    ChainResult(a, b, a / b, TEXT('÷'));
}

// ——退出（PPT用 UnLoad）——
void cmdExit_Click() { form1.UnLoad(); }

// ——入口（PPT：eCommandButton_Click；IconSet 可注释）——
int main() {
    form1.EventAdd(ID_cmdJia,   eCommandButton_Click, cmdJia_Click);
    form1.EventAdd(ID_cmdJian,  eCommandButton_Click, cmdJian_Click);
    form1.EventAdd(ID_cmdCheng, eCommandButton_Click, cmdCheng_Click);
    form1.EventAdd(ID_cmdChu,   eCommandButton_Click, cmdChu_Click);
    form1.EventAdd(ID_cmdExit,  eCommandButton_Click, cmdExit_Click);

   
    form1.IconSet(IDI_ICON1);

    form1.Show();
    return 0;
}
