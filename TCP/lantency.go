// 测量出站（活动）tcp连接的首字节延时
package main

import (
    "fmt"
    "io"
    "net"
    "time"
)

func main() {
    // 目标服务器地址和端口
    address := "example.com:80"

    // 开始计时
    start := time.Now()

    // 创建 TCP 连接
    conn, err := net.DialTimeout("tcp", address, 5*time.Second)
    if err != nil {
        fmt.Printf("连接失败: %v\n", err)
        return
    }
    defer conn.Close()

    // 记录连接建立时间
    connectionEstablished := time.Since(start)
·
    // 构造 HTTP GET 请求
    request := "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n"

    // 发送请求
    _, err = conn.Write([]byte(request))
    if err != nil {
        fmt.Printf("发送请求失败: %v\n", err)
        return
    }

    // 记录发送请求的时间
    requestSent := time.Since(start)

    // 读取响应的第一个字节
    buffer := make([]byte, 1)
    _, err = io.ReadFull(conn, buffer)
    if err != nil {
        fmt.Printf("读取响应失败: %v\n", err)
        return
    }

    // 计算首字节延时
    firstByteDuration := time.Since(start)

    // 输出结果
    fmt.Printf("连接建立时间: %v\n", connectionEstablished)
    fmt.Printf("请求发送时间: %v\n", requestSent)
    fmt.Printf("首字节延时: %v\n", firstByteDuration)
    fmt.Printf("服务器处理时间 (首字节延时 - 请求发送时间): %v\n", firstByteDuration-requestSent)
}