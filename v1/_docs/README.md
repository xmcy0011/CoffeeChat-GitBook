# Gitbook

## Quick Start

1. depends
```bash
$ brew install node.js # 安装nodejs
$ node -v 
v15.12.0
$ npm -v # npm 是node自带的包管理工具
7.6.3
$ npm config set registry https://registry.npm.taobao.org # 使用淘宝镜像，解决安装npm包慢的问题
$ npm config get registry # 查看是否生效
$ npm install gitbook-cli -g # 安装gitbook命令行，-g 全局安装
$ gitbook --version # 此时可以使用gitbook命令了
```

2. install & run
```bash
$ cd /CoffeeChat-GitBook/v1
$ gitbook install # 安装插件依赖
$ gitbook serve   # 运行
Starting server ...
Serving book on http://localhost:4000
```

## 插件调优

### page-treeview页面内目录，去掉Copyright

去掉：
```markdown
TreeviewCopyright © aleen42 all right reserved, powered by aleen42
```

```bash
$ cd CoffeeChat-GitBook/v1/node_modules/gitbook-plugin-page-treeview/lib
$ vim index.js
$ # 删掉copyRight
> return renderContent ? `<div class="treeview__container">${$copyRight renderContent}</div>` : '';
--
< return renderContent ? `<div class="treeview__container">${renderContent}</div>` : '';
```

重新**gitbook serve**。
