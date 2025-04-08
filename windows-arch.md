application
process
windows/vm1/vm2/wsl2
vmm(virtual machine manager) - hype-v
hal - 硬件抽象层
hardware


process
userspace
-----------------------系统调用syscall sysenter(从用户态进入内核态)
kernelspace


/dev/random 
真随机数发生器 每次生成随机数 需要消耗外部随机输入(敲键盘/调整硬件...)
这些信息是有限的 所以把这些信息消耗完以后 再去访问random会被阻塞
等到信息生成才能返回

/dev/urandom
算法一样，但种子是内核随机生成的 所以不会出现阻塞情况
但有概率在数学的基础上解出种子 芯片


io
发起一个syscall(pread/pwrite/read/write...)
进内核态
如果打开的是文件-先过文件系统-再访问设备 / 设备-直接访问设备
所以打开文件 io延迟中还有文件系统的耗时 直接打开设备则没有文件系统耗时

访问设备的流程分2步 - iostat -dxm 1
1.把请求塞入设备驱动的队列中 - aqu-sz(队列的积压的请求个数)
2.设备驱动从队列中获得一个请求发给固件 - await(r_await/w_await)
真正的io耗时还包含在队列中等待的耗时
rareq-sz

一次真正的io耗时 = 文件系统耗时 + 队列等待耗时 + await(avg)
数据库redo log串行
在保证数据正确性的情况下，每次提交事务，需要fsync redo log
所以每s最大的事务数 < 1s/w_await
dd if=/dev/zero of=1 bs=4k oflag=direct 

await 0.6ms
queue 1.6
(1+1.6)*0.6=1.56ms
1000ms/1.56ms=641
每秒最多处理的事务个数是641个

问题出在分区表没有做4k对齐
文件系统在某个分区上
mysql在文件系统上
虽然文件系统和mysql都做了4k对齐
但是是在分区表的基础上做的4k对齐
现在分区表没有对齐
所以mysql就没对齐
fdisk
intel的ssd盘在地址没有对齐的情况下性能暴跌
三星的盘没有问题