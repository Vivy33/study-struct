package main

import (
	"fmt"
)

// searchInsert函数，返回目标值在数组中的索引，如果目标值不存在，则返回应该插入的位置
func searchInsert(nums []int, target int) int {
	left, right := 0, len(nums)-1
	for left <= right {
		fmt.Printf("循环开始 -> 当前left: %d, right: %d\n", left, right)
		mid := left + (right-left)/2
		fmt.Printf("  计算mid -> left: %d, right: %d, mid: %d\n", left, right, mid)

		if nums[mid] == target {
			fmt.Printf("找到目标值 -> mid: %d\n", mid)
			return mid
		} else if nums[mid] < target {
			fmt.Printf("    目标值大于中间值 -> nums[mid]: %d, target: %d -> 更新left: %d -> %d\n", nums[mid], target, left, mid+1)
			left = mid + 1
		} else {
			fmt.Printf("    目标值小于中间值 -> nums[mid]: %d, target: %d -> 更新right: %d -> %d\n", nums[mid], target, right, mid-1)
			right = mid - 1
		}
	}
	fmt.Printf("退出循环，left即为插入位置 -> left: %d\n", left)
	return left // 当退出循环时，left 即为应该插入的位置
}

func main() {
	// 测试样例
	nums1 := []int{1, 3, 5, 6}
	target1 := 5
	result1 := searchInsert(nums1, target1)
	fmt.Printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target1, result1)
	fmt.Println()

	nums2 := []int{1, 3, 5, 6}
	target2 := 2
	result2 := searchInsert(nums2, target2)
	fmt.Printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target2, result2)
	fmt.Println()

	nums3 := []int{1, 3, 5, 6}
	target3 := 7
	result3 := searchInsert(nums3, target3)
	fmt.Printf("输入: nums = [1,3,5,6], target = %d\n输出: %d\n", target3, result3)
	fmt.Println()
}
