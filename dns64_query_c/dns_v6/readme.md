###概述
####1.对外层接口:getaddrinfo_v6()
 *                   1.host输入为ipv4地址时，申请解析ipv4only.arpa的ip地址，判断前缀-提取前缀合成
 *                   2.host输入为域名时，首先通过自己实现的getaddrinfo，如果返回失败，再调用系统api getaddrinfo 
 *               内部:th_gethostbyname()，解析地址
 *                   1.现在只知道3个dns64服务器的地址，开3个线程解析地址，某线程首先获取到该域名下所对应的IP地址列表即返回
