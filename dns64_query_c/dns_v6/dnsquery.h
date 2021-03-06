#ifndef DNSQUERY_H_
#define DNSQUERY_H_
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
//#include <algorithm>
#include <errno.h>
/**
 *函数名:    socket_gethostbyname
 *功能: 输入域名，可得到该域名下所对应的IP地址列表
 *输入:       _host：输入的要查询的主机域名
 *输入:       _timeout：设置查询超时时间，单位为毫秒
 *输入:       _dnsserver 指定的dns服务器的IP
 *输出:		 _ipinfo为要输出的ip信息结构体
 *返回值:		  当返回-1表示查询失败，当返回0则表示查询成功
 *
 */
#define SOCKET_MAX_IP_COUNT (20)
struct socket_ipinfo_t
{
    int  size;
//    int  cost;
//    struct  in_addr dns;
    struct  in6_addr v6_addr[20];
}socket_ipinfo_t;
int socket_gethostbyname(char* _host, struct socket_ipinfo_t* _ipinfo, int _timeout /*ms*/, const char* _dnsserver);
int getaddrinfo_v6(const char* _host, struct socket_ipinfo_t* _ipinfo, int _timeout /*ms*/);
#endif 


