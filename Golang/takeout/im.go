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

/*
借助redis的MQ实现Rider抢单和派单系统，按照某个策略丢到公共大厅供大家抢，可以做成随机的策略。
User点单后生成OrderlD，Shop拿到OrderlD，把OrderlD发布到MQ，Rider通过消费MQ(Rider拿到OrderlD)，
IM系统拿到OrderlD后读MQ(Order结构体下有UserlD，ShoplD，RiderlD)同时新增-个GrouplD(Group结构体下有ShopIDUserlD，RiderlD，OrderD)，
创建了一个群聊，实现聊天,groupid应该是和orderid绑定的。
*/

// CreateGroup 创建与订单关联的新聊天群组
func CreateGroup(order map[string]interface{}, rp *RedisPool, db *sql.DB) error {
	group := &Group{
		OrderID: int(order["order_id"].(float64)), // JSON 反序列化时数字会被转换为 float64
		UserID:  int(order["user_id"].(float64)),
		ShopID:  int(order["shop_id"].(float64)),
		RiderID: int(order["rider_id"].(float64)),
	}

	groupID, err := insertGroup(rp, db, group)
	if err != nil {
		return fmt.Errorf("创建群组失败: %v", err)
	}

	fmt.Printf("群组创建成功, ID: %d\n", groupID)
	return nil
}

// NewGroupMessage 创建带有时间戳的新消息
func NewGroupMessage(messageID, groupID, senderID int, content string) *Message {
	return &Message{
		MessageID: messageID,
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
		return fmt.Errorf("发布消息到 Redis 失败: %v", err)
	}
	return nil
}

// SaveMessage 将消息保存到数据库中，确保按时间戳排序
func SaveMessage(db *sql.DB, msg *Message) error {
	query := "INSERT INTO messages (group_id, sender_id, content, timestamp) VALUES (?, ?, ?, ?)"
	_, err := db.Exec(query, msg.GroupID, msg.SenderID, msg.Content, msg.Timestamp)
	if err != nil {
		return fmt.Errorf("保存消息失败: %v", err)
	}
	return nil
}

// HandleSendMessage 处理通过 HTTP 请求发送消息
// 先将消息保存到数据库，然后再发布到 Redis
// 如果数据库保存失败，可以立即返回错误，而不必处理已发送的 Redis 消息可能带来的不一致问题
// 避免了在发布消息到 Redis 后数据库写入失败的情况
func HandleSendMessage(db *sql.DB, rp *RedisPool) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var msg Message
		if err := json.NewDecoder(r.Body).Decode(&msg); err != nil {
			log.Printf("解析请求体失败: %v", err)
			http.Error(w, "无效的请求体", http.StatusBadRequest)
			return
		}

		// 设置当前时间戳
		msg.Timestamp = time.Now()

		// 将消息先保存到数据库，确保数据持久化
		if err := SaveMessage(db, &msg); err != nil {
			log.Printf("保存消息失败: %v", err)
			http.Error(w, fmt.Sprintf("保存消息失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 将消息发布到 Redis 频道，供其他组件实时处理
		channel := fmt.Sprintf("group_%d", msg.GroupID)
		if err := PublishMessage(rp, channel, msg.Content); err != nil {
			log.Printf("发布消息失败: %v", err)
			http.Error(w, fmt.Sprintf("发布消息失败: %v", err), http.StatusInternalServerError)
			return
		}

		// 返回成功响应
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

		// 从 Redis 缓存中获取消息
		cachedMessages, err := rdb.Get(context.Background(), groupID).Result()
		if err == redis.Nil {
			// 从数据库中按时间戳顺序获取消息
			rows, err := db.Query("SELECT group_id, sender_id, content, timestamp FROM messages WHERE group_id = ? ORDER BY timestamp ASC", groupID)
			if err != nil {
				http.Error(w, fmt.Sprintf("查询消息失败: %v", err), http.StatusInternalServerError)
				return
			}
			defer rows.Close()

			var messages []Message
			for rows.Next() {
				var msg Message
				if err := rows.Scan(&msg.GroupID, &msg.SenderID, &msg.Content, &msg.Timestamp); err != nil {
					http.Error(w, fmt.Sprintf("解析消息失败: %v", err), http.StatusInternalServerError)
					return
				}
				messages = append(messages, msg)
			}

			messagesJSON, _ := json.Marshal(messages)
			rdb.Set(context.Background(), groupID, messagesJSON, 7*24*time.Hour) // 缓存消息 7 天

			w.Header().Set("Content-Type", "application/json")
			w.Write(messagesJSON)
		} else if err != nil {
			http.Error(w, fmt.Sprintf("Redis 查询失败: %v", err), http.StatusInternalServerError)
			return
		} else {
			w.Header().Set("Content-Type", "application/json")
			w.Write([]byte(cachedMessages))
		}
	}
}

// CleanUpOldRecords 清理旧的订单和消息记录
func CleanUpOldRecords(db *sql.DB) error {
	// 删除两周前的订单记录
	_, err := db.Exec("DELETE FROM orders WHERE YEARWEEK(order_time, 1) = YEARWEEK(NOW() - INTERVAL 2 WEEK, 1)")
	if err != nil {
		return fmt.Errorf("清理订单记录失败: %v", err)
	}

	// 删除两周前的消息记录
	_, err = db.Exec("DELETE FROM messages WHERE YEARWEEK(timestamp, 1) = YEARWEEK(NOW() - INTERVAL 2 WEEK, 1)")
	if err != nil {
		return fmt.Errorf("清理消息记录失败: %v", err)
	}

	log.Println("旧记录清理成功")
	return nil
}

// StartWeeklyCleanUpScheduler 启动每周清理调度器
func StartWeeklyCleanUpScheduler(db *sql.DB) {
	ticker := time.NewTicker(7 * 24 * time.Hour)
	defer ticker.Stop()

	for range ticker.C { // 使用 for range 循环来处理通道接收
		log.Println("开始每周清理任务...")
		err := CleanUpOldRecords(db)
		if err != nil {
			log.Printf("每周清理任务失败: %v", err)
		} else {
			log.Println("每周清理任务成功完成")
		}
	}
}
