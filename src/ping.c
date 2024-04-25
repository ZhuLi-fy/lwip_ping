/** 
 * This is an example of a "ping" sender (with raw API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "ping.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000 
#endif

//ping请求发送间隔
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

//设置自定义icmp标识符
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

//icmp数据长度
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif


static ping_result resultFunc = NULL;//ping回调函数
static u16_t ping_seq_num;//icmp首部序号字段
static u32_t ping_time;//ping往返时间
static struct raw_pcb *ping_pcb = NULL;//ping的控制块
static ip_addr_t ping_dst;//ping目的ip
static uint8_t *stopCondition = NULL;//ping停止条件

static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);//类型
  ICMPH_CODE_SET(iecho, 0);//代码
  iecho->chksum = 0;
  iecho->id     = PING_ID;//标识符
  iecho->seqno  = htons(++ping_seq_num);//序号

  for(i = 0; i < data_len; i++) {//填充数据
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }
  
  iecho->chksum = inet_chksum(iecho, len);//计算校验和
}

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))))
  {
      iecho = (struct icmp_echo_hdr *)((u8_t*)p->payload + PBUF_IP_HLEN);
      if ((iecho->type == ICMP_ER) && (iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) 
			{
          LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
          ip_addr_debug_print(PING_DEBUG, addr);
          LWIP_DEBUGF( PING_DEBUG, (" time=%"U32_F" ms\n", (sys_now()-ping_time)));
					//计算往返时间
          if (resultFunc!=NULL)
          {
            resultFunc(1,ping_seq_num,sys_now()-ping_time);
          }
          pbuf_free(p);
          return 1; //使用了包
      }
  }
  return 0; //不处理该包
}

static void ping_send(struct raw_pcb *raw, ip_addr_t *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
	//回送请求报文大小
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);
 
  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return;
  }
	//一个pbuf
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;
    //填写icmp首部
    ping_prepare_echo(iecho, (u16_t)ping_size);
    //发送数据包，最终是调用ip_output_if
    raw_sendto(raw, p, addr);
    ping_time = sys_now();//记下发生时间

	LWIP_DEBUGF(PING_DEBUG, ("ping:[%"U32_F"] send ", ping_seq_num));
	ip_addr_debug_print(PING_DEBUG, addr);
  LWIP_DEBUGF( PING_DEBUG, ("\n"));
  }
  pbuf_free(p);//释放pbuf
}

//内核超时后调用一次函数
static void ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;
  
  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  resultFunc(-1,0,0);
  if(stopCondition==NULL||stopCondition<=0)
  {
    ping_send(pcb, &ping_dst);//发送回送请求
    sys_timeout(PING_DELAY, ping_timeout, pcb);//再次注册超时事件
  }
  else{
    raw_remove(ping_pcb);//删除
  }
}

static void ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);//申请一个icmp类型pcb
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, (raw_recv_fn)ping_recv, NULL);//注册recv回调函数为ping_recv
  raw_bind(ping_pcb, IP_ADDR_ANY);//绑定ip为0(本机任一端口)
	//设置超时事件PING_DELAY后ping_timeout被内核调用，回调参数为ping_pcb
  sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

void ping_init(ping_result re,uint8_t *stopCondition,ip_addr_t ip)//ping初始化
{
  if(ping_pcb!=NULL){
    raw_remove(ping_pcb);
  }
  resultFunc = re;
  ping_dst = ip;
  IP4_ADDR(&ping_dst, 192,168,10,10);//ping ip
  ping_raw_init();
}

#endif /* LWIP_RAW */

