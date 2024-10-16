package main

import (
	"fmt"
	"math/rand"
	"time"
)

type Job struct {
	Id      int
	RandNum int
}

type Result struct {
	job *Job
	sum int
}

func main() {
	jobChan := make(chan *Job, 128)
	resultChan := make(chan *Result, 128)

	// 设置一个固定的工作池大小
	numWorkers := 64
	numJobs := 100 // 设定一个固定的任务数量

	createPool(numWorkers, jobChan, resultChan)

	go func(resultChan chan *Result) {
		for result := range resultChan {
			fmt.Printf("job id:%v randnum:%v result:%d\n", result.job.Id, result.job.RandNum, result.sum)
		}
	}(resultChan)

	// 创建固定数量的job
	for id := 1; id <= numJobs; id++ {
		r_num := rand.Int()
		job := &Job{
			Id:      id,
			RandNum: r_num,
		}
		jobChan <- job
	}

	close(jobChan) // 关闭jobChan，通知所有worker没有更多的job了

	// 等待所有结果被处理完
	time.Sleep(2 * time.Second) // 简单的等待，可以根据实际情况调整或使用更好的方式
	close(resultChan)           // 关闭结果管道
}

func createPool(num int, jobChan chan *Job, resultChan chan *Result) {
	for i := 0; i < num; i++ {
		go func(jobChan chan *Job, resultChan chan *Result) {
			for job := range jobChan {
				r_num := job.RandNum
				var sum int
				for r_num != 0 {
					tmp := r_num % 10
					sum += tmp
					r_num /= 10
				}
				r := &Result{
					job: job,
					sum: sum,
				}
				resultChan <- r
			}
		}(jobChan, resultChan)
	}
}
