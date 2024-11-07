package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/dgrijalva/jwt-go"
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
		log.Printf("用户名检查失败: %v", err)
		http.Error(w, "用户名检查失败", http.StatusInternalServerError)
		return
	}
	if exists {
		log.Printf("用户名已存在: %s", user.Username)
		http.Error(w, "用户名已存在，请选择其他用户名", http.StatusConflict)
		return
	}

	// 哈希密码
	passwordHash, err := bcrypt.GenerateFromPassword([]byte(user.Password), bcrypt.DefaultCost)
	if err != nil {
		log.Printf("密码哈希失败: %v", err)
		http.Error(w, "密码哈希失败", http.StatusInternalServerError)
		return
	}
	user.Password = string(passwordHash)

	// 插入用户数据
	userID, err := insertUser(rp, db, &user)
	if err != nil {
		log.Printf("用户注册失败: %v", err)
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
	query := "INSERT INTO shops (shop_name, shop_password, phone, address, description) VALUES (?, ?, ?, ?, ?)"
	result, err := db.Exec(query, shop.ShopName, shop.ShopPassword, shop.Phone, shop.Address, shop.Description)
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
		"shop_id":       shopID,
		"shop_name":     shop.ShopName,
		"shop_password": shop.ShopPassword,
		"phone":         shop.Phone,
		"address":       shop.Address,
		"description":   shop.Description,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert shop into Redis: %v", err)
	}

	return shopID, nil
}

func insertRider(rp *RedisPool, db *sql.DB, rider *Rider) (int64, error) {
	// Insert rider into MySQL
	query := "INSERT INTO riders (user_id, vehicle_type, rating, rider_status) VALUES (?, ?, ?, ?)"
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
	query := "INSERT INTO orders (user_id, shop_id, product_id, order_status, order_time, total_price) VALUES (?, ?, ?, ?, ?, ?)"
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
		"order_id":     orderID,
		"user_id":      order.UserID,
		"shop_id":      order.ShopID,
		"product_id":   order.ProductID,
		"order_status": order.OrderStatus,
		"order_time":   order.OrderTime.Format(time.RFC3339),
		"total_price":  order.TotalPrice,
	}).Err()
	if err != nil {
		return 0, fmt.Errorf("failed to insert order into Redis: %v", err)
	}

	return orderID, nil
}

// insertGroup 插入群组信息到数据库和Redis，使用传入的事务对象
func insertGroup(tx *sql.Tx, rp *RedisPool, group *Group) (int64, error) {
	// Insert group into MySQL using the provided transaction
	query := "INSERT INTO `groups` (order_id, user_id, shop_id, rider_id) VALUES (?, ?, ?, ?)"
	result, err := tx.Exec(query, group.OrderID, group.UserID, group.ShopID, group.RiderID)
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

// 生成JWT Token，并存储Token到Redis
func generateTokenAndStoreInRedis(rp *RedisPool, userID int) (string, error) {
	claims := jwt.MapClaims{
		"user_id": userID,
		"exp":     time.Now().Add(time.Hour * 24 * 365).Unix(), // 设置Token有效期为一年
	}
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString([]byte("your_secret_key"))
	if err != nil {
		return "", err
	}

	// 存储Token到Redis，并设置过期时间
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)
	err = rdb.Set(context.Background(), fmt.Sprintf("token:%d", userID), tokenString, 365*24*time.Hour).Err()
	if err != nil {
		return "", err
	}

	return tokenString, nil
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

	token, err := generateTokenAndStoreInRedis(rp, validatedUser.UserID)
	if err != nil {
		http.Error(w, "生成Token失败", http.StatusInternalServerError)
		return
	}

	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":   "登录成功",
		"username": validatedUser.Username,
		"user_id":  validatedUser.UserID,
		"token":    token,
	})
}

// 验证 Token 的中间件
func authenticateToken(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "缺少Token", http.StatusUnauthorized)
			return
		}

		// 去掉 "Bearer " 前缀
		token = strings.TrimPrefix(token, "Bearer ")

		rdb := rp.GetClient()
		defer rp.PutClient(rdb)

		// 用 token:userID 作为 Redis key
		cachedToken, err := rdb.Get(context.Background(), fmt.Sprintf("token:%s", token)).Result()
		if err != nil || cachedToken != token {
			http.Error(w, "Token无效或已过期", http.StatusUnauthorized)
			return
		}

		next.ServeHTTP(w, r)
	}
}

// 一个需要验证 Token 的受保护路由
func protectedEndpoint(w http.ResponseWriter, r *http.Request) {
	// 只有 Token 验证通过的请求才能访问这个资源
	fmt.Fprintf(w, "你已成功访问受保护的资源")
}

// ValidateShop 验证商家凭据
func ValidateShop(db *sql.DB, shopName, password string) (*Shop, error) {
	var shop Shop
	err := db.QueryRow("SELECT shop_id, shop_name, shop_password FROM shops WHERE shop_name = ?", shopName).Scan(&shop.ShopID, &shop.ShopName, &shop.ShopPassword)
	if err != nil {
		if err == sql.ErrNoRows {
			return nil, fmt.Errorf("商家不存在")
		}
		return nil, fmt.Errorf("查询商家信息失败: %v", err)
	}

	// 比较哈希后的密码和输入的密码
	if err := bcrypt.CompareHashAndPassword([]byte(shop.ShopPassword), []byte(password)); err != nil {
		return nil, fmt.Errorf("密码错误")
	}

	return &shop, nil
}

// 生成JWT Token，并存储到Redis
func generateTokenShop(rp *RedisPool, shopID int) (string, error) {
	// 创建JWT的Claims
	claims := jwt.MapClaims{
		"shop_id": shopID,
		"exp":     time.Now().Add(time.Hour * 24).Unix(), // 设置Token有效期为24小时
	}

	// 使用HS256签名算法创建JWT Token
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)

	// 使用全局密钥签名Token
	tokenString, err := token.SignedString([]byte("your_secret_key"))
	if err != nil {
		return "", err
	}

	// 将生成的Token存储到Redis中，设置过期时间为24小时
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	err = rdb.Set(context.Background(), fmt.Sprintf("token:shop:%d", shopID), tokenString, 24*time.Hour).Err()
	if err != nil {
		return "", err
	}

	return tokenString, nil
}

// 商家登录
func handleLoginShop(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "只支持 POST 请求", http.StatusMethodNotAllowed)
		return
	}

	var credentials struct {
		ShopName string `json:"shop_name"`
		Password string `json:"password"`
	}

	// 解析请求体
	if err := json.NewDecoder(r.Body).Decode(&credentials); err != nil {
		http.Error(w, "请求体解析错误", http.StatusBadRequest)
		return
	}

	// 使用 ValidateShop 函数检查商家凭据
	validatedShop, err := ValidateShop(db, credentials.ShopName, credentials.Password)
	if err != nil {
		http.Error(w, fmt.Sprintf("登录失败: %v", err), http.StatusUnauthorized)
		return
	}

	// 生成Token，并存储到Redis
	token, err := generateTokenShop(rp, validatedShop.ShopID)
	if err != nil {
		http.Error(w, "生成Token失败", http.StatusInternalServerError)
		return
	}

	// 返回登录成功信息和Token
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":    "登录成功",
		"shop_name": validatedShop.ShopName,
		"shop_id":   validatedShop.ShopID,
		"token":     token,
	})
}

// 验证Token的中间件，适用于商家
func authenticateTokenShop(rp *RedisPool, next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		// 从请求头获取Authorization字段中的Token
		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "缺少Token", http.StatusUnauthorized)
			return
		}

		// 去掉 "Bearer " 前缀
		token = strings.TrimPrefix(token, "Bearer ")

		// 从Redis中获取Token
		rdb := rp.GetClient()
		defer rp.PutClient(rdb)

		cachedToken, err := rdb.Get(context.Background(), fmt.Sprintf("token:shop:%s", token)).Result()
		if err != nil || cachedToken != token {
			http.Error(w, "Token无效或已过期", http.StatusUnauthorized)
			return
		}

		// 解析并验证Token
		claims, err := jwt.Parse(token, func(token *jwt.Token) (interface{}, error) {
			// 验证Token签名
			if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
				return nil, fmt.Errorf("unexpected signing method: %v", token.Header["alg"])
			}
			return []byte("your_secret_key"), nil
		})

		if err != nil || !claims.Valid {
			http.Error(w, "Token无效或已过期", http.StatusUnauthorized)
			return
		}

		// Token验证成功，继续执行下一个Handler
		next.ServeHTTP(w, r)
	}
}

// 生成JWT Token，适用于骑手，并存储到Redis
func generateTokenRider(rp *RedisPool, riderID int) (string, error) {
	// 创建JWT的Claims
	claims := jwt.MapClaims{
		"rider_id": riderID,
		"exp":      time.Now().Add(time.Hour * 24).Unix(), // 设置Token有效期为24小时
	}

	// 使用HS256签名算法创建JWT Token
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)

	// 使用全局密钥签名Token
	tokenString, err := token.SignedString([]byte("your_secret_key"))
	if err != nil {
		return "", err
	}

	// 将生成的Token存储到Redis中，设置过期时间为24小时
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	err = rdb.Set(context.Background(), fmt.Sprintf("token:rider:%d", riderID), tokenString, 24*time.Hour).Err()
	if err != nil {
		return "", err
	}

	return tokenString, nil
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

	// 解析请求体
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

	// 为骑手生成Token，并存储到Redis
	token, err := generateTokenRider(rp, rider.RiderID)
	if err != nil {
		http.Error(w, "生成Token失败", http.StatusInternalServerError)
		return
	}

	// 返回创建成功信息和Token
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":   "骑手注册成功",
		"rider_id": rider.RiderID,
		"token":    token,
	})
}

// 验证Token的中间件，适用于骑手
func authenticateTokenRider(rp *RedisPool, next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		// 从请求头获取Authorization字段中的Token
		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "缺少Token", http.StatusUnauthorized)
			return
		}

		// 去掉 "Bearer " 前缀
		token = strings.TrimPrefix(token, "Bearer ")

		// 从Redis中获取Token
		rdb := rp.GetClient()
		defer rp.PutClient(rdb)

		cachedToken, err := rdb.Get(context.Background(), fmt.Sprintf("token:rider:%s", token)).Result()
		if err != nil || cachedToken != token {
			http.Error(w, "Token无效或已过期", http.StatusUnauthorized)
			return
		}

		// 解析并验证Token
		claims, err := jwt.Parse(token, func(token *jwt.Token) (interface{}, error) {
			// 验证Token签名
			if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
				return nil, fmt.Errorf("unexpected signing method: %v", token.Header["alg"])
			}
			return []byte("your_secret_key"), nil
		})

		if err != nil || !claims.Valid {
			http.Error(w, "Token无效或已过期", http.StatusUnauthorized)
			return
		}

		// Token验证成功，继续执行下一个Handler
		next.ServeHTTP(w, r)
	}
}

// HTTP 服务启动
func main() {
	http.HandleFunc("/protected", authenticateToken(protectedEndpoint))                // 验证用户
	http.HandleFunc("/protected/shop", authenticateTokenShop(rp, protectedEndpoint))   // 验证商家
	http.HandleFunc("/protected/rider", authenticateTokenRider(rp, protectedEndpoint)) // 验证骑手

	http.HandleFunc("/user/register", handleRegister) // 用户注册
	http.HandleFunc("/user/login", handleLogin)       // 用户登录

	http.HandleFunc("/shops", handleGetShops)           // 获取商家列表
	http.HandleFunc("/shops", handleNearbyShops)        // 获取附近商家
	http.HandleFunc("/products", handleShopProducts)    // 查询商家商品
	http.HandleFunc("/order", handleOrder)              // 订外卖
	http.HandleFunc("/order/status", handleOrderStatus) // 查询订单状态

	http.HandleFunc("/shop/register", handleRegisterShop)              // 商家注册
	http.HandleFunc("/shop/login", handleLoginShop)                    // 商家登录
	http.HandleFunc("/shop/add_product", handleAddProductForShop)      // 商家添加商品
	http.HandleFunc("/shop/accept_order", handleAcceptOrder)           // 商家确认订单
	http.HandleFunc("/shop/publish_order", handlePublishDeliveryOrder) // 商家发布订单

	http.HandleFunc("/notify", handNotifyNearbyRider) // 系统随机通知骑手

	http.HandleFunc("/rider/apply", handleApplyForRider)         // 骑手身份申请
	http.HandleFunc("/rider/grab", handleRiderGrabOrder(db, rp)) // 骑手抢单
	http.HandleFunc("/rider/complete", handleCompleteOrder)      // 骑手完成订单

	http.HandleFunc("/im/send", handleSendMessage(db, rp))     // 发送群组消息
	http.HandleFunc("/im/messages", handleGetMessages(db, rp)) // 获取群组消息

	// 启动每周清理调度器
	go StartWeeklyCleanUpScheduler(db)

	log.Println("服务器启动，端口 :8080")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		log.Fatalf("服务器启动失败: %v", err)
	}
}
