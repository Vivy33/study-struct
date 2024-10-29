package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"time"
)

// Redis 和 MySQL 初始化
var (
	rp *RedisPool // Redis连接池
	db *sql.DB    // MySQL数据库连接
)

func init() {
	// 初始化 Redis 连接池
	rp = NewRedisPool()

	// 初始化 MySQL 数据库
	var err error
	db, err = InitDB()
	if err != nil {
		log.Fatal("数据库初始化失败:", err)
	}
}

// 用户注册
func handleRegister(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var user User
	if err := json.NewDecoder(r.Body).Decode(&user); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 检查用户名是否已存在
	var exists bool
	err := db.QueryRow("SELECT EXISTS(SELECT 1 FROM users WHERE username = ?)", user.Username).Scan(&exists)
	if err != nil {
		http.Error(w, "用户名检查失败", http.StatusInternalServerError)
		return
	}
	if exists {
		http.Error(w, "用户名已存在，请选择其他用户名", http.StatusConflict)
		return
	}

	// 插入用户数据
	userID, err := insertUser(db, &user)
	if err != nil {
		http.Error(w, fmt.Sprintf("用户注册失败: %v", err), http.StatusInternalServerError)
		return
	}

	user.UserID = int(userID)
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(user)
}

// 插入用户到数据库
func insertUser(db *sql.DB, user *User) (int64, error) {
	query := "INSERT INTO users (username, password, phone, address) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, user.Username, user.Password, user.Phone, user.Address)
	if err != nil {
		return 0, fmt.Errorf("用户插入失败: %v", err)
	}
	return result.LastInsertId()
}

// 验证用户凭据
func ValidateUser(db *sql.DB, username, password string) (*User, error) {
	query := "SELECT user_id, username, password, phone, address FROM users WHERE username = ?"
	row := db.QueryRow(query, username)

	var user User
	if err := row.Scan(&user.UserID, &user.Username, &user.Password, &user.Phone, &user.Address); err != nil {
		if err == sql.ErrNoRows {
			return nil, fmt.Errorf("用户不存在")
		}
		return nil, fmt.Errorf("查询用户失败: %v", err)
	}

	// 简单的密码验证
	if user.Password == password {
		return &user, nil
	}
	return nil, fmt.Errorf("密码错误")
}

// 骑手身份申请
func handleApplyForRider(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var request struct {
		UserID      int     `json:"user_id"`
		VehicleType string  `json:"vehicle_type"`
		Status      string  `json:"status"`
		Rating      float64 `json:"rating"`
	}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 验证用户是否存在
	var exists bool
	err := db.QueryRow("SELECT EXISTS(SELECT 1 FROM users WHERE user_id = ?)", request.UserID).Scan(&exists)
	if err != nil || !exists {
		http.Error(w, "用户不存在", http.StatusBadRequest)
		return
	}

	// 为用户生成 RiderID 并插入骑手数据
	rider := Rider{
		User:        User{UserID: request.UserID},
		VehicleType: request.VehicleType,
		Rating:      request.Rating,
		Status:      request.Status,
	}

	riderID, err := insertRider(db, &rider)
	if err != nil {
		http.Error(w, fmt.Sprintf("骑手身份申请失败: %v", err), http.StatusInternalServerError)
		return
	}
	rider.RiderID = int(riderID)

	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(rider)
}

func insertRider(db *sql.DB, rider *Rider) (int64, error) {
	query := "INSERT INTO riders (user_id, vehicle_type, rating, status) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, rider.UserID, rider.VehicleType, rider.Rating, rider.Status)
	if err != nil {
		return 0, fmt.Errorf("骑手插入失败: %v", err)
	}
	return result.LastInsertId()
}

// 查询商家商品
func handleShopProducts(w http.ResponseWriter, r *http.Request) {
	shopIDStr := r.URL.Query().Get("shop_id")
	shopID, err := strconv.Atoi(shopIDStr)
	if err != nil {
		http.Error(w, "无效的商家ID", http.StatusBadRequest)
		return
	}

	cacheKey := fmt.Sprintf("shop_products_%d", shopID)
	data, err := GetFromCache(rp, cacheKey)
	if err == nil {
		w.Write([]byte(data))
		return
	}

	// 缓存未命中，从数据库查询
	products, err := QueryProductsByShopID(db, shopID)
	if err != nil {
		http.Error(w, fmt.Sprintf("查询商品失败: %v", err), http.StatusInternalServerError)
		return
	}

	// 写入 Redis 缓存
	jsonData, _ := json.Marshal(products)
	SetToCache(rp, cacheKey, string(jsonData), time.Hour)
	w.Write(jsonData)
}

// 获取附近的商家
func handleNearbyShops(w http.ResponseWriter, r *http.Request) {
	cacheKey := "nearby_shops"
	data, err := GetFromCache(rp, cacheKey)
	if err == nil {
		w.Write([]byte(data))
		return
	}

	// 随机返回 N 个商家
	shops, err := QueryShops(db, 5)
	if err != nil {
		http.Error(w, fmt.Sprintf("查询商家失败: %v", err), http.StatusInternalServerError)
		return
	}

	jsonData, _ := json.Marshal(shops)
	SetToCache(rp, cacheKey, string(jsonData), time.Hour)
	w.Write(jsonData)
}

// 提交订单并更新 Redis 缓存
func handleOrder(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var order Order
	if err := json.NewDecoder(r.Body).Decode(&order); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 删除与订单相关的 Redis 缓存
	DeleteFromCache(rp, fmt.Sprintf("order_status_%d", order.OrderID))

	// 插入订单到数据库
	orderID, err := InsertOrder(db, &order)
	if err != nil {
		http.Error(w, fmt.Sprintf("订单插入失败: %v", err), http.StatusInternalServerError)
		return
	}

	order.OrderID = int(orderID)

	// 更新 Redis 缓存
	jsonData, _ := json.Marshal(order)
	SetToCache(rp, fmt.Sprintf("order_status_%d", order.OrderID), string(jsonData), time.Hour)

	json.NewEncoder(w).Encode(order)
}

// 查询订单状态
func handleOrderStatus(w http.ResponseWriter, r *http.Request) {
	orderIDStr := r.URL.Query().Get("order_id")
	orderID, err := strconv.Atoi(orderIDStr)
	if err != nil {
		http.Error(w, "无效的订单ID", http.StatusBadRequest)
		return
	}

	cacheKey := fmt.Sprintf("order_status_%d", orderID)
	data, err := GetFromCache(rp, cacheKey)
	if err == nil {
		w.Write([]byte(data))
		return
	}

	// 缓存未命中，从数据库查询订单状态
	status, err := QueryOrderStatus(db, orderID)
	if err != nil {
		http.Error(w, fmt.Sprintf("查询订单状态失败: %v", err), http.StatusInternalServerError)
		return
	}

	jsonData, _ := json.Marshal(map[string]string{"status": status})
	SetToCache(rp, cacheKey, string(jsonData), time.Hour)
	w.Write(jsonData)
}

// 用户登录
func handleLogin(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var user User
	if err := json.NewDecoder(r.Body).Decode(&user); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 使用 ValidateUser 函数检查用户凭据
	validatedUser, err := ValidateUser(db, user.Username, user.Password)
	if err != nil {
		http.Error(w, fmt.Sprintf("登录失败: %v", err), http.StatusUnauthorized)
		return
	}

	// 返回登录成功信息
	json.NewEncoder(w).Encode(map[string]string{"status": "登录成功", "username": validatedUser.Username})
}

// HTTP 服务启动
func main() {
	http.HandleFunc("/order", handleOrder)              // 订外卖
	http.HandleFunc("/shops", handleNearbyShops)        // 获取附近商家
	http.HandleFunc("/products", handleShopProducts)    // 查询商家商品
	http.HandleFunc("/order/status", handleOrderStatus) // 查询订单状态
	http.HandleFunc("/user/register", handleRegister)   // 用户注册
	http.HandleFunc("/user/login", handleLogin)         // 用户登录

	// 即时消息路由
	http.HandleFunc("/im/send", HandleSendMessage(db, rp)) // 发送群组消息
	http.HandleFunc("/im/messages", HandleGetMessages(db)) // 获取群组消息

	log.Println("服务器启动，端口 :8080")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		log.Fatalf("服务器启动失败: %v", err)
	}
}
