#define I8255_Ctr 0x273  // 8255控制端口地址
#define I8255_PA 0x270   // 8255 PA端口地址
#define I8255_PB 0x271   // 8255 PB端口地址
#define I8255_PC 0x272   // 8255 PC端口地址

#define ISDCOMM 0x270    // ISD1420录放音地址/操作模式输入地址
#define ISD1420_AD1 0x00 // ISD1420 0、4号键录放音起始地址
#define ISD1420_AD2 0x28 // ISD1420 1、5号键录放音起始地址
#define ISD1420_AD3 0x50 // ISD1420 2、6号键录放音起始地址
#define ISD1420_AD4 0x78 // ISD1420 3、7号键录放音起始地址

#define COM_ADDR	0x263   // 8259控制端口地址
#define T0_ADDR		0x260
#define T1_ADDR		0x262

#define u8 unsigned char
#define u16 unsigned int

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
u8 buffer[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};	//置显示缓冲器初值
u8 clk, cnt, sclk = 0;       // 时钟当前情况，时钟计数
u8 KeepMode;						//保存REC、PLAYE、PLAYL当前值
u8 voice[] ={
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20,
    0x24, 0x28, 0x2c, 0x32, 0x38, 0x3e, 0x44, 0x4a, 0x50,
    0x56, 0x5c, 0x62, 0x68, 0x6e, 0x74, 0x7a, 0x80
};
u16 during[] = {
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 75, 75, 75, 75, 75, 75, 75, 75,
    75, 75, 75, 75, 75, 75, 400
};                              // 保存录音地址和长度


// 初始化8255芯片
void init_8255() {
    outportb(I8255_Ctr, 0x81); // 8255初始化, PA为输入，PC的低四位为输出
}

// 向8255写入数据
void write_to_8255(u8 data) {
    outportb(I8255_PB, data);
}

void init_buffer()
{
	int i ;
    for(i = 0; i < 8; i++){
        buffer[i] = 0x10;
    }
}

void pulse_test()
{
    int clk_now = (inportb( I8255_PC) & (0x04)) >> 2;
    if ((clk_now ^ clk) & (0x1)){
        timer_interrupt();
        clk = clk_now;
    }
}

void pulse_stopper_test()
{
    int clk_now = (inportb( I8255_PC) & (0x04)) >> 2;
    if ((clk_now ^ sclk) & (0x1)){
        increment_stopwatch();
        sclk = clk_now;
    }
}

//初始化
void initISD()
{
	outportb(I8255_PC, 0x70);					//语音模块初始化，关闭录放音功能
	KeepMode = 0x70;							
	outportb(ISDCOMM, 0);					//允许手动录放音,当A6,A7为高时，无法手动放音
}

//停止录放音
void stopISD()
{
	u8 i = (KeepMode & 0xbf);
	outportb(I8255_PC, i);					//PLAYL:一个负脉冲停止放音
	i |= 0x40;
	outportb(I8255_PC, i);
	i |= 0x30;					
	outportb(I8255_PC, i);					//1->REC,PLAYE, 关闭所有操作指令
	KeepMode = i;
	outportb(ISDCOMM, 0);					//允许手动录放音,当A6,A7为高时，无法手动放音
	delay(1500);
}

//录音
void recISD(u8 startadr)
{
    u8 i;
    pulse_test();
	outportb(ISDCOMM, startadr);			//设置录音起始地址
	i = KeepMode & 0xef;
	outportb(I8255_PC, i);					//REC变低，即开始录音
	KeepMode = i;
}

//放音
void playISD(u8 startadr)
{
	u8 i;
	stopISD();								//暂停之前的录放音操作
	outportb(ISDCOMM, startadr);			//设置放音起始地址
	i = KeepMode & 0xdf;
    pulse_test();
	outportb(I8255_PC, i);					//0->PLAYE 开始放音,边沿放音模式
	i |= 0x20;
	outportb(I8255_PC, i);
	KeepMode = i;
}

//录音子程序	
u8 KEY_REC(u8 startadr, u16 during)     // 起始地址，录音长度
{
	u8 keyResult;
	recISD(startadr);					//调用录音子程序
	while (during--)
	{
		delay(160);
	}
	stopISD();							//停止录音
	return 0xff;
}

//放音
u8 KEY_PLAY(u8 startadr, u16 during)
{
	u8 keyResult;
	playISD(startadr);							//调用录音子程序
	while (during--)
	{
       delay(160);
    }
    stopISD();
	return 0xff;
}

// 播放对应地址的报时音
void play_sound(u8 address) {
	u8 mode;
    outportb(ISDCOMM, address); // 设置放音起始地址
    mode = inportb(I8255_PC) & 0xdf; // 0->PLAYE 开始放音
    outportb(I8255_PC, mode);
    mode |= 0x20;
    outportb(I8255_PC, mode);
}

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
            //  整点报时
            KEY_PLAY(voice[hours], during[hours]);
            KEY_PLAY(voice[24], during[24]);
        }
        // 检查是否到达语音提醒时间
        if (hours == reminder_hour && minutes == reminder_minute) {
            KEY_PLAY(voice[25], during[25]); // 播放欢迎语作为提醒
        }
    }
}

// 延时0.1ms * t
void delay(u16 tms)
{
    while(tms--)    // 1周期
    {
        pulse_test();   // 9周期， 等价于i = 10 while(i--){;}
    }               // 1周期
}


// 将数据加载到缓存
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

void update_stopwatch_buffer()
{
    buffer[7] = 0x0c;
    buffer[6] = 0x10;
    buffer[5] = 0x10;
    buffer[4] = 0x10;
    buffer[3] = (u8)(stopwatch_seconds / 10);
    buffer[2] = (u8)(stopwatch_seconds % 10);
    buffer[1] = (u8)(stopwatch_tenths / 10);
    buffer[0] = (u8)(stopwatch_tenths % 10);
}

void update_voice_buffer()
{
    buffer[7] = 0x0b;
    buffer[6] = 0x10;
    buffer[5] = 0x10;
    buffer[4] = 0x10;
    buffer[3] = 0x10;
    buffer[2] = 0x10;
    buffer[1] = (u8)(ISD_addre / 10);
    buffer[0] = (u8)(ISD_addre % 10);
}

void update_reminder_buffer()
{
    buffer[7] = 0x0d;
    buffer[6] = 0x10;
    buffer[5] = 0x10;
    buffer[4] = 0x10;
    buffer[3] = (u8)(reminder_hour / 10);
    buffer[2] = (u8)(reminder_hour % 10);
    buffer[1] = (u8)(reminder_minute / 10);
    buffer[0] = (u8)(reminder_minute % 10);
}

// 设置语音提醒时间
void set_reminder(u8 hour, u8 minute) {
    reminder_hour = hour;
    reminder_minute = minute;
}

// 清零并开始秒表计时
void init_stopper() {
    stopwatch_seconds = 0;
    stopwatch_tenths = 0;
}

// 增加秒表计时器的值
void increment_stopwatch() {
    stopwatch_tenths++;
    if (stopwatch_tenths >= 100) {
        stopwatch_tenths = 0;
        stopwatch_seconds++;
        if (stopwatch_seconds >= 100) {
            stopwatch_seconds = 0;
        }
    }
}


//扫描数码管
void DIR()
{
	u8 i, dig = 0xfe;
	u8 SegArray[]={0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
			    0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E, 0xFF}; 

	for (i = 0 ; i < 8; i++)
	{
		outportb(I8255_PA, SegArray[buffer[i]]);		//段数据
		outportb(I8255_PB, dig);						//选择数码管
		delay(5);									    //延迟0.5ms
		outportb(I8255_PB, 0xff);
		dig = ((dig << 1) | 1);
	}
}

u8 AllKey()
{
	u8 i;
	outportb(I8255_PB, 0x0);
	i = (~inportb(I8255_PC) & 0x3);
	return i;
}

u8 main_key() // 显示buffer内容，返回按下的键
{
	u8 i, j, keyResult;
	u8 bNoKey = 1;
	while(bNoKey)
	{
        pulse_test();
        update_time_buffer();
		if (AllKey() == 0)		//调用判有无闭合键函数
		{
			DIR();
			DIR();
			continue;
		}
		DIR();
		DIR();
		if (AllKey() == 0)		//调用判有无闭合键函数
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
				if (j & 2)				//1行有键闭合
					keyResult += 8;
			}
			else						//没有键按下
			{
				keyResult++;			//列计数器加1
				i = ((i << 1) | 1);
			}
		}while(bNoKey  && (i != 0xff));
	}
	if (!bNoKey)
	{
		while(AllKey())		//判断释放否
		{
            pulse_test();
            update_time_buffer();
			DIR();
		}
	}
	return keyResult;
}

u8 key() // 显示buffer内容，返回按下的键
{
	u8 i, j, keyResult;
	u8 bNoKey = 1;
	while(bNoKey)
	{
        pulse_test();
		if (AllKey() == 0)		//调用判有无闭合键函数
		{
			continue;
		}
		if (AllKey() == 0)		//调用判有无闭合键函数
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
				if (j & 2)				//1行有键闭合
					keyResult += 8;
			}
			else						//没有键按下
			{
				keyResult++;			//列计数器加1
				i = ((i << 1) | 1);
			}
		}while(bNoKey  && (i != 0xff));
	}
	if (!bNoKey)
	{
		while(AllKey())		//判断释放否
		{
            pulse_test();
		}
	}
	return keyResult;
}

u8 reminder_key()
{
	u8 i, j, keyResult;
	u8 bNoKey = 1;
	while(bNoKey)
	{
        pulse_test();
        update_reminder_buffer();
		if (AllKey() == 0)		//调用判有无闭合键函数
		{
			DIR();
			DIR();
			continue;
		}
		DIR();
		DIR();
		if (AllKey() == 0)		//调用判有无闭合键函数
			continue;
		i = 0xfe;
		keyResult = 0;
		do
		{
            pulse_test();
            update_reminder_buffer();
			outportb(I8255_PB, i);
			j = ~inportb(I8255_PC);
			if (j & 3)
			{
				bNoKey = 0;
				if (j & 2)				//1行有键闭合
					keyResult += 8;
			}
			else						//没有键按下
			{
				keyResult++;			//列计数器加1
				i = ((i << 1) | 1);
			}
		}while(bNoKey  && (i != 0xff));
	}
	if (!bNoKey)
	{
		while(AllKey())		//判断释放否
		{
            pulse_test();
            update_reminder_buffer();
			DIR();
		}
	}
	return keyResult;
}

u8 stopper_key(u8 paused) // 显示buffer内容，返回按下的键
{
	u8 i, j, keyResult;
	u8 bNoKey = 1;
	while(bNoKey)
	{
        if(!paused)pulse_stopper_test();
        update_stopwatch_buffer();
        pulse_test();
		if (AllKey() == 0)		//调用判有无闭合键函数
		{
			DIR();
			DIR();
			continue;
		}
		DIR();
		DIR();
		if (AllKey() == 0)		//调用判有无闭合键函数
			continue;
		i = 0xfe;
		keyResult = 0;
		do
		{
			if(!paused)pulse_stopper_test();
            update_stopwatch_buffer();
            pulse_test();
			outportb(I8255_PB, i);
			j = ~inportb(I8255_PC);
			if (j & 3)
			{
				bNoKey = 0;
				if (j & 2)				//1行有键闭合
					keyResult += 8;
			}
			else						//没有键按下
			{
				keyResult++;			//列计数器加1
				i = ((i << 1) | 1);
			}
		}while(bNoKey  && (i != 0xff));
	}
	if (!bNoKey)
	{
		while(AllKey())		//判断释放否
		{
			if(!paused)pulse_stopper_test();
        	update_stopwatch_buffer();
            pulse_test();
			DIR();
		}
	}
	return keyResult;
}

void Get_ISD_Addr()
{
    ISD_addre = (key() % 10) * 10;
    update_voice_buffer();
    ISD_addre += (key() % 10);
    ISD_addre %= 26;
    update_voice_buffer();
    DIR();
}

/*
A 回主界面显示时间 代码设定初始时间
B 录音功能，数字按键实现地址选择， b录制，c播放
C 秒表 c停止计时/开始计时， d清零
d 时区转换， 按d切换时区
e 语音提醒 数字按键设定时间， e设定
f 时钟加速6000倍，再按取消
*/

// 分支函数
void Voice_Branch()
{
    u8 flag = 0;
    init_buffer();
    initISD();
    while(1) {
        pulse_test();
        update_voice_buffer();
        flag = key();
        switch (flag)
        {
        case (u8)10:
            return;
        case (u8)11:
            Get_ISD_Addr();
            KEY_REC(voice[ISD_addre], during[ISD_addre]);
            break;
        case (u8)12:
            Get_ISD_Addr();
            KEY_PLAY(voice[ISD_addre], during[ISD_addre]);
            break;
        default:
            break;
        }
    }
}

void Stopper_Branch()
{
    u8 paused, flag = 1;
    init_buffer();
    init_stopper();
    while(1) {
        pulse_test();
        if(paused) pulse_stopper_test();
        flag = stopper_key(paused);
        switch (flag)
        {
        case (u8)10:
            return;
        case (u8)12:
        	paused=!paused;
            break;
        case (u8)13:
            init_stopper();
            break;
        default:
            update_stopwatch_buffer();
            DIR();
            break;
        }
    }
}

void Reminder_Branch()
{
    u8 flag = 0;
    init_buffer();
    while(1) {
        pulse_test();
        update_reminder_buffer();
        flag = reminder_key();
        switch (flag)
        {
        case (u8)10:
            return;
        case (u8)14:
            reminder_hour = (reminder_key() % 10) * 10;
            update_reminder_buffer();
            reminder_hour += (reminder_key() % 10);
            update_reminder_buffer();
            reminder_minute = (reminder_key() % 10) * 10;
            update_reminder_buffer();
            reminder_minute += (reminder_key() % 10);
            update_reminder_buffer();
            DIR();
            break;
        default:
            break;
        }
    }
}

// 更新数码管显示
// update_display();

void main() {
	u8 flag;
	
    // 初始化系统
    init_8255();
    init_buffer();

    // 设置时钟，输出100Hz的方波
    outportb( COM_ADDR, 0x35);		//计数器T0设置在模式2状态,BCD码计数
	outportb( T0_ADDR, 0);
	outportb( T0_ADDR, 0x10);		//CLK0/1000
	outportb( COM_ADDR, 0xB7);		//计数器T1为模式3状态，输出方波,BCD码计数
	outportb( T1_ADDR, 0x20);       //写入低字节
	outportb( T1_ADDR, 0x00);		//CLK2/20

    // 设置初始时间
    hours = 9; // 示例初始时间
    minutes = 0;
    seconds = 0;

    // 设置语音提醒时间
    set_reminder(12, 0); // 例如设置中午12点提醒

    // 设置定时器中断
    flag = 0;
    while (1) {
        pulse_test();
        flag = main_key();
        switch (flag)
        {
        case (u8)10:
            update_time_buffer();    
            DIR();
            /* code */
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
            break;
        }
    }
}
