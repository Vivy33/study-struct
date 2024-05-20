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
#include <stdio.h>

using namespace std;

//定义摩尔斯密码的二叉树结点
struct TreeNode{
    char val;
    bool word_end;
    bool root;//根节点
    TreeNode* dot;
    TreeNode* connect;
    TreeNode(char c):val(c),dot(nullptr),connect(nullptr){}
};

//构建摩尔斯密码的二叉树
TreeNode* buildMorseTree(vector<string>& morseCodes){
    TreeNode* root = new TreeNode('*');//根节点

    for(const string& code : morseCodes){
        TreeNode* current = root;
        for(size_t i = 0; i < code.size(); i++){
            printf("当前处理的是code: %s, 它的长度是 %lu\n", code.c_str(), code.size());
            if(code[i] == '.'){
                printf("  当前处理的是dot\n");
                if(!current->dot){
                    current->dot = new TreeNode('.');
                    printf("    开辟一个新dot\n");
                }
                current = current->dot;
                printf("  当前节点指向dot\n");
                if(i == code.size()-1){
                    printf("    morse标码结束，在当前节点加上结束标志\n");
                    current->word_end = true;
                }
            }else if(code[i] == '-'){
                printf("  当前处理的是 - \n");
                if(!current->connect){
                    current->connect = new TreeNode('-');
                    printf("    开辟一个新 - \n");
                }
                current = current->connect;
                printf("  当前节点指向 - \n");
                if(i == code.size()-1){
                    printf("    morse标码结束，在当前节点加上结束标志\n");
                    current->word_end = true;
                }
            }else{
                printf("  当前输入%c，不合法\n", code[i]);
            }
            printf("\n\n");    
        }
        printf("----------------------------------------------------\n");
    }
    return root;
}

//计算不同单词翻译的数量
int quantity(vector<string>& words){
    vector<string> morseCodes = {".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
    unordered_set<string> translations;

    TreeNode* root = buildMorseTree(morseCodes);

    for(string& word:words){
        printf("当前word是 %s\n", word.c_str());
        string translation;
        for(char c:word){
            printf("  当前word中的字符为 %c\n", c);
            string path;
            TreeNode* current = root;
            while (current != nullptr && c != current->val) {
            printf("当前字符c与当前节点值不匹配,继续循环\n");
                if (current->dot && current->dot->val == c) {
                    printf("有左节点\n");
                    path += '.';
                    current = current->dot; // 移动到左节点
                } else if (current->connect && current->connect->val == c) {
                    printf("有右节点\n");
                    path += '-';
                    current = current->connect; // 移动到右节点
                } else {
                    printf("跳出当前循环\n");
                    break;
                }
            }
            printf("\n\n");
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