package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/redis/go-redis/v9"
	"golang.org/x/exp/rand"
)

// 用户下单，将订单信息发布到订单频道
func UserPlaceOrder(orderID, userID, shopID, riderID int, rp *RedisPool) error {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	// 构建订单信息并将其转化为 JSON 格式
	order := map[string]interface{}{
		"order_id": orderID,
		"user_id":  userID,
		"shop_id":  shopID,
		"rider_id": riderID,
	}
	orderJSON, err := json.Marshal(order)
	if err != nil {
		return fmt.Errorf("订单序列化失败: %v", err)
	}

	// 将订单发布到 Redis 的 "order_channel" 频道
	_, err = rdb.Publish(context.Background(), "order_channel", orderJSON).Result()
	if err != nil {
		return fmt.Errorf("订单发布失败: %v", err)
	}
	fmt.Println("订单已成功发布到订单频道")
	return nil
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

	// 设置初始订单状态
	order.OrderStatus = "商家待确认"

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
		w.Header().Set("Content-Type", "application/json")
		w.Write([]byte(data))
		return
	}

	// 缓存未命中，从数据库查询订单状态
	order, err := QueryOrderStatus(db, orderID)
	if err != nil {
		http.Error(w, fmt.Sprintf("订单不存在: %v", err), http.StatusInternalServerError)
		return
	}

	jsonData, err := json.Marshal(order)
	if err != nil {
		http.Error(w, "JSON 序列化错误", http.StatusInternalServerError)
		return
	}
	SetToCache(rp, cacheKey, string(jsonData), time.Hour)
	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)
}

// 商家处理订单并将其发布到公共大厅
func ShopProcessOrder(rp *RedisPool, db *sql.DB) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	subscriber := rdb.Subscribe(context.Background(), "order_channel")
	channel := subscriber.Channel()

	for msg := range channel {
		fmt.Printf("接收到订单消息: %s\n", msg.Payload)

		// 检查消息是否为空
		if len(msg.Payload) == 0 {
			fmt.Println("消息数据为空")
			continue
		}

		// 打印消息内容以进行调试
		fmt.Printf("Payload received: %s\n", msg.Payload)

		var order map[string]interface{}
		err := json.Unmarshal([]byte(msg.Payload), &order)
		if err != nil {
			fmt.Printf("订单 JSON 解析失败: %v\n", err)
			fmt.Printf("Debug: Payload = %s\n", msg.Payload)
			continue
		}

		// 更新订单状态为 "商家已接受"
		orderStatus := map[string]interface{}{
			"order_id":     order["order_id"],
			"order_status": "商家已接受",
		}
		orderStatusJSON, err := json.Marshal(orderStatus)
		if err != nil {
			fmt.Printf("订单状态 JSON 序列化失败: %v\n", err)
			continue
		}

		message := rdb.Publish(context.Background(), "order_channel", orderStatusJSON)
		if message.Err() != nil {
			fmt.Printf("订单状态更新失败: %v\n", message.Err())
			continue
		}
		fmt.Println("订单状态已推送到订单频道")

		// 通知随机骑手新订单
		notifyRandomRider(db, rp, order)
	}
}

// 随机选择一个骑手通知新订单
func notifyRandomRider(db *sql.DB, rp *RedisPool, order map[string]interface{}) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	// 查询数据库获取所有登录状态的 RiderID
	rows, err := db.Query("SELECT rider_id FROM riders WHERE logged_in = 1")
	if err != nil {
		fmt.Printf("获取登录骑手列表失败: %v\n", err)
		return
	}
	defer rows.Close()

	riderIDs := make([]int, 0) // 使用 make 函数初始化切片
	for rows.Next() {
		var riderID int
		if err := rows.Scan(&riderID); err == nil {
			riderIDs = append(riderIDs, riderID)
		}
	}

	if len(riderIDs) == 0 {
		fmt.Println("当前没有登录状态的骑手")
		return
	}

	// 随机选择骑手
	rand.Seed(uint64(time.Now().UnixNano()))
	for len(riderIDs) > 0 {
		riderIndex := rand.Intn(len(riderIDs))
		selectedRiderID := riderIDs[riderIndex]

		// 通知骑手新订单
		notifyChannel := fmt.Sprintf("rider_%d", selectedRiderID)
		orderJSON, _ := json.Marshal(order)
		_, err := rdb.Publish(context.Background(), notifyChannel, orderJSON).Result()
		if err != nil {
			fmt.Printf("通知骑手失败: %v\n", err)
			continue
		}

		// 等待骑手响应
		response, err := waitForRiderResponse(selectedRiderID)
		if err != nil || response != "accept" {
			// 若骑手拒绝接单，继续通知下一个骑手
			fmt.Printf("骑手 %d 拒绝订单，转派其他骑手\n", selectedRiderID)
			// 移除已通知的骑手
			riderIDs = append(riderIDs[:riderIndex], riderIDs[riderIndex+1:]...)
		} else {
			fmt.Printf("骑手 %d 接受订单\n", selectedRiderID)
			break
		}
	}
}

// 等待骑手响应接单请求
func waitForRiderResponse(riderID int) (string, error) {
	// 假设客户端通过某个途径返回接受或拒绝的状态
	// 为简单起见，这里模拟等待响应并返回随机状态
	time.Sleep(2 * time.Second) // 等待时间
	responseOptions := []string{"accept", "reject"}
	return responseOptions[rand.Intn(len(responseOptions))], nil
}

// 获取订单列表供骑手选择
func GetOrderListFromMQ(rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		// 获取 Redis 客户端
		rdb := rp.GetClient()
		defer rp.PutClient(rdb)

		// 消费 MQ 获取订单列表
		orderList := []map[string]interface{}{}

		for i := 0; i < 10; i++ { // 假设每次获取最多 10 个订单
			orderJSON, err := rdb.LPop(context.Background(), "public_hall").Result()
			if err == redis.Nil {
				break // 没有更多订单
			} else if err != nil {
				http.Error(w, fmt.Sprintf("订单获取失败: %v", err), http.StatusInternalServerError)
				return
			}

			var order map[string]interface{}
			if err := json.Unmarshal([]byte(orderJSON), &order); err != nil {
				http.Error(w, fmt.Sprintf("订单解析失败: %v", err), http.StatusInternalServerError)
				return
			}
			orderList = append(orderList, order)
		}

		// 返回订单列表
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(orderList)
	}
}

// 处理骑手抢单请求，保证事务处理
func HandleRiderGrabOrder(db *sql.DB, rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "Only supports POST method", http.StatusMethodNotAllowed)
			return
		}

		var requestData struct {
			OrderID int `json:"order_id"`
			RiderID int `json:"rider_id"`
		}
		if err := json.NewDecoder(r.Body).Decode(&requestData); err != nil {
			http.Error(w, "Invalid request body", http.StatusBadRequest)
			return
		}

		tx, err := db.Begin()
		if err != nil {
			http.Error(w, "Transaction start failed", http.StatusInternalServerError)
			return
		}
		defer func() {
			if p := recover(); p != nil {
				tx.Rollback()
				panic(p) // 恢复 panic
			}
		}()

		var existingRiderID sql.NullInt64
		err = tx.QueryRow("SELECT rider_id FROM orders WHERE order_id = ? FOR UPDATE", requestData.OrderID).Scan(&existingRiderID)
		if err != nil {
			if err == sql.ErrNoRows {
				http.Error(w, "Order does not exist", http.StatusNotFound)
			} else {
				http.Error(w, "Failed to query order status", http.StatusInternalServerError)
			}
			tx.Rollback()
			return
		}

		if existingRiderID.Valid && existingRiderID.Int64 != 0 {
			http.Error(w, "Order has already been taken by another rider", http.StatusConflict)
			return
		}

		// 更新订单状态为 "骑手已接单"
		_, err = tx.Exec("UPDATE orders SET rider_id = ?, order_status = ? WHERE order_id = ?", requestData.RiderID, "骑手已接单", requestData.OrderID)
		if err != nil {
			tx.Rollback()
			http.Error(w, "Failed to update order", http.StatusInternalServerError)
			return
		}

		if err := tx.Commit(); err != nil {
			http.Error(w, "Transaction commit failed", http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]string{"status": "Order accepted successfully"})
	}
}
