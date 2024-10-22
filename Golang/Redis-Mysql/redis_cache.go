package main

import (
	"context"
	"time"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

// 创建 Redis 客户端
func NewRedisClient() *redis.Client {
	return redis.NewClient(&redis.Options{
		Addr:     "localhost:6379",
		Password: "",
		DB:       0,
		PoolSize: 100,
	})
}

// 从 Redis 获取数据
func GetFromCache(rdb *redis.Client, key string) (string, error) {
	return rdb.Get(ctx, key).Result()
}

// 将数据写入 Redis
func SetToCache(rdb *redis.Client, key string, value string, expiration time.Duration) error {
	return rdb.Set(ctx, key, value, expiration).Err()
}

// 删除 Redis 中的键
func DeleteFromCache(rdb *redis.Client, key string) error {
	return rdb.Del(ctx, key).Err()
}
