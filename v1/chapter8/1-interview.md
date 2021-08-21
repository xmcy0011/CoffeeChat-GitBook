# 我的面试经历

## 莉莉丝游戏-Golang后端开发-中台部门

开场白，双方自我介绍，对方介绍这个岗位情况，erlang语言，全部转向go。

1. 聊项目，介绍IM架构以及使用Go写的服务简介
2. 动态扩容相关问题：平常30万，现在100万，你们现在是如何应对的？
3. 如果让你调整架构实现动态扩容，你会怎么做？回答了kafka，是一种，应该还要使用docker和k8s以及微服务框架等
4. 如果使用kafka，你是如何确保消息有序？回答了redis，好像没回答到那个点
5. 消息发送失败的问题，如何做到消息不丢？回答持久化存储、拉消息和ACK
6. 发送失败，到客户端的ack丢失，如何做到不重复？回答消息使用guid
7. 1道go的题目，没做对。
```go
func test1()bool{
    a := false
    defer func(){
        a = true
    }
    return a
}
func test2()(a bool){
    a = false
    defer func(){
        a = true
    }
    return a
}
```
8. GoRoutine和调度模型
9. GC算法

时间：2021年08月  
结果：没有复试，理由是胜任力不足  
复盘：可惜了朋友内推，复盘来看，还是准备不足，go有小半年没有高频使用了。我擅长的IM对方也已实现且是微服务架构，自然没有优势。  
措施：  
1. 需要恶补go微服务开发技能和除了IM之外的项目经验。确定了2个目标：[七天用Go从零实现系列](https://geektutu.com/post/gee-day7.html)、[Go进阶训练营: i6cb](https://pan.baidu.com/s/1-b9uqwNO1kwFunu4BEssFQ)
2. 需要学会并使用docker和k8s，不会减分很多。接下来1个月要狂刷《Docker技术入门与实战  第3版》、《Kubernetes进阶实战》等书。
3. go的笔试题没刷，导致题目做错，又减分。下次面试前，这个链接100天的go笔试题要刷完：[go语言面试题](http://www.topgoer.cn/docs/gomianshiti/mian1)
4. 简历还需要不断优化，才优化了5版，参考《InterviewGuide第四版By阿秀》，至少要达到20版。

收获：  
- 唯一让人高兴的是，不是全日制统招本科也有进大厂的希望，至少给我面试机会了，是自己没有把握住。