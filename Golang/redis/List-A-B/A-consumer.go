package main

import (
	"context"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

// 创建 Redis 客户端的函数，支持重连机制
func newRedisClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务器地址
		Password: "",               // 没有密码则留空
		DB:       0,                // 使用默认数据库
	})
}

// Redis 连接检查，失败时重连
func ensureRedisConnection(rdb *redis.Client) *redis.Client {
	for {
		_, err := rdb.Ping(ctx).Result()
		if err != nil {
			fmt.Println("Redis 连接失效，尝试重新连接...")

			// 重连逻辑
			rdb = newRedisClient()

			// 延迟 5 秒后重试
			time.Sleep(5 * time.Second)
		} else {
			// 连接正常
			return rdb
		}
	}
}

func main() {
	rdb := newRedisClient() // 初始化 Redis 客户端

	queueKey := "my_queue" // 队列的键名

	for {
		// 确保 Redis 连接正常
		rdb = ensureRedisConnection(rdb)

		// 尝试阻塞消费消息，阻塞时间为 5 秒
		result, err := rdb.BRPop(ctx, 5*time.Second, queueKey).Result()

		if err == redis.Nil {
			fmt.Println("等待消息超时，继续等待下一轮")
			continue
		} else if err != nil {
			fmt.Println("Redis 消费消息时出错:", err)

			// 如果发生了 Redis 连接错误，将重新连接
			rdb = ensureRedisConnection(rdb)
			continue
		}

		// 成功消费到消息
		fmt.Println("消费到的消息:", result[1])
	}
}
