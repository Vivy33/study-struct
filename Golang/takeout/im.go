package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"github.com/redis/go-redis/v9"
)

// 创建群聊
func CreateGroup(order map[string]interface{}, rp *RedisPool, db *sql.DB) error {
	group := &Group{
		OrderID: int(order["order_id"].(float64)), // JSON反序列化时数字会被解析为float64
		UserID:  int(order["user_id"].(float64)),
		ShopID:  int(order["shop_id"].(float64)),
		RiderID: int(order["rider_id"].(float64)),
	}

	groupID, err := insertGroup(rp, db, group)
	if err != nil {
		return fmt.Errorf("failed to create group: %v", err)
	}

	fmt.Printf("Group created successfully with ID: %d\n", groupID)
	return nil
}

// NewGroupMessage 创建一条新的消息
func NewGroupMessage(groupID, senderID int, content string) *Message {
	return &Message{
		GroupID:   groupID,
		SenderID:  senderID,
		Content:   content,
		Timestamp: time.Now(),
	}
}

// PublishMessage 将消息发布到 Redis 频道
func PublishMessage(rp *RedisPool, channel, message string) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	_, err := rdb.Publish(ctx, channel, message).Result()
	if err != nil {
		return fmt.Errorf("redis 消息发布失败: %v", err)
	}
	return nil
}

// SaveMessage 将消息保存到数据库
func SaveMessage(db *sql.DB, msg *Message) error {
	query := "INSERT INTO messages (group_id, sender_id, content, timestamp) VALUES (?, ?, ?, ?)"
	_, err := db.Exec(query, msg.GroupID, msg.SenderID, msg.Content, msg.Timestamp)
	if err != nil {
		return fmt.Errorf("消息保存失败: %v", err)
	}
	return nil
}

// HandleSendMessage 处理发送消息的 HTTP 请求
// 在处理错误时添加日志记录
func HandleSendMessage(db *sql.DB, rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var msg Message
		if err := json.NewDecoder(r.Body).Decode(&msg); err != nil {
			log.Printf("请求体解析错误: %v", err)
			http.Error(w, "请求体解析错误", http.StatusBadRequest)
			return
		}

		msg.Timestamp = time.Now()
		channel := fmt.Sprintf("group_%d", msg.GroupID)

		if err := PublishMessage(rp, channel, msg.Content); err != nil {
			log.Printf("消息发布失败: %v", err)
			http.Error(w, fmt.Sprintf("消息发布失败: %v", err), http.StatusInternalServerError)
			return
		}

		if err := SaveMessage(db, &msg); err != nil {
			log.Printf("消息保存失败: %v", err)
			http.Error(w, fmt.Sprintf("消息保存失败: %v", err), http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusCreated)
		json.NewEncoder(w).Encode(map[string]string{"status": "消息发送成功"})
	}
}

// HandleGetMessages 处理获取特定群组消息的 HTTP 请求
func HandleGetMessages(db *sql.DB, rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		groupID := r.URL.Query().Get("group_id")
		if groupID == "" {
			http.Error(w, "缺少 group_id 参数", http.StatusBadRequest)
			return
		}

		rdb := rp.GetClient()
		defer rp.PutClient(rdb)

		// 尝试从 Redis 缓存中读取消息
		cachedMessages, err := rdb.Get(context.Background(), groupID).Result()
		if err == redis.Nil { // 如果 Redis 缓存中没有该群组的消息
			// 从数据库中查询
			rows, err := db.Query("SELECT group_id, sender_id, content, timestamp FROM messages WHERE group_id = ? ORDER BY timestamp ASC", groupID)
			if err != nil {
				http.Error(w, fmt.Sprintf("消息查询失败: %v", err), http.StatusInternalServerError)
				return
			}
			defer rows.Close()

			var messages []Message
			for rows.Next() {
				var msg Message
				if err := rows.Scan(&msg.GroupID, &msg.SenderID, &msg.Content, &msg.Timestamp); err != nil {
					http.Error(w, fmt.Sprintf("消息解析失败: %v", err), http.StatusInternalServerError)
					return
				}
				messages = append(messages, msg)
			}

			// 将消息序列化并存储到 Redis 缓存
			messagesJSON, _ := json.Marshal(messages)
			rdb.Set(context.Background(), groupID, messagesJSON, 10*time.Minute) // 设置缓存过期时间为 10 分钟

			w.Header().Set("Content-Type", "application/json")
			w.Write(messagesJSON)
		} else if err != nil {
			http.Error(w, fmt.Sprintf("Redis 查询失败: %v", err), http.StatusInternalServerError)
			return
		} else {
			// 直接返回 Redis 中缓存的消息
			w.Header().Set("Content-Type", "application/json")
			w.Write([]byte(cachedMessages))
		}
	}
}
