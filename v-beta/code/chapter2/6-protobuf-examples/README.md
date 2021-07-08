# prtobobuf examples

## 编译protobuf 

> 只需要编译一次

```bash
$ chmod 777 make-protobuf.sh
$ ./make-protobuf.sh        # 编译protobuf
$ ls /usr/local/protobuf3   # 编译目标在这下面
```

## 生成proto c++文件

```bash
$ cd ./pb
$ sh build.sh
$ # 此时，多出4个文件
```

## build

使用clion打开CMakeList.txt，直接build即可。