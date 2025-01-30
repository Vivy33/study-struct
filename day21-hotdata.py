# 一个整数数组 nums 和一个整数 k，用一个字符串返回其中出现频率前 k 高的元素

from collections import Counter
from typing import List

def topKFrequent(nums: List[int], k: int) -> str:
    # 使用 Counter 统计每个元素的频率
    count = Counter(nums)
    
    # 创建桶，桶的索引代表频率，值为具有该频率的元素列表
    buckets = [[] for _ in range(len(nums) + 1)]
    for num, freq in count.items():
        buckets[freq].append(num)
    
    # 从高频到低频遍历桶，收集前 k 个高频元素
    result = []
    for i in range(len(buckets) - 1, 0, -1):
        result.extend(buckets[i])
        if len(result) >= k:
            break
    
    # 只取前 k 个元素，并按升序排序
    result = sorted(result[:k])
    
    # 将结果转换为字符串
    return ','.join(map(str, result))

# 测试代码
test_cases = [
    ([1, 1, 1, 2, 2, 3], 2),
    ([1], 1),
    ([4, 4, 4, 2, 2, 2, 3, 3, 1], 2)
]

for nums, k in test_cases:
    print(f"Input: nums = {nums}, k = {k}")
    print(f"Output: \"{topKFrequent(nums, k)}\"")
    print()
