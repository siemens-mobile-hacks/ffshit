#include "blocks.h"
#include "help.h"
#include "ex.h"

#include <cctype>
#include <fstream>
#include <cstring>
#include <cerrno>

namespace FULLFLASH {

Blocks::Blocks() { }

void Blocks::load(std::string fullflash_path) {
    std::ifstream file;

    file.open(fullflash_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw Exception("Couldn't open file '{}': {}\n", fullflash_path, std::string(strerror(errno)));
    }

    file.seekg(0, std::ios_base::end);
    size_t data_size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    this->data = RawData(file, 0, data_size);
    
    search_blocks();

    if (blocks_map.size() == 0) {
        search_blocks_x85();
    }
}

static size_t search_end(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned char data = buf[i];

        if (data == 0x0) {
            return i;
        }
    }

    return size;
}

static bool is_printable(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (isprint(static_cast<unsigned char>(buf[i]))) {
            continue;;
        }

        return false;
    }

    return true;
}

static bool is_empty(const char *buf, size_t size) {
    char data[size];

    memcpy(data, buf, size);

    for (size_t i = 0; i < size; ++i) {
        if (data[i] != 0xFF) {
            return false;
        }
    }

    return true;
}

void Blocks::search_blocks() {
    char *buf = data.get_data().get();

    for (size_t offset = 0; offset < data.get_size(); offset += block_size) {
        char *ptr =         buf + offset;
        constexpr size_t    header_size = 4 + 2 + 2 + 8;

        if (is_empty(buf, header_size)) {
            continue;
        }

        BlockHeader header;

        ptr = read_data<char>(header.name, ptr, 8);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_1), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_2), ptr, 1);
        ptr = read_data<uint32_t>(reinterpret_cast<char *>(&header.unknown_3), ptr, 1);

        if (header.unknown_3 != 0xFFFFFFF0) {
            continue;
        }

        // fmt::print("Name: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}\n", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

        if (header.unknown_2 != 0x0000 && header.unknown_3 != 0xFFFFFFF0) {
            // fmt::print("\n");

            continue;
        }

        size_t end_of_name = search_end(header.name, 8);

        if (end_of_name == 8) {
            continue;
        }

        if (!is_printable(header.name, end_of_name)) {
            continue;
        }

        header.name[end_of_name] = '\0';

        std::string block_name(header.name);
        std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };

        bool any_find = false;

        for (const auto &name : names) {
            if (block_name.find(name) != std::string::npos) {
                any_find = true;

                break;
            }
        }

        if (any_find == false) {
            continue;
        }

        if (!blocks_map.count(block_name)) {
            blocks_map[block_name] = std::vector<Block>();
        }

        Block block;

        block.header    = header;
        block.offset    = offset;
        block.count     = 2;
        block.data      = RawData(buf + offset, block_size * block.count);
        // block.size      = block_size * block.count;
        // block.data      = std::shared_ptr<char[]>(new char[block.size]);

        // ptr = buf + offset;
        // memcpy(block.data.get(), ptr, block.size);

        blocks_map[block_name].push_back(block);

        offset += block_size;
    }
}

void Blocks::search_blocks_x85() {
    char *buf = data.get_data().get();

    for (size_t offset = 0; offset < data.get_size(); offset += block_size) {
        char *ptr =         buf + offset;
        constexpr size_t    header_size = 4 + 2 + 2 + 8;

        if (is_empty(buf, header_size)) {
            continue;
        }

        BlockHeader header;

        ptr = read_data<char>(header.name, ptr + block_size - 32, 8);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_1), ptr, 1);
        ptr = read_data<uint16_t>(reinterpret_cast<char *>(&header.unknown_2), ptr, 1);
        ptr = read_data<uint32_t>(reinterpret_cast<char *>(&header.unknown_3), ptr, 1);

        if (header.unknown_3 != 0xFFFFFFF0) {
            continue;
        }

        // fmt::print("Name: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}\n", header.name, header.unknown_1, header.unknown_2, header.unknown_3);

        if (header.unknown_2 != 0x0000 && header.unknown_3 != 0xFFFFFFF0) {
            // fmt::print("\n");

            continue;
        }

        size_t end_of_name = search_end(header.name, 8);

        if (end_of_name == 8) {
            continue;
        }

        if (!is_printable(header.name, end_of_name)) {
            continue;
        }

        header.name[end_of_name] = '\0';

        std::string block_name(header.name);
        std::vector<std::string> names = { "EEFULL", "EELITE", "FFS" };

        bool any_find = false;

        for (const auto &name : names) {
            if (block_name.find(name) != std::string::npos) {
                any_find = true;

                break;
            }
        }

        if (any_find == false) {
            continue;
        }

        if (!blocks_map.count(block_name)) {
            blocks_map[block_name] = std::vector<Block>();
        }

        Block block;

        block.header    = header;
        block.offset    = offset - (block_size * 3);
        block.count     = 4;
        block.data      = RawData(buf + block.offset, block_size * block.count);

        blocks_map[block_name].push_back(block);
        offset += block_size * (block.count - 1);
    }
}

void Blocks::print() {
    for (const auto &pair : blocks_map) {
        fmt::print("{} Count: {}:\n", pair.first, pair.second.size());

        for (const auto &block : pair.second) {
            fmt::print("  0x{:08X}: {}\n", block.offset, block.header.name);
        }
    }
}

Blocks::Map &Blocks::get_blocks() {
    return blocks_map;
}

RawData &Blocks::get_data() {
    return data;
}

};
