# epollicmp

epollicmp是一个测试网络的简单工具，没有发包间隔的限制，没有包大小的限制
epollicmp is a project about ping test without interval limit  

用法 ./epollicmp destaddr(url/ip) icmp_interval(ms) icmp_pkglen(byte) 
USAGE:  ./epollicmp destaddr(url/ip) icmp_interval(ms) icmp_pkglen(byte) 

使用 CTRL+C 停止
use CTRL+C to stop

打印示例：
./epollicmp www.baidu.com 100 200
epollicmp start www.baidu.com 100 200
CEpollIcmp::start start
CEpollIcmp::initSock start
PING www.baidu.com, (183.232.231.174) (200) bytes of data every (100) microsecond.
CEpollIcmp::mainLoop start
200 bytes from 183.232.231.174: icmp_seq=1 ttl=52 rtt=15.685 ms
...
200 bytes from 183.232.231.174: icmp_seq=168 ttl=52 rtt=13.464 ms
^CCEpollIcmp::stop start
200 bytes from 183.232.231.174: icmp_seq=169 ttl=52 rtt=18.799 ms
CEpollIcmp::mainLoop end

PING www.baidu.com, (183.232.231.174) (200) bytes of data every (100) microsecond.

STATISTIC:
ping duration:17.047 second
send count:169
recv count:169
lose rate:0.00%
resp min:13.066 ms
resp max:23.272 ms
resp average:16.044 ms

epollicmp end
