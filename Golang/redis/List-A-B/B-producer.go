package main

import (
	"context"
	"flag"
	"fmt"
	"strconv"
	"time"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

// 创建 Redis 客户端的函数
func newRedisClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务器地址
		Password: "",               // 没有密码则留空
		DB:       0,                // 使用默认数据库
		PoolSize: 100,              // 连接池大小
	})
}

func main() {
	// 从命令行获取 producerID
	producerID := flag.Int("producerID", 1, "Producer ID")
	flag.Parse()

	rdb := newRedisClient() // 初始化 Redis 客户端
	queueKey := "my_queue"  // 队列的键名
	counter := 1

	// 开始推送消息
	for {
		messageID := strconv.Itoa(*producerID) + "-" + strconv.Itoa(counter)
		message := fmt.Sprintf("Producer %d - Message %s", *producerID, messageID)
		counter++

		err := rdb.LPush(ctx, queueKey, message).Err()
		if err != nil {
			fmt.Println("向 Redis 队列推送消息时出错:", err)
			return
		}
		fmt.Println("成功推送消息:", message)

		time.Sleep(3 * time.Second) // 每隔 3 秒推送一条消息
	}
}
