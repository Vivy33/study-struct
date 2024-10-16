package main

import (
	"context"
	"fmt"

	"github.com/redis/go-redis/v9"
)

func main() {
	// 创建 Redis 客户端实例
	client := redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务地址
		Password: "",               // 设置 Redis 密码，如果没有密码则为空字符串
		DB:       0,                // 使用默认的数据库
	})

	// 使用 Ping 命令检查连接是否正常
	pong, err := client.Ping(context.Background()).Result()
	if err != nil {
		fmt.Println("Failed to connect to Redis:", err)
		return
	}
	fmt.Println("Connected to Redis:", pong)

	// 关闭连接
	defer client.Close()

	// 连接成功后可以继续执行其他 Redis 操作
}
