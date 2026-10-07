// Line-based differences and the three-way merge built on them - written here, clean room, for
// Convert's round trip (src/roundtrip.cpp), which carries a user's edit of a converted file back
// into the original without converting the original again.
//
// Lines are kept with their own terminators, so joining what was split gives the text back byte
// for byte; two lines are compared without their terminator, so "\r\n" and "\n" are one line.
// LineDiff is Myers' O(ND) greedy algorithm (1986) over those lines. ThreeWayMerge is diff3's
// idea: two diffs from one base, a change taken from whichever side made it, the same change from
// both taken once, and two different changes to one place left as a conflict, marked.

#ifndef EDITOR_DIFF3_H
#define EDITOR_DIFF3_H

#include <string>
#include <vector>

namespace editor {

// A text as lines, each with the terminator it had. The last may have none.
class LineText {
public:
    static std::vector<std::string> split(const std::string& text);
    static std::string join(const std::vector<std::string>& lines);

    // The line without "\n" or "\r\n" - what two lines are compared by.
    static std::string body(const std::string& line);

    // "\r\n" when the text's first line ends so, "\n" otherwise.
    static std::string ending(const std::vector<std::string>& lines);
};

// One run of a difference: base lines [baseFrom, baseTo) became other lines [otherFrom, otherTo).
// An insertion has baseFrom == baseTo; a deletion otherFrom == otherTo.
struct Hunk {
    int baseFrom = 0, baseTo = 0;
    int otherFrom = 0, otherTo = 0;
};

// The hunks that turn one list of lines into another, in order, with nothing in common inside one.
class LineDiff {
public:
    static std::vector<Hunk> between(const std::vector<std::string>& base,
                                     const std::vector<std::string>& other);
};

// What a three-way merge made: the lines, whether any place conflicted, and where - as 1-based
// line numbers of the result, and of the lines the incoming side changed.
struct MergeResult {
    std::vector<std::string> lines;
    bool clean = true;
    std::vector<int> conflictLines;   // the "<<<<<<<" of each conflict, 1-based
    std::vector<std::string> changes; // "line N: ..." for each place the incoming side moved
};

// base, the common ancestor; mine, kept unless only the other side changed a place; theirs, the
// incoming side. The labels name the sides inside a conflict's markers.
class ThreeWayMerge {
public:
    ThreeWayMerge(const std::string& mineLabel, const std::string& baseLabel,
                  const std::string& theirsLabel);

    MergeResult merge(const std::vector<std::string>& base, const std::vector<std::string>& mine,
                      const std::vector<std::string>& theirs) const;

private:
    std::string mineLabel_, baseLabel_, theirsLabel_;
};

}

#endif
