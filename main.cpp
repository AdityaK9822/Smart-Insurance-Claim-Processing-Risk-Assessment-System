#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "dsa_logic.hpp"

using namespace ftxui;
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  TUI HELPERS
// ─────────────────────────────────────────────────────────────────────────────

inline Color riskColor(int score) {
  if (score >= 70) return Color::Red;
  if (score >= 40) return Color::Yellow;
  return Color::Green;
}

inline Color statusColor(const string &s) {
  if (s == "Approved") return Color::Green;
  if (s == "Rejected") return Color::Red;
  if (s == "Processing") return Color::Yellow;
  return Color::White;
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
    if (p.active) activePolicies++;
    if (p.riskScore >= 70) highRisk++;
    totalPremium += p.premium;
  }

  int totalClaims = gClaims.size();
  int pendingClaims = 0, fraudClaims = 0;
  double totalClaimed = 0;
  for (auto &c : gClaims) {
    if (c.status == "Pending") pendingClaims++;
    if (c.fraudFlag) fraudClaims++;
    totalClaimed += c.amount;
  }

  auto kpiCard = [](string title, string val, Color col) {
    return window(text(title) | bold,
                  vbox({
                      text(val) | color(col) | bold | center,
                  })) |
           flex;
  };

  vector<Claim> pending;
  for (auto &c : gClaims)
    if (c.status == "Pending")
      pending.push_back(c);

  if (!pending.empty()) {
      mergeSort(pending, 0, (int)pending.size() - 1);
  }

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
  auto avlPolicies = gAVLTree.getSorted();

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

  string hashResult = "Enter a Policy ID above and press Enter to verify via Hash Table.";
  if (!searchIdStr.empty()) {
    try {
      int id = stoi(searchIdStr);
      Policy *found = gHashTable.verify(id);
      if (found) {
        hashResult = "✔ Policy #" + to_string(id) + " VERIFIED — Holder: " + found->holder +
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

  string travStr = "Traversal from Claim #" + to_string(startId) + ":  ";
  for (int i = 0; i < (int)traversal.size(); i++) {
    travStr += "#" + to_string(traversal[i]);
    if (i + 1 < (int)traversal.size())
      travStr += "  →  ";
  }

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

  auto hashInput = Input(&hashSearch, "Enter Policy ID…");
  auto linInput = Input(&linSearch, "Enter holder name…");
  auto binInput = Input(&binSearch, "Enter Claim ID…");

  vector<string> tabLabels = {" Dashboard ",     " Policies/AVL ",
                              " Claims/Sort ",   " Queue/Stack ",
                              " Fraud/BFS-DFS ", " Search "};
  int tabSelected = 0;
  auto tabToggle = Toggle(&tabLabels, &tabSelected);

  auto container = Container::Vertical({
      tabToggle,
      hashInput,
      linInput,
      binInput,
  });

  auto renderer = Renderer(container, [&]() -> Element {
    activeTab = tabSelected;

    linResults.clear();
    if (!linSearch.empty())
      linResults = linearSearch(gClaims, linSearch);

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
      body = vbox({
          text(" Search for Policy ID to verify:"),
          hashInput->Render(),
          separator(),
          renderPolicies(0, hashSearch)
      });
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
      body = vbox({
          text(" Search by Holder Name:"),
          linInput->Render(),
          separator(),
          text(" Search by Claim ID (Binary Search):"),
          binInput->Render(),
          separator(),
          renderSearch(linSearch, binSearch, linResults, binResult,
                       claimsSortedById)
      });
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
        }),
    });
  });

  auto eventHandler = CatchEvent(renderer, [&](Event event) -> bool {
    if (event == Event::Character('q') || event == Event::Character('Q')) {
      screen.ExitLoopClosure()();
      return true;
    }
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
    if (activeTab == 3) {
      if (event == Event::Character('p') || event == Event::Character('P')) {
        queueMsg = processNextClaim();
        return true;
      }
      if (event == Event::Character('u') || event == Event::Character('U')) {
        queueMsg = undoLastAction();
        return true;
      }
    }
    if (activeTab == 4) {
      if (event == Event::Character('b') || event == Event::Character('B')) {
        fraudAlgo = 0;
        return true;
      }
      if (event == Event::Character('d') || event == Event::Character('D')) {
        fraudAlgo = 1;
        return true;
      }
      if (event == Event::Character('n') || event == Event::Character('N')) {
        fraudNode++;
        return true;
      }
    }
    return false;
  });

  screen.Loop(eventHandler);
  return 0;
}
