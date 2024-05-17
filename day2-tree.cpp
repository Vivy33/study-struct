/*国际摩尔斯密码定义一种标准编码方式，将每个字母对应于一个由一系列点和短线组成的字符串， 比如:

'a' 对应 ".-" ，
'b' 对应 "-..." ，
'c' 对应 "-.-." ，以此类推。
为了方便，所有 26 个英文字母的摩尔斯密码表如下：

[".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."]
给你一个字符串数组 words ，每个单词可以写成每个字母对应摩尔斯密码的组合。

例如，"cab" 可以写成 "-.-..--..." ，(即 "-.-." + ".-" + "-..." 字符串的结合)。我们将这样一个连接过程称作 单词翻译 。
对 words 中所有单词进行单词翻译，返回不同 单词翻译 的数量。*/

#include <iostream>
#include <vector>
#include <unordered_set>

using namespace std;

//定义摩尔斯密码的二叉树结点
struct TreeNode{
    char val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(char c):val(c),left(nullptr),right(nullptr){}
};

//构建摩尔斯密码的二叉树
TreeNode* buildMorseTree(vector<string>& morseCodes){
    TreeNode* root = new TreeNode('*');//根节点

    for(const string& code:morseCodes){
        TreeNode* node = root;
        for(char c:code){
            if(c == '.'){
                if(!node->left){
                    node->left = new TreeNode('*');
                }
                node = node->left;

            }else if(c == '-'){
                if(!node->right){
                    node->right = new TreeNode('*');
                }
                node = node->right;

            }
        }
    }
    return root;
}

//计算不同单词翻译的数量
int quantity(vector<string>& words){
    vector<string> morseCodes = {".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
    unordered_set<string> translations;

    TreeNode* root = buildMorseTree(morseCodes);

    for(string& word:words){
        string translation;
        for(char c:word){
            string path;
            TreeNode* node=root;
            while(c != node->val){
                if(node->left && node->left->val == c){
                    path += '.';
                    node = node->left;//移动到左节点
                }else if(node->right && node->right->val == c){
                    path += '-';
                    node = node->right;//移动到右节点
                }else{
                    //处理当前字符无法匹配当前节点的情况（既不是 左节点 又不是 右节点）
                    break;
                }
            }
            translation += path;
        }
        translations.insert(translation);
    }
    return translations.size();
}

int main(){
    vector<string> words = {"gin", "zen", "gig", "msg"};
    cout<<"单词翻译数量："<<quantity(words)<<endl;
    return 0;   
}