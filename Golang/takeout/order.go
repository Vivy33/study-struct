package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/redis/go-redis/v9"
)

// 用户下单函数，将订单信息发布到订单频道
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
func ShopProcessOrder(rp *RedisPool) {
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

		// 将订单再转化为 JSON 格式，推送到公共大厅的队列
		orderJSON, err := json.Marshal(order)
		if err != nil {
			fmt.Printf("订单JSON序列化失败: %v\n", err)
			continue
		}

		// 将订单推送到 Redis 列表 "public_hall"，等待骑手抢单
		_, err = rdb.RPush(context.Background(), "public_hall", orderJSON).Result()
		if err != nil {
			fmt.Printf("订单推送到公共大厅失败: %v\n", err)
		} else {
			fmt.Println("订单已推送到公共大厅")
		}
	}
}

// 骑手抢单函数，从公共大厅获取订单并将其发布到骑手专属频道
func RiderGrabOrder(rp *RedisPool, db *sql.DB, riderID int) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	for {
		// 从公共大厅 "public_hall" 列表中获取订单
		orderJSON, err := rdb.LPop(context.Background(), "public_hall").Result()
		if err == redis.Nil {
			fmt.Println("公共大厅没有订单")
			time.Sleep(1 * time.Second)
			continue
		} else if err != nil {
			fmt.Printf("订单获取失败: %v\n", err)
			time.Sleep(1 * time.Second)
			continue
		}

		fmt.Printf("从公共大厅获取到订单: %s\n", orderJSON)

		// 检查订单数据是否为空
		if len(orderJSON) == 0 {
			fmt.Println("订单数据为空")
			time.Sleep(1 * time.Second)
			continue
		}

		// 将订单 JSON 数据反序列化为 map 结构
		var order map[string]interface{}
		err = json.Unmarshal([]byte(orderJSON), &order)
		if err != nil {
			fmt.Printf("订单JSON解析失败: %v\n", err)
			fmt.Printf("Debug: orderJSON = %s\n", orderJSON)
			time.Sleep(1 * time.Second)
			continue
		}

		// 处理抢单逻辑，将当前骑手 ID 添加到订单中
		order["rider_id"] = riderID
		orderJSONBytes, err := json.Marshal(order)
		if err != nil {
			fmt.Printf("订单JSON序列化失败: %v\n", err)
			continue
		}

		// 发布到骑手专属频道，通知骑手自己抢到的订单
		riderChannel := fmt.Sprintf("rider_%d", riderID)
		_, err = rdb.Publish(context.Background(), riderChannel, orderJSONBytes).Result()
		if err != nil {
			fmt.Printf("订单推送到骑手频道失败: %v\n", err)
		} else {
			fmt.Printf("订单已推送到骑手频道: %s\n", orderJSONBytes)
		}

		// 调用函数创建群聊，方便骑手和商家联系
		err = CreateGroup(order, rp, db)
		if err != nil {
			fmt.Printf("群聊创建失败: %v\n", err)
		}
	}
}
