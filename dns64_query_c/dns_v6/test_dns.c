
#include "dnsquery.h"
int main()
{
    struct socket_ipinfo_t *test_ipv6=(struct socket_ipinfo_t *)malloc(sizeof(socket_ipinfo_t));
    char host[64]="192.168.0.2";
    char dnsserver1[64]="2001:67c:27e4:15::64";
    char dnsserver2[64]="2001:67c:27e4::60";
    char v6_ip[64] = {0};
    //socket_gethostbyname(host, test_ipv6,2000/*ms*/,dnsserver1);
    multi_pthread_getaddrinfo(host, test_ipv6,3000/*ms*/);
    for(int i=0;i<(test_ipv6->size);i++)
    {
        inet_ntop(AF_INET6, &test_ipv6->v6_addr[i].s6_addr, v6_ip, sizeof(v6_ip));
        printf("%s\n",v6_ip);
    }
    free(test_ipv6);

    return 0;
}
