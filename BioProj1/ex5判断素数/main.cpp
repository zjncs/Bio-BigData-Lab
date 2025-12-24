#include "resource.h"
#include "BForm.h"
#include <tchar.h>
#include <cstdio>
#include <math.h>
#include <windows.h> // GetTickCount

// ——窗体（按 PPT 模板）——
CBForm form1(ID_form1);

// ——整数 → 字符串（VS2010 兼容）——
static tstring IntToStr(long long v) {
    TCHAR buf[64] = {0};
#if defined(_MSC_VER)
    _stprintf_s(buf, TEXT("%lld"), v);
#else
    _stprintf(buf, TEXT("%lld"), v);
#endif
    return tstring(buf);
}

// ——状态栏更新（若未放 ID_lblStatus，自动忽略）——
static void SetStatus(const tstring& s) {
#ifdef ID_lblStatus
    form1.Control(ID_lblStatus).TextSet(s.c_str());
#endif
}

// ——素数判断（试除到 sqrt(n)，奇数步进）——
static bool IsPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    long long r = (long long)floor(sqrt((double)n));
    for (long long d = 3; d <= r; d += 2) {
        if (n % d == 0) return false;
    }
    return true;
}

// ——质因数分解：如 221 → “13×17”，100 → “2×2×5×5”——
static tstring Factorize(long long n) {
    long long x = n;
    tstring res;

    while (x % 2 == 0) {
        if (!res.empty()) res += TEXT("×");
        res += TEXT("2");
        x /= 2;
    }
    long long d = 3;
    long long r = (long long)floor(sqrt((double)x));
    while (d <= r && x > 1) {
        while (x % d == 0) {
            if (!res.empty()) res += TEXT("×");
            res += IntToStr(d);
            x /= d;
            r = (long long)floor(sqrt((double)x));
        }
        d += 2;
    }
    if (x > 1) {
        if (!res.empty()) res += TEXT("×");
        res += IntToStr(x);
    }
    return res;
}

// ——按钮：判断素数（写入左侧结果区 ID_txtResultN）——
void cmdPrime_Click() {
    DWORD t0 = GetTickCount();

    long long n = (long long)form1.Control(ID_txtData1).TextVal();

    tstring out;
    if (n < 2) {
        out = IntToStr(n) + TEXT(" 不是素数（n ≥ 2）。");
    } else if (IsPrime(n)) {
        out = IntToStr(n) + TEXT(" 是素数。");
    } else {
        // 计算最小因子（用于可解释输出）
        long long smallest = 2;
        if (n % 2 != 0) {
            smallest = 0;
            long long r = (long long)floor(sqrt((double)n));
            for (long long d = 3; d <= r; d += 2) {
                if (n % d == 0) { smallest = d; break; }
            }
            if (smallest == 0) smallest = n; // 理论上不应到此
        }
        tstring fac = Factorize(n);
        out = IntToStr(n) + TEXT(" 不是素数；最小因子 ")
            + IntToStr(smallest) + TEXT("；分解：")
            + IntToStr(n) + TEXT(" = ") + fac + TEXT("。");
    }

#ifdef ID_txtResultN
    form1.Control(ID_txtResultN).TextSet(out.c_str());
#else
    // 若未放置左侧结果区，则回落到通用结果框 ID_txtResult（可选）
    form1.Control(ID_txtResult).TextSet(out.c_str());
#endif

    DWORD t1 = GetTickCount();
    // 状态：耗时 ＋ √n 边界
    TCHAR tb[64] = {0};
#if defined(_MSC_VER)
    _stprintf_s(tb, TEXT("%lu"), (unsigned long)(t1 - t0));
#else
    _stprintf(tb, TEXT("%lu"), (unsigned long)(t1 - t0));
#endif
    tstring stat = TEXT("耗时 ") + tstring(tb) + TEXT(" ms；检查到 ⌊√n⌋。");
    SetStatus(stat);
}

// ——按钮：区间素数（写入右侧结果区 ID_txtResultR）——
void cmdRange_Click() {
    DWORD t0 = GetTickCount();

    long long L = (long long)form1.Control(ID_txtL).TextVal();
    long long R = (long long)form1.Control(ID_txtR).TextVal();

    if (R < L) {
#ifdef ID_txtResultR
        form1.Control(ID_txtResultR).TextSet(TEXT("起点必须 ≤ 终点。"));
#else
        form1.Control(ID_txtResult).TextSet(TEXT("起点必须 ≤ 终点。"));
#endif
        SetStatus(TEXT(""));
        return;
    }

    // 收集素数列表与统计（每行输出三个素数）
    tstring list;
    int count = 0;
    int perLine = 3;
    int col = 0;

    for (long long x = L; x <= R; x++) {
        if (IsPrime(x)) {
            if (!list.empty()) list += TEXT("，");
            list += IntToStr(x);
            count++;
            col++;
            if (col == perLine) {
                list += TEXT("\r\n");
                col = 0;
            }
        }
    }

    tstring out = IntToStr(L) + TEXT("～") + IntToStr(R) + TEXT(" 素数：") + list;

    double total = (double)(R - L + 1);
    double ratio = (total > 0 ? (count * 100.0 / total) : 0.0);

    TCHAR buf[64] = {0};
#if defined(_MSC_VER)
    _stprintf_s(buf, TEXT("\r\n共 %d 个素数，占 %.1f%%。"), count, ratio);
#else
    _stprintf(buf, TEXT("\r\n共 %d 个素数，占 %.1f%%。"), count, ratio);
#endif
    out += buf;

#ifdef ID_txtResultR
    form1.Control(ID_txtResultR).TextSet(out.c_str());
#else
    form1.Control(ID_txtResult).TextSet(out.c_str());
#endif

    DWORD t1 = GetTickCount();
    // 状态：耗时 ＋ √R 边界说明
    TCHAR tb[64] = {0};
#if defined(_MSC_VER)
    _stprintf_s(tb, TEXT("%lu"), (unsigned long)(t1 - t0));
#else
    _stprintf(tb, TEXT("%lu"), (unsigned long)(t1 - t0));
#endif
    tstring stat = TEXT("耗时 ") + tstring(tb) + TEXT(" ms；奇数试除到 ⌊√R⌋。");
    SetStatus(stat);
}

// ——可选：退出——
#ifdef ID_cmdExit
void cmdExit_Click() { form1.UnLoad(); }
#endif

// ——入口（按 PPT 事件风格）——
int main() {
    // 事件绑定
    form1.EventAdd(ID_cmdPrime, eCommandButton_Click, cmdPrime_Click);
    form1.EventAdd(ID_cmdRange, eCommandButton_Click, cmdRange_Click);
#ifdef ID_cmdExit
    form1.EventAdd(ID_cmdExit,  eCommandButton_Click, cmdExit_Click);
#endif

    // 设置图标（若 resource.h 中已定义 IDI_MAIN，且资源中有同名图标）
    form1.IconSet(IDI_ICON1);

    // 状态栏初始为空
    SetStatus(TEXT(""));

    form1.Show();
    return 0;
}
