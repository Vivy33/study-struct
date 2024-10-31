package main

import (
	"database/sql"
	"fmt"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

// 初始化数据库连接并设置连接池
func InitDB() (*sql.DB, error) {
	dsn := fmt.Sprintf(
		"%s:%s@tcp(%s:%d)/%s?charset=%s&parseTime=True&loc=Local",
		"root", "admin", "127.0.0.1", 3307, "go_test", "utf8",
	)
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

// 插入订单到数据库，使用事务
func InsertOrder(db *sql.DB, order *Order) (int64, error) {
	tx, err := db.Begin()
	if err != nil {
		return 0, fmt.Errorf("无法开始事务: %v", err)
	}

	query := "INSERT INTO orders (user_id, shop_id, status, total_price) VALUES (?, ?, ?, ?)"
	result, err := tx.Exec(query, order.UserID, order.ShopID, order.OrderStatus, order.TotalPrice)
	if err != nil {
		tx.Rollback()
		return 0, fmt.Errorf("订单插入失败: %v", err)
	}

	orderID, err := result.LastInsertId()
	if err != nil {
		tx.Rollback()
		return 0, fmt.Errorf("获取订单ID失败: %v", err)
	}

	return orderID, tx.Commit()
}

// 更新订单状态，使用事务
func UpdateOrderStatus(db *sql.DB, orderID int, status string) error {
	tx, err := db.Begin()
	if err != nil {
		return fmt.Errorf("无法开始事务: %v", err)
	}

	query := "UPDATE orders SET status = ? WHERE order_id = ?"
	_, err = tx.Exec(query, status, orderID)
	if err != nil {
		tx.Rollback()
		return fmt.Errorf("订单状态更新失败: %v", err)
	}

	return tx.Commit()
}

// 删除订单，使用事务
func DeleteOrder(db *sql.DB, orderID int) error {
	tx, err := db.Begin()
	if err != nil {
		return fmt.Errorf("无法开始事务: %v", err)
	}

	query := "DELETE FROM orders WHERE order_id = ?"
	_, err = tx.Exec(query, orderID)
	if err != nil {
		tx.Rollback()
		return fmt.Errorf("订单删除失败: %v", err)
	}

	return tx.Commit()
}

// 根据订单ID查询订单状态
func QueryOrderStatus(db *sql.DB, orderID int) (string, error) {
	query := "SELECT status FROM orders WHERE order_id = ?"
	row := db.QueryRow(query, orderID)

	var status string
	if err := row.Scan(&status); err != nil {
		return "", fmt.Errorf("查询订单状态失败: %v", err)
	}

	return status, nil
}

// 查询商家的商品列表
func QueryProductsByShopID(db *sql.DB, shopID int) ([]Product, error) {
	query := "SELECT product_id, name, description, price, stock FROM products WHERE shop_id = ?"
	rows, err := db.Query(query, shopID)
	if err != nil {
		return nil, fmt.Errorf("查询商品失败: %v", err)
	}
	defer rows.Close()

	var products []Product
	for rows.Next() {
		var product Product
		if err := rows.Scan(&product.ProductID, &product.ProductName, &product.Description, &product.Price, &product.Stock); err != nil {
			return nil, err
		}
		products = append(products, product)
	}
	return products, nil
}

// 查询附近的商家
func QueryShops(db *sql.DB, limit int) ([]Shop, error) {
	query := "SELECT shop_id, name, phone, address, description FROM shops LIMIT ?"
	rows, err := db.Query(query, limit)
	if err != nil {
		return nil, fmt.Errorf("查询商家失败: %v", err)
	}
	defer rows.Close()

	var shops []Shop
	for rows.Next() {
		var shop Shop
		if err := rows.Scan(&shop.ShopID, &shop.ShopName, &shop.Phone, &shop.Address, &shop.Description); err != nil {
			return nil, err
		}
		shops = append(shops, shop)
	}
	return shops, nil
}
