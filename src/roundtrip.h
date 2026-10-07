// Convert's round trip: converting a file back to the language it was converted from reopens the
// original rather than converting again, and carries only the user's own edits back into it.

// The record of each conversion RIDE made sits beside the files, in a hidden folder .ride-convert/
// of their directory - names in it are leaves, so moving the folder moves the records with it.
// One file per converted file, <converted>.record: the original's leaf, its text, and c2s's text.

// Back again, three answers. Nothing edited: the original is reopened and c2s is not run. Edited:
// c2s turns both the untouched conversion and the edited one back, and the difference between those
// two - the edit, the round-trip noise cancelling - is merged into the original (ThreeWayMerge).

// A conflict leaves the original alone and writes <stem>.merge.<ext> with markers; a changed
// original, or no record, converts afresh - and no conversion ever writes over a different file.

#ifndef EDITOR_ROUNDTRIP_H
#define EDITOR_ROUNDTRIP_H

#include <string>

#include "compile.h"

namespace editor {

// What one Convert did, for any front end to say and open.
struct RoundTripResult {
    bool ran = false;       // c2s ran, or was not needed
    bool ok = false;        // what is opened is a clean result, nothing marked BEYOND or conflicting
    std::string open;       // the file to open, empty when nothing was made
    std::string said;       // one line for the status bar
    std::string output;     // c2s's own words
    std::string report;     // what the round trip did, line by line: the changed lines, the conflicts
};

// One conversion RIDE made, as kept in .ride-convert/<converted>.record.
class ConversionRecord {
public:
    static std::string folderFor(const std::string& file);

    // Reads the record of the file at this path, if RIDE made it by a conversion.
    bool load(const std::string& converted);
    bool save(const std::string& converted) const;

    std::string original;       // the leaf of the file it was converted from, in the same directory
    std::string originalText;   // that file, as it was when converted - or last merged into
    std::string convertedText;  // the converted file, as c2s wrote it - or as last carried back
};

// Convert, with its round trip. The converter is c2s; the sink hears its output as it runs.
class RoundTrip {
public:
    RoundTrip(const std::string& converter, LineSink sink = 0, void* context = 0);

    RoundTripResult convert(const std::string& source, bool toShalimar);

    // Reading and writing a file byte for byte; public for the tests.
    static bool readFile(const std::string& file, std::string& text);
    static bool writeFile(const std::string& file, const std::string& text);

    // The first of stem.2.ext, stem.3.ext ... that does not exist.
    static std::string freeName(const std::string& wanted);

private:
    RoundTripResult reopen(const std::string& file, const std::string& why);
    RoundTripResult mergeBack(const std::string& source, const std::string& sourceText,
                              ConversionRecord& record, const std::string& original,
                              const std::string& originalText, bool toShalimar);
    RoundTripResult fresh(const std::string& source, const std::string& sourceText,
                          bool toShalimar, const std::string& note);

    // c2s over one file into another, the text it wrote returned; false when it wrote nothing.
    bool runConverter(const std::string& from, const std::string& into, bool toShalimar,
                      std::string& text, RoundTripResult& result);
    void tell(RoundTripResult& result, const std::string& line);

    std::string converter_;
    LineSink sink_;
    void* context_;
};

}

#endif
