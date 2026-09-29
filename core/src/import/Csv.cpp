#include "tracker/import/GameImportParse.hpp"

namespace tracker {
namespace {

std::string_view stripBom(std::string_view text) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.remove_prefix(3);
    }
    return text;
}

char sniffDelimiter(std::string_view text) {
    int commas = 0;
    int semicolons = 0;
    bool inQuotes = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '"') {
            if (inQuotes && i + 1 < text.size() && text[i + 1] == '"') {
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
            continue;
        }
        if (inQuotes) continue;
        if (c == '\n' || c == '\r') break;
        if (c == ',') ++commas;
        if (c == ';') ++semicolons;
    }
    return (semicolons > commas) ? ';' : ',';
}

}  // namespace

Result<std::vector<std::vector<std::string>>> parseCsv(std::string_view text) {
    const std::string_view in = stripBom(text);
    if (in.empty()) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "Import file is empty.");
    }

    const char delim = sniffDelimiter(in);
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool inQuotes = false;

    const auto flushRow = [&]() {
        row.push_back(std::move(field));
        field.clear();
        rows.push_back(std::move(row));
        row.clear();
    };

    for (std::size_t i = 0; i < in.size(); ++i) {
        const char c = in[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < in.size() && in[i + 1] == '"') {
                    field.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field.push_back(c);
            }
            continue;
        }

        if (c == '"') {
            inQuotes = true;
        } else if (c == delim) {
            row.push_back(std::move(field));
            field.clear();
        } else if (c == '\n') {
            flushRow();
        } else if (c == '\r') {
            flushRow();
            if (i + 1 < in.size() && in[i + 1] == '\n') ++i;
        } else {
            field.push_back(c);
        }
    }

    if (inQuotes) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "Unclosed quote in CSV.");
    }

    if (!field.empty() || !row.empty()) {
        flushRow();
    }

    if (rows.empty()) {
        return Result<std::vector<std::vector<std::string>>>::err(
            "Import file is empty.");
    }

    return Result<std::vector<std::vector<std::string>>>::ok(std::move(rows));
}

}  // namespace tracker
