####1.原理
设备上的进程会创建3个socket，一个监听的socket，一个去connect服务器S的socket，一个去连接B的socket，关键点就在于这3个socket都需要bind一对相同的IP和端口。SO_REUSEADDR和SO_REUSEPORT解决了这个问题。
UML时序表:
![](./img/tcp.jpg)
####2.各种情况
![](./img/打洞.png)
####3.理论上连接的情况
![](./img/ceshih.jpg)
####4.tcp打洞测试的结果:
![](./img/ceshi.png)
####4.测试图
![测试c情况client端,打洞成功](./img/ceshic.png)
![测试c情况server端,打洞成功](./img/ceshic1.png)
![测试e情况,打洞失败](./img/ceshie.png)