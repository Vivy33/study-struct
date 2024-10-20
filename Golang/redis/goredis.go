package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"

	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

// 定义进程结构体
type Process struct {
	PID       int    `json:"pid"`        // 进程ID
	ProcName  string `json:"proc_name"`  // 进程名称
	Cmdline   string `json:"cmdline"`    // 命令行
	ExePath   string `json:"exe_path"`   // 可执行文件路径
	StartTime uint64 `json:"start_time"` // 启动时间
	NumVmas   int    `json:"num_vmas"`   // VMA数量
}

func main() {
	// 创建 Redis 客户端
	rdb := redis.NewClient(&redis.Options{
		Addr:     "localhost:6379",
		Password: "",
		DB:       0,
	})

	// 测试连接
	pong, err := rdb.Ping(ctx).Result()
	if err != nil {
		fmt.Println("无法连接到 Redis:", err)
		os.Exit(1)
	}
	fmt.Println("连接成功:", pong)

	// 从终端获取用户输入的进程信息
	process := Process{}
	fmt.Print("请输入进程 ID: ")
	fmt.Scanln(&process.PID)
	fmt.Print("请输入进程名称: ")
	fmt.Scanln(&process.ProcName)
	fmt.Print("请输入命令行: ")
	fmt.Scanln(&process.Cmdline)
	fmt.Print("请输入可执行文件路径: ")
	fmt.Scanln(&process.ExePath)
	fmt.Print("请输入启动时间 (Unix 时间戳): ")
	fmt.Scanln(&process.StartTime)
	fmt.Print("请输入 VMA 数量: ")
	fmt.Scanln(&process.NumVmas)

	// 将结构体序列化为 JSON
	processJSON, err := json.Marshal(process)
	if err != nil {
		fmt.Println("JSON 序列化失败:", err)
		return
	}

	// 将 JSON 数据存储到 Redis
	err = rdb.Set(ctx, fmt.Sprintf("process:%d", process.PID), processJSON, 0).Err()
	if err != nil {
		fmt.Println("写入 Redis 失败:", err)
		return
	}
	fmt.Println("数据已存入 Redis")

	// 从 Redis 中读取数据
	val, err := rdb.Get(ctx, fmt.Sprintf("process:%d", process.PID)).Result()
	if err != nil {
		fmt.Println("从 Redis 读取数据失败:", err)
		return
	}

	// 将 JSON 反序列化为结构体
	var retrievedProcess Process
	err = json.Unmarshal([]byte(val), &retrievedProcess)
	if err != nil {
		fmt.Println("JSON 反序列化失败:", err)
		return
	}

	// 打印反序列化后的数据
	fmt.Println("从 Redis 中读取的进程信息:")
	fmt.Printf("PID: %d, 进程名称: %s, 命令行: %s, 可执行文件路径: %s, 启动时间: %d, VMA 数量: %d\n",
		retrievedProcess.PID, retrievedProcess.ProcName, retrievedProcess.Cmdline,
		retrievedProcess.ExePath, retrievedProcess.StartTime, retrievedProcess.NumVmas)
}
