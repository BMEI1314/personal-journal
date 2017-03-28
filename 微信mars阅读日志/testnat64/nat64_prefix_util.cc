#include "nat64_prefix_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<netdb.h>

static const uint8_t kWellKnownV4Addr1[4] = {192, 0, 0, 170};
static const uint8_t kWellKnownV4Addr2[4] = {192, 0, 0, 171};
static const uint8_t kOurDefineV4Addr[4] = {192, 0, 2, 1};

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

//static bool IsIPv4Addr(const std::string& _str) {
//	struct in_addr v4_addr= {0};
//	return socket_inet_pton(AF_INET, _str.c_str(), &v4_addr)==0; //1 for success, 0 for invalid ip, -1 for other error
//}
typedef enum TLocalIPStack {
    ELocalIPStack_None = 0,
    ELocalIPStack_IPv4 = 1,
    ELocalIPStack_IPv6 = 2,
    ELocalIPStack_Dual = 3,
}danaipstack_type_t;

static danaipstack_type_t dananet_family = ELocalIPStack_IPv4;

static int _test_connect(int pf, struct sockaddr *addr, size_t addrlen) 
{
    int s = socket(pf, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
        return 0;
    int ret;
    do {
        ret = connect(s, addr, addrlen);
    } while (ret < 0 && errno == EINTR);
    int success = (ret == 0);
    
    return success;
}

static int _have_ipv6() 
{
    struct sockaddr_in6 sin_test; 
    memset(&sin_test, 0, sizeof(sin_test));
    sin_test.sin6_family = AF_INET6;
    sin_test.sin6_port = htons(0XFFFF);
    //uint8_t ip[16] = {0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //memcpy(sin_test.sin6_addr.s6_addr, ip, sizeof(sin_test.sin6_addr.s6_addr));
    //sin_test.sin6_addr.s6_addr = {0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //2001:4860:4860::8888
     inet_pton(AF_INET6, "2001:4860:4860::8888", &(sin_test.sin6_addr));  

    return _test_connect(PF_INET6, (struct sockaddr *)&sin_test, sizeof(sin_test));
}

static int _have_ipv4() 
{
    struct sockaddr_in sin_test; 
    memset(&sin_test, 0, sizeof(sin_test));
    sin_test.sin_family = AF_INET;
    sin_test.sin_port = htons(0XFFFF);
    sin_test.sin_addr.s_addr = htonl(0x08080808L);
    return _test_connect(PF_INET, (struct sockaddr *)&sin_test, sizeof(sin_test));
}
bool checknetfamily()
{
    dananet_family = ELocalIPStack_None;
    int have_ipv4 = _have_ipv4();
    int have_ipv6 = _have_ipv6();
    if (have_ipv4) {
        dananet_family = ELocalIPStack_IPv4;      
        return true;
    }
    if (have_ipv6) {
        dananet_family = ELocalIPStack_IPv6;    
        return true;
    }
    dananet_family = ELocalIPStack_IPv4;
    return true;

}



static size_t GetSuffixZeroCount(uint8_t* _buf, size_t _buf_len) {
	size_t zero_count = 0;
	for(size_t i=0; i<_buf_len; i++) {
		if ((uint8_t)0==_buf[_buf_len-1-i])
			zero_count++;
		else
			break;

	}
	return zero_count;
}
static bool IsNat64AddrValid(const struct in6_addr* _replaced_nat64_addr) {
	bool is_iOS_above_9_2 = false;
#ifdef __APPLE__
	 if (publiccomponent_GetSystemVersion() >= 9.2f) is_iOS_above_9_2 = true;
#endif
	size_t suffix_zero_count = GetSuffixZeroCount((uint8_t*)_replaced_nat64_addr, sizeof(struct in6_addr));
	if (0!=suffix_zero_count) {
		
	}
	bool is_valid = false;
	switch(suffix_zero_count) {
		case 3:
			//Pref64::/64
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+9, kOurDefineV4Addr, 4)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+9, kWellKnownV4Addr1, 4)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+9, kWellKnownV4Addr2, 4)) {
					is_valid = true;
				}
			}
			break;
		case 4:
			//Pref64::/56
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+7, kOurDefineV4Addr_index1, 5)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+7, kWellKnownV4Addr1_index1, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+7, kWellKnownV4Addr2_index1, 5)) {
					is_valid = true;
				}
			}
			break;
		case 5:
			//Pref64::/48
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+6, kOurDefineV4Addr_index2, 5)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+6, kWellKnownV4Addr1_index2, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+6, kWellKnownV4Addr2_index2, 5)) {
					is_valid = true;
				}
			}
			break;
		case 6:
			//Pref64::/40
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+5, kOurDefineV4Addr_index3, 5)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+5, kWellKnownV4Addr1_index3, 5)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+5, kWellKnownV4Addr2_index3, 5)) {
					is_valid = true;
				}
			}
			break;
		case 8: //7bytes suffix and 1 bytes u(RFC6052)
			//Pref64::/32
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+4, kOurDefineV4Addr, 4)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+4, kWellKnownV4Addr1, 4)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+4, kWellKnownV4Addr2, 4)) {
					is_valid = true;
				}
			}
			break;
		case 0:
			//Pref64::/96
			if (is_iOS_above_9_2) {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+12, kOurDefineV4Addr, 4)) {
					is_valid = true;
				}
			} else {
				if (0==memcmp(((uint8_t*)_replaced_nat64_addr)+12, kWellKnownV4Addr1, 4)
					|| 0==memcmp(((uint8_t*)_replaced_nat64_addr)+12, kWellKnownV4Addr2, 4)) {
					is_valid = true;
				}
			}
			break;
		
			
	}
	return is_valid;
}
static void ReplaceNat64WithV4IP(struct in6_addr* _replaced_nat64_addr, const struct in_addr* _v4_addr) {
	size_t suffix_zero_count = GetSuffixZeroCount((uint8_t*)_replaced_nat64_addr, sizeof(struct in6_addr));
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

bool ConvertV4toNat64V6(const struct in_addr& _v4_addr, struct in6_addr& _v6_addr) {
    checknetfamily();
     printf("localIPstack is:%d\n",dananet_family);
    if (ELocalIPStack_IPv6 != dananet_family) {
    	printf("Current Network is not ELocalIPStack_IPv6, no need GetNetworkNat64Prefix.\n");
		return false;
    }
	struct addrinfo hints, *res=NULL, *res0=NULL;
	int error = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_ADDRCONFIG;

	char v4_ip[16] = {0};
	inet_ntop(AF_INET, &_v4_addr, v4_ip, sizeof(v4_ip));
#ifdef __APPLE__
	if (publiccomponent_GetSystemVersion() >= 9.2f) {//higher than iOS9.2
		error = getaddrinfo(v4_ip, NULL, &hints, &res0);
	} else {//lower than iOS9.2 or other platform
#endif
		error = getaddrinfo("ipv4only.arpa", NULL, &hints, &res0);
#ifdef __APPLE__
	}
#endif
	bool ret = false;
    if (error==0) {
    	for (res = res0; res; res = res->ai_next) {
    		char ip_buf[64] = {0};

    		if (AF_INET6 == res->ai_family) {
#ifdef __APPLE__
				if (publiccomponent_GetSystemVersion() >= 9.2f) { //higher than iOS9.2
					//copy all 16 bytes
					memcpy ( (char*)&_v6_addr, (char*)&((((sockaddr_in6*)res->ai_addr)->sin6_addr).s6_addr32), 16);
					ret = true;
					break;
				} else { //lower than iOS9.2 or other platform
#endif

	    			if (IsNat64AddrValid((struct in6_addr*)&(((sockaddr_in6*)res->ai_addr)->sin6_addr))) {
						ReplaceNat64WithV4IP((struct in6_addr*)&(((sockaddr_in6*)res->ai_addr)->sin6_addr) , &_v4_addr);
						memcpy ( (char*)&_v6_addr, (char*)&((((sockaddr_in6*)res->ai_addr)->sin6_addr).s6_addr32), 16);
						const char* ip_str = inet_ntop(AF_INET6, &_v6_addr, ip_buf, sizeof(ip_buf));
						
		    			ret = true;
		    			break;
	    			} else {
	    			
	    				ret = false;
	    			}
#ifdef __APPLE__
				}
#endif

    		} else if (AF_INET == res->ai_family){
    			const char* ip_str = inet_ntop(AF_INET, &(((sockaddr_in*)res->ai_addr)->sin_addr), ip_buf, sizeof(ip_buf));
    			
    			ret = false;
    		} else {
    			
    			ret = false;
    		}

    	}
    } else {
    	
    	ret = false;
    }

    freeaddrinfo(res0);
    return ret;

}

bool ConvertV4toNat64V6(const std::string& _v4_ip, std::string& _nat64_v6_ip) {
	struct in_addr v4_addr = {0};
	int pton_ret = inet_pton(AF_INET, _v4_ip.c_str(), &v4_addr);
	if (0==pton_ret) {
        printf("error!!!ipv4 addrass is illegal\n");
    	return false;
    }
	struct in6_addr v6_addr = {{{0}}};
	if (ConvertV4toNat64V6(v4_addr, v6_addr)) {
		
            char v6_ip[64] = {0};
		inet_ntop(AF_INET6, &v6_addr, v6_ip, sizeof(v6_ip));
		_nat64_v6_ip = std::string(v6_ip);
		return true;
	}
     
	return false;
}

///----------------------------------------------------------------------------
bool  GetNetworkNat64Prefix(struct in6_addr& _nat64_prefix_in6) {
   
      checknetfamily();
  
    if (ELocalIPStack_IPv6 != dananet_family) {
    	
		return false;
    }
	struct addrinfo hints, *res=NULL, *res0=NULL;
	int error = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_ADDRCONFIG;

	bool ret = false;
#ifdef __APPLE__
	if (publiccomponent_GetSystemVersion() >= 9.2f) {
		error = getaddrinfo("192.0.2.1", NULL, &hints, &res0);
	} else {
#endif
		error = getaddrinfo("ipv4only.arpa", NULL, &hints, &res0);
#ifdef __APPLE__
	}
#endif
    if (error==0) {
    	for (res = res0; res; res = res->ai_next) {
    		char ip_buf[64] = {0};

    		if (AF_INET6 == res->ai_family) {
    			memcpy ( (char*)&(_nat64_prefix_in6.s6_addr32), (char*)&((((sockaddr_in6*)res->ai_addr)->sin6_addr).s6_addr32), 12);

    			ret = true;
    			break;

    		} else if (AF_INET == res->ai_family){
    			const char* ip_str = inet_ntop(AF_INET, &(((sockaddr_in*)res->ai_addr)->sin_addr), ip_buf, sizeof(ip_buf));
    			
    			ret = false;
    		} else {
    			
    			ret = false;
    		}

    	}
    } else {
    	
    	ret = false;
    }

    freeaddrinfo(res0);
    return ret;
}

bool  GetNetworkNat64Prefix(std::string& _nat64_prefix) {
	struct in6_addr nat64_prefix_in6;
	memset(&nat64_prefix_in6, 0, sizeof(nat64_prefix_in6));

	if (GetNetworkNat64Prefix(nat64_prefix_in6)) {
		char ip_buf[64] = {0};
		const char* prefix_str =inet_ntop(AF_INET6, &nat64_prefix_in6, ip_buf, sizeof(ip_buf));
		_nat64_prefix = std::string(prefix_str);
		return true;
	}
	return false;
}
