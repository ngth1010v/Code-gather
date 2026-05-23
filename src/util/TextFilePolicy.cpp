#include "util/TextFilePolicy.h"

#include "util/PathUtil.h"
#include "util/StringUtil.h"

#include <fstream>
#include <set>
#include <vector>

namespace cgt::util
{

    bool IsTextCandidate(const std::filesystem::path& filePath) 
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return false; // Không mở được file
        }

        // Đọc 4 byte đầu tiên để kiểm tra BOM nhanh
        unsigned char bom[4] = {0};
        file.read(reinterpret_cast<char*>(bom), 4);
        std::streamsize bytesRead = file.gcount();

        if (bytesRead >= 2) {
            // UTF-16 LE (Thường thấy nhất ở file .iss của Inno Setup)
            if (bom[0] == 0xFF && bom[1] == 0xFE) return true;
            // UTF-16 BE
            if (bom[0] == 0xFE && bom[1] == 0xFF) return true;
        }
        if (bytesRead >= 3) {
            // UTF-8 với BOM
            if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) return true;
        }
        if (bytesRead >= 4) {
            // UTF-32 LE
            if (bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00) return true;
            // UTF-32 BE
            if (bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF) return true;
        }

        // --- TRƯỜNG HỢP KHÔNG CÓ BOM (ANSI, UTF-8 không BOM, hoặc UTF-16 không BOM) ---
        // Quay trở về vị trí đầu file để đọc một buffer lớn hơn phân tích xác suất
        file.clear();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(1024);
        file.read(buffer.data(), buffer.size());
        std::streamsize totalBytes = file.gcount();

        if (totalBytes == 0) {
            return true; // File trống rỗng coi như text hợp lệ
        }

        size_t nullCount = 0;
        size_t continuousNulls = 0;
        size_t maxContinuousNulls = 0;

        for (std::streamsize i = 0; i < totalBytes; ++i) {
            unsigned char ch = static_cast<unsigned char>(buffer[i]);

            if (ch == 0x00) {
                nullCount++;
                continuousNulls++;
                if (continuousNulls > maxContinuousNulls) {
                    maxContinuousNulls = continuousNulls;
                }
            } else {
                continuousNulls = 0;
                
                // Tiêu chuẩn điều hướng nâng cao:
                // Nếu xuất hiện các ký tự điều khiển dị thường (nhỏ hơn 0x09) 
                // ngoại trừ các dấu xuống dòng/tab quen thuộc, thì khả năng cao là binary
                if (ch < 0x07) { 
                    return false; 
                }
            }
        }

        // Logic phân định thông minh:
        // 1. Nếu xuất hiện >= 4 null byte đứng LIÊN TIẾP nhau -> Chắc chắn là cấu trúc file nhị phân (Binary).
        if (maxContinuousNulls >= 4) {
            return false;
        }

        // 2. Kiểm tra tỷ lệ Null Byte:
        // Trong file UTF-16 không BOM (chữ Latinh), lượng null byte chiếm khoảng 45% - 50%.
        // Trong file nhị phân thông thường, null byte hiếm khi chiếm mật độ dày đặc mà không đi liền nhau mà không có lý do.
        double nullRatio = static_cast<double>(nullCount) / totalBytes;
        
        if (nullCount > 0) {
            // Nếu có null byte nhưng phân bổ cách quãng (maxContinuousNulls bé) và tỷ lệ lấp đầy cao quanh mức 40-50%
            // -> Đây là đặc trưng của UTF-16 text không chứa BOM.
            if (nullRatio >= 0.35 && nullRatio <= 0.55) {
                return true; 
            }
            // Mật độ null rải rác linh tinh khác -> Nhị phân
            return false;
        }

        // Không có bất kỳ null byte nào -> An tâm 100% là UTF-8 hoặc ANSI thuần túy
        return true;
    }

    bool IsFilterToken(const std::wstring& token)
    {
        if (token.size() < 2 || token[0] != L'.')
        {
            return false;
        }

        if (token.find(L'\\') != std::wstring::npos || token.find(L'/') != std::wstring::npos)
        {
            return false;
        }

        return true;
    }

    bool MatchesFilters(const fs::path& path, const std::vector<std::wstring>& filters)
    {
        if (filters.empty())
        {
            return true;
        }

        const std::wstring ext = ToLower(path.extension().wstring());
        for (const std::wstring& filter : filters)
        {
            if (ToLower(filter) == ext)
            {
                return true;
            }
        }

        return false;
    }
}
