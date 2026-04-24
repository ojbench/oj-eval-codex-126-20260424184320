// Conway's Game of Life - header submission (src.hpp)
// Implements Initialize, Tick, PrintGame, GetLiveCell with sparse storage and RLE I/O.

#pragma once

#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

// Global grid size
static int g_rows = 0;
static int g_cols = 0;

// Sparse live cell set: key encodes (x,y) into 64-bit value
static std::unordered_set<unsigned long long> g_live;

static inline unsigned long long enc(int x, int y) {
  return (static_cast<unsigned long long>(static_cast<unsigned int>(y)) << 32) | static_cast<unsigned int>(x);
}

static inline std::pair<int,int> dec(unsigned long long k) {
  int y = static_cast<int>(k >> 32);
  int x = static_cast<int>(k & 0xffffffffu);
  return {x,y};
}

// Read entire RLE pattern (possibly multiline) until '!' encountered.
inline void Initialize() {
  using namespace std;
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  g_live.clear();
  if (!(cin >> g_cols >> g_rows)) return;
  string line;
  string pattern;
  // consume endline after the sizes
  getline(cin, line);
  while (getline(cin, line)) {
    pattern += line;
    if (line.find('!') != string::npos) break;
  }
  // parse RLE tokens
  long long num = -1;
  int x = 0, y = 0;
  for (size_t i = 0; i < pattern.size(); ++i) {
    char c = pattern[i];
    if (c == '!') {
      break;
    } else if (c >= '0' && c <= '9') {
      if (num < 0) num = 0;
      num = num * 10 + (c - '0');
    } else if (c == 'b' || c == 'o' || c == '$') {
      long long cnt = (num >= 0 ? num : 1);
      num = -1;
      if (c == 'b') {
        // advance x by cnt (dead cells)
        x += static_cast<int>(cnt);
      } else if (c == 'o') {
        // set cnt live cells starting at (x,y)
        for (long long k = 0; k < cnt; ++k) {
          if (x >= 0 && x < g_cols && y >= 0 && y < g_rows) {
            g_live.insert(enc(x, y));
          }
          ++x;
        }
      } else if (c == '$') {
        // move to next line(s) start
        y += static_cast<int>(cnt);
        x = 0;
      }
    } else {
      // ignore any other characters/spaces
      continue;
    }
  }
}

inline void Tick() {
  using namespace std;
  static const int dx[8] = {-1,0,1,-1,1,-1,0,1};
  static const int dy[8] = {-1,-1,-1,0,0,1,1,1};

  unordered_map<unsigned long long, int> cnt;
  cnt.reserve(g_live.size() * 6 + 16);

  // Count live neighbors for cells adjacent to current live cells
  for (auto key : g_live) {
    auto p = dec(key);
    int x = p.first, y = p.second;
    for (int d = 0; d < 8; ++d) {
      int nx = x + dx[d];
      int ny = y + dy[d];
      if (nx < 0 || ny < 0 || nx >= g_cols || ny >= g_rows) continue;
      ++cnt[enc(nx, ny)];
    }
  }

  unordered_set<unsigned long long> next;
  next.reserve(g_live.size() * 2 + 16);

  // Any cell with exactly 3 neighbors becomes alive; any live cell with 2 neighbors survives
  for (auto &kv : cnt) {
    int n = kv.second;
    unsigned long long k = kv.first;
    if (n == 3) {
      next.insert(k);
    } else if (n == 2) {
      if (g_live.find(k) != g_live.end()) next.insert(k);
    }
  }

  g_live.swap(next);
}

// Helper: append a token with optional count (omit 1)
static inline void append_token(std::string &out, long long cnt, char sym) {
  if (cnt <= 0) return;
  if (cnt != 1) out += std::to_string(cnt);
  out.push_back(sym);
}

inline void PrintGame() {
  using namespace std;
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  cout << g_cols << ' ' << g_rows << n;

  if (g_live.empty()) {
    cout << '!' << n;
    return;
  }

  // Group live cells by row
  unordered_map<int, vector<int>> row_to_xs;
  row_to_xs.reserve(g_live.size() * 2 + 16);
  for (auto k : g_live) {
    auto p = dec(k);
    int x = p.first, y = p.second;
    row_to_xs[y].push_back(x);
  }

  // Collect all rows with live cells sorted
  vector<int> ys; ys.reserve(row_to_xs.size());
  for (auto &kv : row_to_xs) ys.push_back(kv.first);
  sort(ys.begin(), ys.end());

  string out; out.reserve(g_live.size() * 4 + ys.size() * 2 + 16);

  int cur_y = 0; // current row index the encoder is at
  for (size_t idx = 0; idx < ys.size(); ++idx) {
    int y = ys[idx];
    if (y > cur_y) {
      // skip empty rows to reach y
      append_token(out, y - cur_y, '$');
      cur_y = y;
    }

    auto &xs = row_to_xs[y];
    sort(xs.begin(), xs.end());
    xs.erase(unique(xs.begin(), xs.end()), xs.end());
    int xcur = 0;
    // emit runs up to last live cell
    for (size_t i = 0; i < xs.size(); ) {
      int run_start = xs[i];
      int run_end = run_start;
      // dead run before this live run
      if (run_start > xcur) {
        append_token(out, run_start - xcur, 'b');
      }
      // merge contiguous live cells
      ++i;
      while (i < xs.size() && xs[i] == run_end + 1) {
        ++run_end;
        ++i;
      }
      append_token(out, run_end - run_start + 1, 'o');
      xcur = run_end + 1;
    }

    // Do not emit trailing dead cells in the row (they default to dead)

    // If there are more rows with live cells, move to next line(s)
    if (idx + 1 < ys.size()) {
      int next_y = ys[idx + 1];
      append_token(out, next_y - y, '$');
      cur_y = next_y;
    }
  }

  out.push_back('!');
  cout << out << n;
}

inline int GetLiveCell() {
  return static_cast<int>(g_live.size());
}
