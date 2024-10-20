package main

import (
	"context"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

func main() {
	// 创建 Redis 客户端
	rdb := redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务器地址
		Password: "",               // 没有密码则留空
		DB:       0,                // 使用默认数据库
	})

	queueKey := "my_queue" // 队列的键名
	counter := 1           // 消息计数

	for {
		// 创建要推送的消息
		message := fmt.Sprintf("消息编号 %d", counter)
		counter++

		// 将消息推送到队列
		err := rdb.LPush(ctx, queueKey, message).Err()
		if err != nil {
			fmt.Println("向 Redis 队列推送消息时出错:", err)
			return
		}

		fmt.Println("成功推送消息:", message)

		// 每隔 3 秒推送一条消息
		time.Sleep(3 * time.Second)
	}
}
