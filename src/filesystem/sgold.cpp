#include "filesystem/sgold.h"
#include "filesystem/ex.h"

#include "help.h"
#include <sys/stat.h>

#include <fstream>

// Судя по всему, структура заголовка такая
// Например

// 6C 01 20 01 00 00 21 30 6D 01 00 00 FF FF 6E 01 68 61 74 34 2E 70 6E 67 00

// 6C 01 - ID записи
// 20 01 - ID Родителя (родительский каталог)
// 00 00 21 30 Пока не понятно
// 6D 01 - Вероятно, ID блока с данныем, который идёт хвостом

// 00 00 FF FF - Какие-то аттрибуты если каталог
// 10 00 FF FF - Какие-то аттрибуты если файл

// 6E 01 - Если файл умещается в 1024 байта оно равно FF FF, в противном случае - ID следующего описательного блока
// 68 61 74 34 2E 70 6E 67 00 - Имя файла/каталога

// За загаловком следует блок данных файла

// =============================================================

// Т.е. если файл умещается в 1 блок 1024
// Загаловок
// Данные

// Если файл не умещается в 1024
// Заголовок
// Данные
// Загаловок part
// Данные
// Загаловок part
// Данные
// ....
// пока не влезем

// =============================================================

// Загаловок part - пока не полностью понятно
// 6E 01 20 01 00 00 21 30 6F 01 00 00 6C 01 70 01 
// 6E 01 - ID
// 20 01 - Родитель
// 00 00 21 30 Пока не понятно
// 6F 01 - ID блока с данными
// 6C 01 - ID предыдущего блока
// 70 01 - ID следующего описательного part блока

// Последние 4 байта - ID следующего блока. Если последний - FF FF

// =============================================================

// Например hat4.png
// 6C 01 20 01 00 00 21 30 6D 01 00 00 FF FF 6E 01 68 61 74 34 2E 70 6E 67 00 
// Данные ID 6D 01 - исходя из заголовка
// 6E 01 20 01 00 00 21 30 6F 01 00 00 6C 01 70 01 
// Данные ID 6F 01 - исходя из заголовка
// 70 01 20 01 00 00 21 30 71 01 00 00 6E 01 FF FF
// Данные ID 71 01 - исходя из заголовка


namespace FULLFLASH {
namespace Filesystem {

SGOLD::SGOLD(Blocks &blocks) : blocks(blocks) { }

void SGOLD::load() {
    parse_FIT();
}

void SGOLD::extract(std::string path) {
    fmt::print("Extracting filesystem\n");

    int r = mkdir(path.c_str(), 0755);

    if (r == -1) {
        throw Exception("Couldn't create directory '{}': {}", path, strerror(errno));
    }

    for (const auto &fs : fs_map) {
        std::string fs_name = fs.first;
        auto root           = fs.second;

        fmt::print("Extracting {}\n", fs_name);

        unpack(root, path + "/" + fs_name);
    }
}

void SGOLD::print_fit_header(const SGOLD::FITHeader &header) {
    fmt::print("FIT:\n");
    fmt::print("Flags:  {:08X}\n",  header.flags);
    fmt::print("ID:     {:d}\n",    header.id);
    fmt::print("Size:   {:d}\n",    header.size);
    fmt::print("Offset: {:08X}\n",  header.offset);
    fmt::print("===========================\n");
}

void SGOLD::print_file_header(const SGOLD::FileHeader &header) {
    fmt::print("File:\n");
    fmt::print("ID:           {}\n",      header.id);
    fmt::print("Parent ID:    {}\n",      header.parent_id);
    fmt::print("Unknown:      {:04X}\n",  header.unknown);
    fmt::print("Data ID:      {}\n",      header.data_id);
    fmt::print("Attributes:   {:04X}\n",  header.attributes);
    fmt::print("Next part ID: {}\n",      header.next_part);
    fmt::print("Name:         {}\n",      header.name);
    fmt::print("===========================\n");
}

void SGOLD::print_file_part(const SGOLD::FilePart &part) {
    fmt::print("File part:\n");
    fmt::print("ID:           {}\n",      part.id);
    fmt::print("Parent ID:    {}\n",      part.parent_id);
    fmt::print("Unknown:      {:04X}\n",  part.unknown);
    fmt::print("Data ID:      {}\n",      part.data_id);
    fmt::print("Unknown2:     {:04X}\n",  part.unknown2);
    fmt::print("Prev ID:      {}\n",      part.prev_id);
    fmt::print("Next part ID: {}\n",      part.next_part);
    fmt::print("===========================\n");
}

SGOLD::FileHeader SGOLD::read_file_header(const RawData &data) {
    SGOLD::FileHeader   header;
    size_t              offset = 0;

    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.data_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.attributes), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    data.read_string(offset, header.name);
    
    return header;
}

SGOLD::FilePart SGOLD::read_file_part(const RawData &data) {
    SGOLD::FilePart part;
    size_t          offset = 0;

    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.unknown), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.data_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.unknown2), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.prev_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void SGOLD::parse_FIT() {
    Blocks::Map &bl = blocks.get_blocks();

    for (const auto &block : blocks.get_blocks()) {
        const auto &ffs_block_name  = block.first;
        const auto &ffs             = block.second;
        FSBlocksMap ffs_map;

        if (ffs_block_name.find("FFS") != std::string::npos) {
            fmt::print("{} Blocks:\n", ffs_block_name, ffs.size());
        } else {
            continue;
        }

        bool data_end = true;

        size_t max_block = 0;
        for (auto &block : ffs) {
            fmt::print("  Block {:08X} Size: {}\n", block.offset, block.data.get_size());

            const RawData & block_data = block.data;
            size_t          block_size = block_data.get_size();

            for (ssize_t offset = block_size - 16; offset > 0; offset -= 16) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                if (fs_block.header.flags == 0xFFFFFFFF) {
                    break;
                }

                if (fs_block.header.flags == 0xFFFFFF00) {
                    continue;
                }

                // fs_block.data = RawData(block_data_ptr + fs_block.header.offset, fs_block.header.size);
                fs_block.data = RawData(block_data, fs_block.header.offset, fs_block.header.size);

                if (ffs_map.count(fs_block.header.id)) {
                    throw Exception("Duplicate id {}", fs_block.header.id);
                }

                ffs_map[fs_block.header.id] = fs_block;

                if (fs_block.header.id > max_block) {
                    max_block = fs_block.header.id;
                }
            }
        }

        const FFSBlock &    root_block      = ffs_map.at(6);
        FileHeader          root_header     = read_file_header(root_block.data);
        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_map, root, root_header);

        fs_map[ffs_block_name] = root;
    }
}

void SGOLD::unpack(Directory::Ptr dir, std::string path) {
    int r = mkdir(path.c_str(), 0755);

    if (r == -1) {
        throw Exception("Couldn't create directory '{}': {}", path, strerror(errno));
    }

    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::string     file_path = path + "/" + file->get_name();
        std::ofstream   file_stream;

        fmt::print("  {}\n", file_path);

        file_stream.open(file_path, std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw Exception("Couldn't create file '{}': {}", file_path, strerror(errno));
        }

        file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());

        file_stream.close();
    }

    for (const auto &subdir : subdirs) {
        unpack(subdir, path + "/" + subdir->get_name());
    }
}

void SGOLD::scan(FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, std::string path) {
    RawData data    = read_full_data(ffs_map, header);
    size_t  offset  = 0;

    while (offset < data.get_size()) {
        uint32_t raw;

        data.read<uint32_t>(offset, reinterpret_cast<char *>(&raw), 1);

        uint16_t    id = raw & 0xFFFF;
        uint16_t    unk = (raw >> 16) & 0xFFFF;

        if (id == 0xFFFF) {
            continue;
        }

        if (id == 0) {
            continue;
        }

        fmt::print("  {:5d}: ", id);
        fmt::print("  {}", path);

        if (ffs_map.count(id)) {
            const FFSBlock &tmp     = ffs_map.at(id);
            FileHeader      title   = read_file_header(tmp.data);

            fmt::print("{}\n", title.name);

            if (title.attributes & 0x10) {
                Directory::Ptr dir_next = Directory::build(title.name);

                dir->add_subdir(dir_next);

                scan(ffs_map, dir_next, title, path + title.name + "/");
            } else {
                try {
                    RawData file_data;

                    if (ffs_map.count(title.data_id)) {
                        file_data = read_full_data(ffs_map, title);
                    }
                
                    File::Ptr file = File::build(title.name, file_data);

                    dir->add_file(file);
                } catch (const Exception &e) {
                    fmt::print("Warning! Broken file: {}\n", e.what());
                }
            }
        }
    }
}

void SGOLD::read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id) {
    if (!ffs_map.count(next_id)) {
        throw Exception("Reading part. Couldn't find block with id: {}", next_id);
    }

    const FFSBlock &block      = ffs_map.at(next_id);

    FilePart        part       = read_file_part(block.data);

    if (!ffs_map.count(part.data_id)) {
        throw Exception("Reading part data. Couldn't find block with id: {}", part.data_id);
    }

    FFSBlock &      block_data = ffs_map.at(part.data_id);

    data.add(block_data.data);

    if (part.next_part != 0xFFFF) {
        read_recurse(ffs_map, data, part.next_part);
    }
}

RawData SGOLD::read_full_data(FSBlocksMap &ffs_map, const FileHeader &header) {
    if (!ffs_map.count(header.data_id)) {
        throw Exception("Reading file data. Couldn't find block with id: {}", header.data_id);
    }

    const FFSBlock &block = ffs_map.at(header.data_id);
    RawData         data_full(block.data);

    if (header.next_part != 0xFFFF) {
        read_recurse(ffs_map, data_full, header.next_part);
    }

    return data_full;
}

};
};
