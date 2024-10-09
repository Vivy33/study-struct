// 1. 并发100个任务，但是同一时间最多运行的10个任务
// 利用channel缓冲管道队列满了需要等待，其他goroutine读取消息释放特性
package main

import (
	"fmt"
	"sync"
	"time"
)

// 一个模拟的任务函数
func task(id int) {
	fmt.Printf("Task %d is starting\n", id)
	// 模拟任务处理时间
	time.Sleep(2 * time.Second)
	fmt.Printf("Task %d is done\n", id)
}

func main() {
	const maxConcurrentTasks = 10
	const totalTasks = 100

	var wg sync.WaitGroup
	taskCh := make(chan int, maxConcurrentTasks)

	// 启动 worker goroutines
	for i := 0; i < totalTasks; i++ {
		wg.Add(1)
		go func(taskID int) {
			defer wg.Done()
			taskCh <- taskID
			task(taskID)
			<-taskCh
		}(i)
	}

	// 等待所有任务完成
	wg.Wait()
	close(taskCh)
	fmt.Println("All tasks completed")
}
