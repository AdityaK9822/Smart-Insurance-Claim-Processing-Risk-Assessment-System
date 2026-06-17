/*
 * Smart Insurance Claim Processing & Risk Assessment System
 * DSA Logic Layer - Handles all data structures and algorithms
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  DATA MODELS
// ─────────────────────────────────────────────────────────────────────────────

struct Policy {
  int id;
  string holder;
  string type; // "Health", "Auto", "Life", "Property"
  double premium;
  int riskScore; // 0-100
  bool active;
};

struct Claim {
  int claimId;
  int policyId;
  string holder;
  string type;
  double amount;
  int priority;  // 1-10 (10 = highest)
  string status; // "Pending", "Processing", "Approved", "Rejected"
  bool fraudFlag;
};

struct ClaimAction {
  string description;
  Claim snapshot;
};

// ─────────────────────────────────────────────────────────────────────────────
//  SAMPLE DATA
// ─────────────────────────────────────────────────────────────────────────────

inline vector<Policy> gPolicies = {
    {101, "Aarav Sharma", "Health", 12000, 35, true},
    {102, "Bhavna Patel", "Auto", 8500, 62, true},
    {103, "Chetan Gupta", "Life", 22000, 18, true},
    {104, "Divya Iyer", "Property", 15000, 74, true},
    {105, "Eshan Rao", "Health", 9800, 55, true},
    {106, "Falguni Shah", "Auto", 7200, 41, true},
    {107, "Gautam Nair", "Life", 18500, 28, false},
    {108, "Hina Malik", "Property", 21000, 88, true},
    {109, "Ishaan Reddy", "Health", 11500, 60, true},
    {110, "Jiya Kapoor", "Auto", 6800, 33, true},
    {111, "Kabir Singh", "Health", 10500, 42, true},
    {112, "Lara Croft", "Auto", 9200, 38, true},
    {113, "Myra Khan", "Life", 25000, 22, true},
    {114, "Naveen Kumar", "Property", 18000, 65, true},
    {115, "Olive Young", "Health", 11000, 58, true},
    {116, "Prisha Paul", "Auto", 7800, 31, true},
    {117, "Quentin Tarantino", "Life", 30000, 19, false},
    {118, "Rhea Chakraborty", "Property", 16000, 70, true},
    {119, "Sahil Vohra", "Health", 10200, 47, true},
    {120, "Tara Sutaria", "Auto", 8300, 36, true},
};

inline vector<Claim> gClaims = {
    {1001, 101, "Aarav Sharma", "Health", 45000, 8, "Pending", true},
    {1002, 102, "Bhavna Patel", "Auto", 12000, 5, "Processing", false},
    {1003, 103, "Chetan Gupta", "Property", 120000, 9, "Pending", false},
    {1004, 104, "Divya Iyer", "Property", 95000, 10, "Pending", true},
    {1005, 105, "Eshan Rao", "Health", 22000, 6, "Approved", false},
    {1006, 106, "Falguni Shah", "Health", 18000, 7, "Pending", false},
    {1007, 107, "Gautam Nair", "Auto", 8500, 3, "Rejected", true},
    {1008, 108, "Hina Malik", "Auto", 14000, 5, "Processing", false},
    {1009, 109, "Ishaan Reddy", "Life", 200000, 9, "Pending", false},
    {1010, 110, "Jiya Kapoor", "Auto", 9500, 4, "Pending", false},
};

inline unordered_map<int, vector<int>> gFraudGraph = {
    {1001, {1004}}, {1004, {1001, 1007}}, {1007, {1004}}, {1002, {}},
    {1003, {}},     {1005, {}},           {1006, {}},     {1008, {}},
    {1009, {}},     {1010, {}},
};

// ─────────────────────────────────────────────────────────────────────────────
//  HASHING
// ─────────────────────────────────────────────────────────────────────────────

class PolicyHashTable {
public:
  unordered_map<int, Policy *> table;

  void build(vector<Policy> &policies) {
    for (auto &p : policies)
      table[p.id] = &p;
  }

  Policy *verify(int id) {
    auto it = table.find(id);
    return (it != table.end()) ? it->second : nullptr;
  }
};

inline PolicyHashTable gHashTable;

// ─────────────────────────────────────────────────────────────────────────────
//  BST + AVL TREE
// ─────────────────────────────────────────────────────────────────────────────

struct AVLNode {
  Policy data;
  AVLNode *left = nullptr;
  AVLNode *right = nullptr;
  int height = 1;
};

class AVLTree {
  int height(AVLNode *n) { return n ? n->height : 0; }
  int balanceFactor(AVLNode *n) {
    return n ? height(n->left) - height(n->right) : 0;
  }
  void updateHeight(AVLNode *n) {
    if (n)
      n->height = 1 + max(height(n->left), height(n->right));
  }
  AVLNode *rotateRight(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
  }
  AVLNode *rotateLeft(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
  }
  AVLNode *balance(AVLNode *n) {
    updateHeight(n);
    int bf = balanceFactor(n);
    if (bf > 1) {
      if (balanceFactor(n->left) < 0)
        n->left = rotateLeft(n->left);
      return rotateRight(n);
    }
    if (bf < -1) {
      if (balanceFactor(n->right) > 0)
        n->right = rotateRight(n->right);
      return rotateLeft(n);
    }
    return n;
  }
  AVLNode *insert(AVLNode *node, const Policy &p) {
    if (!node)
      return new AVLNode{p};
    if (p.id < node->data.id)
      node->left = insert(node->left, p);
    else if (p.id > node->data.id)
      node->right = insert(node->right, p);
    return balance(node);
  }
  void inorder(AVLNode *node, vector<Policy> &out) {
    if (!node)
      return;
    inorder(node->left, out);
    out.push_back(node->data);
    inorder(node->right, out);
  }
  AVLNode *search(AVLNode *node, int id) {
    if (!node || node->data.id == id)
      return node;
    if (id < node->data.id)
      return search(node->left, id);
    return search(node->right, id);
  }

public:
  AVLNode *root = nullptr;
  void insert(const Policy &p) { root = insert(root, p); }
  vector<Policy> getSorted() {
    vector<Policy> v;
    inorder(root, v);
    return v;
  }
  Policy *find(int id) {
    AVLNode *n = search(root, id);
    return n ? &n->data : nullptr;
  }
};

inline AVLTree gAVLTree;

// ─────────────────────────────────────────────────────────────────────────────
//  SORTING ALGORITHMS
// ─────────────────────────────────────────────────────────────────────────────

inline void bubbleSort(vector<Claim> &v) {
  int n = v.size();
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - i - 1; j++)
      if (v[j].priority < v[j + 1].priority)
        swap(v[j], v[j + 1]);
}

inline void selectionSort(vector<Claim> &v) {
  int n = v.size();
  for (int i = 0; i < n - 1; i++) {
    int mx = i;
    for (int j = i + 1; j < n; j++)
      if (v[j].priority > v[mx].priority)
        mx = j;
    swap(v[i], v[mx]);
  }
}

inline void insertionSort(vector<Claim> &v) {
  int n = v.size();
  for (int i = 1; i < n; i++) {
    Claim key = v[i];
    int j = i - 1;
    while (j >= 0 && v[j].priority < key.priority) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = key;
  }
}

inline void merge(vector<Claim> &v, int l, int m, int r) {
  vector<Claim> left(v.begin() + l, v.begin() + m + 1);
  vector<Claim> right(v.begin() + m + 1, v.begin() + r + 1);
  int i = 0, j = 0, k = l;
  while (i < (int)left.size() && j < (int)right.size())
    v[k++] = (left[i].priority >= right[j].priority) ? left[i++] : right[j++];
  while (i < (int)left.size())
    v[k++] = left[i++];
  while (j < (int)right.size())
    v[k++] = right[j++];
}

inline void mergeSort(vector<Claim> &v, int l, int r) {
  if (l >= r)
    return;
  int m = l + (r - l) / 2;
  mergeSort(v, l, m);
  mergeSort(v, m + 1, r);
  merge(v, l, m, r);
}

inline int partition(vector<Claim> &v, int l, int r) {
  int pivot = v[r].priority;
  int i = l - 1;
  for (int j = l; j < r; j++)
    if (v[j].priority >= pivot)
      swap(v[++i], v[j]);
  swap(v[i + 1], v[r]);
  return i + 1;
}

inline void quickSort(vector<Claim> &v, int l, int r) {
  if (l >= r)
    return;
  int p = partition(v, l, r);
  quickSort(v, l, p - 1);
  quickSort(v, p + 1, r);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SEARCHING ALGORITHMS
// ─────────────────────────────────────────────────────────────────────────────

inline vector<Claim> linearSearch(const vector<Claim> &claims, const string &name) {
  vector<Claim> result;
  for (const auto &c : claims)
    if (c.holder.find(name) != string::npos)
      result.push_back(c);
  return result;
}

inline int binarySearch(const vector<Claim> &claims, int targetId) {
  int lo = 0, hi = (int)claims.size() - 1;
  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    if (claims[mid].claimId == targetId)
      return mid;
    else if (claims[mid].claimId < targetId)
      lo = mid + 1;
    else
      hi = mid - 1;
  }
  return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  STACK & QUEUE
// ─────────────────────────────────────────────────────────────────────────────

inline stack<ClaimAction> gUndoStack;

// Forward declaration for enqueuePendingClaims
void enqueuePendingClaims();

inline string undoLastAction() {
  if (gUndoStack.empty())
    return "Nothing to undo.";
  ClaimAction a = gUndoStack.top();
  gUndoStack.pop();
  for (auto &c : gClaims) {
    if (c.claimId == a.snapshot.claimId) {
      c = a.snapshot;
      break;
    }
  }
  enqueuePendingClaims();
  return "Undone: " + a.description;
}

inline void pushAction(const string &desc, const Claim &c) {
  gUndoStack.push({desc, c});
}

inline queue<int> gClaimQueue;

inline void enqueuePendingClaims() {
  while (!gClaimQueue.empty())
    gClaimQueue.pop();
  vector<Claim> pending;
  for (auto &c : gClaims)
    if (c.status == "Pending")
      pending.push_back(c);
  if (!pending.empty()) {
      mergeSort(pending, 0, (int)pending.size() - 1);
  }
  for (auto &c : pending)
    gClaimQueue.push(c.claimId);
}

inline string processNextClaim() {
  if (gClaimQueue.empty())
    return "Queue is empty. No pending claims.";
  int id = gClaimQueue.front();
  gClaimQueue.pop();
  for (auto &c : gClaims) {
    if (c.claimId == id) {
      pushAction("Status changed from " + c.status, c);
      if (c.fraudFlag) {
        c.status = "Rejected";
        return "❌ Claim #" + to_string(id) + " for " + c.holder +
               " REJECTED: Fraud suspected!";
      }
      c.status = "Processing";
      return "Processing claim #" + to_string(id) + " for " + c.holder;
    }
  }
  return "Claim not found.";
}

// ─────────────────────────────────────────────────────────────────────────────
//  BFS & DFS
// ─────────────────────────────────────────────────────────────────────────────

inline vector<int> bfsFraud(int startClaimId) {
  vector<int> visited_order;
  unordered_set<int> visited;
  queue<int> q;
  q.push(startClaimId);
  visited.insert(startClaimId);
  while (!q.empty()) {
    int node = q.front();
    q.pop();
    visited_order.push_back(node);
    if (gFraudGraph.count(node)) {
        for (int neighbor : gFraudGraph[node]) {
          if (!visited.count(neighbor)) {
            visited.insert(neighbor);
            q.push(neighbor);
          }
        }
    }
  }
  return visited_order;
}

inline void dfsHelper(int node, unordered_set<int> &visited, vector<int> &order) {
  visited.insert(node);
  order.push_back(node);
  if (gFraudGraph.count(node)) {
      for (int neighbor : gFraudGraph[node])
        if (!visited.count(neighbor))
          dfsHelper(neighbor, visited, order);
  }
}

inline vector<int> dfsFraud(int startClaimId) {
  vector<int> order;
  unordered_set<int> visited;
  dfsHelper(startClaimId, visited, order);
  return order;
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────

inline string fmt(double v) {
  ostringstream ss;
  ss << "₹" << fixed << setprecision(0) << v;
  return ss.str();
}

inline void initSystem() {
  gHashTable.build(gPolicies);
  for (auto &p : gPolicies)
    gAVLTree.insert(p);
  enqueuePendingClaims();
}
