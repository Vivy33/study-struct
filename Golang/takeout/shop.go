package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"

	_ "github.com/go-sql-driver/mysql"
)

// addProductForShop 添加商品到指定店铺
func addProductForShop(rp *RedisPool, db *sql.DB, shopID int, product *Product) (int64, error) {
	// 插入商品到数据库
	query := "INSERT INTO products (shop_id, product_name, description, price, stock) VALUES (?, ?, ?, ?, ?)"
	result, err := db.Exec(query, product.ShopID, product.ProductName, product.Description, product.Price, product.Stock)
	if err != nil {
		return 0, fmt.Errorf("failed to insert product into MySQL: %v", err)
	}

	productID, err := result.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("failed to get product ID from MySQL: %v", err)
	}

	// 插入商品到 Redis
	rdb := rp.GetClient()
	defer rp.PutClient(rdb)

	// 使用 HSet 插入多个字段到 Redis
	err = rdb.HSet(context.Background(), fmt.Sprintf("product:%d", productID), map[string]interface{}{
		"product_id":   productID,
		"shop_id":      product.ShopID,
		"product_name": product.ProductName,
		"description":  product.Description,
		"price":        product.Price,
		"stock":        product.Stock,
	}).Err()

	if err != nil {
		return 0, fmt.Errorf("failed to insert product into Redis: %v", err)
	}

	return productID, nil
}

// handleAddProductForShop HTTP处理函数
func handleAddProductForShop(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Only supports POST method", http.StatusMethodNotAllowed)
		return
	}

	// 解析请求体中的商品数据
	var product Product
	if err := json.NewDecoder(r.Body).Decode(&product); err != nil {
		http.Error(w, "Request body parse error", http.StatusBadRequest)
		return
	}

	// 确保 shopID 是有效的
	if product.ShopID == 0 {
		http.Error(w, "Invalid shop ID", http.StatusBadRequest)
		return
	}

	// 添加商品到店铺
	productID, err := addProductForShop(rp, db, product.ShopID, &product)
	if err != nil {
		http.Error(w, fmt.Sprintf("Failed to add product: %v", err), http.StatusInternalServerError)
		return
	}

	// 返回成功信息
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":     "Product added successfully",
		"product_id": productID,
		"product":    product, // 返回商品详细信息
	})
}
