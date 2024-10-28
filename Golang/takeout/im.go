package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/gomodule/redigo/redis"
)

// IM
func NewGroupMessage(groupID, senderID int, content string) *Message {
	return &Message{
		GroupID:   groupID,
		SenderID:  senderID,
		Content:   content,
		Timestamp: time.Now(),
	}
}

// 使用 Redis 发布消息
func PublishMessage(rp *redis.Pool, channel, message string) error {
	conn := rp.Get() // 从连接池获取连接
	defer conn.Close()

	_, err := conn.Do("PUBLISH", channel, message) // 发布消息到频道
	if err != nil {
		return fmt.Errorf("Redis 消息发布失败: %v", err)
	}
	return nil
}

// 保存消息到数据库
func SaveMessage(db *sql.DB, msg *Message) error {
	query := "INSERT INTO messages (group_id, sender_id, content, timestamp) VALUES (?, ?, ?, ?)"
	_, err := db.Exec(query, msg.GroupID, msg.SenderID, msg.Content, msg.Timestamp)
	if err != nil {
		return fmt.Errorf("消息保存失败: %v", err)
	}
	return nil
}

// 处理发送消息的HTTP请求
func HandleSendMessage(db *sql.DB, rp *redis.Pool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var msg Message
		if err := json.NewDecoder(r.Body).Decode(&msg); err != nil {
			http.Error(w, "请求体解析错误", http.StatusBadRequest)
			return
		}

		// 创建消息对象
		msg.Timestamp = time.Now()

		// 从连接池获取 Redis 连接
		conn := rp.Get()
		defer conn.Close()

		// 发布消息到 Redis 的 Pub/Sub 频道
		channel := fmt.Sprintf("group_%d", msg.GroupID)
		_, err := conn.Do("PUBLISH", channel, msg.Content)
		if err != nil {
			http.Error(w, fmt.Sprintf("消息发布失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 保存消息到数据库
		if err := SaveMessage(db, &msg); err != nil {
			http.Error(w, fmt.Sprintf("消息保存失败: %v", err), http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusCreated)
		json.NewEncoder(w).Encode(map[string]string{"status": "消息发送成功"})
	}
}

// 查询特定群的消息
func HandleGetMessages(db *sql.DB) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		groupID := r.URL.Query().Get("group_id")
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

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(messages)
	}
}

// 处理订阅群组消息
func HandleSubscribeMessages(rp *redis.Pool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		groupID := r.URL.Query().Get("group_id")
		channel := fmt.Sprintf("group_%s", groupID)

		// 从连接池获取 Redis 连接
		conn := rp.Get()
		defer conn.Close()

		// 使用 Pub/Sub 连接
		psc := redis.PubSubConn{Conn: conn}
		if err := psc.Subscribe(channel); err != nil {
			http.Error(w, fmt.Sprintf("订阅失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 等待接收消息
		for {
			switch v := psc.Receive().(type) {
			case redis.Message:
				// 收到消息后返回给客户端
				w.Header().Set("Content-Type", "application/json")
				json.NewEncoder(w).Encode(map[string]string{"message": string(v.Data)})
				return
			case error:
				// 处理错误
				http.Error(w, fmt.Sprintf("接收消息失败: %v", v), http.StatusInternalServerError)
				return
			}
		}
	}
}
