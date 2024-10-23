package main

import (
	"database/sql"
	"fmt"
	"log"
	"time"
)

// redis 作为 mysql 的 cache，每次查询，先查 redis
// 如果查不到，再查 db，查询结果写入 redis
// 更新，先删除 redis，再更新 db，再写入 redis
// 更新 = update/insert，删除，先删除 redis，再删除 db

func main() {
	// 初始化 Redis 连接池和 MySQL 客户端
	rp := NewRedisPool() // 使用 Redis 连接池
	db, err := InitDB()  // 初始化 MySQL 数据库
	if err != nil {
		log.Fatal("数据库初始化失败:", err)
	}

	// 插入或更新数据的操作
	process := &Process{
		ProcName:  "test_process",
		Cmdline:   "/usr/bin/test_process",
		ExePath:   "/usr/bin/test_process",
		StartTime: 1638386400,
		NumVmas:   10,
	}

	// 执行插入或更新操作（更新与插入流程一致）
	err = UpsertProcessWithCache(db, rp, process)
	if err != nil {
		log.Fatal("插入/更新数据失败:", err)
	}
	fmt.Printf("插入/更新的数据的 PID 为: %d\n", process.PID)

	// 查询数据的操作
	pid := process.PID
	processFromDB, err := GetProcessWithCache(db, rp, pid)
	if err != nil {
		log.Fatal("查询失败:", err)
	}
	fmt.Printf("获取到的 Process: %+v\n", processFromDB)

	// 删除数据的操作
	err = DeleteProcessWithCache(db, rp, pid)
	if err != nil {
		log.Fatal("删除失败:", err)
	}
	fmt.Println("删除成功！")
}

// 查询数据，先查 Redis，再查 MySQL，并将结果写入 Redis
func GetProcessWithCache(db *sql.DB, rp *RedisPool, pid int) (*Process, error) {
	cacheKey := fmt.Sprintf("process:%d", pid)
	// 尝试从 Redis 缓存中获取数据
	cachedData, err := GetFromCache(rp, cacheKey) // 使用 redis_cache.go 中的 GetFromCache 函数
	if err == nil {
		// 从 Redis 获取到缓存
		return FromJSON(cachedData)
	}

	// Redis 中没有，从 MySQL 获取数据
	process, err := QueryProcess(db, pid) // 使用 mysql_db.go 中的 QueryProcess 函数
	if err != nil {
		return nil, err
	}

	// 将从 MySQL 获取的数据写入 Redis 缓存
	jsonData, err := process.ToJSON()
	if err != nil {
		return nil, err
	}
	if err := SetToCache(rp, cacheKey, jsonData, 10*time.Minute); err != nil { // 使用 redis_cache.go 中的 SetToCache 函数
		return nil, err
	}

	return process, nil
}

// 插入或更新数据，先删除 Redis 缓存，再更新 MySQL 并将结果写入 Redis
func UpsertProcessWithCache(db *sql.DB, rp *RedisPool, process *Process) error {
	cacheKey := fmt.Sprintf("process:%d", process.PID)

	// 删除 Redis 缓存
	if err := DeleteFromCache(rp, cacheKey); err != nil { // 使用 redis_cache.go 中的 DeleteFromCache 函数
		return err
	}

	// 更新或插入 MySQL 数据
	if process.PID == 0 {
		// 插入数据
		id, err := InsertProcess(db, process) // 使用 mysql_db.go 中的 InsertProcess 函数
		if err != nil {
			return err
		}
		process.PID = int(id)
	} else {
		// 更新数据
		if err := UpdateProcess(db, process); err != nil { // 使用 mysql_db.go 中的 UpdateProcess 函数
			return err
		}
	}

	// 将更新后的数据写入 Redis 缓存
	jsonData, err := process.ToJSON()
	if err != nil {
		return err
	}
	return SetToCache(rp, cacheKey, jsonData, 10*time.Minute) // 使用 redis_cache.go 中的 SetToCache 函数
}

// 删除数据，先删除 Redis 缓存，再删除 MySQL 数据
func DeleteProcessWithCache(db *sql.DB, rp *RedisPool, pid int) error {
	cacheKey := fmt.Sprintf("process:%d", pid)

	// 删除 Redis 缓存
	if err := DeleteFromCache(rp, cacheKey); err != nil { // 使用 redis_cache.go 中的 DeleteFromCache 函数
		return err
	}

	// 删除 MySQL 数据
	return DeleteProcess(db, pid) // 使用 mysql_db.go 中的 DeleteProcess 函数
}
