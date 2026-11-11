// 10-trees  --  stubs for recursion-on-structure problems.
//
// OWNERSHIP RULE: functions never free nodes; TreeArena owns every node (also those created
// by deserialize, which therefore takes an arena -- a deliberate deviation from LeetCode's signature).
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace lc {

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    explicit TreeNode(int v = 0) : val(v), left(nullptr), right(nullptr) {}
};

using Level = std::vector<std::optional<int>>;  // LeetCode level-order array, std::nullopt == null

class TreeArena {
public:
    TreeNode* make(int v) {
        nodes_.push_back(std::make_unique<TreeNode>(v));
        return nodes_.back().get();
    }
    // Build from LeetCode's level-order format, e.g. {3, 9, 20, nullopt, nullopt, 15, 7}.
    TreeNode* build(const Level& level) {
        if (level.empty() || !level[0]) return nullptr;
        TreeNode* root = make(*level[0]);
        std::queue<TreeNode*> q;
        q.push(root);
        std::size_t i = 1;
        while (!q.empty() && i < level.size()) {
            TreeNode* cur = q.front();
            q.pop();
            if (i < level.size() && level[i]) { cur->left = make(*level[i]); q.push(cur->left); }
            ++i;
            if (i < level.size() && level[i]) { cur->right = make(*level[i]); q.push(cur->right); }
            ++i;
        }
        return root;
    }

private:
    std::vector<std::unique_ptr<TreeNode>> nodes_;
};

// Inverse of build(): trailing nulls trimmed. Handy for comparing whole trees in tests.
inline Level to_level(const TreeNode* root) {
    Level out;
    if (root == nullptr) return out;
    std::queue<const TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        const TreeNode* n = q.front();
        q.pop();
        if (n == nullptr) { out.push_back(std::nullopt); continue; }
        out.push_back(n->val);
        q.push(n->left);
        q.push(n->right);
    }
    while (!out.empty() && !out.back()) out.pop_back();
    return out;
}

// 104  Maximum Depth of Binary Tree.  Context: depth of a nested protocol-header / TLV tree.
int max_depth(const TreeNode* root);

// 226  Invert Binary Tree.  Context: mirror a decision tree (swap the 0/1 branches of a Huffman-like code).
TreeNode* invert_tree(TreeNode* root);

// 102  Binary Tree Level Order Traversal.  Context: breadth-first dump of a scheduler's hierarchy, one tier per line.
std::vector<std::vector<int>> level_order(const TreeNode* root);

// 98   Validate Binary Search Tree.  Context: verify a sorted index (e.g. frequency table) was not corrupted.
bool is_valid_bst(const TreeNode* root);

// 297  Serialize and Deserialize Binary Tree.  Context: ship a tree over a byte stream (preorder + null markers).
// Format used here: comma separated preorder, '#' for null, e.g. "1,2,#,#,3,#,#".
std::string serialize(const TreeNode* root);
TreeNode* deserialize(const std::string& data, TreeArena& arena);

}  // namespace lc
