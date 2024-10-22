package main

import (
	"fmt"
	"log"
	"time"

	"github.com/redis/go-redis/v9"
)

// redis 作为 mysql 的 cache，每次查询，先查 redis
// 如果查不到，再查 db，查询结果写入 redis
// 更新，先删除 redis，再更新 db，再写入 redis
// 更新 = update/insert，删除，先删除 redis，再删除 db

func main() {
	// 初始化 Redis 和 MySQL 客户端
	rdb := NewRedisClient()
	if err := InitDB(); err != nil {
		log.Fatal("数据库初始化失败:", err)
	}
	defer rdb.Close()

	// 插入或更新数据的操作
	process := Process{
		ProcName:  "test_process",
		Cmdline:   "/usr/bin/test_process",
		ExePath:   "/usr/bin/test_process",
		StartTime: 1638386400,
		NumVmas:   10,
	}

	// 执行插入或更新操作（更新与插入流程一致）
	err := UpsertProcessWithCache(rdb, &process)
	if err != nil {
		log.Fatal("插入/更新数据失败:", err)
	}
	fmt.Printf("插入/更新的数据的 PID 为: %d\n", process.PID)

	// 查询数据的操作
	pid := process.PID
	processFromDB, err := GetProcessWithCache(rdb, pid)
	if err != nil {
		log.Fatal("查询失败:", err)
	}
	fmt.Printf("获取到的 Process: %+v\n", processFromDB)

	// 删除数据的操作
	err = DeleteProcessWithCache(rdb, pid)
	if err != nil {
		log.Fatal("删除失败:", err)
	}
	fmt.Println("删除成功！")
}

// 查询数据，先查 Redis，再查 MySQL，并将结果写入 Redis
func GetProcessWithCache(rdb *redis.Client, pid int) (*Process, error) {
	cacheKey := fmt.Sprintf("process:%d", pid)

	// 尝试从 Redis 缓存中获取数据
	cachedData, err := GetFromCache(rdb, cacheKey)
	if err == nil {
		// 从 Redis 获取到缓存
		return FromJSON(cachedData)
	}

	// Redis 中没有，从 MySQL 获取数据
	process, err := GetProcess(pid)
	if err != nil {
		return nil, err
	}

	// 将从 MySQL 获取的数据写入 Redis 缓存
	jsonData, err := process.ToJSON()
	if err != nil {
		return nil, err
	}
	if err := SetToCache(rdb, cacheKey, jsonData, 10*time.Minute); err != nil {
		return nil, err
	}

	return process, nil
}

// 插入或更新数据，先删除 Redis 缓存，再更新 MySQL 并将结果写入 Redis
func UpsertProcessWithCache(rdb *redis.Client, process *Process) error {
	cacheKey := fmt.Sprintf("process:%d", process.PID)

	// 删除 Redis 缓存
	if err := DeleteFromCache(rdb, cacheKey); err != nil {
		return err
	}

	// 更新或插入 MySQL 数据
	if process.PID == 0 {
		// 插入数据
		id, err := InsertProcess(*process)
		if err != nil {
			return err
		}
		process.PID = int(id)
	} else {
		// 更新数据
		if err := UpdateProcess(*process); err != nil {
			return err
		}
	}

	// 将更新后的数据写入 Redis 缓存
	jsonData, err := process.ToJSON()
	if err != nil {
		return err
	}
	return SetToCache(rdb, cacheKey, jsonData, 10*time.Minute)
}

// 删除数据，先删除 Redis 缓存，再删除 MySQL 数据
func DeleteProcessWithCache(rdb *redis.Client, pid int) error {
	cacheKey := fmt.Sprintf("process:%d", pid)

	// 删除 Redis 缓存
	if err := DeleteFromCache(rdb, cacheKey); err != nil {
		return err
	}

	// 删除 MySQL 数据
	return DeleteProcess(pid)
}
