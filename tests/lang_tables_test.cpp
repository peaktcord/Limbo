#include <cstdio>
#include <string>
#include <vector>

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/text_resources.hpp"
#include "src/storage/resource_store.hpp"

namespace {

struct Walk {
    int32_t records = 0;
    int32_t maxId = -1;
    int32_t leftover = 0;
    std::string error;
};

Walk walk(const std::vector<uint8_t> &data) {
    Walk out;
    int32_t n3 = 0;
    int32_t n4 = 0;
    const int32_t len = (int32_t)data.size();
    while (true) {
        int32_t scan = n4;
        while (scan < len && data[scan] != '|') ++scan;
        if (scan >= len) break;
        n4 = scan;
        if (n3 != 0) n3 += 2;
        std::string record((const char *)data.data() + n3, (size_t)(n4 - n3));
        size_t space = record.find(' ');
        if (space == std::string::npos) {
            out.error = "record " + std::to_string(out.records + 1) + " has no space: " + record;
            return out;
        }
        std::string token = record.substr(0, space);
        for (char ch : token) {
            if (ch < '0' || ch > '9') {
                out.error = "record " + std::to_string(out.records + 1) +
                            " does not start with an id: " + token;
                return out;
            }
        }
        int32_t id = std::stoi(token);
        if (id > out.maxId) out.maxId = id;
        ++out.records;
        n3 = n4 + 1;
        ++n4;
    }
    out.leftover = len - n3;
    return out;
}

bool readFile(const std::string &path, std::vector<uint8_t> *out) {
    std::FILE *f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out->resize((size_t)(size < 0 ? 0 : size));
    if (!out->empty() && std::fread(out->data(), 1, out->size(), f) != out->size()) {
        std::fclose(f);
        return false;
    }
    std::fclose(f);
    return true;
}

}

int main(int argc, char **argv) {
    std::string dir = argc > 1 ? argv[1] : "artifacts/extracted/v1010";
    SessionContext session;
    SessionScope scope(session);
    voyage::storage::DirectoryResourceStore resources(dir);
    session.resources = &resources;
    int failures = 0;
    int checked = 0;
    for (int32_t n = 0; n <= 12; ++n) {
        std::string path = dir + "/lang_" + std::to_string(n) + ".txt";
        std::vector<uint8_t> data;
        if (!readFile(path, &data)) {
            std::printf("lang_%d.txt  missing, skipped\n", n);
            continue;
        }
        ++checked;
        Walk w = walk(data);
        if (!w.error.empty()) {
            std::printf("lang_%d.txt  UNPARSEABLE: %s\n", n, w.error.c_str());
            ++failures;
            continue;
        }
        bool ok = true;
        if (w.records != TextResources::record_count(n)) {
            std::printf("lang_%d.txt  record_count(%d) says %d records, the file has %d\n", n, n,
                        TextResources::record_count(n), w.records);
            ok = false;
        }
        if (w.maxId != TextResources::max_string_id(n)) {
            std::printf("lang_%d.txt  max_string_id(%d) says max id %d, the file's is %d\n", n, n,
                        TextResources::max_string_id(n), w.maxId);
            ok = false;
        }
        if (w.leftover != 0) {
            std::printf("lang_%d.txt  %d bytes past the last record\n", n, w.leftover);
            ok = false;
        }
        if (!ok) ++failures;
    }
    if (checked == 0) {
        std::printf("no language files found under %s -- extract the JAR first\n", dir.c_str());
        return 1;
    }

    std::vector<uint8_t> copyrightBytes;
    if (!readFile(dir + "/copywrite.txt", &copyrightBytes)) {
        std::printf("copywrite.txt missing\n");
        ++failures;
    } else {
        std::string expected(copyrightBytes.begin(), copyrightBytes.end());
        std::string actual = TextResources().load_copyright_text();
        if (actual != expected) {
            std::printf("copywrite.txt character decoding disagrees with TextResources\n");
            ++failures;
        }
    }
    std::printf("%d language files checked, %d disagree with TextResources metadata\n", checked, failures);
    return failures == 0 ? 0 : 1;
}
