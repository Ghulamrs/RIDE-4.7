// LineDiff and ThreeWayMerge - see diff3.h. Myers' greedy O(ND) walk finds the longest run of
// lines the two texts share; the merge groups the two sides' hunks by where they touch the base.

// Written from the papers' ideas, not from any implementation: E. W. Myers, "An O(ND) Difference
// Algorithm and Its Variations" (1986), and the diff3 merge rule as described for RCS's merge.

#include "diff3.h"

#include <algorithm>
#include <map>

namespace editor {

// ---- LineText

std::vector<std::string> LineText::split(const std::string& text) {
    std::vector<std::string> lines;
    size_t from = 0;
    while (from < text.size()) {
        const size_t newline = text.find('\n', from);
        const size_t to = newline == std::string::npos ? text.size() : newline + 1;
        lines.push_back(text.substr(from, to - from));
        from = to;
    }
    return lines;
}

std::string LineText::join(const std::vector<std::string>& lines) {
    std::string text;
    for (const std::string& line : lines) text += line;
    return text;
}

std::string LineText::body(const std::string& line) {
    size_t end = line.size();
    if (end > 0 && line[end - 1] == '\n') --end;
    if (end > 0 && line[end - 1] == '\r') --end;
    return line.substr(0, end);
}

std::string LineText::ending(const std::vector<std::string>& lines) {
    if (!lines.empty()) {
        const std::string& first = lines.front();
        if (first.size() >= 2 && first.compare(first.size() - 2, 2, "\r\n") == 0) return "\r\n";
    }
    return "\n";
}

// ---- LineDiff

namespace {

// Each distinct line body as a number, so the walk compares integers rather than strings.
class Interner {
public:
    std::vector<int> numbers(const std::vector<std::string>& lines) {
        std::vector<int> out;
        out.reserve(lines.size());
        for (const std::string& line : lines) {
            const std::string key = LineText::body(line);
            std::map<std::string, int>::iterator found = ids_.find(key);
            if (found == ids_.end()) found = ids_.insert(std::make_pair(key, int(ids_.size()))).first;
            out.push_back(found->second);
        }
        return out;
    }

private:
    std::map<std::string, int> ids_;
};

// The matched pairs (i in a, j in b) of a shortest edit script, in order.
std::vector<std::pair<int, int>> myersMatches(const std::vector<int>& a, const std::vector<int>& b) {
    const int n = int(a.size()), m = int(b.size()), most = n + m;
    const int offset = most + 1;
    std::vector<int> v(2 * most + 3, 0);
    std::vector<std::vector<int>> trace;
    int steps = 0;
    for (int d = 0; d <= most; ++d) {
        trace.push_back(v);
        bool done = false;
        for (int k = -d; k <= d; k += 2) {
            int x = (k == -d || (k != d && v[offset + k - 1] < v[offset + k + 1]))
                        ? v[offset + k + 1] : v[offset + k - 1] + 1;
            int y = x - k;
            while (x < n && y < m && a[x] == b[y]) { ++x; ++y; }
            v[offset + k] = x;
            if (x >= n && y >= m) { done = true; break; }
        }
        if (done) { steps = d; break; }
    }

    // Walked back from the end: each step's snake is a run of matches, then one edit.
    std::vector<std::pair<int, int>> matches;
    int x = n, y = m;
    for (int d = steps; d > 0; --d) {
        const std::vector<int>& before = trace[d];
        const int k = x - y;
        const int previousK = (k == -d || (k != d && before[offset + k - 1] < before[offset + k + 1]))
                                  ? k + 1 : k - 1;
        const int previousX = before[offset + previousK];
        const int previousY = previousX - previousK;
        while (x > previousX && y > previousY) { --x; --y; matches.push_back(std::make_pair(x, y)); }
        x = previousX;
        y = previousY;
    }
    while (x > 0 && y > 0) { --x; --y; matches.push_back(std::make_pair(x, y)); }
    std::reverse(matches.begin(), matches.end());
    return matches;
}

}

std::vector<Hunk> LineDiff::between(const std::vector<std::string>& base,
                                    const std::vector<std::string>& other) {
    Interner interner;
    const std::vector<int> a = interner.numbers(base);
    const std::vector<int> b = interner.numbers(other);
    std::vector<std::pair<int, int>> matches = myersMatches(a, b);
    matches.push_back(std::make_pair(int(a.size()), int(b.size())));

    // A hunk is every gap between two consecutive matches.
    std::vector<Hunk> hunks;
    int i = 0, j = 0;
    for (const std::pair<int, int>& match : matches) {
        if (match.first > i || match.second > j) {
            Hunk hunk;
            hunk.baseFrom = i; hunk.baseTo = match.first;
            hunk.otherFrom = j; hunk.otherTo = match.second;
            hunks.push_back(hunk);
        }
        i = match.first + 1;
        j = match.second + 1;
    }
    return hunks;
}

// ---- ThreeWayMerge

namespace {

struct Sided {
    Hunk hunk;
    bool mine;
};

bool bySpan(const Sided& one, const Sided& other) {
    if (one.hunk.baseFrom != other.hunk.baseFrom) return one.hunk.baseFrom < other.hunk.baseFrom;
    return one.hunk.baseTo < other.hunk.baseTo;
}

// Whether a hunk over base [from, to) touches a group over [low, high): they overlap, or an
// insertion stands at the group's start, where which goes first could not be said.
bool touches(int low, int high, int from, int to) {
    if (from < high && low < to) return true;
    return from == low && (from == to || low == high);
}

std::vector<std::string> slice(const std::vector<std::string>& lines, int from, int count) {
    return std::vector<std::string>(lines.begin() + from, lines.begin() + from + count);
}

bool sameBodies(const std::vector<std::string>& one, const std::vector<std::string>& other) {
    if (one.size() != other.size()) return false;
    for (size_t i = 0; i < one.size(); ++i)
        if (LineText::body(one[i]) != LineText::body(other[i])) return false;
    return true;
}

// The line with this text's own ending, so an incoming line does not bring another's "\r\n".
std::string withEnding(const std::string& line, const std::string& ending) {
    const bool ended = !line.empty() && line[line.size() - 1] == '\n';
    return ended ? LineText::body(line) + ending : line;
}

// Within one line, the words their side changed - base to theirs, out to whitespace on either
// side - carried into mine where those words stand exactly once: the edit moves, the noise stays.
bool carryWord(const std::string& base, const std::string& mine, const std::string& theirs,
               std::string& out) {
    const std::string b = LineText::body(base), t = LineText::body(theirs);
    if (b == t) { out = mine; return true; }
    size_t front = 0;
    while (front < b.size() && front < t.size() && b[front] == t[front]) ++front;
    size_t back = 0;
    while (back < b.size() - front && back < t.size() - front &&
           b[b.size() - 1 - back] == t[t.size() - 1 - back]) ++back;
    while (front > 0 && b[front - 1] != ' ' && b[front - 1] != '\t') --front;
    while (back > 0 && b[b.size() - back] != ' ' && b[b.size() - back] != '\t') --back;
    const std::string old = b.substr(front, b.size() - back - front);
    const std::string made = t.substr(front, t.size() - back - front);
    if (old.empty()) return false;
    const size_t at = mine.find(old);
    if (at == std::string::npos || mine.find(old, at + 1) != std::string::npos) return false;
    out = mine.substr(0, at) + made + mine.substr(at + old.size());
    return true;
}

bool carryWords(const std::vector<std::string>& base, const std::vector<std::string>& mine,
                const std::vector<std::string>& theirs, std::vector<std::string>& out) {
    if (base.size() != mine.size() || base.size() != theirs.size()) return false;
    out.clear();
    for (size_t i = 0; i < base.size(); ++i) {
        std::string line;
        if (!carryWord(base[i], mine[i], theirs[i], line)) return false;
        out.push_back(line);
    }
    return true;
}

std::string terminated(const std::string& line, const std::string& ending) {
    return (!line.empty() && line[line.size() - 1] == '\n') ? line : line + ending;
}

}

ThreeWayMerge::ThreeWayMerge(const std::string& mineLabel, const std::string& baseLabel,
                             const std::string& theirsLabel)
    : mineLabel_(mineLabel), baseLabel_(baseLabel), theirsLabel_(theirsLabel) {}

MergeResult ThreeWayMerge::merge(const std::vector<std::string>& base,
                                 const std::vector<std::string>& mine,
                                 const std::vector<std::string>& theirs) const {
    std::vector<Sided> all;
    for (const Hunk& hunk : LineDiff::between(base, mine)) all.push_back(Sided{hunk, true});
    for (const Hunk& hunk : LineDiff::between(base, theirs)) all.push_back(Sided{hunk, false});
    std::stable_sort(all.begin(), all.end(), bySpan);

    const std::string ending = LineText::ending(mine.empty() ? theirs : mine);
    MergeResult result;
    int b = 0, m = 0, t = 0;
    size_t next = 0;
    while (next < all.size()) {
        // One group: every hunk, of either side, that touches the first one's span as it grows.
        int low = all[next].hunk.baseFrom, high = all[next].hunk.baseTo;
        int mineGrowth = 0, theirsGrowth = 0;
        bool mineMoved = false, theirsMoved = false;
        while (next < all.size() && touches(low, high, all[next].hunk.baseFrom, all[next].hunk.baseTo)) {
            const Hunk& hunk = all[next].hunk;
            high = std::max(high, hunk.baseTo);
            const int growth = (hunk.otherTo - hunk.otherFrom) - (hunk.baseTo - hunk.baseFrom);
            if (all[next].mine) { mineGrowth += growth; mineMoved = true; }
            else { theirsGrowth += growth; theirsMoved = true; }
            ++next;
        }

        // The lines before it are the same in all three; mine's are kept, terminators and all.
        const int same = low - b;
        std::vector<std::string> carried;
        for (int i = 0; i < same; ++i) result.lines.push_back(mine[m + i]);
        m += same; t += same; b = low;

        const std::vector<std::string> baseRun = slice(base, low, high - low);
        const std::vector<std::string> mineRun = slice(mine, m, high - low + mineGrowth);
        const std::vector<std::string> theirsRun = slice(theirs, t, high - low + theirsGrowth);
        const int at = int(result.lines.size()) + 1;

        if (!theirsMoved || sameBodies(mineRun, theirsRun)) {
            result.lines.insert(result.lines.end(), mineRun.begin(), mineRun.end());
        } else if (!mineMoved) {
            std::string said = "line " + std::to_string(at) + ":";
            for (const std::string& line : mineRun) said += "\n  - " + LineText::body(line);
            for (const std::string& line : theirsRun) {
                result.lines.push_back(withEnding(line, ending));
                said += "\n  + " + LineText::body(line);
            }
            result.changes.push_back(said);
        } else if (carryWords(baseRun, mineRun, theirsRun, carried)) {
            std::string said = "line " + std::to_string(at) + ":";
            for (size_t i = 0; i < carried.size(); ++i) {
                if (carried[i] != mineRun[i]) {
                    said += "\n  - " + LineText::body(mineRun[i]) + "\n  + " + LineText::body(carried[i]);
                }
                result.lines.push_back(carried[i]);
            }
            result.changes.push_back(said);
        } else {
            result.clean = false;
            result.conflictLines.push_back(at);
            result.lines.push_back("<<<<<<< " + mineLabel_ + ending);
            for (const std::string& line : mineRun) result.lines.push_back(terminated(line, ending));
            result.lines.push_back("||||||| " + baseLabel_ + ending);
            for (const std::string& line : baseRun) result.lines.push_back(terminated(line, ending));
            result.lines.push_back("=======" + ending);
            for (const std::string& line : theirsRun) result.lines.push_back(terminated(line, ending));
            result.lines.push_back(">>>>>>> " + theirsLabel_ + ending);
        }
        m += int(mineRun.size()); t += int(theirsRun.size()); b = high;
    }
    for (size_t i = size_t(m); i < mine.size(); ++i) result.lines.push_back(mine[i]);
    return result;
}

}
