package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"
	"time"
)

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
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	_, err := rdb.Publish(context.Background(), channel, message).Result()
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
func HandleSendMessage(db *sql.DB, rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var msg Message
		if err := json.NewDecoder(r.Body).Decode(&msg); err != nil {
			http.Error(w, "请求体解析错误", http.StatusBadRequest)
			return
		}

		msg.Timestamp = time.Now()
		channel := fmt.Sprintf("group_%d", msg.GroupID)

		if err := PublishMessage(rp, channel, msg.Content); err != nil {
			http.Error(w, fmt.Sprintf("消息发布失败: %v", err), http.StatusInternalServerError)
			return
		}

		if err := SaveMessage(db, &msg); err != nil {
			http.Error(w, fmt.Sprintf("消息保存失败: %v", err), http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusCreated)
		json.NewEncoder(w).Encode(map[string]string{"status": "消息发送成功"})
	}
}

// HandleGetMessages 处理获取特定群组消息的 HTTP 请求
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
