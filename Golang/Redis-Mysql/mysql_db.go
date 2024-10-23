package main

import (
	"database/sql"
	"fmt"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

// 初始化数据库连接并设置连接池
func InitDB() (*sql.DB, error) {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=%s&parseTime=True&loc=Local",
		"root", "admin", "127.0.0.1", 3307, "go_test", "utf8")

	db, err := sql.Open("mysql", dsn)
	if err != nil {
		return nil, fmt.Errorf("数据库连接失败: %v", err)
	}

	// 设置数据库连接池的参数
	db.SetMaxOpenConns(100)
	db.SetMaxIdleConns(10)
	db.SetConnMaxLifetime(time.Hour)

	// 检查数据库连接
	if err = db.Ping(); err != nil {
		return nil, fmt.Errorf("数据库连接检查失败: %v", err)
	}

	fmt.Println("数据库连接成功并已配置连接池！")
	return db, nil
}

// 插入数据到数据库，使用事务
func InsertProcess(db *sql.DB, p *Process) (int64, error) {
	// 开始事务
	tx, err := db.Begin()
	if err != nil {
		return 0, fmt.Errorf("无法开始事务: %v", err)
	}

	// 插入数据
	query := "INSERT INTO Process (ProcName, Cmdline, ExePath, StartTime, NumVmas) VALUES (?, ?, ?, ?, ?)"
	result, err := tx.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas)
	if err != nil {
		tx.Rollback() // 出现错误则回滚事务
		return 0, fmt.Errorf("插入失败: %v", err)
	}

	// 提交事务
	if err := tx.Commit(); err != nil {
		return 0, fmt.Errorf("事务提交失败: %v", err)
	}

	return result.LastInsertId()
}

// 更新数据库中的数据，使用事务
func UpdateProcess(db *sql.DB, p *Process) error {
	// 开始事务
	tx, err := db.Begin()
	if err != nil {
		return fmt.Errorf("无法开始事务: %v", err)
	}

	// 更新数据
	query := "UPDATE Process SET ProcName = ?, Cmdline = ?, ExePath = ?, StartTime = ?, NumVmas = ? WHERE PID = ?"
	_, err = tx.Exec(query, p.ProcName, p.Cmdline, p.ExePath, p.StartTime, p.NumVmas, p.PID)
	if err != nil {
		tx.Rollback() // 出现错误则回滚事务
		return fmt.Errorf("更新失败: %v", err)
	}

	// 提交事务
	return tx.Commit()
}

// 删除数据库中的数据，使用事务
func DeleteProcess(db *sql.DB, pid int) error {
	// 开始事务
	tx, err := db.Begin()
	if err != nil {
		return fmt.Errorf("无法开始事务: %v", err)
	}

	// 删除数据
	query := "DELETE FROM Process WHERE PID = ?"
	_, err = tx.Exec(query, pid)
	if err != nil {
		tx.Rollback() // 出现错误则回滚事务
		return fmt.Errorf("删除失败: %v", err)
	}

	// 提交事务
	return tx.Commit()
}

// 根据 PID 获取数据
func QueryProcess(db *sql.DB, pid int) (*Process, error) {
	query := "SELECT PID, ProcName, Cmdline, ExePath, StartTime, NumVmas FROM Process WHERE PID = ?"
	row := db.QueryRow(query, pid)

	var p Process
	if err := row.Scan(&p.PID, &p.ProcName, &p.Cmdline, &p.ExePath, &p.StartTime, &p.NumVmas); err != nil {
		return nil, err
	}
	return &p, nil
}
