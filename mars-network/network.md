###network
####1.getdnssvraddrs.cc <获取Dns的地址>
对于安卓编译环境
```
调用__system_property_get获取本机的net.dns
将其加到socket_address结构的容器中

```
对于苹果编译环境
```
调用res_getservers获取本机的dns
将其加到socket_address结构的容器中

```
对于win32编译环境
```
GetNetworkParams接口

```
####2.getgateway.c<获取网关>
* 说明:没有一种很简单的方法去获取默认网关.因此,下面有三种不同的方法去实现它.
对于linux是去分解 /proc/net/route
对于BSD系统通过sysctl可以去获取一些信息
另外,一些系统提供网关信息通过PF_ROUTE
 
* 具体的实现
 - 对于linux编译环境
>   读取/proc/net/route文件
 - 对于苹果编译环境
>  int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET,
       NET_RT_FLAGS, RTF_GATEWAY};
>   sysctl(mib, sizeof(mib)/sizeof(int), buf, &l, 0, 0)
  - 对于bsd编译环境
  > socket(PF_ROUTE, SOCK_RAW, 0)
  - 对于win32编译环境
  >   char networkCardsPath[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    char interfacesPath[] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
    
####3.getifaddrs.cc<获取本机ipv4地址,ipv6地址,ipv4_filter,ipv6_filter>
> 调用ifaddrs.h库来获取本机的ip地址,ifaddr结构等等很多东西

####4.netinfo_util.cc
  将前面的函数功能聚合起来
  包含: 
  - 1.网络信息
   >  连接网络的状态:KnoNet,看kwifi,kmobile,kotherNet
     > 本地支持的网络协议栈

  -  2.网络配置信息
  >  默认网关,dns server,路由表
     
-  3.网卡信息          
