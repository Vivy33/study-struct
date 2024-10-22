package main

import (
	"database/sql"
	"fmt"

	_ "github.com/go-sql-driver/mysql"
)

var db *sql.DB

// 初始化数据库连接
func InitDB() error {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=%s&parseTime=True&loc=Local",
		"root", "admin", "127.0.0.1", 3307, "go_test", "utf8")
	var err error
	db, err = sql.Open("mysql", dsn)
	if err != nil {
		return fmt.Errorf("数据库连接失败: %v", err)
	}
	if err = db.Ping(); err != nil {
		return fmt.Errorf("数据库连接检查失败: %v", err)
	}
	fmt.Println("数据库连接成功！")
	return nil
}

// 插入数据到数据库
func InsertProcess(p Process) (int64, error) {
	query := "INSERT INTO Process (ProcName, Cmdline, ExePath, StartTime, NumVmas) VALUES (?, ?, ?, ?, ?)"
	result, err := db.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas)
	if err != nil {
		return 0, err
	}
	return result.LastInsertId()
}

// 更新数据库中的数据
func UpdateProcess(p Process) error {
	query := "UPDATE Process SET ProcName = ?, Cmdline = ?, ExePath = ?, StartTime = ?, NumVmas = ? WHERE PID = ?"
	_, err := db.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas, p.PID)
	return err
}

// 删除数据库中的数据
func DeleteProcess(pid int) error {
	query := "DELETE FROM Process WHERE PID = ?"
	_, err := db.Exec(query, pid)
	return err
}

// 根据 PID 获取数据
func GetProcess(pid int) (*Process, error) {
	query := "SELECT PID, ProcName, Cmdline, ExePath, StartTime, NumVmas FROM Process WHERE PID = ?"
	row := db.QueryRow(query, pid)
	var p Process
	if err := row.Scan(&p.PID, &p.ProcName, &p.Cmdline, &p.ExePath, &p.StartTime, &p.NumVmas); err != nil {
		return nil, err
	}
	return &p, nil
}
