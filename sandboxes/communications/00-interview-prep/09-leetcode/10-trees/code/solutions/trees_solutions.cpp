#include "trees.hpp"

#include <algorithm>
#include <climits>
#include <stack>
#include <utility>

namespace lc {

// 104 -- depth(n) = 1 + max(depth(left), depth(right)); depth(null) = 0. O(n) time, O(h) stack.
int max_depth(const TreeNode* root) {
    if (root == nullptr) return 0;
    return 1 + std::max(max_depth(root->left), max_depth(root->right));
}

// 226 -- mirror = swap children, then mirror each child.
TreeNode* invert_tree(TreeNode* root) {
    if (root == nullptr) return nullptr;
    std::swap(root->left, root->right);
    invert_tree(root->left);
    invert_tree(root->right);
    return root;
}

// 102 -- queue + "snapshot the size" trick: the queue holds exactly one level at the top of each round.
std::vector<std::vector<int>> level_order(const TreeNode* root) {
    std::vector<std::vector<int>> out;
    if (root == nullptr) return out;
    std::queue<const TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        const std::size_t width = q.size();
        std::vector<int> row;
        row.reserve(width);
        for (std::size_t i = 0; i < width; ++i) {
            const TreeNode* n = q.front();
            q.pop();
            row.push_back(n->val);
            if (n->left) q.push(n->left);
            if (n->right) q.push(n->right);
        }
        out.push_back(std::move(row));
    }
    return out;
}

// 98 -- each node must lie in an OPEN interval (lo, hi) inherited from ALL its ancestors, not just its parent.
// long long bounds so INT_MIN / INT_MAX node values are not confused with "no bound".
namespace {
bool bst_ok(const TreeNode* n, long long lo, long long hi) {
    if (n == nullptr) return true;
    if (n->val <= lo || n->val >= hi) return false;
    return bst_ok(n->left, lo, n->val) && bst_ok(n->right, n->val, hi);
}
}  // namespace

bool is_valid_bst(const TreeNode* root) { return bst_ok(root, LLONG_MIN, LLONG_MAX); }

// 297 -- preorder with explicit null markers is uniquely decodable: the recursion consumes
// exactly the tokens belonging to each subtree.
namespace {
void ser(const TreeNode* n, std::string& out) {
    if (n == nullptr) {
        out += "#,";
        return;
    }
    out += std::to_string(n->val);
    out.push_back(',');
    ser(n->left, out);
    ser(n->right, out);
}

TreeNode* des(const std::string& s, std::size_t& pos, TreeArena& arena) {
    const std::size_t end = s.find(',', pos);
    const std::string tok = s.substr(pos, end - pos);
    pos = end + 1;
    if (tok == "#") return nullptr;
    TreeNode* n = arena.make(std::stoi(tok));
    n->left = des(s, pos, arena);
    n->right = des(s, pos, arena);
    return n;
}
}  // namespace

std::string serialize(const TreeNode* root) {
    std::string out;
    ser(root, out);
    return out;
}

TreeNode* deserialize(const std::string& data, TreeArena& arena) {
    if (data.empty()) return nullptr;
    std::size_t pos = 0;
    return des(data, pos, arena);
}

}  // namespace lc
