// RoundTrip and ConversionRecord - see roundtrip.h for what Convert now does and why.
// The record is length-prefixed raw text, so a file with any bytes in it comes back exactly.

#include "roundtrip.h"

#include <cstdio>
#include <fstream>
#include <sstream>

#include "convert.h"
#include "diff3.h"
#include "path.h"

namespace editor {

namespace {

const char* const Folder = ".ride-convert";
const char* const Magic = "ride-convert 1";

std::string leaf(const std::string& file) { return path::filename(file); }

// A path made here, spelt the way the source was: the window opens it beside the source's own tab.
std::string likeSource(const std::string& made, const std::string& source) {
    if (source.find('\\') == std::string::npos) return made;
    std::string out = made;
    for (char& c : out) if (c == '/') c = '\\';
    return out;
}

void splitName(const std::string& file, std::string& stem, std::string& extension) {
    const size_t slash = file.find_last_of("/\\");
    const size_t dot = file.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        stem = file; extension.clear();
    } else {
        stem = file.substr(0, dot); extension = file.substr(dot);
    }
}

}

// ---- ConversionRecord

std::string ConversionRecord::folderFor(const std::string& file) {
    return path::join(path::parent(file), Folder);
}

bool ConversionRecord::load(const std::string& converted) {
    std::string text;
    const std::string where = path::join(folderFor(converted), leaf(converted) + ".record");
    if (!RoundTrip::readFile(where, text)) return false;

    std::istringstream in(text);
    std::string magic, name;
    size_t originalBytes = 0, convertedBytes = 0;
    if (!std::getline(in, magic) || magic != Magic) return false;
    if (!std::getline(in, name) || name.compare(0, 9, "original ") != 0) return false;
    if (!(in >> originalBytes >> convertedBytes)) return false;
    in.get();
    const size_t at = size_t(in.tellg());
    if (at + originalBytes + convertedBytes != text.size()) return false;
    original = name.substr(9);
    originalText = text.substr(at, originalBytes);
    convertedText = text.substr(at + originalBytes);
    return true;
}

bool ConversionRecord::save(const std::string& converted) const {
    const std::string folder = folderFor(converted);
    if (!path::isDirectory(folder) && !path::makeDirectories(folder)) return false;
    std::string text = std::string(Magic) + "\noriginal " + original + "\n" +
                       std::to_string(originalText.size()) + " " +
                       std::to_string(convertedText.size()) + "\n";
    text += originalText;
    text += convertedText;
    return RoundTrip::writeFile(path::join(folder, leaf(converted) + ".record"), text);
}

// ---- RoundTrip

RoundTrip::RoundTrip(const std::string& converter, LineSink sink, void* context)
    : converter_(converter), sink_(sink), context_(context) {}

bool RoundTrip::readFile(const std::string& file, std::string& text) {
    std::ifstream in(file.c_str(), std::ios::binary);
    if (!in) return false;
    std::ostringstream all;
    all << in.rdbuf();
    text = all.str();
    return true;
}

bool RoundTrip::writeFile(const std::string& file, const std::string& text) {
    std::ofstream out(file.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
    return bool(out);
}

std::string RoundTrip::freeName(const std::string& wanted) {
    std::string stem, extension;
    splitName(wanted, stem, extension);
    for (int n = 2;; ++n) {
        const std::string candidate = stem + "." + std::to_string(n) + extension;
        if (!path::exists(candidate)) return candidate;
    }
}

void RoundTrip::tell(RoundTripResult& result, const std::string& line) {
    result.report += line + "\n";
    if (sink_) sink_(context_, line);
}

bool RoundTrip::runConverter(const std::string& from, const std::string& into, bool toShalimar,
                             std::string& text, RoundTripResult& result) {
    path::remove(into);
    Conversion made = editor::convert(converter_, from, into, toShalimar, sink_, context_);
    result.output += made.output;
    if (!made.ran) return false;
    result.ran = true;
    return !made.produced.empty() && readFile(made.produced, text);
}

RoundTripResult RoundTrip::reopen(const std::string& file, const std::string& why) {
    RoundTripResult result;
    result.ran = true;
    result.ok = true;
    result.open = file;
    result.said = "reopened the original " + leaf(file) + " - " + why;
    tell(result, result.said);
    return result;
}

RoundTripResult RoundTrip::convert(const std::string& source, bool toShalimar) {
    RoundTripResult failed;
    std::string sourceText;
    if (!readFile(source, sourceText)) {
        failed.said = "could not read " + leaf(source);
        return failed;
    }

    // Back the way it came: the source is a file RIDE converted, and its original is still there.
    std::string note;
    ConversionRecord back;
    if (back.load(source)) {
        const std::string original = likeSource(path::join(path::parent(source), back.original), source);
        std::string originalText;
        if (readFile(original, originalText) && originalText == back.originalText) {
            if (sourceText == back.convertedText)
                return reopen(original, leaf(source) + " was not changed since it was converted");
            return mergeBack(source, sourceText, back, original, originalText, toShalimar);
        }
        note = back.original + " was changed since " + leaf(source) + " was made from it";
    }

    // Forward again, with nothing changed on either side since: the conversion is reopened.
    const std::string target = convertedName(source, toShalimar);
    ConversionRecord ahead;
    std::string targetText;
    if (note.empty() && ahead.load(target) && ahead.original == leaf(source) &&
        readFile(target, targetText) && targetText == ahead.convertedText &&
        sourceText == ahead.originalText) {
        RoundTripResult result;
        result.ran = result.ok = true;
        result.open = target;
        result.said = "reopened " + leaf(target) + " - " + leaf(source) +
                      " was not changed since it was converted";
        tell(result, result.said);
        return result;
    }
    return fresh(source, sourceText, toShalimar, note);
}

RoundTripResult RoundTrip::fresh(const std::string& source, const std::string& sourceText,
                                 bool toShalimar, const std::string& note) {
    RoundTripResult result;
    const std::string target = convertedName(source, toShalimar);
    const std::string work = path::join(ConversionRecord::folderFor(source), "work");
    path::makeDirectories(work);

    std::string made;
    if (!runConverter(source, path::join(work, leaf(target)), toShalimar, made, result)) {
        result.said = result.ran ? leaf(source) + " - not converted: see the console"
                                 : "could not run " + converter_;
        return result;
    }

    // Written over only when it is what RIDE itself put there last, or already says the same.
    std::string there;
    ConversionRecord previous;
    const bool ours = previous.load(target) && previous.original == leaf(source) &&
                      readFile(target, there) && there == previous.convertedText;
    std::string where = target;
    if (path::exists(target) && !ours && !(readFile(target, there) && there == made))
        where = freeName(target);

    if (!writeFile(where, made)) {
        result.said = "could not write " + where;
        return result;
    }
    // c2s's own record (.c2s-original/<name>), when it writes one, belongs beside the file it describes.
    const std::string kept = path::join(path::join(work, ".c2s-original"), leaf(target));
    if (path::exists(kept)) {
        const std::string home = path::join(path::parent(where), ".c2s-original");
        path::makeDirectories(home);
        path::remove(path::join(home, leaf(where)));
        path::rename(kept, path::join(home, leaf(where)));
        // And c2s's exact copy of the original, under its own name, goes with its record.
        const std::string copy = path::join(path::join(work, ".c2s-original"), leaf(source));
        if (path::exists(copy)) {
            path::remove(path::join(home, leaf(source)));
            path::rename(copy, path::join(home, leaf(source)));
        }
    }

    ConversionRecord record;
    record.original = leaf(source);
    record.originalText = sourceText;
    record.convertedText = made;
    record.save(where);

    result.open = where;
    result.ok = made.find("BEYOND") == std::string::npos;
    if (!note.empty()) tell(result, note + " - converted afresh");
    if (where != target)
        tell(result, leaf(target) + " is there already and is not RIDE's conversion - written as " +
                     leaf(where) + " instead");
    result.said = leaf(where) + (result.ok ? " - converted"
                                           : " - written with unconverted parts marked; search for BEYOND");
    if (where != target) result.said += " (not over " + leaf(target) + ")";
    return result;
}

RoundTripResult RoundTrip::mergeBack(const std::string& source, const std::string& sourceText,
                                     ConversionRecord& record, const std::string& original,
                                     const std::string& originalText, bool toShalimar) {
    RoundTripResult result;
    const std::string folder = ConversionRecord::folderFor(source);
    const std::string untouched = path::join(folder, "work/untouched");
    const std::string edited = path::join(folder, "work/edited");
    path::makeDirectories(untouched);
    path::makeDirectories(edited);

    // The untouched conversion and the edited one, each turned back by c2s: their noise is the same.
    const std::string asConverted = path::join(untouched, leaf(source));
    std::string base, theirs;
    // The untouched copy is read by c2s with the sidecar it was written with, as the edited one is.
    std::string kept;
    const std::string keptHere = path::join(path::join(untouched, ".c2s-original"), leaf(source));
    path::remove(keptHere);
    if (readFile(path::join(path::join(path::parent(source), ".c2s-original"), leaf(source)), kept)) {
        path::makeDirectories(path::parent(keptHere));
        writeFile(keptHere, kept);
    }
    if (!writeFile(asConverted, record.convertedText) ||
        !runConverter(asConverted, path::join(untouched, record.original), toShalimar, base, result) ||
        !runConverter(source, path::join(edited, record.original), toShalimar, theirs, result)) {
        result.said = "c2s could not turn " + leaf(source) + " back - " + record.original +
                      " was not changed";
        return result;
    }

    ThreeWayMerge merger("the original " + record.original, "c2s's round trip of the conversion",
                         "your edit, from " + leaf(source));
    const MergeResult merged = merger.merge(LineText::split(base), LineText::split(originalText),
                                            LineText::split(theirs));
    const std::string text = LineText::join(merged.lines);

    if (!merged.clean) {
        std::string stem, extension;
        splitName(original, stem, extension);
        std::string where = stem + ".merge" + extension;
        if (path::exists(where)) where = freeName(where);
        writeFile(where, text);
        std::string lines;
        for (int line : merged.conflictLines)
            lines += (lines.empty() ? "" : ", ") + std::to_string(line);
        tell(result, "your edit of " + leaf(source) + " meets a part of " + record.original +
                     " c2s does not carry back - conflicts at line(s) " + lines + " of " + leaf(where));
        for (const std::string& change : merged.changes) tell(result, change);
        result.ran = true;
        result.open = where;
        result.said = record.original + " was not changed - the merge conflicts; see " + leaf(where);
        return result;
    }

    result.ran = result.ok = true;
    result.open = original;
    if (text == originalText) {
        result.said = "reopened the original " + record.original + " - the edit to " +
                      leaf(source) + " is in a part c2s does not carry back (a BEYOND line or a comment)";
    } else {
        writeFile(path::join(folder, record.original + ".before"), originalText);
        if (!writeFile(original, text)) {
            result.ok = false;
            result.said = "could not write " + record.original;
            return result;
        }
        for (const std::string& change : merged.changes) tell(result, change);
        result.said = "carried your edit of " + leaf(source) + " into the original " +
                      record.original + " (" + std::to_string(merged.changes.size()) +
                      " place(s); the one before is .ride-convert/" + record.original + ".before)";
    }
    tell(result, result.said);

    // The two now agree: the next Convert of the unedited file reopens the original again.
    record.originalText = text;
    record.convertedText = sourceText;
    record.save(source);
    return result;
}

}
