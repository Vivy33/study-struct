package main

import (
	"database/sql"
	"fmt"
	"log"

	_ "github.com/go-sql-driver/mysql"
)

// 定义数据库连接的相关参数
var (
	userName  string  = "root"      // 连接数据库的用户名
	password  string  = "admin"     // 连接数据库的密码
	ipAddress string  = "127.0.0.1" // 连接数据库的地址
	port      int     = 3307        // 连接数据库的端口号
	dbName    string  = "go_test"   // 连接数据库的具体数据库名称
	charset   string  = "utf8"      // 连接数据库的编码格式
	db        *sql.DB               // 全局数据库对象
)

// Process 结构体用于映射数据库中的表
type Process struct {
	PID       int    `json:"pid"`        // 进程ID
	ProcName  string `json:"proc_name"`  // 进程名称
	Cmdline   string `json:"cmdline"`    // 命令行
	ExePath   string `json:"exe_path"`   // 可执行文件路径
	StartTime uint64 `json:"start_time"` // 启动时间
	NumVmas   int    `json:"num_vmas"`   // VMA数量
}

// initDB 初始化数据库连接
func initDB() error {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=%s&parseTime=True&loc=Local",
		userName, password, ipAddress, port, dbName, charset)

	var err error
	db, err = sql.Open("mysql", dsn)
	if err != nil {
		return fmt.Errorf("数据库连接失败: %v", err)
	}

	// 检查数据库连接是否成功
	if err = db.Ping(); err != nil {
		return fmt.Errorf("数据库连接检查失败: %v", err)
	}
	fmt.Println("数据库连接成功！")
	return nil
}

// insertProcess 插入进程数据
func insertProcess(p Process) (int64, error) {
	query := "INSERT INTO Process (ProcName, Cmdline, ExePath, StartTime, NumVmas) VALUES (?, ?, ?, ?, ?)"
	result, err := db.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas)
	if err != nil {
		return 0, err
	}
	return result.LastInsertId()
}

// getProcess 获取进程数据
func getProcess(pid int) (*Process, error) {
	query := "SELECT PID, ProcName, Cmdline, ExePath, StartTime, NumVmas FROM Process WHERE PID = ?"
	row := db.QueryRow(query, pid)
	var p Process
	if err := row.Scan(&p.PID, &p.ProcName, &p.Cmdline, &p.ExePath, &p.StartTime, &p.NumVmas); err != nil {
		return nil, err
	}
	return &p, nil
}

// updateProcess 更新进程数据
func updateProcess(p Process) (int64, error) {
	query := "UPDATE Process SET ProcName = ?, Cmdline = ?, ExePath = ?, StartTime = ?, NumVmas = ? WHERE PID = ?"
	result, err := db.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas, p.PID)
	if err != nil {
		return 0, err
	}
	return result.RowsAffected()
}

// deleteProcess 删除进程数据
func deleteProcess(pid int) (int64, error) {
	query := "DELETE FROM Process WHERE PID = ?"
	result, err := db.Exec(query, pid)
	if err != nil {
		return 0, err
	}
	return result.RowsAffected()
}

func main() {
	// 初始化数据库连接
	err := initDB()
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// 插入数据
	process := Process{
		ProcName:  "example",
		Cmdline:   "/usr/bin/example",
		ExePath:   "/usr/bin/example",
		StartTime: 1638386400,
		NumVmas:   10,
	}
	id, err := insertProcess(process)
	if err != nil {
		log.Fatal("Insert Error:", err)
	}
	fmt.Printf("Insert Process ID: %d\n", id)

	// 查询数据
	p, err := getProcess(int(id))
	if err != nil {
		log.Fatal("Get Error:", err)
	}
	fmt.Printf("Get Process: %+v\n", p)

	// 更新数据
	p.NumVmas = 20
	_, err = updateProcess(*p)
	if err != nil {
		log.Fatal("Update Error:", err)
	}
	fmt.Printf("Update Process ID: %d\n", p.PID)

	// 删除数据
	_, err = deleteProcess(p.PID)
	if err != nil {
		log.Fatal("Delete Error:", err)
	}
	fmt.Printf("Delete Process ID: %d\n", p.PID)
}
