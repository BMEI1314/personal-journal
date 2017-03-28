mars_ipv6:
              ios9.2版本以上和其他平台版本,在NAT64网络下都使用getaddrinfo来合成ipv6地址.
             不同的是:ios9.2版本直接getaddrinfo自己想要合成的ipv4,返回已合成好的ipv6地址.
                而其他平台getaddrinfo网址ipv4only.arpa,对返回的ipv6解析,找到前缀,然后在合成.
                我改了一下开源代码,测试一下
