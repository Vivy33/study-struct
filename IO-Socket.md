                            io多路复用
对于读路径来说 read/recv                    对于写路径来说
当数据还没有到内核的时候，读就会阻塞         当内核的buffer用满的时候，写就会阻塞
设备已经把数据传给了内核                    当内核的buffer还大于0的时候，写接口就可以ready
内核会通知对应的fd                          内核会通知到对应的fd
                            数据已经ready                               
                    通知的结果就是select/poll/epoll_wait

默认情况下，所有io操作都是阻塞操作(阻塞没有超时,等到天荒地老)
对于写路径来说，不太容易阻塞
因为内核的buffer一般来说比较大 - sysctl -a |grep wmem |grep tcp
                                net.ipv4.tcp_wmem = 4096        16384   4194304
对于读路径来说，一定要数据被外设处理完后
才能被内核接收，所以大概率被阻塞

在做io多路复用的时候
要感知到读数据没有ready
如果没有ready
要快速失败去处理其他业务逻辑

all_fds包含两类socket
1.listen_fd
2.所有的client_fd
如果是listen_fd有读事件
说明需要调用accept(listen_fd)
所以client_fd = accept(listen_fd)
然后把client_fd加入到all_fds中
如果是client_fd有读事件
就是正常的收发处理请求

设置成非阻塞，如果当前fd没有ready，那么对它进行rw操作，它会立即失败
发现失败，快速处理下一个，如果大部分的fd是非活跃的，就会引入大量的无效操作，浪费大量cpu资源
比如100w个fd中，只有1000个是活跃的，为了处理1000个请求，需要遍历100w
如果用epoll，内核会把有事件的fd返回，就不需要遍历非活跃的fd了
但是对于select/poll，虽然内核返回的是有事件的fd，但是没有提供独立的数据结构
所以用户态依旧需要去遍历整个all_fds找到带有事件标记的fd
时间复杂度依旧很高
select只支持1024个fd，poll/epoll支持无限个fd(只要内存够)

set_nonblocking(listen_fd); // 对于同一个service来说，正常情况下只有1个listen_fd，所以这里是1个

// 死循环         // man signal
int service_stop; // 收到特定退出信号的时候，service_stop设置成1
                  // 收到某个特定的请求，service_stop
for(;!service_stop;) {
    poll(all_fds, &read_ready_fds, /*&write_ready_fds*/NULL); // write_ready_fds 一般不会阻塞
    for(int i = 0; i < read_ready_fds; i++) {
        if(read_ready_fds[i] == listen_fd) {
            client_fd = accept(listen_fd);
            set_nonblocking(client_fd);
            insert_fd(client_fd, &all_fds);
        } else {
            recv(read_ready_fds[i], buffer, size);
            do_sth(); // 不能太慢,否则会导致poll的后面的请求都被阻塞                           大key
                      // 对于redis来说,数据结构的查询时间复杂度要低,空间复杂度也要低(不能出现一个 k-v 几M及以上)
                      // 对于其他业务来说，没有办法保证do_sth业务逻辑是很快的，所以会把do_sth投递给独立的worker线程处理 - 线程池
                      // 所以需要把client_fd传递给do_sth，后端的worker线程先处理完recv，再处理业务逻辑，再send-resp
                      // 需要引入一个任务队列，1个生产者-n个消费者，要保证线程安全
                      // 可以通过定时任务感知任务队列的积压，去做扩缩容 - k8s-service 变更replica数量
            send(read_ready_fds[i], buffer, size);
        }
    }
}

-------------------------------------------------------

poll           
for() {        timeout - 高于定时任务1个数量级的精度
                |      // 表示epoll等待文件描述符上事件发生的超时时间，期间没有任何event，那么会在timeout后返回;如果在timeout之前有event，立即返回
                |
    poll(x, y, 100ms); // 每次poll返回的时候获取当前时间
    gettimeofday(&tv, NULL);
}

假如有一个定时任务
每1s执行1次
poll的返回时间是 <= 100ms
所以一定能在1s前后有一次poll的返回时机
如果记录了这个定时任务上一次的开始执行的时间
那么简单的获取下当前时间
就可以决定是否在当前这次poll的返回以后
需要调用定时任务

比如当前时间距离上次任务执行过了980ms
那么就立马执行定时任务
可以允许上下误差

----------------------------------------

                                  epoll
                            水平触发/边缘触发
内核有数据需要被处理，就会发一个event/内核需要处理增量的数据，才会生成一个event

A               B
发送4k              
                接收4K
                消费1K - 还剩3K
如果B是水平触发，会继续收到一个event，还有一个大于0的消息还没处理
如果B是边缘触发，不会收到event，对内核来说没有接收到新的消息

http2.0 streaming
redis pipeline
上一个请求没有收到回复的时候，可以发送下一个请求，但是服务端需要按序返回

IO流程
cpu对设备说，需要去处理一段IO请求 request_id = xxx 起始地址xxx 长度xxx 操作类型xxx r/w
cpu继续执行下一个线程
设备做完IO操作后，向cpu发起一个中断，提示cpu某请求做完了，request_id = xxx 返回地址是xxx
设备发给哪个cpu是伪随机的
考虑下面这个场景
cpu0正在运行线程0
线程0发起了一个IO请求
cpu0切换到线程1执行
线程1的逻辑执行到一般
设备告诉cpu0你上一个请求做完了
线程1的运行被打断了
如果线程1刚好在系统调用中
系统调用就会返回EINTR

IO函数/系统调用 什么时候会失败
1.非法参数 EINVL
2.系统资源不足 ENOMEM
3.运行当前线程的cpu被中断 EINTR(阻塞/非阻塞)，当遇上系统调用的返回值小于0，并且errno == EINTR，重做

对于文件IO 一般不会走异步IO，而是同步IO（默认是阻塞）
AIO/IOURING 支持文件异步IO 不能用epoll

对于网络IO 尽量用异步IO
考虑下面场景
per connection  per thread
epoll只负责listen一个fd
每次listen_fd收到一个读请求
就会创建新的client_fd
创建一个线程把client_fd传给他
线程死循环send-recv / select send-recv
解决了防止线程池某个请求特别慢
把其他请求阻塞了
参考54行do_sth()

-------------------------------------------------------

Socket

1.TCP
三次握手
c       s
syn
        syn-ack
ack
对于s来说，新建连接是一次读事件
对于c来说，新建连接是一次写事件

断开是读事件，收到第一个fin包
如果read返回的大小是0，没有失败
代表是对端close()
所以自己也可以close了

2.UDP
面向无连接的，没有connection概念
以一个个包交互
对方发多少，自己必须要收多少
如果send的超过了一个报文最大大小，会返回一个报错
UDP没有重传，需要自己做可靠传输
其他参考上述send-recv流程

DNS默认基于UDP实现，也可以走TCP
dns丢失感知不到，只能靠timeout重试

3.send-recv
默认是阻塞操作
切成非阻塞，如果没有事件，立即返回错误
一个连接多长时间不活跃就close
socket增加一个last_actime字段
构建一个堆，根据last_actime做排序，k[last_actime] v[last_actime + socket]
每次send-recv更新last_actime字段
先从堆里面删除掉，再插入回堆
当定时器timer唤醒的时候，只需要遍历这个堆
直到某个last_actime距离当前时间小于当前时间，遍历就停止; 对于大于全部close
每次epoll定时器中定时检查socket是否timeout

fcntl - manipulate file descriptor
setsockopt
这两个api可以实现set_nonblocking
前者可以操作任意文件
后者只能操作socket
linux里可以用setsockopt设置超时时间
但是不同操作系统支持的超时时间设置是有区别的
linux不支持connect timeout，支持recv timeout
如果要实现connect timeout，必须要epoll/select这类接口实现
windows支持connect timeout
sysctl -a |grep tcp |grep syn
net.ipv4.tcp_syn_retries = 6
syn包的RTO是1s
每次超时RTO*2
重连6次耗时 2^(6+5+4+3+2+1+0) = 127s

sysctl -a |grep tcp |grep keep
net.ipv4.tcp_keepalive_intvl = 75
net.ipv4.tcp_keepalive_probes = 9    // 假如keepalive包失败了，最多会重试probe次，每次重试的间隔是tcp_keepalive_intvl
                                        期间任意一次probe收到ack，都认为链路是通常的; 
                                        如果都失败了，自己发送一个reset/rst给对端
                                        下次对同一个socket进行send-recv操作时，会失败
                                        假如对端没有收到reset，接收到对端发送报文的时候，内核会回一个reset，不需要用户态参与
                                        假如对端接收到了这个reset，对端操作的任何send-recv都会失败。仅限于这个socket
net.ipv4.tcp_keepalive_time = 7200s
三个值对应
业务层的in active close timeout设置非常大
在timeout之间网络出现故障
tcp感知不到问题
通过setsockopt设置keepalive
tcp会定期(tcp_keepalive_time)试探连接是否通畅 - 往对端发送一个包，受到了对应的ack就认为链路是正常的

timewait
谁先close谁进入timewait
后close的进入closewait

频繁短连接会导致端口不够用(reset不进timewait状态，直接释放五元组)
需要调整tcp_tw_reuse tcp_tw_bucket

为什么tcp有了可靠传输，业务层还需要做自己的确认
比如http resp请求
确认请求已经收到
A发送了一个报文
B内核接收到了，但是B的用户态进程还没有recv就被kill/segment fault
这个报文对业务来说就是没有收到
所以需要业务层自己去确认

五元组
源IP地址（Source IP Address）：发送数据包的设备的IP地址。
源端口号（Source Port Number）：发送数据的进程或服务在源设备上的端口号。
目的IP地址（Destination IP Address）：接收数据包的设备的IP地址。
目的端口号（Destination Port Number）：目标设备上接收数据的进程或服务的端口号。
传输层协议（Transport Layer Protocol）：使用的协议，通常是TCP（传输控制协议）或UDP（用户数据报协议）。