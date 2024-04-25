# 使用流程
### 1、src文件夹下文件添加到你的项目
### 2、在移植好lwip的lwipopts.h文件中添加
~~~
#define LWIP_RAW 1
~~~
### 3、在需要ping的位置编写回调函数，即可完成，示例如下：
~~~
void pingResult(int state,uint8_t ping_seq_num,uint32_t time){
    int pingState = state;
    uint8_t pingSeqNum = ping_seq_num;
    uint32_t pingTime = time;
}
ip_addr_t det;
IP4_ADDR(&det,192,168,1,8);
ping_init(pingResult,NULL,det);
~~~
目前同时只能有一个ip被ping，请注意；每次调用ping_init时会关闭上次的ping
代码很杂，后面优化（叠盾）
