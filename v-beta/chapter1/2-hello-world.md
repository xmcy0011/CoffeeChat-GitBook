# hello world

当我们安装好go环境之后，我们先不急着使用Goland来写代码，按照国际惯例，先来写一个hello world入门：
```bash
$ vim hello.go
package main
import "fmt"

func main()  {
	fmt.Println("hello world")
}
```

然后执行 `go run hello.go`输出：

```bash
hello world
```

# 代码结构

下面我们来简单解释一下hello.go文件里面的内容：

- package main：package关键字申明一个包名，包的概念类似java中的namespace命名空间和jar包（或则C++中的命名空间和动态库），`通常一个包对应一个同名的文件夹`，但是`main`包除外，它是一个特殊的包，可以在任何文件夹下，但是该文件夹下所有的go文件，都必须是package main，否则编译错误。并且该文件夹中，必须有一个go文件拥有一个名为main的函数，作为程序入口。
- import "fmt"：引入一个包，fmt是系统内置的包，可以在 [官网文档](https://golang.google.cn/pkg/) 中找到，概念类似C++的


# 导学

为了更深入的学习go语言，建议读者先通过系统的看书来掌握go的语法和建立知识体系，以下是推荐的几本书，读者可自行取舍。
1.《Go语言编程》
这是七牛团队写的书，通过和C++的对比，来讲述go带来的革命，适合有一定C++基础的观看。

2.《Go语言实战》
通过一系列例子，深入介绍go语言的各个功能，适合有一定开发经验的人入门。它的特色是短小精悍，跑完书中全部的例子，达到一个星期入门go语言并不是一件不可能的事情。

3.《Go学习笔记 第四版》
上半部分主要介绍go语言的语法，可以当作手册使用。下半部分是源码解读，这本书的定位更像是一个老手的工具书，遗忘go的语法后，通过该书来快速温习go的语法。