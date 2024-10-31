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

// Rider 表示骑手的数据结构
type Rider struct {
	User                // 内嵌 User 结构体，继承用户的基本字段
	RiderID     int     `json:"rider_id"`     // 骑手ID，区分于 UserID
	VehicleType string  `json:"vehicle_type"` // 骑手使用的交通工具类型
	Rating      float64 `json:"rating"`       // 骑手的评分
	RiderStatus string  `json:"riderstatus"`  // 骑手状态（如在线、休息、离线）
}

// 商家结构体
type Shop struct {
	ShopID      int    `json:"shop_id"`
	ShopName    string `json:"shopname"`
	Phone       string `json:"phone"`
	Address     string `json:"address"`
	Description string `json:"description"`
}

// 商品结构体
type Product struct {
	ProductID   int     `json:"product_id"`
	ShopID      int     `json:"shop_id"`
	ProductName string  `json:"name"`
	Description string  `json:"description"`
	Price       float64 `json:"price"`
	Stock       int     `json:"stock"`
}

// 订单结构体
type Order struct {
	OrderID     int       `json:"order_id"`
	UserID      int       `json:"user_id"`
	ShopID      int       `json:"shop_id"`
	RiderID     int       `json:"rider_id"`
	ProductID   int       `json:"product_id"`
	OrderStatus string    `json:"orderstatus"`
	OrderTime   time.Time `json:"order_time"`
	TotalPrice  float64   `json:"total_price"`
	GroupID     int       `json:"group_id,omitempty"`
}

// Group
type Group struct {
	GroupID int `json:"group_id,omitempty"`
	OrderID int `json:"order_id"`
	UserID  int `json:"user_id"`
	ShopID  int `json:"shop_id"`
	RiderID int `json:"rider_id"`
}

// Message 表示一条群聊消息的结构体
type Message struct {
	GroupID   int       `json:"group_id"`  // 群组ID
	SenderID  int       `json:"sender_id"` // 发送者ID
	Content   string    `json:"content"`   // 消息内容
	Timestamp time.Time `json:"timestamp"` // 消息时间戳
}
