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

// 用户下单
func UserPlaceOrder(orderID int, rp *RedisPool) error {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	order := fmt.Sprintf(`{"order_id": %d, "user_id": 123, "shop_id": 456}`, orderID)

	_, err := rdb.Publish(context.Background(), "order_channel", order).Result()
	if err != nil {
		return fmt.Errorf("订单发布失败: %v", err)
	}
	return nil
}

// 商家处理订单并将其发布到公共大厅
func ShopProcessOrder(rp *RedisPool) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	subscriber := rdb.Subscribe(context.Background(), "order_channel")
	channel := subscriber.Channel()

	for msg := range channel {
		fmt.Printf("接收到消息: %s\n", msg.Payload)

		// 检查 msg.Payload 是否为空或不是有效的 JSON 字符串
		if len(msg.Payload) == 0 {
			fmt.Println("消息数据为空")
			continue
		}

		var order map[string]interface{}
		err := json.Unmarshal([]byte(msg.Payload), &order)
		if err != nil {
			fmt.Printf("订单JSON解析失败: %v\n", err)
			fmt.Printf("Debug: Payload = %s\n", msg.Payload) // 打印出具体的 Payload 值以便调试
			continue
		}

		orderJSON, err := json.Marshal(order)
		if err != nil {
			fmt.Printf("订单JSON序列化失败: %v\n", err)
			continue
		}

		_, err = rdb.RPush(context.Background(), "public_hall", orderJSON).Result()
		if err != nil {
			fmt.Printf("订单推送到公共大厅失败: %v\n", err)
		}
	}
}

// 骑手抢单
func RiderGrabOrder(rp *RedisPool, db *sql.DB) {
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	for {
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

		// 打印 orderJSON 以便调试
		fmt.Printf("Debug: orderJSON = %s\n", orderJSON)

		// 检查 orderJSON 是否为空或不是有效的 JSON 字符串
		if len(orderJSON) == 0 {
			fmt.Println("订单数据为空")
			time.Sleep(1 * time.Second)
			continue
		}

		var order map[string]interface{}
		err = json.Unmarshal([]byte(orderJSON), &order)
		if err != nil {
			fmt.Printf("订单JSON解析失败: %v\n", err)
			fmt.Printf("Debug: orderJSON = %s\n", orderJSON) // 打印出具体的 orderJSON 值以便调试
			time.Sleep(1 * time.Second)
			continue
		}

		// 处理抢单逻辑
		order["rider_id"] = 789 // 示例骑手ID
		orderJSONBytes, err := json.Marshal(order)
		if err != nil {
			fmt.Printf("订单JSON序列化失败: %v\n", err)
			continue
		}

		// 发布到骑手的专属频道
		riderChannel := fmt.Sprintf("rider_%d", int(order["rider_id"].(float64)))
		_, err = rdb.Publish(context.Background(), riderChannel, orderJSONBytes).Result()
		if err != nil {
			fmt.Printf("订单推送到骑手频道失败: %v\n", err)
		} else {
			fmt.Printf("订单已推送到骑手频道: %s\n", orderJSONBytes)
		}

		// 创建群聊
		err = CreateGroup(order, rp, db)
		if err != nil {
			fmt.Printf("群聊创建失败: %v\n", err)
		}
	}
}
