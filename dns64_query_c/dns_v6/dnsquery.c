/* 
 * ===================================================================================== 
 * 
 *       Filename:  dnsquery.c
 * 
 *    Description: 对外层接口:getaddrinfo_v6()
 *                   1.host输入为ipv4地址时，申请解析ipv4only.arpa的ip地址，判断前缀-提取前缀合成
 *                   2.host输入为域名时，首先通过自己实现的getaddrinfo，如果返回失败，再调用系统api getaddrinfo    
 *                 内部:th_gethostbyname()，解析地址
 *                   1.现在只知道3个dns64服务器的地址，开3个线程解析地址，某线程首先获取到该域名下所对应的IP地址列表即返回
 *
 * 
 *        Version:  1.0 
 *        Created:  2017/04.17
 *       Compiler:  gcc 
 *       Author:    ZQ
 * 
 * ===================================================================================== 
 */ 

#include "dnsquery.h"

#define TRAFFIC_LIMIT_RET_CODE (INT_MIN)
#define DNS_PORT (53)
#define DEFAULT_TIMEOUT (3000)
#define NAME_SVR ("nameserver")
#define NAME_SVR_LEN (40)

// Type field of Query and Answer
#define A         1       /* host address */
#define NS        2       /* authoritative server */
#define CNAME     5       /* canonical name */
#define SOA       6       /* start of authority zone */
#define PTR       12      /* domain name pointer */
#define MX        15      /* mail routing information */
#define AAAA      0x1c
#define min(a,b) ((a)<(b))?(a):(b)
#define    false    0
#define     true   1
#define   dbg   printf
typedef unsigned char   uint8_t; 
// DNS header structure
#pragma pack(push, 1)
struct DNS_HEADER {
    unsigned    short id;           // identification number

    unsigned    char rd     : 1;    // recursion desired
    unsigned    char tc     : 1;    // truncated message
    unsigned    char aa     : 1;    // authoritive answer
    unsigned    char opcode : 4;    // purpose of message
    unsigned    char qr     : 1;    // query/response flag

    unsigned    char rcode  : 4;    // response code
    unsigned    char cd     : 1;    // checking disabled
    unsigned    char ad     : 1;    // authenticated data
    unsigned    char z      : 1;    // its z! reserved
    unsigned    char ra     : 1;    // recursion available

    unsigned    short q_count;      // number of question entries
    unsigned    short ans_count;    // number of answer entries
    unsigned    short auth_count;   // number of authority entries
    unsigned    short add_count;    // number of resource entries
};

static pthread_mutex_t dnsquery_mutex,dnsquery_mutex1;
//static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int DNS64SERVERNUM =3;
static  int dnsquery_failcount=0;
struct multi_dns
{
  const char* _host;
  struct socket_ipinfo_t* _ipinfo;
  int _timeout;
   const char *_dnsserver;
};
// Constant sized fields of query structure
struct QUESTION {
    unsigned short qtype;
    unsigned short qclass;
};


// Constant sized fields of the resource record structure
struct  R_DATA {
    unsigned short type;
    unsigned short _class;
    unsigned int   ttl;
    unsigned short data_len;
};
#pragma pack(pop)

// Pointers to resource record contents
struct RES_RECORD {
    unsigned char*  name;
    struct R_DATA*  resource;
    unsigned char*  rdata;
};

// Structure of a Query
typedef struct {
    unsigned char*       name;
    struct QUESTION*     ques;
} QUERY;
static const uint8_t kWellKnownV4Addr1[4] = {192, 0, 0, 170};
static const uint8_t kWellKnownV4Addr2[4] = {192, 0, 0, 171};
static const uint8_t kOurDefineV4Addr[4] = {192, 0, 2, 1};
//根据rfc6052，第64-72位必需为0;
//insert 0 after first byte 
static const uint8_t kWellKnownV4Addr1_index1[5] = {192, 0, 0, 0, 170};
static const uint8_t kWellKnownV4Addr2_index1[5] = {192, 0, 0, 0, 171};
static const uint8_t kOurDefineV4Addr_index1[5] = {192, 0, 0, 2, 1};

//insert 0 after second byte
static const uint8_t kWellKnownV4Addr1_index2[5] = {192, 0, 0, 0, 170};
static const uint8_t kWellKnownV4Addr2_index2[5] = {192, 0, 0, 0, 171};
static const uint8_t kOurDefineV4Addr_index2[5] = {192, 0, 0, 2, 1};

//insert 0 after third byte
static const uint8_t kWellKnownV4Addr1_index3[5] = {192, 0, 0, 0, 170};
static const uint8_t kWellKnownV4Addr2_index3[5] = {192, 0, 0, 0, 171};
static const uint8_t kOurDefineV4Addr_index3[5] = {192, 0, 2, 0, 1};

//函数原型声明
static int            isValidIpv4Address(char* _ipaddress);
static void           ChangetoDnsNameFormat(unsigned char*,char *);
static unsigned char* ReadName(unsigned char*, unsigned char*, int*);
static void           PrepareDnsQueryPacket(unsigned char* _buf, struct DNS_HEADER* _dns, unsigned char* _qname, char * _host);
static void           ReadRecvAnswer(unsigned char* _buf, struct DNS_HEADER* _dns, unsigned char* _reader, struct RES_RECORD* _answers);
static int            RecvWithinTime(int _fd, char* _buf, size_t _buf_n, struct sockaddr* _addr, socklen_t* _len, unsigned int _sec, unsigned _usec);
static int            SendWithinTime(int _fd, char* _buf, size_t _buf_n, struct sockaddr* _addr, socklen_t* _len, unsigned int _sec, unsigned _usec);
static void           FreeAll(struct RES_RECORD* _answers);
static int            GetSuffixZeroCount(uint8_t* _buf, int _buf_len);
static int            IsNat64AddrValid(struct in6_addr* _replaced_nat64_addr);
static void           ReplaceNat64WithV4IP(struct in6_addr* _replaced_nat64_addr,struct in_addr* _v4_addr);

static int IsNat64AddrValid(struct in6_addr* _replaced_nat64_addr) {
	int  suffix_zero_count = GetSuffixZeroCount((uint8_t*)_replaced_nat64_addr, sizeof(struct in6_addr));
	int is_valid = false;

	switch(suffix_zero_count) {
		case 3:
			//Pref64::/64
			
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+9, kWellKnownV4Addr1, 4)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+9, kWellKnownV4Addr2, 4)) {
					is_valid = true;
				}
			
			break;
		case 4:
			//Pref64::/56
			
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+7, kWellKnownV4Addr1_index1, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+7, kWellKnownV4Addr2_index1, 5)) {
					is_valid = true;
				}
			
			break;
		case 5:
			//Pref64::/48
			
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+6, kWellKnownV4Addr1_index2, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+6, kWellKnownV4Addr2_index2, 5)) {
					is_valid = true;
				}
			
			break;
		case 6:
			//Pref64::/40
			
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+5, kWellKnownV4Addr1_index3, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+5, kWellKnownV4Addr2_index3, 5)) {
					is_valid = true;
				}
			
			break;
		case 8: //7bytes suffix and 1 bytes u(RFC6052)
			//Pref64::/32
			
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+4, kWellKnownV4Addr1, 4)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+4, kWellKnownV4Addr2, 4)) {
					is_valid = true;
				}
			
			break;
		case 0:
			//Pref64::/96
			if ((0==memcmp(((uint8_t*)_replaced_nat64_addr)+12, kWellKnownV4Addr1, 4))||( 0==memcmp(((uint8_t*)_replaced_nat64_addr)+12, kWellKnownV4Addr2, 4))) {
					is_valid = true;
				}
			
			break;
		
			
	}
	return true;
}
static void ReplaceNat64WithV4IP(struct in6_addr* _replaced_nat64_addr, struct in_addr* _v4_addr) {
	int  suffix_zero_count = GetSuffixZeroCount((uint8_t*)_replaced_nat64_addr, sizeof(struct in6_addr));
	uint8_t zero = (uint8_t)0;

	switch(suffix_zero_count) {
		case 3:
			//Pref64::/64
			memcpy(((uint8_t*)_replaced_nat64_addr)+9, (uint8_t*)_v4_addr, 4);
			break;
		case 4:
			//Pref64::/56
			memcpy(((uint8_t*)_replaced_nat64_addr)+7, (uint8_t*)_v4_addr, 1);
			memcpy(((uint8_t*)_replaced_nat64_addr)+8, &zero, 1);
			memcpy(((uint8_t*)_replaced_nat64_addr)+9, ((uint8_t*)_v4_addr)+1, 3);

			break;
		case 5:
			//Pref64::/48
			memcpy(((uint8_t*)_replaced_nat64_addr)+6, (uint8_t*)_v4_addr, 2);
			memcpy(((uint8_t*)_replaced_nat64_addr)+8, &zero, 1);
			memcpy(((uint8_t*)_replaced_nat64_addr)+9, ((uint8_t*)_v4_addr)+2, 2);
			break;
		case 6:
			//Pref64::/40
			memcpy(((uint8_t*)_replaced_nat64_addr)+5, (uint8_t*)_v4_addr, 3);
			memcpy(((uint8_t*)_replaced_nat64_addr)+8, &zero, 1);
			memcpy(((uint8_t*)_replaced_nat64_addr)+9, ((uint8_t*)_v4_addr)+3, 1);
			break;
		case 8:
			//Pref64::/32
			memcpy(((uint8_t*)_replaced_nat64_addr)+4, (uint8_t*)_v4_addr, 4);
			break;
		case 0:
			//Pref64::/96
			memcpy(((uint8_t*)_replaced_nat64_addr)+12, (uint8_t*)_v4_addr, 4);
			break;
		default:
			memcpy(((uint8_t*)_replaced_nat64_addr)+12, (uint8_t*)_v4_addr, 4);
			
	}
}

static int GetSuffixZeroCount(uint8_t* _buf, int _buf_len) {
	int zero_count = 0;
	for(int i=0; i<_buf_len; i++) {
		if ((uint8_t)0==_buf[_buf_len-1-i])
			zero_count++;
		else
			break;

	}
	return zero_count;
}
static int getaddrinfo_api(char *_host,struct socket_ipinfo_t* _ipinfo)
{ 
  struct addrinfo hints, *res=NULL, *res0=NULL;
   int error = 0;
   int ret = -1;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = PF_INET6;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_ADDRCONFIG;
   error = getaddrinfo(_host, NULL, &hints, &res0);
    if (error==0) {
        
    	for (res = res0; res; res = res->ai_next) {
    		char ip_buf[64] = {0};

            if (AF_INET6 == res->ai_family) {

                memcpy ( (char*)&(_ipinfo->v6_addr[_ipinfo->size].s6_addr32), (char*)&((((struct sockaddr_in6*)res->ai_addr)->sin6_addr).s6_addr32), 12);
                //_ipinfo->v6_addr[_ipinfo->size]=res->ai_addr->s6_addr;
                _ipinfo->size+=1;
      
                ret = 0;
                break;
            }
    		else {
    			
    			ret = -1;
    		}

    	}

    } else {	
    	ret = -1;
    }
   freeaddrinfo(res0);
   return ret;
}


void *th_socket_gethostbyname(void *data) {
  dbg("[%s] -- [%d] -- th_socket_gethostbyname enter\r\n", __FUNCTION__, __LINE__);
    struct multi_dns *tmp = (struct multi_dns *)data;
   if(socket_gethostbyname(tmp->_host,tmp->_ipinfo,tmp->_timeout,tmp->_dnsserver)==-1)
     {
      pthread_mutex_lock(&dnsquery_mutex1);
      dnsquery_failcount++;
      pthread_mutex_unlock(&dnsquery_mutex1);
     }
 dbg("[%s] -- [%d] -- th_socket_gethostbyname out \r\n", __FUNCTION__, __LINE__);
}
static int th_gethostbyname(char* _host, struct socket_ipinfo_t* _ipinfo, int _timeout /*ms*/)
{    
      int ret=-1;
      dbg("[%s] -- [%d] -- th_gethostbyname enter\r\n", __FUNCTION__, __LINE__);  
          struct multi_dns *data=(struct multi_dns *)malloc(sizeof(struct multi_dns));
	   pthread_t dnsserver1,dnsserver2,dnsserver3;
           pthread_mutex_init(&dnsquery_mutex,NULL);
           pthread_mutex_init(&dnsquery_mutex1,NULL);
           data->_host=_host;
           data->_ipinfo=_ipinfo;
           data->_timeout=_timeout;
           data->_dnsserver  = "2001:67c:27e4:15::64";
	   if(pthread_create(&dnsserver1,NULL,th_socket_gethostbyname,data))
           {      
            dbg("[%s] -- [%d] -- pthread_create failed\r\n", __FUNCTION__, __LINE__);   
                  DNS64SERVERNUM--;
            }
	   data->_dnsserver = "2001:67c:27e4:15::6411";
           if(pthread_create(&dnsserver2,NULL,th_socket_gethostbyname,data))
           {      
            dbg("[%s] -- [%d] -- pthread_create failed\r\n", __FUNCTION__, __LINE__);   
               DNS64SERVERNUM--;
            }
           data->_dnsserver = "2001:67c:27e4::60";
           if(pthread_create(&dnsserver3,NULL,th_socket_gethostbyname,data))
           {      
            dbg("[%s] -- [%d] -- pthread_create failed\r\n", __FUNCTION__, __LINE__);   
                 DNS64SERVERNUM--;
            }
    
   do
  {     
     
       if(_ipinfo->size>0) 
        {
         dbg("[%s] -- [%d] -- _ipinfo->size[%d]\r\n", __FUNCTION__, __LINE__,_ipinfo->size); 
         
        free(data); 
         
        return 0; 
        }
       usleep(1);
      
       if(dnsquery_failcount==DNS64SERVERNUM)  
      { 
      dbg("[%s] -- [%d] -- pthreadNUM[%d]==dnsquery_failcount\r\n", __FUNCTION__, __LINE__,dnsquery_failcount); 
      free(data);
      return -1;
       }
    
  }while(1);    
}

/**
 *函数名:    getaddrinfo_v6（外部借口调用）
 *功能: 输入域名，可得到该域名下所对应的IP地址列表保存在结构体_ipinfo中
 *输入:       _host：输入的要查询的主机域名
 *输入:       _timeout：设置查询超时时间，单位为毫秒
 *输出:        _ipinfo为要输出的ip信息结构体
 *返回值:          当返回-1表示查询失败，当返回0则表示查询成功
 *
 */
int getaddrinfo_v6(const char* _host, struct socket_ipinfo_t* _ipinfo, int _timeout /*ms*/) 
{   
   dbg("[%s] -- [%d] -- getaddrinfo_v6 enter\r\n", __FUNCTION__, __LINE__);
      if (NULL == _host) 
    {
        dbg("[%s] -- [%d] -- NULL == _host \r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (NULL == _ipinfo) {
        dbg("[%s] -- [%d] -- NULL == _ipinfo \r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    int re=-1;
    char host[64];
    memset(host, 0, sizeof(host));
    strncpy(host, _host, strlen(_host));
    _ipinfo->size = 0;
    if(isValidIpv4Address(host))//输入的是ipv4地址 
    {  
        struct in_addr _v4_addr = {0};
        inet_pton(AF_INET,host,&_v4_addr);
        char ipv4_host[20]="ipv4only.arpa";
        if(th_gethostbyname(ipv4_host,_ipinfo, _timeout)!=0) 
        {
            dbg("[%s] -- [%d] -- socket_gethostbyname failed\r\n", __FUNCTION__, __LINE__);
            return -1; 
        }
        _ipinfo->size=1; 
        if(IsNat64AddrValid(&(_ipinfo->v6_addr[0]))==true) {
            ReplaceNat64WithV4IP((&_ipinfo->v6_addr[0]) , &_v4_addr);
   
            return 0;
        } else {
            dbg("[%s] -- [%d] -- isValidIpv4Address failed\r\n", __FUNCTION__, __LINE__);
            return -1;
        }
    } 
    else
    {
        re=th_gethostbyname(host,_ipinfo,_timeout);
        if(-1 == re) {
            dbg("[%s] -- [%d] -- socket_gethostbyname failed\r\n", __FUNCTION__, __LINE__);
            re=getaddrinfo_api(host,_ipinfo);
        }
    }
    //dbg("re : %d\r\n",re); 
    return re;
    /*getaddrinfo*/
}
/**
 *函数名:    socket_gethostbyname
 *功能: 输入域名，可得到该域名下所对应的IP地址列表
 *输入:       _host：输入的要查询的主机域名
 *输入:       _timeout：设置查询超时时间，单位为毫秒
 *输入:       _dnsserver 指定的dns服务器的IP
 *输出:        _ipinfo为要输出的ip信息结构体
 *返回值:          当返回-1表示查询失败，当返回0则表示查询成功
 *
 */
int socket_gethostbyname(char* _host, struct socket_ipinfo_t* _ipinfo, int _timeout /*ms*/, const char* _dnsserver) {
   dbg("[%s] -- [%d] -- socket_gethostbyname enter\r\n", __FUNCTION__, __LINE__);
    
    if(NULL==_dnsserver) {
        dbg("[%s] -- [%d] -- NULL==_dnsserver \r\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (_timeout <= 0) _timeout = DEFAULT_TIMEOUT;
    int sockfd = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP); 
    if (sockfd  < 0) 
    { 
        dbg("[%s] -- [%d] -- socket failed\r\n", __FUNCTION__, __LINE__);
        return -1;  
    }  
    //dbg("sockfd:%d\n",sockfd);
    struct sockaddr_in6 dest = {0};
    dest.sin6_family = AF_INET6;
    inet_pton(AF_INET6, _dnsserver, &(dest.sin6_addr)); 
    dest.sin6_port = htons(DNS_PORT);
    struct RES_RECORD answers[SOCKET_MAX_IP_COUNT];  // the replies from the DNS server
    memset(answers, 0, sizeof(struct RES_RECORD)*SOCKET_MAX_IP_COUNT);
    int ret = -1;
    do {
        unsigned int BUF_LEN=2048;
        unsigned char send_buf[2048] = {0};
        unsigned char recv_buf[2048] = {0};//C99标准引入了变长数组，它允许使用变量定义数组各维。变长数组必须是自动存储类，而且声明时不可以进行初始化。
        struct DNS_HEADER* dns = (struct DNS_HEADER*)send_buf;
        unsigned char* qname = (unsigned char*)&send_buf[sizeof(struct DNS_HEADER)];
        PrepareDnsQueryPacket(send_buf, dns, qname, _host);
        unsigned long send_packlen = sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION);
        int sendfd;
        struct sockaddr_in6 recv_src = {0};

        int recvPacketLen = 0;
        for(int i=0;i<3;i++)
        {
            sendfd = sendto(sockfd, (char*)send_buf, send_packlen, 0, (struct sockaddr*)&dest, sizeof(dest));
            //dbg("sendPacketlen:%d\n",sendfd);
            if (sendfd==-1)
            {  
                dbg("[%s] -- [%d] -- sendto failed\r\n", __FUNCTION__, __LINE__);
                break;
            }
            socklen_t recv_src_len = sizeof(recv_src);
            if ((recvPacketLen = RecvWithinTime(sockfd, (char*)recv_buf, BUF_LEN, (struct sockaddr*)&recv_src, &recv_src_len, _timeout / 1000, (_timeout % 1000) * 1000)) > -1) 
            {
                break;
            }
            else {  
                dbg("[%s] -- [%d] -- RecvWithinTime failed\r\n", __FUNCTION__, __LINE__);
            }
        }

        // move ahead of the dns header and the query field
        unsigned char* reader = &recv_buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION)];
        dns = (struct DNS_HEADER*)recv_buf;   // 指向recv_buf的header
        ReadRecvAnswer(recv_buf, dns, reader, answers);

        // 把查询到的IP放入返回参数_ipinfo结构体中
        int answer_count = min(SOCKET_MAX_IP_COUNT, (int)ntohs(dns->ans_count));
        pthread_mutex_lock(&dnsquery_mutex); 
        for (int i = 0; i < answer_count; ++i) {
            if (AAAA == ntohs(answers[i].resource->type)) {  // IPv6 address
               for(int p=0;p<16;p++)
                {  
                    _ipinfo->v6_addr[_ipinfo->size].s6_addr[p]=answers[i].rdata[p];
                }
                _ipinfo->size++;          
                   //pthread_mutex_unlock(&mutex); 
            }
        }
          

        if (0 >= _ipinfo->size) {  
          pthread_mutex_unlock(&dnsquery_mutex);
          dbg("[%s] -- [%d] -- _ipinfo->size <= 0\r\n", __FUNCTION__, __LINE__);
            break;
        
        }       
        ret = 0;
       
    } while (false);

    FreeAll(answers);
    close(sockfd);
   dbg("[%s] -- [%d] -- socket_gethostbyname out\r\n", __FUNCTION__, __LINE__);
    return ret;  //* 查询DNS服务器超时

}



static int isValidIpv4Address(char* _ipaddress) {
   
    struct sockaddr_in sa;
    int result =inet_pton(AF_INET, _ipaddress, (void*) & (sa.sin_addr));
    return result != 0;
}

void FreeAll(struct RES_RECORD* _answers) {
    int i;

    for (i = 0; i < SOCKET_MAX_IP_COUNT; i++) {
        if (_answers[i].name != NULL)
            free(_answers[i].name);

        if (_answers[i].rdata != NULL)
            free(_answers[i].rdata);
    }
}

void ReadRecvAnswer(unsigned char* _buf, struct DNS_HEADER* _dns, unsigned char* _reader, struct RES_RECORD* _answers) {
    // reading answers
    int i, j, stop = 0;
    int answer_count =min(SOCKET_MAX_IP_COUNT, (int)ntohs(_dns->ans_count));
    for (i = 0; i < answer_count; i++) {
        _answers[i].name = ReadName(_reader, _buf, &stop);
        _reader = _reader + stop;

        _answers[i].resource = (struct R_DATA*)(_reader);
        _reader = _reader + sizeof(struct R_DATA);//指针偏移

       // if (ntohs(_answers[i].resource->type) == 1) {  // if its an ipv4 address
         if (ntohs(_answers[i].resource->type) == AAAA) {  // if its an ipv6 address
           _answers[i].rdata = (unsigned char*)malloc(ntohs(_answers[i].resource->data_len));
          
            if (NULL == _answers[i].rdata) 
            {
                return;
            }

            for (j = 0 ; j < ntohs(_answers[i].resource->data_len) ; j++)
                { _answers[i].rdata[j] = _reader[j];
                  
             }
            
            _answers[i].rdata[ntohs(_answers[i].resource->data_len)] = '\0';
            _reader = _reader + ntohs(_answers[i].resource->data_len);
        } else {
            _answers[i].rdata = ReadName(_reader, _buf, &stop);
            _reader = _reader + stop;
        }
    }
}

unsigned char* ReadName(unsigned char* _reader, unsigned char* _buffer, int* _count) {
    unsigned char* name;
    unsigned int p = 0, jumped = 0, offset;
    const unsigned int INIT_SIZE = 256, INCREMENT = 64;
    int timesForRealloc = 0;
    int i , j;

    *_count = 1;
    name   = (unsigned char*)malloc(INIT_SIZE);

    if (NULL == name) {
        
        return NULL;
    }

    name[0] = '\0';

    // read the names in 3www6google3com format
    while (*_reader != 0) {
        if (*_reader >= 192) {  // 192 = 11000000 ,如果该字节前两位bit为11，则表示使用的是地址偏移来表示name
            offset = (*_reader) * 256 + *(_reader + 1) - 49152;  // 49152 = 11000000 00000000  计算相对于报文起始地址的偏移字节数，即去除两位为11的bit，剩下的14位表示的值
            _reader = _buffer + offset - 1;
            jumped = 1;  // we have jumped to another location so counting wont go up!
        } else
            name[p++] = *_reader;

        _reader = _reader + 1;

        if (jumped == 0) *_count = *_count + 1;  // if we have not jumped to another location then we can count up

        if (*_count >= (int)(INIT_SIZE + INCREMENT * timesForRealloc)) {
            timesForRealloc++;

            unsigned char* more_name = NULL;
            more_name = (unsigned char*)realloc(name, (INIT_SIZE + INCREMENT * timesForRealloc));

            if (NULL == more_name) {
             
                free(name);
                return NULL;
            }

            name = more_name;
        }
    }

    name[p] = '\0';  // string complete

    if (jumped == 1) *_count = *_count + 1;  // number of steps we actually moved forward in the packet

    // now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int)strlen((const char*)name); i++) {
        p = name[i];

        for (j = 0; j < (int)p; j++) {
            name[i] = name[i + 1];
            i = i + 1;
        }

        name[i] = '.';
    }

    name[i - 1] = '\0';  // remove the last dot
    return name;
}

// this will convert www.google.com to 3www6google3com
void ChangetoDnsNameFormat(unsigned char* _qname, char* _hostname) {
    int lock = 0 , i;
    strncat(_hostname,".",1);
    const char* host = _hostname;

    for (i = 0; i < (int)strlen(host); i++) {
        if (host[i] == '.') {
            *_qname++ = i - lock;

            for (; lock < i; lock++) {
                *_qname++ = host[lock];
            }

            lock++;
        }
    }

    *_qname++ = '\0';
}

void PrepareDnsQueryPacket(unsigned char* _buf, struct DNS_HEADER* _dns, unsigned char* _qname,  char* _host) {
    struct QUESTION*  qinfo = NULL;
    // Set the DNS structure to standard queries
    _dns->id = getpid();
    _dns->qr = 0;      // This is a query
    _dns->opcode = 0;  // This is a standard query
    _dns->aa = 0;      // Not Authoritative
    _dns->tc = 0;      // This message is not truncated
    _dns->rd = 1;      // Recursion Desired
    _dns->ra = 0;      // Recursion not available!
    _dns->z  = 0;
    _dns->ad = 0;
    _dns->cd = 0;
    _dns->rcode = 0;
    _dns->q_count = htons(1);   // we have only 1 question
    _dns->ans_count  = 0;
    _dns->auth_count = 0;
    _dns->add_count  = 0;
    // point to the query portion
    _qname = (unsigned char*)&_buf[sizeof(struct DNS_HEADER)];
    ChangetoDnsNameFormat(_qname, _host);  // 将传入的域名host转换为标准的DNS报文可用的格式，存入qname中
    qinfo = (struct QUESTION*)&_buf[sizeof(struct DNS_HEADER) + (strlen((const char*)_qname) + 1)];  // fill it
    qinfo->qtype = htons(AAAA);  //查询 ipv6 address
    qinfo->qclass = htons(1);  // its internet
}

static  int RecvWithinTime(int _fd, char* _buf, size_t _buf_n, struct sockaddr* _addr, socklen_t* _len, unsigned int _sec, unsigned _usec) {
    struct timeval tv;
    fd_set readfds, exceptfds;
    int n = 0;

    FD_ZERO(&readfds);
    FD_SET(_fd, &readfds);
    FD_ZERO(&exceptfds);
    FD_SET(_fd, &exceptfds);

    tv.tv_sec = _sec;
    tv.tv_usec = _usec;

    int ret = -1;
label:
    ret = select(_fd + 1, &readfds, NULL, &exceptfds, &tv);

    if (-1 == ret) {
        if (EINTR == errno) {
            // select被信号中断 handler
            FD_ZERO(&readfds);
            FD_SET(_fd, &readfds);
            FD_ZERO(&exceptfds);
            FD_SET(_fd, &exceptfds);
            goto label;
        }
    }

    if (FD_ISSET(_fd, &exceptfds)) {
        // socket异常处理
        return -1;
    }

    if (FD_ISSET(_fd, &readfds)) {
        if ((n = (int)recvfrom(_fd, _buf, _buf_n, 0, _addr, _len)) >= 0) {
            return n;
        }
    }

    return -1;  // 超时或者select失败
}



