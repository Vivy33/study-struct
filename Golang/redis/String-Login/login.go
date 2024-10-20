package main

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

// Redis 客户端上下文
var ctx = context.Background()

// 定义用户登录状态结构体
type LoginStatus struct {
	UserID    string    `json:"user_id"`
	LoginTime time.Time `json:"login_time"`
}

// 初始化 Redis 客户端
func newRedisClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:     "localhost:6379", // Redis 服务器地址
		Password: "",               // 没有密码则留空
		DB:       0,                // 使用默认数据库
	})
}

// 设置用户登录状态，将其序列化为 JSON 并存储到 Redis
func setLoginStatus(rdb *redis.Client, userID string, status LoginStatus, expiration time.Duration) error {
	// 序列化登录状态为 JSON
	statusJSON, err := json.Marshal(status)
	if err != nil {
		return fmt.Errorf("序列化用户 %s 的登录状态失败: %v", userID, err)
	}

	// Redis 键名
	key := fmt.Sprintf("user:login:%s", userID)

	// 存储到 Redis 并设置过期时间
	err = rdb.Set(ctx, key, statusJSON, expiration).Err()
	if err != nil {
		return fmt.Errorf("存储用户 %s 的登录状态失败: %v", userID, err)
	}

	return nil
}

// 从 Redis 获取用户登录状态
func getLoginStatus(rdb *redis.Client, userID string) (*LoginStatus, error) {
	// Redis 键名
	key := fmt.Sprintf("user:login:%s", userID)

	// 从 Redis 获取 JSON 字符串
	statusJSON, err := rdb.Get(ctx, key).Result()
	if err == redis.Nil {
		// 如果键不存在，说明状态已过期或从未设置
		return nil, nil
	} else if err != nil {
		return nil, fmt.Errorf("获取用户 %s 的登录状态失败: %v", userID, err)
	}

	// 反序列化 JSON 字符串为 LoginStatus 对象
	var status LoginStatus
	err = json.Unmarshal([]byte(statusJSON), &status)
	if err != nil {
		return nil, fmt.Errorf("反序列化用户 %s 的登录状态失败: %v", userID, err)
	}

	return &status, nil
}

func main() {
	// 创建 Redis 客户端
	rdb := newRedisClient()

	// 模拟一个用户 ID
	userID := "12345"

	// 创建一个登录状态
	loginStatus := LoginStatus{
		UserID:    userID,
		LoginTime: time.Now(),
	}

	// 设置用户登录状态，过期时间为 30 分钟
	expiration := 30 * time.Minute
	err := setLoginStatus(rdb, userID, loginStatus, expiration)
	if err != nil {
		fmt.Println(err)
		return
	}
	fmt.Println("用户登录状态已设置，过期时间为 30 分钟。")

	// 从 Redis 获取用户登录状态
	storedStatus, err := getLoginStatus(rdb, userID)
	if err != nil {
		fmt.Println(err)
		return
	}

	// 判断是否获取到状态
	if storedStatus == nil {
		fmt.Println("用户登录状态已过期或不存在。")
	} else {
		fmt.Printf("获取到的用户登录状态: %+v\n", storedStatus)
	}
}
