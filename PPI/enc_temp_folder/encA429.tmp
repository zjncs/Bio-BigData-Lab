#include "resource.h"
#include "BForm.h"
#include "BADO.h"  

// 定义全局窗体对象
CBForm form1(ID_form1); 

// 函数声明
void ListTaxIDs();

// -----------------------------------------------------------
// 主函数
// -----------------------------------------------------------
int main()
{
    // 1. 初始化窗体图标
    form1.IconSet(IDI_ICON1); 

    // 2. 显示窗体
    form1.Show();

    // 3. 连接数据库
    // ADOConn 是 BADO.h 中定义的全局变量
    if ( !ADOConn.Open(TEXT("PPI.accdb")) ) 
    {
        MsgBox(ADOConn.ErrorLastStr(), 
               TEXT("数据库连接失败"), 
               mb_OK, mb_IconExclamation);
    }

    // 4. 从数据库获得并显示所有支持的 taxID
    ListTaxIDs();

    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// -----------------------------------------------------------
// 从数据库获得所有的 taxID 并显示到列表框
// -----------------------------------------------------------
void ListTaxIDs()
{
    CBAdoRecordset rs;  
    tstring sItem;
    tstring sName; // 新增变量用于辅助拼接
    int iTaxID = 0, idx = 0;

    if ( !rs.Open(TEXT("SELECT * FROM taxes")) )
    {
        MsgBox(ADOConn.ErrorLastStr(), TEXT("获取 taxes 表失败！"), 
               mb_OK, mb_IconExclamation);
        return;
    }

    form1.Control(ID_cboTaxIDs).ListClear();

    while (!rs.EOFRs()) 
    {
        // 1. 获取 TaxID 字符串
        sItem = rs.GetField(TEXT("TaxID"));
        
        // 2. 转换为整数
        iTaxID = (int)Val(sItem.c_str()); 

        // 3. 【修复错误2】安全的字符串拼接
        // 先获取名称存为 tstring 对象，确保 "+" 运算符生效
        sName = rs.GetField(TEXT("Organism")); 
        
        // 拼接格式: "Organism (TaxID)"
        // 此时 sName 是对象，sItem 是对象，可以相加
        sItem = sName + TEXT(" (") + sItem + TEXT(")");

        // 4. 添加到界面
        idx = form1.Control(ID_cboTaxIDs).AddItem(sItem);
        form1.Control(ID_cboTaxIDs).ItemDataSet(idx, iTaxID);

        rs.MoveNext(); 
    }
    
    rs.Close(); 

    // 默认选择人类 (Homo sapiens)
    if (form1.Control(ID_cboTaxIDs).ListCount() > 0)
    {
        form1.Control(ID_cboTaxIDs).ListIndexSet(488); 
    }
}