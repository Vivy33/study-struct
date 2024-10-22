package main

import "encoding/json"

// Process 结构体用于映射数据库中的表
type Process struct {
	PID       int    `json:"pid"`
	ProcName  string `json:"proc_name"`
	Cmdline   string `json:"cmdline"`
	ExePath   string `json:"exe_path"`
	StartTime uint64 `json:"start_time"`
	NumVmas   int    `json:"num_vmas"`
}

// 序列化 Process 为 JSON
func (p *Process) ToJSON() (string, error) {
	data, err := json.Marshal(p)
	if err != nil {
		return "", err
	}
	return string(data), nil
}

// 从 JSON 反序列化为 Process
func FromJSON(data string) (*Process, error) {
	var p Process
	err := json.Unmarshal([]byte(data), &p)
	if err != nil {
		return nil, err
	}
	return &p, nil
}
