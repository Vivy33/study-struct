package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/redis/go-redis/v9"
	"golang.org/x/exp/rand"
)

// 用户下单，将订单信息发布到订单频道
func UserPlaceOrder(orderID, userID, shopID int, rp *RedisPool) error {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	// 构建订单信息并将其转化为 JSON 格式
	order := map[string]interface{}{
		"order_id": orderID,
		"user_id":  userID,
		"shop_id":  shopID,
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

// 商家处理订单并将其发布到公共大厅
func ShopProcessOrder(rp *RedisPool, db *sql.DB) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	// 订阅 "order_channel" 频道以接收新订单
	subscriber := rdb.Subscribe(context.Background(), "order_channel")
	channel := subscriber.Channel()

	// 处理接收到的每个订单
	for msg := range channel {
		fmt.Printf("接收到订单消息: %s\n", msg.Payload)

		// 检查消息是否为空
		if len(msg.Payload) == 0 {
			fmt.Println("消息数据为空")
			continue
		}

		// 将订单 JSON 数据反序列化为 map 结构
		var order map[string]interface{}
		err := json.Unmarshal([]byte(msg.Payload), &order)
		if err != nil {
			fmt.Printf("订单JSON解析失败: %v\n", err)
			fmt.Printf("Debug: Payload = %s\n", msg.Payload)
			continue
		}

		// 将订单 JSON 格式数据推送到公共大厅
		orderJSON, err := json.Marshal(order)
		if err != nil {
			fmt.Printf("订单JSON序列化失败: %v\n", err)
			continue
		}
		_, err = rdb.RPush(context.Background(), "public_hall", orderJSON).Result()
		if err != nil {
			fmt.Printf("订单推送到公共大厅失败: %v\n", err)
			continue
		} else {
			fmt.Println("订单已推送到公共大厅")
		}

		// 随机选择骑手通知新订单
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
		// 解析请求体获取订单 ID 和骑手 ID
		var requestData struct {
			OrderID int `json:"order_id"`
			RiderID int `json:"rider_id"`
		}
		if err := json.NewDecoder(r.Body).Decode(&requestData); err != nil {
			http.Error(w, "无效的请体", http.StatusBadRequest)
			return
		}

		// 开启事务
		tx, err := db.Begin()
		if err != nil {
			http.Error(w, "事务开启失败", http.StatusInternalServerError)
			return
		}
		defer func() {
			if p := recover(); p != nil {
				tx.Rollback()
				panic(p) // 恢复 panic
			}
		}()

		// 查询订单是否已被抢
		var existingRiderID int
		err = tx.QueryRow("SELECT rider_id FROM orders WHERE order_id = ? FOR UPDATE", requestData.OrderID).Scan(&existingRiderID)
		if err != nil && err != sql.ErrNoRows {
			tx.Rollback()
			http.Error(w, "查询订单状态失败", http.StatusInternalServerError)
			return
		}

		if existingRiderID != 0 {
			tx.Rollback()
			http.Error(w, "订单已被其他骑手抢走", http.StatusConflict)
			return
		}

		// 通知骑手抢单成功，并等待骑手响应
		rdb := rp.GetClient()
		defer rp.PutClient(rdb)
		orderNotification := map[string]interface{}{
			"order_id": requestData.OrderID,
			"rider_id": requestData.RiderID,
			"status":   "pending",
		}
		orderNotificationJSON, _ := json.Marshal(orderNotification)
		riderChannel := fmt.Sprintf("rider_%d", requestData.RiderID)
		_, err = rdb.Publish(context.Background(), riderChannel, orderNotificationJSON).Result()
		if err != nil {
			http.Error(w, fmt.Sprintf("订单通知失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 等待骑手响应
		response, err := waitForRiderResponse(requestData.RiderID)
		if err != nil || response != "accept" {
			tx.Rollback()
			http.Error(w, "骑手拒绝了订单", http.StatusConflict)
			return
		}

		// 更新订单，将骑手 ID 添加到订单中
		_, err = tx.Exec("UPDATE orders SET rider_id = ? WHERE order_id = ?", requestData.RiderID, requestData.OrderID)
		if err != nil {
			tx.Rollback()
			http.Error(w, "抢单失败", http.StatusInternalServerError)
			return
		}

		// 提交事务
		if err := tx.Commit(); err != nil {
			http.Error(w, "事务提交失败", http.StatusInternalServerError)
			return
		}

		// 发布成功消息到骑手专属频道
		orderNotification["status"] = "success"
		orderNotificationJSON, _ = json.Marshal(orderNotification)
		_, err = rdb.Publish(context.Background(), riderChannel, orderNotificationJSON).Result()
		if err != nil {
			http.Error(w, fmt.Sprintf("订单通知失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 返回成功响应
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]string{"status": "抢单成功"})
	}
}
