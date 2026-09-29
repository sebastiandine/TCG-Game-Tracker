#include "tracker/export/GameExportWrite.hpp"

#include "ExportUtil.hpp"

#include <array>
#include <string>
#include <string_view>

namespace tracker {
namespace {

bool needsQuotes(std::string_view s) {
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') return true;
    }
    return false;
}

std::string csvField(std::string_view s) {
    if (!needsQuotes(s)) return std::string(s);
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') {
            out.push_back('"');
            out.push_back('"');
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

void appendRow(std::string& out, const std::array<std::string, 8>& cells) {
    for (std::size_t i = 0; i < cells.size(); ++i) {
        if (i > 0) out.push_back(',');
        out += csvField(cells[i]);
    }
    out.push_back('\n');
}

}  // namespace

Result<std::string> writeGameExportCsv(const std::vector<GameExportRow>& rows) {
    std::string out;
    out.append("\xEF\xBB\xBF");

    std::array<std::string, 8> header{};
    for (std::size_t i = 0; i < export_detail::kGameExportHeaders.size(); ++i) {
        header[i] = std::string(export_detail::kGameExportHeaders[i]);
    }
    appendRow(out, header);

    for (const auto& row : rows) {
        appendRow(out, export_detail::cellsForRow(row));
    }
    return Result<std::string>::ok(std::move(out));
}

}  // namespace tracker
