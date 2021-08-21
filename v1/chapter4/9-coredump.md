# 故障排查：CoreDump配置

## 引言

为了尽可能的提高服务的可用性，在程序部署时，我们通常会打开很多的辅助功能：

- 监控报警类：帮助我们迅速发现问题快速恢复服务。如端口监控、健康检查等
- 日志类：通常在线上我们会禁用应用程序TRACE、DEBUG等级别的日志，启用INFO等级以上的日志，便于留痕和事后追溯。另外一个方面，我们会启用linux的coredump功能，一旦程序崩溃，让其把程序的堆栈、内存等信息进行转储，方便我们事后确定问题原因。
- 守护类：如monit等进程监测程序，当进程崩溃后，可以重新拉起进程，避免我们手动重启时间太长，影响用户。在实际的互联网产品服务中，当用户发现问题，到运营反馈给产品，产品再反馈给技术排查，技术找到原因让运维重启等这一串流程下来，可能已经过去1个小时了，这个时候写故障报告就不可避免。

在这里，主要介绍一下coredump如何启用，网上的教程也很多，除此之外，再介绍一下可能会遇到的坑。

## 启用coredump转储

PS：本文的方法适用于CentOS6.6 - CentOS 7.2

>  如果按照下列步骤操作无效，请先手动编辑/etc/security/limits.conf（需手动删除文件内部分内容，见coredump.sh）、/etc/sysctl.conf（可直接删除）
>

### 创建coredump.sh

注意，下面的shell脚本执行后，会把coredump文件存储在/data/imcorefile路径下面。

```bash
$ vim coredump.sh

#!/bin/bash

# Filename: coredumpshell.sh
# Description: enable coredump and format the name of core file on centos system

# enable coredump whith unlimited file-size for all users
echo -e "\n# enable coredump whith unlimited file-size for all users\n* soft core unlimited" >> /etc/security/limits.conf

# 存储路径，改了这里别忘记同时修改下面的/data/imcorefile
cd /data && mkdir imcorefile && chmod 777 imcorefile

# format the name of core file.   
# %% – 符号%
# %p – 进程号
# %u – 进程用户id
# %g – 进程用户组id
# %s – 生成core文件时收到的信号
# %t – 生成core文件的时间戳(seconds since 0:00h, 1 Jan 1970)
# %h – 主机名
# %e – 程序文件名    
# for centos7 system(update 2017.4.2 21:44)
# /data/imcorefile/：是core文件存储的路径，这里比较重要
echo -e "\nkernel.core_pattern=/data/imcorefile/core-%e-%s-%u-%g-%p-%t" >> /etc/sysctl.conf
echo -e "\nkernel.core_uses_pid = 1" >> /etc/sysctl.conf

sysctl -p /etc/sysctl.conf
```



### 永久启用core dump功能

```bash
$ chmod 777 coredump.sh
$ ./coredump.sh
# 重新打开终端
$ ulimit -a # 显示 unlimited 代表成功

core file size          (blocks, -c) unlimited  # 生成core转储文件的大小，这里是不限制
data seg size           (kbytes, -d) unlimited  
scheduling priority             (-e) 0
file size               (blocks, -f) unlimited
pending signals                 (-i) 15085
max locked memory       (kbytes, -l) 64
max memory size         (kbytes, -m) unlimited
open files                      (-n) 65536         # 最大可打开文件句柄数，约等于tcp最大连接数
pipe size            (512 bytes, -p) 8
POSIX message queues     (bytes, -q) 819200
real-time priority              (-r) 0
stack size              (kbytes, -s) 8192
cpu time               (seconds, -t) unlimited
max user processes              (-u) 131072
virtual memory          (kbytes, -v) unlimited
file locks                      (-x) unlimited
```



### 验证

```bash
$ vim test.c  // 输入如下内容

#include <stdio.h>
int main( int argc, char * argv[] ) { char a[1]; scanf( "%s", a ); return 0; }

$ gcc test.c -o test  # 编译
$ ./test              # 执行test，然后任意输入一串字符后按回车，如zhaogang.com
$ ls /data/imcorefile # 在此目录下如果生成了相应的core文件core-test-*，代表成功
```



 ### 关闭

 > core文件比较大，有些时候希望关闭这个功能，节省存储空间

```bash
$ ulimit -c 				# 查看core dump状态，0代表关闭，unlimited代表打开
$ vim /etc/profile 	# 加入如下一句话

# No core files by default
ulimit -S -c 0 > /dev/null 2>&1 

# 重新打开终端
$ ulimit -c  # 如果输出0，代表关闭成功，如果要重新启用，把上面那句话注释，重新打开终端即可
```

## 实战踩坑记录

### GDB看不到具体源代码或者显示问号

1. 可能是少了-g指令，在CMakeLists.txt增加一下：

```cmake
# -g：添加gdb调试选项。
ADD_DEFINITIONS(-g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
        -DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
        -DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++11)
```

2. 数组越界了也会导致这种问题

```bash
$ vim test.c
#include <stdio.h>
int main( int argc, char * argv[] ) { char a[1]; scanf( "%s", a ); return 0; }

$ gcc -g test.c -o test
$ ./test 
Segmentation fault (core dumped)

$ gdb test imcorefile/core-test-11-0-0-4240-1621495946
#0  0x0000000000400066 in ?? ()
#1  0x00007ffef9a48198 in ?? ()
#2  0x0000000100000000 in ?? ()
#3  0x0000000000000000 in ?? ()
```



### GDB看到的堆栈很少，感觉错误位置莫名其妙

解决方法：把程序和core文件拷贝到编译机器，使用gdb调试。

额外说一下，现在很多互联网公司都有自己的CI/CD（持续集成）平台，可以实现一键代码部署和机器分发。其本质上是通过脚本来编译程序，然后使用jenkins来进行程序的分发和部署。假设有几套环境：编译机器 -> DEV（开发） -> TEST（测试） -> PRE（预发） -> PROD（生产），这中间我们的任何代码的改动都需要经过这一串的完整流程。故在生产环境运行的程序都不是本机编译的。

所以，这个问题需要注意一下，调试core文件的时候，需要在编译机器上，否则堆栈信息可能误导你！

## 参考

- [CentOS开启coredump转储并生成core文件的配置](https://typecodes.com/linux/centoscoredumpcfgshell.html)

