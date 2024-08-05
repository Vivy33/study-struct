#include <stdio.h>
#include <stdlib.h>

// 定义二叉树节点结构
typedef struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// 定义树根的别名
typedef struct {
    TreeNode *root;
} Tree;

// 创建一个新的二叉树节点
TreeNode* createNode(int val) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    node->val = val;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// 创建一个新的树
Tree* createTree() {
    Tree* tree = (Tree*)malloc(sizeof(Tree));
    if (tree == NULL) {
        perror("Failed to allocate memory for new tree");
        exit(EXIT_FAILURE);
    }
    tree->root = NULL;
    return tree;
}

// 插入节点到二叉树中
TreeNode* insertNode(TreeNode* root, int val) {
    if (root == NULL) {
        printf("Cannot insert into an empty tree. Create the root first.\n");
        return NULL;
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

// 在树中插入根节点
void insertRoot(Tree* tree, int val) {
    if (tree->root != NULL) {
        printf("Root already exists. Inserting into existing tree.\n");
        insertNode(tree->root, val);
    } else {
        printf("Creating root node with value %d\n", val);
        tree->root = createNode(val);
    }
}

// 递归计算二叉树的最大深度
int maxDepth(const TreeNode* root) {
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
void freeTree(TreeNode* root) {
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
    // 创建一棵新的树
    Tree* tree = createTree();
    
    // 构建测试二叉树
    insertRoot(tree, 3);   // 创建根节点
    insertRoot(tree, 9);   // 插入节点
    insertRoot(tree, 20);  // 插入节点
    insertRoot(tree, 15);  // 插入节点
    insertRoot(tree, 7);   // 插入节点
    
    // 计算最大深度
    printf("Calculating the maximum depth of the binary tree...\n");
    int depth = maxDepth(tree->root);
    printf("The maximum depth of the binary tree is: %d\n", depth);
    
    // 释放二叉树内存
    printf("Freeing the binary tree...\n");
    freeTree(tree->root);
    
    // 释放树的结构
    free(tree);
    
    return 0;
}
