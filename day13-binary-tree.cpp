/*给定一个二叉树 root ，返回其最大深度。

二叉树的 最大深度 是指从根节点到最远叶子节点的最长路径上的节点数。

输入：root = [3,9,20,null,null,15,7]
输出：3用c语言，能加const的加const，需要释放内存的释放，在每个循环处加printf来表示当前的操作*/

#include <stdio.h>
#include <stdlib.h>

// 定义二叉树节点结构
struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

// 创建一个新的二叉树节点
struct TreeNode* createNode(int val) {
    struct TreeNode* node = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    node->val = val;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// 插入节点到二叉树中
struct TreeNode* insertNode(struct TreeNode* root, int val) {
    if (root == NULL) {
        printf("Inserting new node with value %d\n", val);
        return createNode(val);
    }
    if (val < root->val) {
        printf("Going left from node with value %d\n", root->val);
        root->left = insertNode(root->left, val);
    } else {
        printf("Going right from node with value %d\n", root->val);
        root->right = insertNode(root->right, val);
    }
    return root;
}

// 递归计算二叉树的最大深度
int maxDepth(const struct TreeNode* root) {
    if (root == NULL) {
        printf("Reached a leaf node, returning 0\n");
        return 0;
    }
    
    printf("Visiting node with value %d\n", root->val);
    int leftDepth = maxDepth(root->left);
    int rightDepth = maxDepth(root->right);
    
    int max_depth = (leftDepth > rightDepth ? leftDepth : rightDepth) + 1;
    printf("Node %d - leftDepth: %d, rightDepth: %d, maxDepth: %d\n", root->val, leftDepth, rightDepth, max_depth);
    
    return max_depth;
}

// 释放二叉树的内存
void freeTree(struct TreeNode* root) {
    if (root == NULL) {
        return;
    }
    
    printf("Freeing node with value %d\n", root->val);
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

// 主函数进行测试
int main() {
    // 构建测试二叉树
    struct TreeNode* root = NULL;
    root = insertNode(root, 3);
    root = insertNode(root, 9);
    root = insertNode(root, 20);
    root = insertNode(root, 15);
    root = insertNode(root, 7);
    
    // 计算最大深度
    printf("Calculating the maximum depth of the binary tree...\n");
    int depth = maxDepth(root);
    printf("The maximum depth of the binary tree is: %d\n", depth);
    
    // 释放二叉树内存
    printf("Freeing the binary tree...\n");
    freeTree(root);
    
    return 0;
}

