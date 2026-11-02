#include "zip.h"
#include "log.h"
#include <windows.h>
#include <map>

struct ZipEntry {
    std::string  name;    
    unsigned int crc;
    unsigned int dosTime;
    unsigned int dosDate;
    std::string  data;
};

static unsigned int Crc32(const std::string& data) {
    DBG(L"data=[%d байт]", (int)data.size());
    unsigned int crc = 0xFFFFFFFF;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
    }
    return ~crc;
}

static void Put16(std::string& buf, unsigned int v) {
    DBG(L"buf=[%d байт], v=0x%04X", (int)buf.size(), v & 0xFFFF);
    buf += (char)(v & 0xFF);
    buf += (char)((v >> 8) & 0xFF);
}

static void Put32(std::string& buf, unsigned int v) {
    DBG(L"buf=[%d байт], v=0x%08X", (int)buf.size(), v);
    Put16(buf, v & 0xFFFF);
    Put16(buf, v >> 16);
}

static unsigned int Get16(const std::string& buf, size_t pos) {
    return (unsigned char)buf[pos] | ((unsigned char)buf[pos + 1] << 8);
}

static unsigned int Get32(const std::string& buf, size_t pos) {
    return Get16(buf, pos) | (Get16(buf, pos + 2) << 16);
}

static void DosDateTime(unsigned long long writeTime, unsigned int& dosTime, unsigned int& dosDate) {
    DBG(L"writeTime=%llu", writeTime);
    FILETIME ft, local;
    ft.dwLowDateTime  = (DWORD)(writeTime & 0xFFFFFFFF);
    ft.dwHighDateTime = (DWORD)(writeTime >> 32);
    FileTimeToLocalFileTime(&ft, &local);
    SYSTEMTIME st;
    FileTimeToSystemTime(&local, &st);
    if (st.wYear < 1980) { st.wYear = 1980; st.wMonth = 1; st.wDay = 1; }
    dosTime = (st.wHour << 11) | (st.wMinute << 5) | (st.wSecond / 2);
    dosDate = ((st.wYear - 1980) << 9) | (st.wMonth << 5) | st.wDay;
}

static bool ReadWholeFile(const std::wstring& path, std::string& out) {
    DBG(L"path=\"%ls\"", path.c_str());
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;

    out.clear();
    char buf[65536];
    DWORD got;
    while (ReadFile(h, buf, sizeof(buf), &got, NULL) && got > 0)
        out.append(buf, got);
    CloseHandle(h);
    return true;
}

static void MakeDirsFor(const std::wstring& filePath) {
    DBG(L"filePath=\"%ls\"", filePath.c_str());
    size_t pos = 0;
    while ((pos = filePath.find(L'\\', pos + 1)) != std::wstring::npos)
        CreateDirectoryW(filePath.substr(0, pos).c_str(), NULL);
}

static bool WriteInPlace(const std::wstring& path, const std::string& data) {
    DBG(L"path=\"%ls\", data=[%d байт]", path.c_str(), (int)data.size());
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(h, data.data(), (DWORD)data.size(), &written, NULL)
              && written == data.size()
              && SetEndOfFile(h);         
    FlushFileBuffers(h);
    CloseHandle(h);
    return ok != FALSE;
}

static std::wstring Key(const std::string& utf8Name) {
    std::wstring w = FromUtf8(utf8Name);
    if (!w.empty()) CharLowerBuffW(&w[0], (DWORD)w.size());
    return w;
}

static bool ReadExistingZip(const std::wstring& zipPath, std::vector<ZipEntry>& entries) {
    DBG(L"zipPath=\"%ls\"", zipPath.c_str());
    entries.clear();
    std::string zip;
    if (!ReadWholeFile(zipPath, zip) || zip.size() < 22) return false;

    size_t eocd = std::string::npos;
    size_t minPos = zip.size() > 22 + 65535 ? zip.size() - 22 - 65535 : 0;
    for (size_t p = zip.size() - 22 + 1; p-- > minPos; ) {
        if (Get32(zip, p) == 0x06054b50) { eocd = p; break; }
    }
    if (eocd == std::string::npos) return false;

    unsigned int count    = Get16(zip, eocd + 10);
    unsigned int cdSize   = Get32(zip, eocd + 12);
    unsigned int cdOffset = Get32(zip, eocd + 16);
    if ((size_t)cdOffset + cdSize > eocd) return false;

    size_t p = cdOffset;
    for (unsigned int i = 0; i < count; i++) {
        if (p + 46 > eocd || Get32(zip, p) != 0x02014b50) return false;
        unsigned int method   = Get16(zip, p + 10);
        unsigned int dosTime  = Get16(zip, p + 12);
        unsigned int dosDate  = Get16(zip, p + 14);
        unsigned int crc      = Get32(zip, p + 16);
        unsigned int compSize = Get32(zip, p + 20);
        unsigned int size     = Get32(zip, p + 24);
        unsigned int nameLen  = Get16(zip, p + 28);
        unsigned int extraLen = Get16(zip, p + 30);
        unsigned int commLen  = Get16(zip, p + 32);
        unsigned int lho      = Get32(zip, p + 42);
        if (p + 46 + nameLen > eocd) return false;
        std::string name = zip.substr(p + 46, nameLen);
        p += 46 + nameLen + extraLen + commLen;

        if (method != 0 || compSize != size) return false; 

        if ((size_t)lho + 30 > zip.size() || Get32(zip, lho) != 0x04034b50) return false;
        size_t dataPos = (size_t)lho + 30 + Get16(zip, lho + 26) + Get16(zip, lho + 28);
        if (dataPos + size > zip.size()) return false;

        if (!name.empty() && name.back() == '/') continue; 

        ZipEntry e;
        e.name = name;
        e.crc = crc;
        e.dosTime = dosTime;
        e.dosDate = dosDate;
        e.data = zip.substr(dataPos, size);
        entries.push_back(e);
    }
    return true;
}

static std::string Serialize(const std::vector<ZipEntry>& entries) {
    DBG(L"entries=[%d]", (int)entries.size());
    std::string zip;
    std::string centralDir;

    for (const ZipEntry& e : entries) {
        unsigned int size   = (unsigned int)e.data.size();
        unsigned int offset = (unsigned int)zip.size();

        Put32(zip, 0x04034b50);
        Put16(zip, 20);                      
        Put16(zip, 0x0800);              
        Put16(zip, 0);                   
        Put16(zip, e.dosTime);
        Put16(zip, e.dosDate);
        Put32(zip, e.crc);
        Put32(zip, size);
        Put32(zip, size);
        Put16(zip, (unsigned int)e.name.size());
        Put16(zip, 0);
        zip += e.name;
        zip += e.data;

        Put32(centralDir, 0x02014b50);
        Put16(centralDir, 20);
        Put16(centralDir, 20);
        Put16(centralDir, 0x0800);
        Put16(centralDir, 0);
        Put16(centralDir, e.dosTime);
        Put16(centralDir, e.dosDate);
        Put32(centralDir, e.crc);
        Put32(centralDir, size);
        Put32(centralDir, size);
        Put16(centralDir, (unsigned int)e.name.size());
        Put16(centralDir, 0);
        Put16(centralDir, 0);
        Put16(centralDir, 0);
        Put16(centralDir, 0);
        Put32(centralDir, 0);
        Put32(centralDir, offset);
        centralDir += e.name;
    }

    unsigned int cdOffset = (unsigned int)zip.size();
    zip += centralDir;

    Put32(zip, 0x06054b50);
    Put16(zip, 0);
    Put16(zip, 0);
    Put16(zip, (unsigned int)entries.size());
    Put16(zip, (unsigned int)entries.size());
    Put32(zip, (unsigned int)centralDir.size());
    Put32(zip, cdOffset);
    Put16(zip, 0);
    return zip;
}

bool UpdateZip(const std::vector<FileInfo>& files, const std::wstring& zipPath, ZipStats& st) {
    DBG(L"files=[%d], zipPath=\"%ls\"", (int)files.size(), zipPath.c_str());
    st = ZipStats();

    std::vector<ZipEntry> entries;
    bool existed = ReadExistingZip(zipPath, entries);
    if (!existed && GetFileAttributesW(zipPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        Log(L"Существующий архив будет перезаписан");

    std::map<std::wstring, size_t> index;
    for (size_t i = 0; i < entries.size(); i++) index[Key(entries[i].name)] = i;

    for (const FileInfo& f : files) {
        std::string data;
        if (!ReadWholeFile(f.fullPath, data)) {
            Log(L"Не удалось прочитать файл: " + f.fullPath);
            return false;
        }
        ZipEntry e;
        e.name = ToUtf8(f.nameInZip);
        e.crc  = Crc32(data);
        DosDateTime(f.writeTime, e.dosTime, e.dosDate);
        e.data.swap(data);

        auto it = index.find(Key(e.name));
        if (it == index.end()) {                       
            index[Key(e.name)] = entries.size();
            entries.push_back(e);
            st.added++;
            Log(L"  + добавлен: " + f.nameInZip);
        } else {
            ZipEntry& old = entries[it->second];
            if (old.crc == e.crc && old.data.size() == e.data.size()) {
                st.unchanged++;                        
            } else {                 
                old = e;
                st.replaced++;
                Log(L"  * заменён: " + f.nameInZip);
            }
        }
    }
    st.total = (unsigned int)entries.size();

    if (existed && st.added == 0 && st.replaced == 0) return true;

    MakeDirsFor(zipPath);
    if (!WriteInPlace(zipPath, Serialize(entries))) {
        Log(L"Не удалось обновить архив");
        return false;
    }
    st.written = true;
    return true;
}
