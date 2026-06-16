/*
 * Smart Insurance Claim Processing & Risk Assessment System
 * DSA Concepts Used (from curriculum):
 *   - Sorting      : Bubble, Selection, Insertion, Merge, Quick (claim priority
 * ranking)
 *   - Searching    : Linear, Binary (policyholder lookup)
 *   - Stack        : Claim correction / undo history (LIFO)
 *   - Queue        : Claim processing pipeline (FIFO)
 *   - BST/AVL      : Insurance policy database
 *   - BFS / DFS    : Fraud network analysis
 *   - Hashing      : Rapid policy verification
 */

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

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

using namespace ftxui;
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

// ─────────────────────────────────────────────────────────────────────────────
//  SAMPLE DATA
// ─────────────────────────────────────────────────────────────────────────────

vector<Policy> gPolicies = {
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

vector<Claim> gClaims = {
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

// Fraud network edges (who is connected to whom — for BFS/DFS)
// Node IDs are claim IDs
unordered_map<int, vector<int>> gFraudGraph = {
    {1001, {1004}}, {1004, {1001, 1007}}, {1007, {1004}}, {1002, {}},
    {1003, {}},     {1005, {}},           {1006, {}},     {1008, {}},
    {1009, {}},     {1010, {}},
};

// ─────────────────────────────────────────────────────────────────────────────
//  HASHING  — rapid policy verification
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

PolicyHashTable gHashTable;

// ─────────────────────────────────────────────────────────────────────────────
//  BST + AVL TREE  — policy database
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

AVLTree gAVLTree;

// ─────────────────────────────────────────────────────────────────────────────
//  SORTING ALGORITHMS  — claim priority ranking
// ─────────────────────────────────────────────────────────────────────────────

void bubbleSort(vector<Claim> &v) {
  int n = v.size();
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - i - 1; j++)
      if (v[j].priority < v[j + 1].priority)
        swap(v[j], v[j + 1]);
}

void selectionSort(vector<Claim> &v) {
  int n = v.size();
  for (int i = 0; i < n - 1; i++) {
    int mx = i;
    for (int j = i + 1; j < n; j++)
      if (v[j].priority > v[mx].priority)
        mx = j;
    swap(v[i], v[mx]);
  }
}

void insertionSort(vector<Claim> &v) {
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

void merge(vector<Claim> &v, int l, int m, int r) {
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
void mergeSort(vector<Claim> &v, int l, int r) {
  if (l >= r)
    return;
  int m = l + (r - l) / 2;
  mergeSort(v, l, m);
  mergeSort(v, m + 1, r);
  merge(v, l, m, r);
}

int partition(vector<Claim> &v, int l, int r) {
  int pivot = v[r].priority;
  int i = l - 1;
  for (int j = l; j < r; j++)
    if (v[j].priority >= pivot)
      swap(v[++i], v[j]);
  swap(v[i + 1], v[r]);
  return i + 1;
}
void quickSort(vector<Claim> &v, int l, int r) {
  if (l >= r)
    return;
  int p = partition(v, l, r);
  quickSort(v, l, p - 1);
  quickSort(v, p + 1, r);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SEARCHING ALGORITHMS  — policyholder lookup
// ─────────────────────────────────────────────────────────────────────────────

// Linear search by holder name
vector<Claim> linearSearch(const vector<Claim> &claims, const string &name) {
  vector<Claim> result;
  for (const auto &c : claims)
    if (c.holder.find(name) != string::npos)
      result.push_back(c);
  return result;
}

// Binary search by claimId (requires sorted by claimId)
int binarySearch(const vector<Claim> &claims, int targetId) {
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
//  STACK  — claim correction / undo history (LIFO)
// ─────────────────────────────────────────────────────────────────────────────

struct ClaimAction {
  string description;
  Claim snapshot;
};

stack<ClaimAction> gUndoStack;

void pushAction(const string &desc, const Claim &c) {
  gUndoStack.push({desc, c});
}

// Forward declaration
void enqueuePendingClaims();

string undoLastAction() {
  if (gUndoStack.empty())
    return "Nothing to undo.";
  ClaimAction a = gUndoStack.top();
  gUndoStack.pop();
  // Restore the claim
  for (auto &c : gClaims) {
    if (c.claimId == a.snapshot.claimId) {
      c = a.snapshot;
      break;
    }
  }
  // Rebuild queue to reflect updated claim statuses
  enqueuePendingClaims();
  return "Undone: " + a.description;
}

// ─────────────────────────────────────────────────────────────────────────────
//  QUEUE  — claim processing pipeline (FIFO)
// ─────────────────────────────────────────────────────────────────────────────

queue<int> gClaimQueue; // stores claimIds

void enqueuePendingClaims() {
  // Clear and rebuild
  while (!gClaimQueue.empty())
    gClaimQueue.pop();
  // Enqueue pending claims sorted by priority (highest first using mergeSort)
  vector<Claim> pending;
  for (auto &c : gClaims)
    if (c.status == "Pending")
      pending.push_back(c);
  mergeSort(pending, 0, (int)pending.size() - 1);
  for (auto &c : pending)
    gClaimQueue.push(c.claimId);
}

string processNextClaim() {
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
//  BFS & DFS  — fraud network analysis
// ─────────────────────────────────────────────────────────────────────────────

vector<int> bfsFraud(int startClaimId) {
  vector<int> visited_order;
  unordered_set<int> visited;
  queue<int> q;
  q.push(startClaimId);
  visited.insert(startClaimId);
  while (!q.empty()) {
    int node = q.front();
    q.pop();
    visited_order.push_back(node);
    for (int neighbor : gFraudGraph[node]) {
      if (!visited.count(neighbor)) {
        visited.insert(neighbor);
        q.push(neighbor);
      }
    }
  }
  return visited_order;
}

void dfsHelper(int node, unordered_set<int> &visited, vector<int> &order) {
  visited.insert(node);
  order.push_back(node);
  for (int neighbor : gFraudGraph[node])
    if (!visited.count(neighbor))
      dfsHelper(neighbor, visited, order);
}

vector<int> dfsFraud(int startClaimId) {
  vector<int> order;
  unordered_set<int> visited;
  dfsHelper(startClaimId, visited, order);
  return order;
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────

const string APP_VERSION =
    "v1.1.0-FraudFixed"; // Version tracking to verify build

string fmt(double v) {
  ostringstream ss;
  ss << "₹" << fixed << setprecision(0) << v;
  return ss.str();
}

Color riskColor(int score) {
  if (score >= 70)
    return Color::Red;
  if (score >= 40)
    return Color::Yellow;
  return Color::Green;
}

Color statusColor(const string &s) {
  if (s == "Approved")
    return Color::Green;
  if (s == "Rejected")
    return Color::Red;
  if (s == "Processing")
    return Color::Yellow;
  return Color::White;
}

void initSystem() {
  gHashTable.build(gPolicies);
  for (auto &p : gPolicies)
    gAVLTree.insert(p);
  enqueuePendingClaims();
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 0 : DASHBOARD OVERVIEW
// ─────────────────────────────────────────────────────────────────────────────

Element renderDashboard() {
  int totalPolicies = gPolicies.size();
  int activePolicies = 0;
  int highRisk = 0;
  double totalPremium = 0;
  for (auto &p : gPolicies) {
    if (p.active)
      activePolicies++;
    if (p.riskScore >= 70)
      highRisk++;
    totalPremium += p.premium;
  }

  int totalClaims = gClaims.size();
  int pendingClaims = 0, fraudClaims = 0;
  double totalClaimed = 0;
  for (auto &c : gClaims) {
    if (c.status == "Pending")
      pendingClaims++;
    if (c.fraudFlag)
      fraudClaims++;
    totalClaimed += c.amount;
  }

  // KPI cards
  auto kpiCard = [](string title, string val, Color col) {
    return window(text(title) | bold,
                  vbox({
                      text(val) | color(col) | bold | center,
                  })) |
           flex;
  };

  // Recent pending claims table
  vector<Claim> pending;
  for (auto &c : gClaims)
    if (c.status == "Pending")
      pending.push_back(c);
  mergeSort(pending, 0, (int)pending.size() - 1);

  vector<Element> rows;
  rows.push_back(hbox({
                     text("Claim ID") | bold | size(WIDTH, EQUAL, 10),
                     text("Holder") | bold | size(WIDTH, EQUAL, 18),
                     text("Type") | bold | size(WIDTH, EQUAL, 12),
                     text("Amount") | bold | size(WIDTH, EQUAL, 14),
                     text("Priority") | bold | size(WIDTH, EQUAL, 10),
                     text("Fraud?") | bold | size(WIDTH, EQUAL, 8),
                 }) |
                 color(Color::Cyan));
  rows.push_back(separator());
  for (int i = 0; i < min((int)pending.size(), 5); i++) {
    auto &c = pending[i];
    rows.push_back(hbox({
        text(to_string(c.claimId)) | size(WIDTH, EQUAL, 10),
        text(c.holder) | size(WIDTH, EQUAL, 18),
        text(c.type) | size(WIDTH, EQUAL, 12),
        text(fmt(c.amount)) | size(WIDTH, EQUAL, 14),
        text(to_string(c.priority)) | size(WIDTH, EQUAL, 10) |
            color(c.priority >= 8 ? Color::Red : Color::Yellow),
        text(c.fraudFlag ? "⚠ YES" : "No") | size(WIDTH, EQUAL, 8) |
            color(c.fraudFlag ? Color::Red : Color::Green),
    }));
  }

  // Queue snapshot
  vector<int> qSnap;
  {
    queue<int> tmp = gClaimQueue;
    while (!tmp.empty()) {
      qSnap.push_back(tmp.front());
      tmp.pop();
    }
  }
  string qStr = "Queue (FIFO): ";
  for (int id : qSnap)
    qStr += "#" + to_string(id) + " → ";
  if (!qSnap.empty())
    qStr.erase(qStr.size() - 3);

  return vbox({
      text("  Smart Insurance Claim Processing & Risk Assessment System") |
          bold | color(Color::Cyan) | center,
      text("  DSA-Powered Dashboard") | color(Color::GrayLight) | center,
      separator(),
      hbox({
          kpiCard("Total Policies", to_string(totalPolicies), Color::Cyan),
          kpiCard("Active Policies", to_string(activePolicies), Color::Green),
          kpiCard("High Risk", to_string(highRisk), Color::Red),
          kpiCard("Total Premium", fmt(totalPremium), Color::Yellow),
      }),
      hbox({
          kpiCard("Total Claims", to_string(totalClaims), Color::Cyan),
          kpiCard("Pending", to_string(pendingClaims), Color::Yellow),
          kpiCard("Fraud Alerts", to_string(fraudClaims), Color::Red),
          kpiCard("Total Claimed", fmt(totalClaimed), Color::White),
      }),
      separator(),
      window(text(" Top Pending Claims (sorted by Merge Sort)"), vbox(rows)),
      separator(),
      text(" " + qStr) | color(Color::Cyan),
      text("  [Stack] Undo stack depth: " + to_string(gUndoStack.size())) |
          color(Color::GrayLight),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 1 : POLICIES (AVL Tree)
// ─────────────────────────────────────────────────────────────────────────────

Element renderPolicies(int selectedPolicySearch, const string &searchIdStr) {
  auto avlPolicies = gAVLTree.getSorted(); // inorder = sorted by ID

  vector<Element> rows;
  rows.push_back(hbox({
                     text("ID") | bold | size(WIDTH, EQUAL, 6),
                     text("Holder") | bold | size(WIDTH, EQUAL, 18),
                     text("Type") | bold | size(WIDTH, EQUAL, 12),
                     text("Premium") | bold | size(WIDTH, EQUAL, 14),
                     text("Risk") | bold | size(WIDTH, EQUAL, 8),
                     text("Status") | bold | size(WIDTH, EQUAL, 10),
                 }) |
                 color(Color::Cyan));
  rows.push_back(separator());

  for (auto &p : avlPolicies) {
    rows.push_back(hbox({
        text(to_string(p.id)) | size(WIDTH, EQUAL, 6),
        text(p.holder) | size(WIDTH, EQUAL, 18),
        text(p.type) | size(WIDTH, EQUAL, 12),
        text(fmt(p.premium)) | size(WIDTH, EQUAL, 14),
        text(to_string(p.riskScore) + "%") | size(WIDTH, EQUAL, 8) |
            color(riskColor(p.riskScore)),
        text(p.active ? "Active" : "Inactive") | size(WIDTH, EQUAL, 10) |
            color(p.active ? Color::Green : Color::Red),
    }));
  }

  // Hash verification result
  string hashResult =
      "Enter a Policy ID above and press Enter to verify via Hash Table.";
  if (!searchIdStr.empty()) {
    try {
      int id = stoi(searchIdStr);
      Policy *found = gHashTable.verify(id);
      if (found) {
        hashResult = "✔ Policy #" + to_string(id) +
                     " VERIFIED — Holder: " + found->holder +
                     " | Risk: " + to_string(found->riskScore) + "%";
      } else {
        hashResult = "✘ Policy #" + to_string(id) + " NOT FOUND in hash table.";
      }
    } catch (...) {
      hashResult = "Invalid ID entered.";
    }
  }

  return vbox({
      text(" Policy Database (AVL Tree — InOrder Traversal = Sorted by ID)") |
          bold | color(Color::Cyan),
      text(" Time Complexity: Insert O(log n), Search O(log n), Space O(n)") |
          color(Color::GrayLight),
      separator(),
      window(text(" Policy Hash Verification  [Hashing → O(1) avg]"),
             text(" " + hashResult) | color(Color::Yellow)),
      separator(),
      window(text(" All Policies (AVL InOrder)"), vbox(rows)),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 2 : CLAIMS & SORTING
// ─────────────────────────────────────────────────────────────────────────────

Element renderClaims(int sortAlgo) {
  vector<Claim> claims = gClaims;

  string algoName, complexity;
  switch (sortAlgo) {
  case 0:
    bubbleSort(claims);
    algoName = "Bubble Sort";
    complexity = "O(n²) time | O(1) space";
    break;
  case 1:
    selectionSort(claims);
    algoName = "Selection Sort";
    complexity = "O(n²) time | O(1) space";
    break;
  case 2:
    insertionSort(claims);
    algoName = "Insertion Sort";
    complexity = "O(n²) worst, O(n) best | O(1) space";
    break;
  case 3:
    mergeSort(claims, 0, (int)claims.size() - 1);
    algoName = "Merge Sort";
    complexity = "O(n log n) time | O(n) space";
    break;
  case 4:
    quickSort(claims, 0, (int)claims.size() - 1);
    algoName = "Quick Sort";
    complexity = "O(n log n) avg | O(log n) space";
    break;
  }

  vector<Element> rows;
  rows.push_back(hbox({
                     text("ID") | bold | size(WIDTH, EQUAL, 7),
                     text("Holder") | bold | size(WIDTH, EQUAL, 16),
                     text("Type") | bold | size(WIDTH, EQUAL, 10),
                     text("Amount") | bold | size(WIDTH, EQUAL, 14),
                     text("Pri") | bold | size(WIDTH, EQUAL, 5),
                     text("Status") | bold | size(WIDTH, EQUAL, 12),
                     text("Fraud") | bold | size(WIDTH, EQUAL, 8),
                 }) |
                 color(Color::Cyan));
  rows.push_back(separator());

  for (auto &c : claims) {
    rows.push_back(hbox({
        text(to_string(c.claimId)) | size(WIDTH, EQUAL, 7),
        text(c.holder) | size(WIDTH, EQUAL, 16),
        text(c.type) | size(WIDTH, EQUAL, 10),
        text(fmt(c.amount)) | size(WIDTH, EQUAL, 14),
        text(to_string(c.priority)) | size(WIDTH, EQUAL, 5) |
            color(c.priority >= 8   ? Color::Red
                  : c.priority >= 5 ? Color::Yellow
                                    : Color::White),
        text(c.status) | size(WIDTH, EQUAL, 12) | color(statusColor(c.status)),
        text(c.fraudFlag ? "⚠ YES" : "-") | size(WIDTH, EQUAL, 8) |
            color(c.fraudFlag ? Color::Red : Color::GrayLight),
    }));
  }

  return vbox({
      text(" Claims — Sorted by Priority (Rank-based)") | bold |
          color(Color::Cyan),
      text(" Algorithm: " + algoName + "  |  " + complexity) |
          color(Color::Yellow),
      text(" Use ← → arrow keys to switch sorting algorithm") |
          color(Color::GrayLight),
      separator(),
      window(text(" Claim List (sorted by priority, descending)"), vbox(rows)),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 3 : QUEUE & STACK OPERATIONS
// ─────────────────────────────────────────────────────────────────────────────

Element renderQueueStack(const string &lastMessage) {
  // Queue snapshot
  vector<int> qItems;
  {
    queue<int> tmp = gClaimQueue;
    while (!tmp.empty()) {
      qItems.push_back(tmp.front());
      tmp.pop();
    }
  }

  vector<Element> qRows;
  qRows.push_back(hbox({
                      text("Position") | bold | size(WIDTH, EQUAL, 12),
                      text("Claim ID") | bold | size(WIDTH, EQUAL, 12),
                      text("Holder") | bold | size(WIDTH, EQUAL, 18),
                      text("Priority") | bold | size(WIDTH, EQUAL, 10),
                      text("Amount") | bold | size(WIDTH, EQUAL, 14),
                  }) |
                  color(Color::Cyan));
  qRows.push_back(separator());
  for (int i = 0; i < (int)qItems.size(); i++) {
    int id = qItems[i];
    for (auto &c : gClaims) {
      if (c.claimId == id) {
        qRows.push_back(hbox({
            text(i == 0 ? "→ FRONT" : "  " + to_string(i + 1)) |
                size(WIDTH, EQUAL, 12) |
                color(i == 0 ? Color::Green : Color::White),
            text(to_string(id)) | size(WIDTH, EQUAL, 12),
            text(c.holder) | size(WIDTH, EQUAL, 18),
            text(to_string(c.priority)) | size(WIDTH, EQUAL, 10),
            text(fmt(c.amount)) | size(WIDTH, EQUAL, 14),
        }));
        break;
      }
    }
  }
  if (qItems.empty())
    qRows.push_back(text("  Queue is empty.") | color(Color::GrayLight));

  // Stack snapshot (top 5)
  vector<ClaimAction> stackItems;
  {
    stack<ClaimAction> tmp = gUndoStack;
    while (!tmp.empty() && stackItems.size() < 5) {
      stackItems.push_back(tmp.top());
      tmp.pop();
    }
  }
  vector<Element> sRows;
  sRows.push_back(hbox({
                      text("Position") | bold | size(WIDTH, EQUAL, 12),
                      text("Action") | bold | size(WIDTH, EQUAL, 30),
                      text("Claim ID") | bold | size(WIDTH, EQUAL, 10),
                  }) |
                  color(Color::Cyan));
  sRows.push_back(separator());
  for (int i = 0; i < (int)stackItems.size(); i++) {
    sRows.push_back(hbox({
        text(i == 0 ? "→ TOP" : "  " + to_string(i + 1)) |
            size(WIDTH, EQUAL, 12) |
            color(i == 0 ? Color::Yellow : Color::White),
        text(stackItems[i].description) | size(WIDTH, EQUAL, 30),
        text(to_string(stackItems[i].snapshot.claimId)) |
            size(WIDTH, EQUAL, 10),
    }));
  }
  if (stackItems.empty())
    sRows.push_back(text("  Stack is empty.") | color(Color::GrayLight));

  return vbox({
      text(" Queue & Stack Operations") | bold | color(Color::Cyan),
      separator(),
      text(" Last Action: " + lastMessage) | color(Color::Yellow),
      separator(),
      hbox({
          window(
              text(" Claim Processing Queue (FIFO)  [Press P to process next]"),
              vbox(qRows)) |
              flex,
          window(text(" Undo Stack (LIFO)  [Press U to undo last action]"),
                 vbox(sRows)) |
              flex,
      }),
      separator(),
      text(" Queue Theory: FIFO — first claim enqueued is processed first") |
          color(Color::GrayLight),
      text(" Stack Theory: LIFO — last correction can be undone first") |
          color(Color::GrayLight),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 4 : FRAUD ANALYSIS (BFS/DFS)
// ─────────────────────────────────────────────────────────────────────────────

Element renderFraud(int fraudAlgo, int startNode) {
  // Start nodes are the fraud-flagged claims
  vector<int> fraudClaims;
  for (auto &c : gClaims)
    if (c.fraudFlag)
      fraudClaims.push_back(c.claimId);
  if (fraudClaims.empty())
    return text("No fraud detected.") | color(Color::Green);

  int startId = fraudClaims[startNode % fraudClaims.size()];
  vector<int> traversal =
      (fraudAlgo == 0) ? bfsFraud(startId) : dfsFraud(startId);

  string algoName =
      fraudAlgo == 0 ? "BFS (Breadth-First)" : "DFS (Depth-First)";
  string complexity = "O(V + E)";

  // Build traversal display
  string travStr = "Traversal from Claim #" + to_string(startId) + ":  ";
  for (int i = 0; i < (int)traversal.size(); i++) {
    travStr += "#" + to_string(traversal[i]);
    if (i + 1 < (int)traversal.size())
      travStr += "  →  ";
  }

  // Show all graph edges
  vector<Element> edgeRows;
  edgeRows.push_back(hbox({
                         text("Claim") | bold | size(WIDTH, EQUAL, 10),
                         text("Holder") | bold | size(WIDTH, EQUAL, 18),
                         text("Connected To") | bold | size(WIDTH, EQUAL, 20),
                         text("Fraud?") | bold | size(WIDTH, EQUAL, 8),
                     }) |
                     color(Color::Cyan));
  edgeRows.push_back(separator());
  for (auto &[node, neighbors] : gFraudGraph) {
    string neighStr = "";
    for (int n : neighbors)
      neighStr += "#" + to_string(n) + " ";
    if (neighStr.empty())
      neighStr = "—";
    string holder = "";
    bool isFraud = false;
    for (auto &c : gClaims) {
      if (c.claimId == node) {
        holder = c.holder;
        isFraud = c.fraudFlag;
        break;
      }
    }
    edgeRows.push_back(hbox({
        text("#" + to_string(node)) | size(WIDTH, EQUAL, 10) |
            color(isFraud ? Color::Red : Color::White),
        text(holder) | size(WIDTH, EQUAL, 18),
        text(neighStr) | size(WIDTH, EQUAL, 20) | color(Color::Yellow),
        text(isFraud ? "⚠ YES" : "No") | size(WIDTH, EQUAL, 8) |
            color(isFraud ? Color::Red : Color::GrayLight),
    }));
  }

  return vbox({
      text(" Fraud Network Analysis") | bold | color(Color::Cyan),
      text(" Algorithm: " + algoName + "  |  Complexity: " + complexity) |
          color(Color::Yellow),
      text(" Press B=BFS, D=DFS, N=next fraud node") | color(Color::GrayLight),
      separator(),
      window(text(" Traversal Result"),
             text(" " + travStr) | color(Color::Green)),
      separator(),
      window(text(" Fraud Network Graph (Adjacency List)"), vbox(edgeRows)),
      separator(),
      text(" BFS: Explores fraud ring layer by layer (all direct contacts "
           "first)") |
          color(Color::GrayLight),
      text(" DFS: Follows a fraud chain to its deepest connection first") |
          color(Color::GrayLight),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAB 5 : SEARCH
// ─────────────────────────────────────────────────────────────────────────────

Element renderSearch(const string &query, const string &idStr,
                     const vector<Claim> &linearResults, int binaryResult,
                     const vector<Claim> &sortedById) {
  vector<Element> linRows;
  linRows.push_back(hbox({
                        text("Claim ID") | bold | size(WIDTH, EQUAL, 10),
                        text("Holder") | bold | size(WIDTH, EQUAL, 18),
                        text("Type") | bold | size(WIDTH, EQUAL, 10),
                        text("Amount") | bold | size(WIDTH, EQUAL, 14),
                        text("Status") | bold | size(WIDTH, EQUAL, 12),
                    }) |
                    color(Color::Cyan));
  linRows.push_back(separator());
  if (linearResults.empty()) {
    linRows.push_back(text("  No results found.") | color(Color::GrayLight));
  }
  for (auto &c : linearResults) {
    linRows.push_back(hbox({
        text(to_string(c.claimId)) | size(WIDTH, EQUAL, 10),
        text(c.holder) | size(WIDTH, EQUAL, 18),
        text(c.type) | size(WIDTH, EQUAL, 10),
        text(fmt(c.amount)) | size(WIDTH, EQUAL, 14),
        text(c.status) | size(WIDTH, EQUAL, 12) | color(statusColor(c.status)),
    }));
  }

  string binMsg = "Enter a Claim ID and press Enter to binary search.";
  if (!idStr.empty()) {
    if (binaryResult == -2)
      binMsg = "Invalid input.";
    else if (binaryResult == -1)
      binMsg = "✘ Claim ID #" + idStr +
               " not found. (O(log n) binary search on sorted array)";
    else
      binMsg = "✔ Found at index " + to_string(binaryResult) +
               " | Holder: " + sortedById[binaryResult].holder +
               " | Amount: " + fmt(sortedById[binaryResult].amount);
  }

  return vbox({
      text(" Search Operations") | bold | color(Color::Cyan),
      separator(),
      window(text(" Linear Search by Holder Name  [O(n)]  — type name, press "
                  "Enter"),
             vbox(linRows)),
      separator(),
      window(
          text(" Binary Search by Claim ID  [O(log n)]  — array sorted by ID"),
          text(" " + binMsg) | color(Color::Yellow)),
      separator(),
      text(" Linear Search: Checks each record sequentially — no preprocessing "
           "needed") |
          color(Color::GrayLight),
      text(" Binary Search: Requires sorted input — halves search space each "
           "step") |
          color(Color::GrayLight),
  });
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────────────────────

int main() {
  initSystem();

  auto screen = ScreenInteractive::Fullscreen();

  // ── state
  int activeTab = 0;
  int sortAlgo = 3;  // default MergeSort
  int fraudAlgo = 0; // 0=BFS 1=DFS
  int fraudNode = 0;
  string queueMsg = "Press P to process next claim, U to undo last action.";
  string hashSearch = "";
  string linSearch = "";
  string binSearch = "";
  vector<Claim> linResults;
  int binResult = -1;
  vector<Claim> claimsSortedById = gClaims;
  sort(claimsSortedById.begin(), claimsSortedById.end(),
       [](const Claim &a, const Claim &b) { return a.claimId < b.claimId; });

  // ── components

  // Policy hash input
  auto hashInput = Input(&hashSearch, "Enter Policy ID…");

  // Linear search input
  auto linInput = Input(&linSearch, "Enter holder name…");

  // Binary search input
  auto binInput = Input(&binSearch, "Enter Claim ID…");

  // Tab menu
  vector<string> tabLabels = {" Dashboard ",     " Policies/AVL ",
                              " Claims/Sort ",   " Queue/Stack ",
                              " Fraud/BFS-DFS ", " Search "};
  int tabSelected = 0;
  auto tabToggle = Toggle(&tabLabels, &tabSelected);

  // Main container
  auto container = Container::Vertical({
      tabToggle,
      hashInput,
      linInput,
      binInput,
  });

  auto renderer = Renderer(container, [&]() -> Element {
    activeTab = tabSelected;

    // Handle linear search
    linResults.clear();
    if (!linSearch.empty())
      linResults = linearSearch(gClaims, linSearch);

    // Handle binary search
    binResult = -1;
    if (!binSearch.empty()) {
      try {
        int id = stoi(binSearch);
        binResult = binarySearch(claimsSortedById, id);
      } catch (...) {
        binResult = -2;
      }
    }

    Element body;
    switch (activeTab) {
    case 0:
      body = renderDashboard();
      break;
    case 1:
      body = renderPolicies(0, hashSearch);
      break;
    case 2:
      body = renderClaims(sortAlgo);
      break;
    case 3:
      body = renderQueueStack(queueMsg);
      break;
    case 4:
      body = renderFraud(fraudAlgo, fraudNode);
      break;
    case 5:
      body = renderSearch(linSearch, binSearch, linResults, binResult,
                          claimsSortedById);
      break;
    default:
      body = text("Unknown tab");
    }

    return vbox({
        tabToggle->Render(),
        separator(),
        body | flex,
        separator(),
        hbox({
            text(" [Tab/Click] Navigate  ") | color(Color::GrayLight),
            text("[←→] Sort Algo  ") | color(Color::GrayLight),
            text("[P] Process Claim  ") | color(Color::GrayLight),
            text("[U] Undo  ") | color(Color::GrayLight),
            text("[B/D] BFS/DFS  ") | color(Color::GrayLight),
            text("[N] Next Fraud Node  ") | color(Color::GrayLight),
            text("[Q] Quit") | color(Color::Red),
            separator(),
            text(" Build: " + APP_VERSION) | color(Color::Yellow),
        }),
    });
  });

  auto eventHandler = CatchEvent(renderer, [&](Event event) -> bool {
    // Quit
    if (event == Event::Character('q') || event == Event::Character('Q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    // Sort algorithm cycling (Claims tab)
    if (activeTab == 2) {
      if (event == Event::ArrowLeft) {
        sortAlgo = (sortAlgo + 4) % 5;
        return true;
      }
      if (event == Event::ArrowRight) {
        sortAlgo = (sortAlgo + 1) % 5;
        return true;
      }
    }
    // Queue/Stack operations
    if (event == Event::Character('p') || event == Event::Character('P')) {
      queueMsg = processNextClaim();
      return true;
    }
    if (event == Event::Character('u') || event == Event::Character('U')) {
      queueMsg = undoLastAction();
      return true;
    }
    // Fraud traversal
    if (event == Event::Character('b') || event == Event::Character('B')) {
      fraudAlgo = 0;
      tabSelected = 4;
      return true;
    }
    if (event == Event::Character('d') || event == Event::Character('D')) {
      fraudAlgo = 1;
      tabSelected = 4;
      return true;
    }
    if (event == Event::Character('n') || event == Event::Character('N')) {
      fraudNode++;
      return true;
    }
    return false;
  });

  screen.Loop(eventHandler);
  return 0;
}
