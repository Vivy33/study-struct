# 在一个由 '0' 和 '1' 组成的二维矩阵内，找到只包含 '1' 的最大正方形，并返回其面积。

# 示例 1：
# 输入：matrix = [["1","0","1","0","0"],["1","0","1","1","1"],["1","1","1","1","1"],["1","0","0","1","0"]]
# 输出：4

# 示例 2：
# 输入：matrix = [["0","1"],["1","0"]]
# 输出：1
# 示例 3：

# 输入：matrix = [["0"]]
# 输出：0

def maximalSquare(matrix):
    if not matrix or not matrix[0]:
        return 0
    
    rows = len(matrix)
    cols = len(matrix[0])
    max_side = 0
    
    # 初始化 dp 数组
    dp = [[0] * (cols + 1) for _ in range(rows + 1)]
    
    for i in range(1, rows + 1):
        for j in range(1, cols + 1):
            if matrix[i-1][j-1] == '1':
                # 更新 dp[i][j] 为左上、上、左三个方向的最小值加 1
                dp[i][j] = min(dp[i-1][j], dp[i][j-1], dp[i-1][j-1]) + 1
                # 更新最大边长
                max_side = max(max_side, dp[i][j])
    
    # 返回最大正方形的面积
    return max_side * max_side

# 示例测试
matrix1 = [
    ["1","0","1","0","0"],
    ["1","0","1","1","1"],
    ["1","1","1","1","1"],
    ["1","0","0","1","0"]
]

matrix2 = [
    ["0","1"],
    ["1","0"]
]

matrix3 = [
    ["0"]
]

print(maximalSquare(matrix1))  
print(maximalSquare(matrix2))  
print(maximalSquare(matrix3))  