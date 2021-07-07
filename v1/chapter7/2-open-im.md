# OpenIM

- github: [Github](https://github.com/OpenIMSDK/Open-IM-Server)
- star: 40（2021.06.08）

## 部署

### Etcd

参考官网[Quickstart](https://etcd.io/docs/v3.4/quickstart/)，建立启动脚本：

```bash
$ vim start-etcd.sh
#!/bin/sh
ETCD_BIN=/data/open-im/test-etcd
echo "start $ETCD_BIN/etcd"
cd $ETCD_BIN/
# 后台运行,2>&1 &作用见：https://blog.csdn.net/qq_27870421/article/details/90753948
nohup ./etcd > etcd.out 2>&1 &
ps aux|grep etcd

# 使用下面的命令测试效果
$./etcdctl put test sa123456
$./etcdctl get test
```

### Mongodb

参考[官网](https://www.mongodb.com/try/download/community) 和 [这篇文章](https://www.cnblogs.com/d0usr/p/12583162.html)

```bash
$ wget https://repo.mongodb.org/yum/redhat/7/mongodb-org/4.4/x86_64/RPMS/mongodb-org-server-4.4.6-1.el7.x86_64.rpm
$ rpm -ivh mongodb-org-server-4.4.6-1.el7.x86_64.rpm
$ vim /etc/mongod.conf
bindIp: 127.0.0.1 -> bindIp: 0.0.0.0
$ systemctl restart mongod # 启动mongodb
```

### Kafka集群

参考官网[Quickstart](http://kafka.apache.org/quickstart)（java -version查看jdk是1.8以上，CentOS7默认1.8）

#### Zookeeper集群

1. 创建3个目录

```bash
# 程序
$ mkdir -p /data/open-im/zookeeper/server1
$ mkdir -p /data/open-im/zookeeper/server2
$ mkdir -p /data/open-im/zookeeper/server3
# 数据存储目录和log目录
$ mkdir -p /data/open-im/zookeeper/server1/data && mkdir -p /data/open-im/zookeeper/server1/log
$ mkdir -p /data/open-im/zookeeper/server2/data && mkdir -p /data/open-im/zookeeper/server2/log
$ mkdir -p /data/open-im/zookeeper/server3/data && mkdir -p /data/open-im/zookeeper/server3/log
# 权限
$ sudo chown 777 /data/open-im/zookeeper/*
```

2. 下载ZK，拷贝到3个目录

```bash
# 官网：https://zookeeper.apache.org/releases.html
$ wget https://mirrors.bfsu.edu.cn/apache/zookeeper/zookeeper-3.7.0/apache-zookeeper-3.7.0-bin.tar.gz
$ tar -zxvf apache-zookeeper-3.7.0-bin.tar.gz
$ cp -rf apache-zookeeper-3.7.0-bin /data/open-im/zookeeper/server1
$ cp -rf apache-zookeeper-3.7.0-bin /data/open-im/zookeeper/server2
$ cp -rf apache-zookeeper-3.7.0-bin /data/open-im/zookeeper/server3
```

3. 修改配置文件
   - server1

```bash
$ cd /data/open-im/zookeeper/server1/apache-zookeeper-3.7.0-bin/conf
$ cp -rf zoo_sample.cfg zoo.cfg
$ vim zoo.cfg
tickTime=2000
initLimit=10
syncLimit=5
# 注意这里，数字要变
clientPort=2181
# 注意这里，数字要变
dataDir=/data/open-im/zookeeper/server1/data
dataLogDir=/data/open-im/zookeeper/server1/log
server.1=127.0.0.1:2888:3888
server.2=127.0.0.1:2889:3889
server.3=127.0.0.1:2890:3890

$ echo "1" > /data/open-im/zookeeper/server1/data/myid
```

   - server2的操作和上面一样

```bash
clientPort=2182
dataDir=/data/open-im/zookeeper/server2/data
dataLogDir=/data/open-im/zookeeper/server2/log

$ echo "2" > /data/open-im/zookeeper/server2/data/myid
```

   - server3的操作和上面一样

```bash
clientPort=2183
dataDir=/data/open-im/zookeeper/server3/data
dataLogDir=/data/open-im/zookeeper/server3/log

$ echo "3" > /data/open-im/zookeeper/server3/data/myid
```

4. 启动

```bash
$ /data/open-im/zookeeper/server1/apache-zookeeper-3.7.0-bin/bin/zkServer.sh start
$ /data/open-im/zookeeper/server2/apache-zookeeper-3.7.0-bin/bin/zkServer.sh start
$ /data/open-im/zookeeper/server3/apache-zookeeper-3.7.0-bin/bin/zkServer.sh start

# 3个都启动后，通过status命令查看运行状态，输出Mode: follower则代表成功
$ /data/open-im/zookeeper/server3/apache-zookeeper-3.7.0-bin/bin/zkServer.sh status
ZooKeeper JMX enabled by default
Using config: /data/open-im/zookeeper/server1/apache-zookeeper-3.7.0/bin/../conf/zoo.cfg
Starting zookeeper ... STARTED
Mode: follower
```

#### Kafka集群

1. 下载

```bash
$ mkdir -p /data/open-im/kafka
$ cd /data/open-im/kafka
$ wget https://mirrors.bfsu.edu.cn/apache/kafka/2.8.0/kafka_2.13-2.8.0.tgz
$ tar -zxvf kafka_2.13-2.8.0.tgz
```

2. 复制3份配置文件

```bash
$ cd /data/open-im/kafka/kafka_2.13-2.8.0/config
$ cp server.properties server1.properties
$ cp server.properties server2.properties
$ cp server.properties server3.properties
```

3. 修改配置文件
   - server1.properties

```bash
$ vim server1.properties
# 改以下配置，其他默认即可
broker.id=0
log.dirs =/data/open-im/kafka/log1
listeners=PLAINTEXT://:9092
# advertised.listeners作用参考：https://www.cnblogs.com/gyyyl/p/13446673.html
advertised.listeners=PLAINTEXT://10.0.56.153:9092
zookeeper.connect=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
```

 - server2.properties

```bash
broker.id=1
log.dirs =/data/open-im/kafka/log2
listeners=PLAINTEXT://:9093
advertised.listeners=PLAINTEXT://10.0.56.153:9093
zookeeper.connect=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
```

- server3.properties：和上面保持一致

```bash
broker.id=2
log.dirs =/data/open-im/kafka/log3
listeners=PLAINTEXT://:9094
advertised.listeners=PLAINTEXT://10.0.56.153:9094
zookeeper.connect=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
```

- consumer.properties

```bash
$ vim consumer.properties
zookeeper.connect=127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183
group.id=logGroup
```

4. 启动，可以新建1个start-kafka.sh脚本

```bash
#!/bin/sh

KAFKA_BIN=/data/open-im/kafka/kafka_2.13-2.8.0
cd $KAFKA_BIN
echo "start kafka"
# Start the Kafka broker service
nohup bin/kafka-server-start.sh config/server1.properties > kafka1.out 2>&1 &
nohup bin/kafka-server-start.sh config/server2.properties > kafka2.out 2>&1 &
nohup bin/kafka-server-start.sh config/server3.properties > kafka3.out 2>&1 &

ps aux|grep kafka
```

5. 验证kafka集群

```bash
# 创建一个测试topic
$ bin/kafka-topics.sh --create --zookeeper 127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183 --replication-factor 2 --partitions 3 --topic demo_topics
WARNING: Due to limitations in metric names, topics with a period ('.') or underscore ('_') could collide. To avoid issues it is best to use either, but not both.
Created topic demo_topics.

# 列出所有topic
$ bin/kafka-topics.sh --list --zookeeper 127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183

# 查看topic详情
$ bin/kafka-topics.sh --describe --zookeeper 127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183 demo_topics
```





参考：

- [一台机器上安装zookeeper+kafka集群](https://blog.csdn.net/u013244038/article/details/53938997?utm_source=blogxgwz9)

### Redis

```bash
$ yum install redis
$ vim /etc/redis.conf
bind 127.0.0.1 -> bind 0.0.0.0

$ systemctl restart redis
```

### MySQL

```bash
$ yum install mariadb.x86_64
$ systemctl restart mariadb
```



mistake upon solve arrange two vital ladder fortune laugh desert bid interest

21f00170929dc9d2

test1



f6cc31ec79c3b6cb

test2



# 参考

- [kafka集群部署与验证](https://blog.51cto.com/u_13231454/2457088)

