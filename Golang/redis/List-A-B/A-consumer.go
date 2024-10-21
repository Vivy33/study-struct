package main

import (
	"context"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

// 创建 Redis 客户端的函数，使用连接池
func newRedisClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务器地址
		Password: "",               // 没有密码则留空
		DB:       0,                // 使用默认数据库
		PoolSize: 100,              // 连接池大小
	})
}

// 消费者函数
func consumer(rdb *redis.Client, queueKey string) {
	for {
		result, err := rdb.BRPop(ctx, 5*time.Second, queueKey).Result()
		if err == redis.Nil {
			fmt.Println("消息已被消费完，继续等待下一轮")
			continue
		} else if err != nil {
			fmt.Println("Consumer 出错:", err)
			continue
		}
		fmt.Println("消费到的消息:", result[1])
	}
}

func main() {
	rdb := newRedisClient() // 初始化 Redis 客户端
	queueKey := "my_queue"  // 队列的键名

	// 启动多个消费者
	for i := 1; i <= 3; i++ {
		go consumer(rdb, queueKey) // 启动 3 个消费者
	}

	// 阻塞主程序，保持 Goroutine 运行
	select {}
}
