#include "tracker/import/GameImportParse.hpp"

#include "ImportUtil.hpp"

#include <miniz.h>

#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>

namespace tracker {
namespace {

using import_detail::iequals;
using import_detail::trim;

std::string xmlUnescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') {
            out.push_back(s[i]);
            continue;
        }
        if (s.substr(i, 5) == "&amp;") {
            out.push_back('&');
            i += 4;
        } else if (s.substr(i, 4) == "&lt;") {
            out.push_back('<');
            i += 3;
        } else if (s.substr(i, 4) == "&gt;") {
            out.push_back('>');
            i += 3;
        } else if (s.substr(i, 6) == "&quot;") {
            out.push_back('"');
            i += 5;
        } else if (s.substr(i, 6) == "&apos;") {
            out.push_back('\'');
            i += 5;
        } else {
            out.push_back('&');
        }
    }
    return out;
}

bool isNameChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' ||
           c == '-';
}

bool isTagEnd(char c) {
    return c == '>' || c == '/' || c == ' ' || c == '\t' || c == '\n' ||
           c == '\r';
}

// Position of '<' for the next opening tag with the given local name.
std::size_t findOpenTag(std::string_view xml, std::string_view name,
                        std::size_t from) {
    std::size_t pos = from;
    while (pos < xml.size()) {
        pos = xml.find('<', pos);
        if (pos == std::string_view::npos) return std::string_view::npos;
        std::size_t i = pos + 1;
        if (i >= xml.size()) return std::string_view::npos;
        if (xml[i] == '/' || xml[i] == '!' || xml[i] == '?') {
            ++pos;
            continue;
        }
        std::size_t nameStart = i;
        std::size_t q = i;
        while (q < xml.size() && isNameChar(xml[q])) ++q;
        if (q < xml.size() && xml[q] == ':') {
            nameStart = q + 1;
            q = nameStart;
            while (q < xml.size() && isNameChar(xml[q])) ++q;
        }
        const std::string_view found = xml.substr(nameStart, q - nameStart);
        if (found == name && (q >= xml.size() || isTagEnd(xml[q]))) {
            return pos;
        }
        pos = (q > pos) ? q : pos + 1;
    }
    return std::string_view::npos;
}

std::size_t findTagClose(std::string_view xml, std::size_t openPos) {
    const std::size_t gt = xml.find('>', openPos);
    return gt;
}

bool isSelfClosing(std::string_view xml, std::size_t openPos, std::size_t gt) {
    if (gt == std::string_view::npos || gt == 0) return false;
    (void)openPos;
    return xml[gt - 1] == '/';
}

std::string_view tagAttributes(std::string_view xml, std::size_t openPos,
                               std::size_t gt) {
    std::size_t i = openPos + 1;
    while (i < gt && !isTagEnd(xml[i]) && xml[i] != ':') ++i;
    if (i < gt && xml[i] == ':') {
        ++i;
        while (i < gt && !isTagEnd(xml[i])) ++i;
    }
    while (i < gt && (xml[i] == ' ' || xml[i] == '\t' || xml[i] == '\n' ||
                      xml[i] == '\r')) {
        ++i;
    }
    std::size_t end = gt;
    if (end > i && xml[end - 1] == '/') --end;
    if (end < i) return {};
    return xml.substr(i, end - i);
}

std::string xmlAttr(std::string_view attrs, std::string_view name) {
    std::size_t pos = 0;
    while (pos < attrs.size()) {
        pos = attrs.find(name, pos);
        if (pos == std::string_view::npos) return {};
        const bool boundary =
            (pos == 0 || !isNameChar(attrs[pos - 1])) &&
            (pos + name.size() < attrs.size() &&
             (attrs[pos + name.size()] == '=' ||
              attrs[pos + name.size()] == ' ' ||
              attrs[pos + name.size()] == ':'));
        // Allow r:id by matching the local name after a colon.
        const bool prefixed =
            pos > 0 && attrs[pos - 1] == ':' &&
            pos + name.size() < attrs.size() &&
            attrs[pos + name.size()] == '=';
        if (!boundary && !prefixed &&
            !(pos + name.size() < attrs.size() &&
              attrs[pos + name.size()] == '=')) {
            pos += name.size();
            continue;
        }
        std::size_t eq = attrs.find('=', pos + name.size());
        if (eq == std::string_view::npos || eq > pos + name.size() + 2) {
            pos += name.size();
            continue;
        }
        std::size_t q = eq + 1;
        while (q < attrs.size() && (attrs[q] == ' ' || attrs[q] == '\t')) ++q;
        if (q >= attrs.size()) return {};
        const char quote = attrs[q];
        if (quote != '"' && quote != '\'') return {};
        const std::size_t start = q + 1;
        const std::size_t end = attrs.find(quote, start);
        if (end == std::string_view::npos) return {};
        return xmlUnescape(attrs.substr(start, end - start));
    }
    return {};
}

std::size_t findCloseTag(std::string_view xml, std::string_view name,
                         std::size_t from) {
    std::size_t pos = from;
    while (pos < xml.size()) {
        pos = xml.find('<', pos);
        if (pos == std::string_view::npos) return std::string_view::npos;
        if (pos + 1 >= xml.size() || xml[pos + 1] != '/') {
            ++pos;
            continue;
        }
        std::size_t i = pos + 2;
        std::size_t nameStart = i;
        std::size_t q = i;
        while (q < xml.size() && isNameChar(xml[q])) ++q;
        if (q < xml.size() && xml[q] == ':') {
            nameStart = q + 1;
            q = nameStart;
            while (q < xml.size() && isNameChar(xml[q])) ++q;
        }
        const std::string_view found = xml.substr(nameStart, q - nameStart);
        if (found == name) return pos;
        pos = (q > pos) ? q : pos + 1;
    }
    return std::string_view::npos;
}

std::string innerTextOf(std::string_view xml, std::string_view name,
                        std::size_t from, std::size_t until) {
    const std::size_t open = findOpenTag(xml, name, from);
    if (open == std::string_view::npos || open >= until) return {};
    const std::size_t gt = findTagClose(xml, open);
    if (gt == std::string_view::npos || gt >= until) return {};
    if (isSelfClosing(xml, open, gt)) return {};
    const std::size_t close = findCloseTag(xml, name, gt + 1);
    if (close == std::string_view::npos || close > until) return {};
    return xmlUnescape(xml.substr(gt + 1, close - (gt + 1)));
}

std::string collectTText(std::string_view xml, std::size_t from,
                         std::size_t until) {
    std::string out;
    std::size_t pos = from;
    while (pos < until) {
        const std::size_t open = findOpenTag(xml, "t", pos);
        if (open == std::string_view::npos || open >= until) break;
        const std::size_t gt = findTagClose(xml, open);
        if (gt == std::string_view::npos || gt >= until) break;
        if (isSelfClosing(xml, open, gt)) {
            pos = gt + 1;
            continue;
        }
        const std::size_t close = findCloseTag(xml, "t", gt + 1);
        if (close == std::string_view::npos || close > until) break;
        out += xmlUnescape(xml.substr(gt + 1, close - (gt + 1)));
        pos = close + 1;
    }
    return out;
}

bool parseCellRef(std::string_view ref, int& col, int& row) {
    col = 0;
    std::size_t i = 0;
    while (i < ref.size() &&
           std::isupper(static_cast<unsigned char>(ref[i])) != 0) {
        col = col * 26 + (ref[i] - 'A' + 1);
        ++i;
    }
    if (i == 0 || i == ref.size() || col <= 0) return false;
    row = 0;
    while (i < ref.size() &&
           std::isdigit(static_cast<unsigned char>(ref[i])) != 0) {
        row = row * 10 + (ref[i] - '0');
        ++i;
    }
    if (i != ref.size() || row <= 0) return false;
    col -= 1;
    return true;
}

std::string normalizeZipName(std::string name) {
    for (char& c : name) {
        if (c == '\\') c = '/';
    }
    while (name.size() > 1 && name.front() == '/') name.erase(name.begin());
    return name;
}

bool zipNameEquals(std::string_view a, std::string_view b) {
    return iequals(normalizeZipName(std::string(a)),
                   normalizeZipName(std::string(b)));
}

Result<std::string> extractZipEntry(mz_zip_archive& zip, std::string_view wanted) {
    const mz_uint n = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < n; ++i) {
        char name[512];
        if (!mz_zip_reader_get_filename(&zip, i, name, sizeof(name))) continue;
        if (!zipNameEquals(name, wanted)) continue;
        size_t outSize = 0;
        void* p = mz_zip_reader_extract_to_heap(&zip, i, &outSize, 0);
        if (p == nullptr) {
            return Result<std::string>::err(
                "Failed to extract \"" + std::string(wanted) + "\" from XLSX.");
        }
        std::string s(static_cast<char*>(p), outSize);
        mz_free(p);
        return Result<std::string>::ok(std::move(s));
    }
    return Result<std::string>::err(
        "XLSX is missing \"" + std::string(wanted) + "\".");
}

std::vector<std::string> parseSharedStrings(std::string_view xml) {
    std::vector<std::string> strings;
    std::size_t pos = 0;
    while (pos < xml.size()) {
        const std::size_t open = findOpenTag(xml, "si", pos);
        if (open == std::string_view::npos) break;
        const std::size_t gt = findTagClose(xml, open);
        if (gt == std::string_view::npos) break;
        if (isSelfClosing(xml, open, gt)) {
            strings.emplace_back();
            pos = gt + 1;
            continue;
        }
        const std::size_t close = findCloseTag(xml, "si", gt + 1);
        if (close == std::string_view::npos) break;
        strings.push_back(collectTText(xml, gt + 1, close));
        pos = close + 1;
    }
    return strings;
}

std::string resolveSheetPath(std::string_view workbookXml,
                             std::string_view relsXml) {
    std::string rId;
    std::size_t pos = findOpenTag(workbookXml, "sheet", 0);
    if (pos != std::string_view::npos) {
        const std::size_t gt = findTagClose(workbookXml, pos);
        if (gt != std::string_view::npos) {
            const auto attrs = tagAttributes(workbookXml, pos, gt);
            rId = xmlAttr(attrs, "id");
        }
    }
    if (rId.empty()) return "xl/worksheets/sheet1.xml";

    pos = 0;
    while (pos < relsXml.size()) {
        const std::size_t open = findOpenTag(relsXml, "Relationship", pos);
        if (open == std::string_view::npos) break;
        const std::size_t gt = findTagClose(relsXml, open);
        if (gt == std::string_view::npos) break;
        const auto attrs = tagAttributes(relsXml, open, gt);
        if (xmlAttr(attrs, "Id") == rId) {
            std::string target = xmlAttr(attrs, "Target");
            target = normalizeZipName(target);
            if (target.rfind("xl/", 0) != 0) {
                target = "xl/" + target;
            }
            return target;
        }
        pos = gt + 1;
    }
    return "xl/worksheets/sheet1.xml";
}

std::string cellValue(std::string_view xml, std::size_t open, std::size_t gt,
                      std::size_t close, const std::vector<std::string>& sst) {
    const auto attrs = tagAttributes(xml, open, gt);
    const std::string type = xmlAttr(attrs, "t");
    const std::size_t contentUntil =
        (close == std::string_view::npos) ? gt : close;

    if (type == "s") {
        const std::string v = trim(innerTextOf(xml, "v", open, contentUntil));
        if (v.empty()) return {};
        std::size_t idx = 0;
        for (char c : v) {
            if (c < '0' || c > '9') return {};
            idx = idx * 10 + static_cast<std::size_t>(c - '0');
        }
        if (idx >= sst.size()) return {};
        return sst[idx];
    }
    if (type == "inlineStr") {
        return collectTText(xml, open, contentUntil);
    }
    if (type == "b") {
        const std::string v = trim(innerTextOf(xml, "v", open, contentUntil));
        return (v == "1" || iequals(v, "true")) ? "TRUE" : "FALSE";
    }
    if (type == "str" || type == "e") {
        return trim(innerTextOf(xml, "v", open, contentUntil));
    }
    return trim(innerTextOf(xml, "v", open, contentUntil));
}

Result<std::vector<std::vector<std::string>>> parseSheet(
    std::string_view xml, const std::vector<std::string>& sst) {
    std::map<std::pair<int, int>, std::string> cells;
    int maxRow = 0;
    int maxCol = 0;

    std::size_t pos = 0;
    while (pos < xml.size()) {
        const std::size_t open = findOpenTag(xml, "c", pos);
        if (open == std::string_view::npos) break;
        const std::size_t gt = findTagClose(xml, open);
        if (gt == std::string_view::npos) break;
        const auto attrs = tagAttributes(xml, open, gt);
        const std::string ref = xmlAttr(attrs, "r");
        int col = 0;
        int row = 0;
        if (ref.empty() || !parseCellRef(ref, col, row)) {
            pos = gt + 1;
            continue;
        }

        std::size_t close = std::string_view::npos;
        std::size_t until = gt;
        if (!isSelfClosing(xml, open, gt)) {
            close = findCloseTag(xml, "c", gt + 1);
            if (close != std::string_view::npos) until = close;
        }
        const std::string value = cellValue(xml, open, gt, until, sst);
        cells[{row, col}] = value;
        if (row > maxRow) maxRow = row;
        if (col > maxCol) maxCol = col;
        pos = (close == std::string_view::npos) ? gt + 1 : close + 1;
    }

    if (maxRow == 0) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "XLSX worksheet is empty.");
    }

    std::vector<std::vector<std::string>> grid(
        static_cast<std::size_t>(maxRow),
        std::vector<std::string>(static_cast<std::size_t>(maxCol + 1)));
    for (const auto& [rc, value] : cells) {
        grid[static_cast<std::size_t>(rc.first - 1)]
            [static_cast<std::size_t>(rc.second)] = value;
    }
    return Result<std::vector<std::vector<std::string>>>::ok(std::move(grid));
}

}  // namespace

Result<std::vector<std::vector<std::string>>> parseXlsx(std::string_view bytes) {
    if (bytes.empty()) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "Import file is empty.");
    }
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<mz_uint>::max())) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "XLSX file is too large.");
    }

    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, bytes.data(), bytes.size(), 0)) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "File is not a valid XLSX (ZIP) archive.");
    }

    auto workbook = extractZipEntry(zip, "xl/workbook.xml");
    auto rels = extractZipEntry(zip, "xl/_rels/workbook.xml.rels");
    std::string sheetPath = "xl/worksheets/sheet1.xml";
    if (workbook && rels) {
        sheetPath = resolveSheetPath(workbook.value(), rels.value());
    }

    std::vector<std::string> sst;
    auto sstXml = extractZipEntry(zip, "xl/sharedStrings.xml");
    if (sstXml) {
        sst = parseSharedStrings(sstXml.value());
    }

    auto sheetXml = extractZipEntry(zip, sheetPath);
    mz_zip_reader_end(&zip);
    if (!sheetXml) {
        return Result<std::vector<std::vector<std::string>>>::err(
            sheetXml.error());
    }
    return parseSheet(sheetXml.value(), sst);
}

}  // namespace tracker
