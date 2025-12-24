#include "resource.h"
#include "BForm.h"
#include <math.h>
#include <tchar.h>
#include <windows.h>

CBForm form1(ID_form1);

// 函数声明
void cmdSolve_Click();
void cmdExit_Click();
void DrawParabola(double a, double b, double c);

// 解方程按钮点击事件
void cmdSolve_Click()
{
    double a, b, c;
    double x1, x2, delta;
    
    // --- 修正 1: 使用 TextVal() ---
    // 获取输入值（使用 BForm 的 .TextVal() 函数）
    a = form1.Control(ID_txtA).TextVal();
    b = form1.Control(ID_txtB).TextVal();
    c = form1.Control(ID_txtC).TextVal();
    
    // 验证输入
    if(a == 0)
    {
        MessageBox(NULL, TEXT("系数 a 不能为 0！\n这不是二次方程。"), 
                       TEXT("输入错误"), MB_ICONERROR);
        return;
    }
    
    // 计算判别式
    delta = b*b - 4*a*c;
    
    // 根据判别式求解
    if (delta < 0)
    {
        // 无实根
        form1.Control(ID_txtX1).TextSet(TEXT("方程无实根!"));
        form1.Control(ID_txtX2).TextSet(TEXT("方程无实根!"));
    }
    else if (fabs(delta) < 1e-10) // delta == 0
    {
        // 一个根（重根）
        x1 = (-b) / (2*a);
        form1.Control(ID_txtX1).TextSet(x1);
        form1.Control(ID_txtX2).TextSet(TEXT(""));
    }
    else
    {
        // 两个不等实根
        x1 = (-b + sqrt(delta)) / (2*a);
        x2 = (-b - sqrt(delta)) / (2*a);
        form1.Control(ID_txtX1).TextSet(x1);
        form1.Control(ID_txtX2).TextSet(x2);
    }
    
    // ========== 绘制抛物线 ==========
    DrawParabola(a, b, c);
}

// 绘制抛物线函数
void DrawParabola(double a, double b, double c)
{
    // --- 修正 2: 使用 hWnd() ---
    // 获取Picture Control控件的窗口句柄
    HWND hForm = form1.hWnd(); // 使用 hWnd() 获取窗口句柄
    HWND hPicture = ::GetDlgItem(hForm, IDC_PICTURE_GRAPH);
    if(!hPicture) return;
    
    // 获取设备上下文
    HDC hdc = ::GetDC(hPicture);
    if(!hdc) return;
    
    // ( ... 您的 GDI 绘图代码 ... )
    // ( ... (代码保持不变) ... )

    // 获取绘图区域大小
    RECT rect;
    ::GetClientRect(hPicture, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // 创建内存DC和位图（双缓冲）
    HDC memDC = ::CreateCompatibleDC(hdc);
    HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)::SelectObject(memDC, memBitmap);
    
    // 填充白色背景
    HBRUSH whiteBrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
    ::FillRect(memDC, &rect, whiteBrush);
    
    // 坐标系参数
    int centerX = width / 2;
    int centerY = height / 2;
    int scale = 30; // 缩放比例
    
    // === 绘制网格线（浅灰色） ===
    HPEN gridPen = ::CreatePen(PS_DOT, 1, RGB(220, 220, 220));
    HPEN oldPen = (HPEN)::SelectObject(memDC, gridPen);
    
    // 垂直网格线
    for(int i = -10; i <= 10; i++)
    {
        int x = centerX + i * scale;
        if(x >= 0 && x < width)
        {
            ::MoveToEx(memDC, x, 0, NULL);
            ::LineTo(memDC, x, height);
        }
    }
    
    // 水平网格线
    for(int i = -10; i <= 10; i++)
    {
        int y = centerY - i * scale;
        if(y >= 0 && y < height)
        {
            ::MoveToEx(memDC, 0, y, NULL);
            ::LineTo(memDC, width, y);
        }
    }
    
    // === 绘制坐标轴（黑色粗线） ===
    ::SelectObject(memDC, oldPen);
    ::DeleteObject(gridPen);
    
    HPEN axisPen = ::CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    ::SelectObject(memDC, axisPen);
    
    // X轴
    ::MoveToEx(memDC, 0, centerY, NULL);
    ::LineTo(memDC, width, centerY);
    
    // Y轴
    ::MoveToEx(memDC, centerX, 0, NULL);
    ::LineTo(memDC, centerX, height);
    
    // X轴箭头
    ::MoveToEx(memDC, width - 10, centerY - 5, NULL);
    ::LineTo(memDC, width, centerY);
    ::LineTo(memDC, width - 10, centerY + 5);
    
    // Y轴箭头
    ::MoveToEx(memDC, centerX - 5, 10, NULL);
    ::LineTo(memDC, centerX, 0);
    ::LineTo(memDC, centerX + 5, 10);
    
    // === 绘制坐标轴标签 ===
    ::SetBkMode(memDC, TRANSPARENT);
    ::SetTextColor(memDC, RGB(0, 0, 0));
    ::TextOut(memDC, width - 20, centerY + 10, TEXT("x"), 1);
    ::TextOut(memDC, centerX + 10, 5, TEXT("y"), 1);
    
    // === 绘制刻度 ===
    HPEN tickPen = ::CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    ::SelectObject(memDC, tickPen);
    
    TCHAR numText[10];
    for(int i = -10; i <= 10; i++)
    {
        if(i == 0) continue;
        
        // X轴刻度
        int x = centerX + i * scale;
        if(x >= 0 && x < width)
        {
            ::MoveToEx(memDC, x, centerY - 3, NULL);
            ::LineTo(memDC, x, centerY + 3);
            
            _stprintf_s(numText, 10, TEXT("%d"), i);
            ::TextOut(memDC, x - 5, centerY + 5, numText, _tcslen(numText));
        }
        
        // Y轴刻度
        int y = centerY - i * scale;
        if(y >= 0 && y < height)
        {
            ::MoveToEx(memDC, centerX - 3, y, NULL);
            ::LineTo(memDC, centerX + 3, y);
            
            _stprintf_s(numText, 10, TEXT("%d"), i);
            ::TextOut(memDC, centerX + 5, y - 8, numText, _tcslen(numText));
        }
    }
    
    // === 绘制抛物线（红色粗线） ===
    ::SelectObject(memDC, oldPen);
    ::DeleteObject(tickPen);
    
    HPEN curvePen = ::CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
    ::SelectObject(memDC, curvePen);
    
    bool firstPoint = true;
    for(int px = 0; px < width; px++)
    {
        // 屏幕坐标转数学坐标
        double x = (px - centerX) / (double)scale;
        double y = a*x*x + b*x + c;
        
        // 数学坐标转屏幕坐标
        int py = centerY - (int)(y * scale);
        
        // 只绘制在范围内的点
        if(py >= 0 && py < height)
        {
            if(firstPoint)
            {
                ::MoveToEx(memDC, px, py, NULL);
                firstPoint = false;
            }
            else
            {
                ::LineTo(memDC, px, py);
            }
        }
        else
        {
            firstPoint = true;
        }
    }
    
    // === 标注顶点 ===
    double vertexX = -b / (2*a);
    double vertexY = (4*a*c - b*b) / (4*a);
    
    int vx = centerX + (int)(vertexX * scale);
    int vy = centerY - (int)(vertexY * scale);
    
    if(vx >= 0 && vx < width && vy >= 0 && vy < height)
    {
        // 绘制顶点（蓝色圆点）
        HPEN vertexPen = ::CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
        HBRUSH vertexBrush = ::CreateSolidBrush(RGB(0, 0, 255));
        
        ::SelectObject(memDC, vertexPen);
        HBRUSH oldBrush = (HBRUSH)::SelectObject(memDC, vertexBrush);
        
        ::Ellipse(memDC, vx - 5, vy - 5, vx + 5, vy + 5);
        
        // 标注坐标
        ::SetTextColor(memDC, RGB(0, 0, 255));
        TCHAR vertexLabel[100];
        _stprintf_s(vertexLabel, 100, TEXT("顶点(%.2lf,%.2lf)"), vertexX, vertexY);
        ::TextOut(memDC, vx + 10, vy - 20, vertexLabel, _tcslen(vertexLabel));
        
        ::SelectObject(memDC, oldBrush);
        ::DeleteObject(vertexBrush);
        ::DeleteObject(vertexPen);
    }
    
    // === 标注根（如果有） ===
    double delta_roots = b*b - 4*a*c; // (delta 变量名在GDI函数中可能重名，换一个)
    
    if(delta_roots >= 0)
    {
        double x1_root = (-b + sqrt(delta_roots)) / (2*a);
        double x2_root = (-b - sqrt(delta_roots)) / (2*a);
        
        HPEN rootPen = ::CreatePen(PS_SOLID, 2, RGB(0, 150, 0));
        HBRUSH rootBrush = ::CreateSolidBrush(RGB(0, 150, 0));
        
        ::SelectObject(memDC, rootPen);
        HBRUSH oldBrush = (HBRUSH)::SelectObject(memDC, rootBrush);
        
        // 根1
        int rx1 = centerX + (int)(x1_root * scale);
        if(rx1 >= 0 && rx1 < width)
        {
            ::Ellipse(memDC, rx1 - 5, centerY - 5, rx1 + 5, centerY + 5);
            
            ::SetTextColor(memDC, RGB(0, 150, 0));
            TCHAR rootLabel[50];
            _stprintf_s(rootLabel, 50, TEXT("x1=%.2lf"), x1_root);
            ::TextOut(memDC, rx1 - 25, centerY + 10, rootLabel, _tcslen(rootLabel));
        }
        
        // 根2（如果不是重根）
        if(delta_roots > 1e-10)
        {
            int rx2 = centerX + (int)(x2_root * scale);
            if(rx2 >= 0 && rx2 < width)
            {
                ::Ellipse(memDC, rx2 - 5, centerY - 5, rx2 + 5, centerY + 5);
                
                TCHAR rootLabel[50];
                _stprintf_s(rootLabel, 50, TEXT("x2=%.2lf"), x2_root);
                ::TextOut(memDC, rx2 - 25, centerY + 10, rootLabel, _tcslen(rootLabel));
            }
        }
        
        ::SelectObject(memDC, oldBrush);
        ::DeleteObject(rootBrush);
        ::DeleteObject(rootPen);
    }
    
    // === 显示方程 ===
    ::SetTextColor(memDC, RGB(255, 0, 0));
    TCHAR equation[100];
    _stprintf_s(equation, 100, TEXT("y = %.2lfx² + %.2lfx + %.2lf"), a, b, c);
    ::TextOut(memDC, 10, 10, equation, _tcslen(equation));
    
    // 将内存DC内容复制到屏幕
    ::BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    
    // 清理资源
    ::SelectObject(memDC, oldPen);
    ::SelectObject(memDC, oldBitmap);
    ::DeleteObject(curvePen);
    ::DeleteObject(axisPen);
    ::DeleteObject(memBitmap);
    ::DeleteDC(memDC);
    ::ReleaseDC(hPicture, hdc);
}

// 退出按钮
void cmdExit_Click()
{
    form1.UnLoad();
}

// 主函数
int main()
{
    // 绑定事件
    form1.EventAdd(ID_cmdSolve, eCommandButton_Click, cmdSolve_Click);
    form1.EventAdd(ID_cmdExit, eCommandButton_Click, cmdExit_Click);
    
    // 设置窗口图标
    form1.IconSet(IDI_ICON1);
    
    // 显示窗体
    form1.Show();
    
    return 0;
}