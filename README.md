# Clock
西电微机原理课程设计：数字时钟和自动报时系统设计。

## 写在前面
题目密码锁和音乐比较简单

试运行：创建项目，把code.c内容拷到项目文件夹中，按连线表/连线图连好线，编译连接，装载DOB文件，全速运行即可

尽量在原基础上改一改

## 实验环境
星研集成开发环境

## 系统功能
1.数码管上实时显示时、分、秒信息

2.整点报时

3.固定时间语音提醒

4.秒表计时

5.时区转换

6.加速6000倍显示

## 系统执行逻辑
F5区16位按键:

a 按a回主界面显示时间(在所有分支的主界面)（最高位显示A）

b 进录音功能分支 数字按键实现地址选择，(再按b后输入十进制地址录制对应的音频)(再按c后输入十进制地址后播放地址对应的音频）（从进入录音功能分支后到开始某一功能要按三次）(无显示/无效显示)

c 进入秒表分支  按c停止计时/开始计时   按d清零秒表(可能为00.01s)(显示计时结果00.00--99.99，最高位显示C)

d 时区转换分支 按d切换时区

e 语音提醒设置分支 按e开始设定 数字按键设定时间 (显示设定的提醒时间)（最高位显示E）

f 时钟加速6000倍 再按f取消

注意：由于8255只有ABC三个口,录音功能启动时不能启动显示地址,此时地址显示为乱码但是时钟仍然运行(注意A口复用了！！！)

## 文件说明
1. code.c 微机系统C语言源代码
   
2. libcode.c 当时验收时的程序(少了点注释，多了些没用的函数)，100%可运行

3. corecode.c 只保留核心部分的代码，删除掉复用函数和例程函数，便于理解

4. 硬件连接图.jpg  一定要对着这张图连线，本选题连线比较复杂(图上的8253CS要连A3的CS1)

5. 完整硬件连线.jpg  实际连接图； 无总线硬件连线.jpg  去掉遮挡方便查看细节

6. 程序执行流程图.pptx 交报告用

## 连线表
A3: A0、A1、CS1 <--> D3: A0、A1、CS 

A3: A0、A1、CS2 <--> C4: A0、A1、CS 

C4: CLK0 <--> B2: 1M

C4: OUT0 <--> C4: CLK2

C4: OUT2 <--> D3: PC2

C4: GATE <--> C1: VCC

D3: PC4、PC5、PC6 <--> B1: REC、PLAYE、PLAYL

D3: PC0、PC1 <--> F5: KL1、KL2

总线部分(别在连线的时候把线扭了，不然有惊喜)

D3: JP20、B、C <--> F5: A、B、C

D3: JP23 <--> B1: 就一个接口别扭就行

## 一些建议
如果时钟不动，可以先看看OUT2的50Hz的信号有没有正确生成

可以把加速分支改成设定时间





