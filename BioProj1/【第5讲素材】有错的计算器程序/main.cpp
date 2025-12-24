#include "resource.h"
#include "BForm.h"

CBForm form1(ID_form1);

enum EOperatorType		// 表示各种运算类型的枚举类型
{
	eOperNone = 0, 
	eOperJia = 1,
	eOperJian = 2,
	eOperCheng = 3,
	eOperChu = 4
};

#define MAX_DIGITS  14							// 最大支持的数字位数
tstring m_sNum1(TEXT("")), m_sNum2(TEXT(""));	// 保存两个数字字符串，
												//   用 m_sNum1 接收用户正在输入的数字字符串，
												//   用 m_sNum2 倒换前一个数字字符串
bool m_fInNum = false, m_fDoted = false;		// 是否在输入数字的中途、是否按过小数点键
enum EOperatorType m_operTask;					// 前一次的计算任务：+ - * /；==eOperNone 表示无任务

// 函数的声明
void NumberPress(LPCTSTR szNumber);				// 统一处理 0-9 的数字按键
void OperatorPress(EOperatorType oper);			// 统一处理 +、-、*、/ 的运算符按键
bool Calculate();								// 计算


// 处理1个数字按键
// szNumber 是所按键的数字的文本字符串首地址，如"1"、"2"……
void NumberPress(LPCTSTR szNumber)
{
	if (m_fInNum)		// 此前已按过数字键
	{
		if ( m_sNum1.length() >= MAX_DIGITS )	// 数字位过长，不能再拼接
			MsgBeep(mb_SoundSpeaker);			// 发出报警声
		else
			m_sNum1 += szNumber;				// 在 m_sNum1 后拼接字符串
	}
	else				// 此前还未按过数字键（m_blInNum==false）
	{
		m_sNum1 = szNumber;		// 将 szNumber 存入字符串对象
			//   作为 m_sNum1 的第1个字符，
			//   以后将在 m_sNum1 之后继续拼接用户的其他按键字符
		m_fInNum = true;		// 设置标志：已按过数字键
	}

	// 显示内容到控件
	if ( m_fDoted )	// 如果之前按过小数点键，m_sNum1 中间会含小数点，直接显示 m_sNum1
		form1.Control(ID_txtResu).TextSet(m_sNum1);
	else			// 如果之前没按过小数点键，显示 m_sNum1 后再显示一个“小数点”
	{
		form1.Control(ID_txtResu).TextSet( m_sNum1 );
		form1.Control(ID_txtResu).TextAdd( TEXT(".") );
	}
}

void cmdNum_Click()
{
	// 获得最近一次生成事件的控件ID，通过此来判断是被按下了哪个数字的按钮
	unsigned short idCtrl = form1.IDRaisingEvent();

	// 获得所按按钮（idCtrl）控件中的文字，即用户所按键的数字的文本如"1"、"2"
	LPTSTR szNum = form1.Control(idCtrl).Text();

	// 调用 NumPress() 处理
	NumberPress(szNum);
}

// + - * / 按键被按下的处理，oper==1,2,3,4（枚举）分别表示 + - * /
void OperatorPress(EOperatorType oper)
{
	if (m_operTask)					// 之前有计算任务 if ( m_operTask != 0 )
		if (! Calculate()) return;	// 先计算完之前的任务：如果计算失败，退出本函数
		// 如果计算成功，Calculate 函数已将结果转换为字符串后存入了 m_sNum2
		//   用户可继续输入数据由 m_sNum1 接收，以连续计算
	else							// 之前无计算任务（m_operTask==0）
		m_sNum2 = m_sNum1;	// 将目前数字字符串倒换到 m_sNum2
							// 腾出 m_sNum1 准备接收新的数字字符串
							// 之后待按下  + - * / 或 = 时再予计算
	
	// 保存新的计算任务
	m_operTask = oper;	
							
	// 清除标志变量
	m_fInNum = false;
	m_fDoted = false; 
}

void cmdCalc_Click()
{
	// 获得最近一次生成事件的控件ID，通过此来判断是被按下了哪个运算符的按钮
	unsigned short idCtrl = form1.IDRaisingEvent();

	// 在 form1_Load 函数中，已在 +、-、*、/ 按键中保存了附加整数1、2、3、4
	// 这里通过所保存的附加整数的值，来判断 idCtrl 控件是个什么按钮
	int iTagCtrl = form1.Control(idCtrl).TagInt();

	// 调用 OperatorPress() 处理，将 iTagCtrl 转换为枚举类型作为函数参数
	//（+、-、*、/ 的枚举值恰好对应1、2、3、4）
	OperatorPress( (EOperatorType)iTagCtrl );
}

void cmdDot_Click()
{
	if (m_fDoted)	// 已经按过小数点键，不能重复按
	{ 
		MessageBeep(mb_SoundSpeaker);	// 发出警报声
		return; 
	}
	else			// 没有按过小数点键（m_blDoted==false）
	{
		if (m_fInNum)			// 已经按过数字键，将 . 加入数字后拼接
			m_sNum1 += TEXT(".");
		else					// 未按过数字键，直接按 . ，说明是“0.”
		{
			m_sNum1 = TEXT("0.");
			m_fInNum = true;	// 视为已按过数字键
		}
		
		// 设置标志：已经按过小数点键
		m_fDoted = true;		

		// 显示内容到控件
		form1.Control(ID_txtResu).TextSet(m_sNum1);
	}
}

void cmdSign_Click()
{
	// 若尚未按过数字键就直接按 +/- 键，则按键无效，直接退出函数
	if (! m_fInNum) { MessageBeep(mb_SoundSpeaker); return; }

	if ( m_sNum1[0] == TEXT('-') )		// 若第1个字符是 '-'，说明已经是负数
		m_sNum1.erase(0, 1);			// 删除第1个字符（负号）：从第0个位置开始，删除1个字符
	else
		m_sNum1 = TEXT("-") + m_sNum1;	// 在前面加负号

	// 显示内容到控件
	if ( m_fDoted )	// 如果之前按过小数点键，m_sNum1 中间会含小数点，直接显示 m_sNum1
		form1.Control(ID_txtResu).TextSet(m_sNum1);
	else				// 如果之前没按过小数点键，显示 m_sNum1 后再显示一个“小数点”
	{
		form1.Control(ID_txtResu).TextSet( m_sNum1 );
		form1.Control(ID_txtResu).TextAdd( TEXT(".") );
	}
}


// 按下 = 键的事件处理函数
void cmdEq_Click()
{
	if (m_operTask)	// 如现在有计算任务，按下 = 键才能算
	{
		Calculate();

		// 如果计算成功，Calculate 函数已将结果转换为字符串后存入了 m_sNum2
		// 按下 = 后的结果，可被当做已输入的第1个数据，用户可按 + - * / 继续计算
		//   故这里再将此结果字符串倒换到 m_sNum1
		m_sNum1 = m_sNum2;
	}

	// 清除标志变量
	m_fDoted = false;
	m_fInNum = false;
}

void cmdClear_Click()
{
	m_fInNum = false;
	m_fDoted = false; 
	m_sNum1 = TEXT("");
	m_sNum2 = TEXT("");
	m_operTask = eOperNone;
 	form1.Control(ID_txtResu).TextSet(TEXT("0."));
}


// 计算 m_operTask 中指定的计算任务
// 并将计算结果转换为字符串，存入 m_sNum2
// 成功返回 true，失败返回 false
bool Calculate()
{	double num1, num2;  double result; 

	// 将 m_sNum1 和 m_sNum2 转换为数值
	// string 对象用 .c_str()，获得字符串的首地址，才能用 Val 转换为数值
	num1 = Val( m_sNum1.c_str() );
	num2 = Val( m_sNum2.c_str() );

	// 按不同运算符计算
	switch(m_operTask)
	{
	case eOperJia:	// + 
		result = num1 + num2;	
		break;
	case eOperJian:	// - 注意是 Num2-Num1，而不是 Num1-Num2
		result = num2 - num1;	
		break;
	case eOperCheng:	// * 
		result = num1 * num2;	
		break;
	case eOperChu:	// / 注意是 Num2/Num1，而不是 Num1/Num2
		if ( num1 == 0 )
		{
			cmdClear_Click();	// 清除所有计算
			form1.Control(ID_txtResu).TextSet(TEXT("错误：除数为0！"));
			return false;		// 返回 false，退出本函数
		}
		result = num2 / num1;	
		break;
	default:
		return false;			// 非法计算任务，返回失败
	}

	// m_operTask 所规定的计算任务已经完成，m_operTask 清 0
	m_operTask = eOperNone;

	// 将计算结果（数值型的result）转换为字符串
	// 将计算结果转换为的字符串存入 m_sNum2
	m_sNum2 = Str(result); 

	// 加工结果字符串：若小数前缺 0，补 0
	if (m_sNum2[0] == TEXT('.'))		// 正数
		m_sNum2 = TEXT("0") + m_sNum2;
	if (m_sNum2[0] == TEXT('-')  &&  m_sNum2[1] == TEXT('.') )	// 负数
		m_sNum2 = TEXT("-0") + m_sNum2.erase(0,1);	// 将 "-0" 和 删除第1个字符（-）的 m_sNum2 拼接

	// 显示结果字符串
	form1.Control(ID_txtResu).TextSet(m_sNum2);

	if ( m_sNum2.find(TEXT('.')) != string::npos )	// 结果字符串中无 .，后再加 . 	
													// 例如结果若为 “10”，应显示为“10.”
		form1.Control(ID_txtResu).TextAdd( TEXT(".") );

	return true;
}


// 用窗体接收所有键盘按键
// form1.KeyPreview = true; 时才能由窗体优先接收按键
// 否则按键只由现在获得输入焦点的控件接收，窗体不能接收
void form1_KeyDown(int keyCode, int shift, int pbCancel)
{
	// 如何知道按键对应的 keyCode 是几？
	// 添加下面代码，然后运行程序，你就按键吧——看窗体标题栏就可以喽！
	// form1.TextSet(keyCode);
	// 正式程序编写好后，再把此句注释掉

 	if (shift=0 && keyCode>=48 && keyCode<=57)	// 未按 shift 键的数字键 0～9
 	{
 		NumberPress( Str(keyCode - 48) );		// 调用 NumberPress 函数处理即可
 	}
 	else if (keyCode>=96 && keyCode<=105)		// 小键盘的数字键 0～9
 	{
 		NumberPress( Str(keyCode - 96) );		// 调用 NumberPress 函数处理即可
 	}
 	else
 	{
 		switch (keyCode)
 		{
 		case 187:	// 大键盘的 =/+ 键被按下
 			if (1==shift)	// 同时按下 shift 键，说明按的是 +
 				OperatorPress( eOperJia ); 
 			else			// 未同时按 shift 键，说明按的是 =
 				cmdEq_Click(); 
 			break;
 		case 189:	// 大键盘的 -/_ 键被按下
 			if (0==shift) OperatorPress( eOperJian ); 
 			break;
 		case 56:	// 大键盘的 8/* 键被按下
 			if (1==shift) OperatorPress( eOperCheng ); 
 			break;
 		case 191:	// 大键盘的 //? 键被按下
 			if (0==shift) OperatorPress( eOperChu ); 
 			break;
 
 		case 107:	// 小键盘的 + 键被按下
 			OperatorPress( eOperJia ); break;
 		case 109:	// 小键盘的 - 键被按下
 			OperatorPress( eOperJian ); break;
 		case 106:	// 小键盘的 * 键被按下
 			OperatorPress( eOperCheng ); break;
 		case 111:	// 小键盘的 / 键被按下
 			OperatorPress( eOperChu ); break;
 		case 110:	// 小键盘 . 键被按下
 			cmdDot_Click(); break;
 
 		case 190:	// 大键盘的 ./> 键被按下
 			if (shift==0) cmdDot_Click();
 			break;
 	
 		case 120:	// F9 键被按下，切换 +/-
 			cmdSign_Click(); break;
 
 		case 27:	// Esc 键被按下
 			cmdClear_Click(); break;
 		
 		case 13:	// 回车键被按下
 			cmdEq_Click(); break;
 
 		}	// end switch
 	}	// end if
 
 	// 使 ID_txtResu 文本框获得输入焦点，以便让其他控件失去焦点
 	//   免得按键对那些控件误操作（如按钮具有焦点时，回车键会按下按钮）
 	form1.Control(ID_txtResu).SetFocus();	
}

void form1_Load()
{
	// 使窗体上任何控件内的按键事件，首先传递到窗体本身接收
	// 即用 form1_KeyDown() 就可统一接收无论在哪个控件内的按键了
	// 否则要为每个控件，分别编写一个 ControlXXX_KeyDown() 事件函数
	form1.KeyPreview = true;	

	// 在 +、-、*、/ 按键控件中，增加附加的整数数据 TagInt，以便指示该按钮类型
	// 附加数据类似于变量，可保存内容，但不必通过再定义变量保存，
	// 可直接保存到控件中，比较方便
	form1.Control(ID_cmdAdd).TagIntSet(1);
	form1.Control(ID_cmdAdd).TagIntSet(2);
	form1.Control(ID_cmdAdd).TagIntSet(3);
	form1.Control(ID_cmdAdd).TagIntSet(4);

	// 设置字体
	form1.Control(ID_txtResu).FontSizeSet(16);
	
	// 消除
	cmdClear_Click();
}

int main()
{
	form1.IconSet(IDI_ICON1); 

	form1.EventAdd(0, eForm_Load, form1_Load); 
	form1.EventAdd( ID_cmd0, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd1, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd2, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd3, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd4, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd5, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd6, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd7, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd8, eCommandButton_Click, cmdNum_Click );
	form1.EventAdd( ID_cmd9, eCommandButton_Click, cmdNum_Click );

	form1.EventAdd( ID_cmdAdd, eCommandButton_Click, cmdCalc_Click );
	form1.EventAdd( ID_cmdMinus, eCommandButton_Click, cmdCalc_Click );
	form1.EventAdd( ID_cmdMul, eCommandButton_Click, cmdCalc_Click );
	form1.EventAdd( ID_cmdDiv, eCommandButton_Click, cmdCalc_Click );

	form1.EventAdd( ID_cmdDot, eCommandButton_Click, cmdDot_Click );
	form1.EventAdd( ID_cmdSign, eCommandButton_Click, cmdSign_Click );
	form1.EventAdd( ID_cmdClear, eCommandButton_Click, cmdClear_Click );
	form1.EventAdd( ID_cmdEq, eCommandButton_Click, cmdEq_Click );
	
	form1.EventAdd( 0, eKeyDown, form1_KeyDown);

	form1.Show();

	return 0;
}

