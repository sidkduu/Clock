// 本代码仅展示程序核心函数，无法直接运行，可执行代码为code.c

/*
F5区16位按键:
a 按a回主界面显示时间(在所有分支的主界面)（最高位显示A）
b 进录音功能分支 数字按键实现地址选择，(再按b后输入十进制地址录制对应的音频)(再按c后输入十进制地址后播放地址对应的音频）（从进入录音功能分支后到开始某一功能要按三次）(无显示/无效显示)
c 进入秒表分支  按c停止计时/开始计时   按d清零秒表(可能为00.01s)(显示计时结果00.00--99.99，最高位显示C)
d 时区转换分支 按d切换时区
e 语音提醒设置分支 按e开始设定 数字按键设定时间 (显示设定的提醒时间)（最高位显示E）
f 时钟加速6000倍 再按f取消
注意：由于8255只有ABC三个口,录音功能启动时不能启动显示地址,此时地址显示为乱码但是时钟仍然运行
*/

#define I8255_Ctr 0x273  // 8255控制端口地址
#define I8255_PA 0x270   // 8255 PA端口地址
#define I8255_PB 0x271   // 8255 PB端口地址
#define I8255_PC 0x272   // 8255 PC端口地址

#define ISDCOMM 0x270    // ISD1420录放音地址/操作模式输入地址(端口复用)
#define ISD1420_AD1 0x00 // ISD1420 0、4号键录放音起始地址
#define ISD1420_AD2 0x28 // ISD1420 1、5号键录放音起始地址
#define ISD1420_AD3 0x50 // ISD1420 2、6号键录放音起始地址
#define ISD1420_AD4 0x78 // ISD1420 3、7号键录放音起始地址

#define COM_ADDR    0x263   // 8259控制端口地址
#define T0_ADDR     0x260
#define T2_ADDR     0x262

#define u8 unsigned char
#define u16 unsigned int

// 声明部分函数
extern void outportb(unsigned int, char); // 写I/O端口
extern char inportb(unsigned int);       // 读I/O端口
void timer_interrupt(void);
void delay(u16 tms);
void increment_stopwatch(void);
u8 key(void);
// 定义变量存储当前时间
u8 hours, minutes, seconds;
u8 reminder_hour, reminder_minute = 0; // 语音提醒时间
u8 stopwatch_seconds, stopwatch_tenths; // 秒表计时器
u8 ISD_addre = 0;   // 写入的地址
u8 Faster = 0;
u8 buffer[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};    //置显示缓冲器初值
u8 clk, cnt, sclk = 0;       // 时钟当前情况，时钟计数
u8 KeepMode;                //保存REC、PLAYE、PLAYL当前值
u8 voice[] ={               // 保存录音地址和长度
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20,
    0x24, 0x28, 0x2c, 0x32, 0x38, 0x3e, 0x44, 0x4a, 0x50,
    0x56, 0x5c, 0x62, 0x68, 0x6e, 0x74, 0x7a, 0x80
};
u16 during[] = {
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 75, 75, 75, 75, 75, 75, 75, 75,
    75, 75, 75, 75, 75, 75, 400
};                              

// 初始化8255芯片
void init_8255() {
    outportb(I8255_Ctr, 0x81); // 8255初始化, PA为输入，PC的低四位为输出
}

// 核心函数：保证系统进入其它模块后时钟仍然正常运行
void pulse_test()
{
    int clk_now = (inportb( I8255_PC) & (0x04)) >> 2;
    if ((clk_now ^ clk) & (0x1)){
        timer_interrupt();
        clk = clk_now;
    }
}

// 录音子程序    
u8 KEY_REC(u8 startadr, u16 during)     // 起始地址，录音长度
{
    u8 keyResult;
    recISD(startadr);                   //调用录音子程序
    while (during--)
    {
        delay(160);                     //延时0.25s(测出来的)
    }
    stopISD();                          //停止录音
    return 0xff;
}

// 放音
u8 KEY_PLAY(u8 startadr, u16 during)
{
    u8 keyResult;
    playISD(startadr);                      //调用录音子程序
    while (during--)
    {
        delay(160);                           //延时0.25s(测出来的)
    }
    stopISD();                              //停止放音
    return 0xff;
}

/*--------------------------- 以下是各分支的辅助函数 ---------------------------*/

// 模拟定时器中断处理函数
void timer_interrupt() {
    if (Faster == 1) {
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
        }
        return;
    }
    cnt++;
    if (cnt % 100 != 0) return;
    cnt=0;
    seconds++;
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
            //  整点报时
            KEY_PLAY(voice[hours], during[hours]);
            KEY_PLAY(voice[24], during[24]);
        }
        // 检查是否到达语音提醒时间
        if (hours == reminder_hour && minutes == reminder_minute) {
            KEY_PLAY(voice[25], during[25]); // 播放欢迎语作为提醒
        }
    }
}

/*--------------------------- 以下是显示与等待按键部分 ---------------------------*/

// 将显示数据加载到缓存(每一位对应数码管的输出，在DIR()函数中输出)
void update_time_buffer()
{
    buffer[7] = 0x0a;
    buffer[6] = 0x10;
    buffer[5] = (u8)(hours / 10);
    buffer[4] = (u8)(hours % 10);
    buffer[3] = (u8)(minutes / 10);
    buffer[2] = (u8)(minutes % 10);
    buffer[1] = (u8)(seconds / 10);
    buffer[0] = (u8)(seconds % 10);
}

u8 main_key() // 显示buffer内容，返回按下的键
{
    u8 i, j, keyResult;
    u8 bNoKey = 1;
    while(bNoKey)
    {
        pulse_test();
        update_time_buffer();
        if (AllKey() == 0)      //调用判有无闭合键函数
        {
            DIR();              //扫描数码管并显示
            DIR();
            continue;
        }
        DIR();
        DIR();
        if (AllKey() == 0)      //调用判有无闭合键函数
            continue;
        i = 0xfe;
        keyResult = 0;
        do
        {
            pulse_test();
            outportb(I8255_PB, i);
            j = ~inportb(I8255_PC);
            if (j & 3)
            {
                bNoKey = 0;
                if (j & 2)              //1行有键闭合
                    keyResult += 8;
            }
            else                        //没有键按下
            {
                keyResult++;            //列计数器加1
                i = ((i << 1) | 1);
            }
        }while(bNoKey  && (i != 0xff));
    }
    if (!bNoKey)
    {
        while(AllKey())     //判断释放否
        {
            pulse_test();
            update_time_buffer();
            DIR();           //扫描数码管并显示
        }
    }
    return keyResult;
}

void main() {
    u8 flag;

    // 初始化系统
    init_8255();
    init_buffer();

    // 设置时钟信号源，输出50Hz的方波(接入实验箱1MHz信号源)
    outportb( COM_ADDR, 0x35);      //计数器T0设置在模式2状态,输出负脉冲,BCD码计数
    outportb( T0_ADDR, 0x00);
    outportb( T0_ADDR, 0x10);       //CLK0/1000
    // 如果8259CLK2坏了就试试CLK1，不然就换箱子
    outportb( COM_ADDR, 0xB7);      //计数器T2为模式3状态，输出方波,BCD码计数
    outportb( T2_ADDR, 0x20);       //写入低字节
    outportb( T2_ADDR, 0x00);       //CLK2/20

    // 设置初始时间
    // 设置语音提醒时间
    // 主循环：进入分支
    flag = 0;
    while (1) {
        pulse_test();   // 判断时钟
        flag = main_key();  // 获取按键输入
        switch (flag)
        {
        case (u8)10:
            update_time_buffer();
            DIR();     //扫描数码管并显示
            DIR();
            break;
        case (u8)11:
            Voice_Branch();
            break;
        case (u8)12:
            Stopper_Branch();
            break;
        case (u8)13:
            hours++;
            break;
        case (u8)14:
            Reminder_Branch();
            break;
        case (u8)15:
            Faster = !Faster;
            break;
        default:
            update_time_buffer();   
            DIR();
            DIR();
            break;
        }
    }
}