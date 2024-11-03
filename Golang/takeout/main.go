package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"time"

	"golang.org/x/crypto/bcrypt"
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

	// 哈希密码
	passwordHash, err := bcrypt.GenerateFromPassword([]byte(user.Password), bcrypt.DefaultCost)
	if err != nil {
		http.Error(w, "密码哈希失败", http.StatusInternalServerError)
		return
	}
	user.Password = string(passwordHash)

	// 插入用户数据
	userID, err := insertUser(rp, db, &user)
	if err != nil {
		http.Error(w, fmt.Sprintf("用户注册失败: %v", err), http.StatusInternalServerError)
		return
	}

	user.UserID = int(userID)
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(user)
}

// handleRegisterShop 处理商家注册的 HTTP 请求
func handleRegisterShop(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var shop Shop
	if err := json.NewDecoder(r.Body).Decode(&shop); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 检查商家名称是否已存在
	var exists bool
	err := db.QueryRow("SELECT EXISTS(SELECT 1 FROM shops WHERE shop_name = ?)", shop.ShopName).Scan(&exists)
	if err != nil {
		http.Error(w, "商家名称检查失败", http.StatusInternalServerError)
		return
	}
	if exists {
		http.Error(w, "商家名称已存在，请选择其他名称", http.StatusConflict)
		return
	}

	// 哈希密码
	passwordHash, err := bcrypt.GenerateFromPassword([]byte(shop.ShopPassword), bcrypt.DefaultCost)
	if err != nil {
		http.Error(w, "密码哈希失败", http.StatusInternalServerError)
		return
	}
	shop.ShopPassword = string(passwordHash)

	// 插入商家数据
	shopID, err := insertShop(rp, db, &shop)
	if err != nil {
		http.Error(w, fmt.Sprintf("商家注册失败: %v", err), http.StatusInternalServerError)
		return
	}

	shop.ShopID = int(shopID)
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(shop)
}

func insertUser(rp *RedisPool, db *sql.DB, user *User) (int64, error) {
	// Insert user into MySQL
	query := "INSERT INTO users (username, password, phone, address) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, user.Username, user.Password, user.Phone, user.Address)
	if err != nil {
		return 0, fmt.Errorf("failed to insert user into MySQL: %v", err)
	}
	userID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get user ID from MySQL: %v", err)
	}

	// Insert user into Redis
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.HMSet(ctx, fmt.Sprintf("user:%d", userID), map[string]interface{}{
		"user_id":  userID,
		"username": user.Username,
		"password": user.Password,
		"phone":    user.Phone,
		"address":  user.Address,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert user into Redis: %v", err)
	}

	return userID, nil
}

func insertShop(rp *RedisPool, db *sql.DB, shop *Shop) (int64, error) {
	// Insert shop into MySQL
	query := "INSERT INTO Shop (ShopName, Phone, Address, Description) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, shop.ShopName, shop.Phone, shop.Address, shop.Description)
	if err != nil {
		return 0, fmt.Errorf("failed to insert shop into MySQL: %v", err)
	}
	shopID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get shop ID from MySQL: %v", err)
	}

	// Insert shop into Redis
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.HMSet(ctx, fmt.Sprintf("shop:%d", shopID), map[string]interface{}{
		"shop_id":     shopID,
		"shop_name":   shop.ShopName,
		"phone":       shop.Phone,
		"address":     shop.Address,
		"description": shop.Description,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert shop into Redis: %v", err)
	}

	return shopID, nil
}

func insertRider(rp *RedisPool, db *sql.DB, rider *Rider) (int64, error) {
	// Insert rider into MySQL
	query := "INSERT INTO riders (user_id, vehicle_type, rating, riderstatus) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, rider.UserID, rider.VehicleType, rider.Rating, rider.RiderStatus)
	if err != nil {
		return 0, fmt.Errorf("failed to insert rider into MySQL: %v", err)
	}
	riderID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get rider ID from MySQL: %v", err)
	}

	// Insert rider into Redis
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.HMSet(ctx, fmt.Sprintf("rider:%d", riderID), map[string]interface{}{
		"user_id":      rider.UserID,
		"rider_id":     riderID,
		"username":     rider.Username,
		"password":     rider.Password,
		"phone":        rider.Phone,
		"address":      rider.Address,
		"vehicle_type": rider.VehicleType,
		"rating":       rider.Rating,
		"riderstatus":  rider.RiderStatus,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert rider into Redis: %v", err)
	}

	return riderID, nil
}

func insertOrder(rp *RedisPool, db *sql.DB, order *Order) (int64, error) {
	// Insert order into MySQL
	query := "INSERT INTO orders (user_id, shop_id, product_id, orderstatus, order_time, total_price) VALUES (?, ?, ?, ?, ?, ?)"
	result, err := db.Exec(query, order.UserID, order.ShopID, order.ProductID, order.OrderStatus, order.OrderTime, order.TotalPrice)
	if err != nil {
		return 0, fmt.Errorf("failed to insert order into MySQL: %v", err)
	}
	orderID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get order ID from MySQL: %v", err)
	}

	// Insert order into Redis
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.HMSet(ctx, fmt.Sprintf("order:%d", orderID), map[string]interface{}{
		"order_id":    orderID,
		"user_id":     order.UserID,
		"shop_id":     order.ShopID,
		"product_id":  order.ProductID,
		"orderstatus": order.OrderStatus,
		"order_time":  order.OrderTime.Format(time.RFC3339),
		"total_price": order.TotalPrice,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert order into Redis: %v", err)
	}

	return orderID, nil
}

func insertGroup(rp *RedisPool, db *sql.DB, group *Group) (int64, error) {
	// Insert group into MySQL
	query := "INSERT INTO Groups (order_id, user_id, shop_id, rider_id) VALUES (?, ?, ?, ?)"
	result, err := db.Exec(query, group.OrderID, group.UserID, group.ShopID, group.RiderID)
	if err != nil {
		return 0, fmt.Errorf("failed to insert group into MySQL: %v", err)
	}
	groupID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get group ID from MySQL: %v", err)
	}

	// Insert group into Redis
	ctx := context.Background()
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.HMSet(ctx, fmt.Sprintf("group:%d", groupID), map[string]interface{}{
		"group_id": groupID,
		"order_id": group.OrderID,
		"user_id":  group.UserID,
		"shop_id":  group.ShopID,
		"rider_id": group.RiderID,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert group into Redis: %v", err)
	}

	return groupID, nil
}

// 验证用户凭据
func ValidateUser(db *sql.DB, username, password string) (*User, error) {
	var user User
	err := db.QueryRow("SELECT user_id, username, password FROM users WHERE username = ?", username).Scan(&user.UserID, &user.Username, &user.Password)
	if err != nil {
		if err == sql.ErrNoRows {
			return nil, fmt.Errorf("用户不存在")
		}
		return nil, err
	}

	// 比较哈希后的密码和输入的密码
	err = bcrypt.CompareHashAndPassword([]byte(user.Password), []byte(password))
	if err != nil {
		return nil, fmt.Errorf("密码错误")
	}

	return &user, nil
}

func ValidateShop(shop *Shop) error {
	if shop.ShopName == "" {
		return fmt.Errorf("商家名称不能为空")
	}
	if shop.Address == "" {
		return fmt.Errorf("商家地址不能为空")
	}
	if shop.Phone == "" {
		return fmt.Errorf("商家电话不能为空")
	}

	return nil
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
		RiderStatus: request.Status,
	}

	riderID, err := insertRider(rp, db, &rider)
	if err != nil {
		http.Error(w, fmt.Sprintf("骑手身份申请失败: %v", err), http.StatusInternalServerError)
		return
	}
	rider.RiderID = int(riderID)

	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(rider)
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

	// 插入订单到数据库
	orderID, err := insertOrder(rp, db, &order)
	if err != nil {
		http.Error(w, fmt.Sprintf("订单插入失败: %v", err), http.StatusInternalServerError)
		return
	}

	order.OrderID = int(orderID)

	// 更新 Redis 缓存
	jsonData, _ := json.Marshal(order)
	SetToCache(rp, fmt.Sprintf("order_status_%d", order.OrderID), string(jsonData), time.Hour)

	// 返回订单详情和订单ID
	w.WriteHeader(http.StatusCreated)
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

	var loginRequest struct {
		Username string `json:"username"`
		Password string `json:"password"`
	}
	if err := json.NewDecoder(r.Body).Decode(&loginRequest); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 使用 ValidateUser 函数检查用户凭据
	validatedUser, err := ValidateUser(db, loginRequest.Username, loginRequest.Password)
	if err != nil {
		http.Error(w, fmt.Sprintf("登录失败: %v", err), http.StatusUnauthorized)
		return
	}

	// 返回登录成功信息
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":   "登录成功",
		"username": validatedUser.Username,
		"user_id":  validatedUser.UserID,
	})
}

func handleLoginShop(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var credentials struct {
		ShopName string `json:"shop_name"`
		Password string `json:"password"`
	}

	if err := json.NewDecoder(r.Body).Decode(&credentials); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	var shop Shop
	err := db.QueryRow("SELECT ShopID, ShopName, Address, Phone, ShopPassword FROM Shop WHERE ShopName = ?", credentials.ShopName).Scan(
		&shop.ShopID, &shop.ShopName, &shop.Address, &shop.Phone, &shop.ShopPassword,
	)
	if err == sql.ErrNoRows {
		http.Error(w, "商家名称或密码错误", http.StatusUnauthorized)
		return
	} else if err != nil {
		http.Error(w, "查询商家信息失败", http.StatusInternalServerError)
		return
	}

	// 验证密码
	if err := bcrypt.CompareHashAndPassword([]byte(shop.ShopPassword), []byte(credentials.Password)); err != nil {
		http.Error(w, "商家名称或密码错误", http.StatusUnauthorized)
		return
	}

	// 登录成功
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(shop)
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
	http.HandleFunc("/im/send", HandleSendMessage(db, rp))     // 发送群组消息
	http.HandleFunc("/im/messages", HandleGetMessages(db, rp)) // 获取群组消息

	log.Println("服务器启动，端口 :8080")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		log.Fatalf("服务器启动失败: %v", err)
	}
}
