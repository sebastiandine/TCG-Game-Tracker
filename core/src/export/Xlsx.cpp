#include "tracker/export/GameExportWrite.hpp"

#include "ExportUtil.hpp"

#include <miniz.h>

#include <string>
#include <string_view>
#include <vector>

namespace tracker {
namespace {

std::string xmlEscape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&apos;";
                break;
            default:
                if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
                    break;
                }
                out.push_back(static_cast<char>(c));
                break;
        }
    }
    return out;
}

std::string cellRef(int col, int row) {
    std::string ref;
    ref.push_back(static_cast<char>('A' + col));
    ref += std::to_string(row);
    return ref;
}

std::string inlineCell(int col, int row, std::string_view text) {
    return "<c r=\"" + cellRef(col, row) +
           "\" t=\"inlineStr\"><is><t xml:space=\"preserve\">" +
           xmlEscape(text) + "</t></is></c>";
}

std::string sheetXml(const std::vector<GameExportRow>& rows) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>";

    xml += "<row r=\"1\">";
    for (int c = 0; c < 8; ++c) {
        xml += inlineCell(c, 1, export_detail::kGameExportHeaders[
            static_cast<std::size_t>(c)]);
    }
    xml += "</row>";

    int r = 2;
    for (const auto& row : rows) {
        const auto cells = export_detail::cellsForRow(row);
        xml += "<row r=\"" + std::to_string(r) + "\">";
        for (int c = 0; c < 8; ++c) {
            xml += inlineCell(c, r, cells[static_cast<std::size_t>(c)]);
        }
        xml += "</row>";
        ++r;
    }

    xml += "</sheetData></worksheet>";
    return xml;
}

bool addMem(mz_zip_archive& zip, const char* name, const std::string& data) {
    return mz_zip_writer_add_mem(&zip, name, data.data(), data.size(),
                                 MZ_DEFAULT_LEVEL) != 0;
}

}  // namespace

Result<std::string> writeGameExportXlsx(const std::vector<GameExportRow>& rows) {
    const std::string contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" "
        "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
        "</Types>";

    const std::string rootRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
        "Target=\"xl/workbook.xml\"/>"
        "</Relationships>";

    const std::string workbook =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Games\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>";

    const std::string workbookRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
        "Target=\"worksheets/sheet1.xml\"/>"
        "<Relationship Id=\"rId2\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
        "Target=\"styles.xml\"/>"
        "</Relationships>";

    const std::string styles =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
        "<fills count=\"2\">"
        "<fill><patternFill patternType=\"none\"/></fill>"
        "<fill><patternFill patternType=\"gray125\"/></fill>"
        "</fills>"
        "<borders count=\"1\"><border/></borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs>"
        "</styleSheet>";

    const std::string sheet = sheetXml(rows);

    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
        return Result<std::string>::err("Failed to create XLSX archive.");
    }

    const bool added =
        addMem(zip, "[Content_Types].xml", contentTypes) &&
        addMem(zip, "_rels/.rels", rootRels) &&
        addMem(zip, "xl/workbook.xml", workbook) &&
        addMem(zip, "xl/_rels/workbook.xml.rels", workbookRels) &&
        addMem(zip, "xl/styles.xml", styles) &&
        addMem(zip, "xl/worksheets/sheet1.xml", sheet);
    if (!added) {
        mz_zip_writer_end(&zip);
        return Result<std::string>::err("Failed to write XLSX archive entries.");
    }

    void* buf = nullptr;
    size_t size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buf, &size) ||
        buf == nullptr) {
        mz_zip_writer_end(&zip);
        return Result<std::string>::err("Failed to finalize XLSX archive.");
    }
    std::string out(static_cast<char*>(buf), size);
    mz_free(buf);
    mz_zip_writer_end(&zip);
    return Result<std::string>::ok(std::move(out));
}

}  // namespace tracker
