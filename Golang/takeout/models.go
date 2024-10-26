package main

import "time"

// 用户结构体
type User struct {
	UserID   int    `json:"user_id"`
	Username string `json:"username"`
	Password string `json:"password"`
	Phone    string `json:"phone"`
	Address  string `json:"address"`
}

// 商家结构体
type Shop struct {
	ShopID      int    `json:"shop_id"`
	Name        string `json:"name"`
	Phone       string `json:"phone"`
	Address     string `json:"address"`
	Description string `json:"description"`
}

// 商品结构体
type Product struct {
	ProductID   int     `json:"product_id"`
	ShopID      int     `json:"shop_id"`
	Name        string  `json:"name"`
	Description string  `json:"description"`
	Price       float64 `json:"price"`
	Stock       int     `json:"stock"`
}

// 订单结构体
type Order struct {
	OrderID    int       `json:"order_id"`
	UserID     int       `json:"user_id"`
	ShopID     int       `json:"shop_id"`
	Status     string    `json:"status"`
	OrderTime  time.Time `json:"order_time"`
	TotalPrice float64   `json:"total_price"`
}
