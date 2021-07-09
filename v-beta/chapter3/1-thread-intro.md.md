# 多线程概念

以下内容来自《Linux-UNIX系统编程手册（上、下册）》

## 概述

一个进程可以包含多个线程。同一程序中的所有线程均会独立执行相同程序， 且共享同一份全局内存区域，其中包括初始化数据段(initialized data)、未初始化数据段(uninitialized data)，以及堆内存段(heap segment)。