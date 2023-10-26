#include "filesystem/newsgold.h"
#include "filesystem/ex.h"

#include "help.h"
#include <fstream>

#include <iconv.h>
#include <sys/stat.h>

namespace FULLFLASH {
namespace Filesystem {

NewSGOLD::NewSGOLD(Blocks &blocks) : blocks(blocks) { }

void NewSGOLD::load() {
    parse_FIT();
}

void NewSGOLD::extract(std::string path) {
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

void NewSGOLD::print_fit_header(const NewSGOLD::FITHeader &header) {
    fmt::print("FIT:\n");
    fmt::print("Flags:      {:08X}\n",  header.flags);
    fmt::print("ID:         {:d}\n",    header.id);
    fmt::print("Size:       {:d}\n",    header.size);
    fmt::print("Offset:     {:04X}\n",  header.offset);
    fmt::print("===========================\n");
}

void NewSGOLD::print_file_header(const NewSGOLD::FileHeader &header) {
    fmt::print("File:\n");
    fmt::print("ID:           {}\n",      header.id);
    fmt::print("Parent ID:    {}\n",      header.parent_id);
    fmt::print("Next part ID: {}\n",      header.next_part);
    fmt::print("Unknown2:     {:04X} {}\n",      header.unknown2, header.unknown2);
    fmt::print("Unknown3:     {:04X} {}\n",      header.unknown3, header.unknown3);
    fmt::print("Unknown4:     {:04X} {}\n",      header.unknown4, header.unknown4);
    fmt::print("Unknown5:     {:04X} {}\n",      header.unknown5, header.unknown5);
    fmt::print("Unknown6:     {:04X} {}\n",      header.unknown6, header.unknown6);
    fmt::print("Unknown7:     {:04X} {}\n",      header.unknown7, header.unknown7);
    fmt::print("Name:         {}\n",      header.name);
    fmt::print("===========================\n");
}

void NewSGOLD::print_file_part(const FilePart &part) {
    fmt::print("Part:\n");
    fmt::print("ID:             {}\n", part.id);
    fmt::print("Parent ID:      {}\n", part.parent_id);
    fmt::print("Next part ID:   {}\n", part.next_part);
    fmt::print("===========================\n");
}

NewSGOLD::FileHeader NewSGOLD::read_file_header(const RawData &data) {
    FileHeader  header;
    size_t      offset = 0;

    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown1), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown2), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown3), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown4), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown5), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown6), 1);
    data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

    // // header.data_id = header.id + 1;
 
    // // offset = 28;
    // data.read_string(offset, header.name, 2);
    offset = 28;
    size_t  str_size    = data.get_size() - 28;
    size_t  from_n      = str_size;
    char *  from        = new char[str_size];

    size_t  to_n        = str_size;
    char *  to          = new char[str_size];

    data.read<char>(offset, from, from_n);

    iconv_t iccd = iconv_open("utf8", "utf16");

    if (iccd == ((iconv_t)-1)) {
        throw Exception("iconv_open(): {}", strerror(errno));
    }

    char *inptr = from;
    char *ouptr = to;

    int r = iconv(iccd, &inptr, &from_n, &ouptr, &to_n);

    if (r == -1) {
        throw Exception("iconv(): {}", strerror(errno));
    }

    to[str_size - to_n] = 0x00;

    header.name = std::string(to);

    delete []from;
    delete []to;

    return header;
}

NewSGOLD::FilePart NewSGOLD::read_file_part(const RawData &data)  {
    FilePart    part;
    size_t      offset = 0;

    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void NewSGOLD::parse_FIT() {
    Blocks::Map &bl = blocks.get_blocks();

    for (const auto &block : blocks.get_blocks()) {
        const auto &ffs_block_name  = block.first;
        const auto &ffs             = block.second;

        FSBlocksMap ffs_map;

        if (ffs_block_name.find("FFS") != std::string::npos) {
            fmt::print("{} Blocks: {}\n", ffs_block_name, ffs.size());
        } else {
            continue;
        }

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

                print_fit_header(fs_block.header);

                for (size_t i = 0; i < 16; ++i) {
                    char *c = block_data.get_data().get() + offset + i;

                    fmt::print("{:02X} ", (unsigned char) *c);

                    if ((i + 1) % 4 == 0) {
                        fmt::print("\n");
                    }
                }

                fs_block.data = RawData(block_data, fs_block.header.offset, fs_block.header.size);

                if (ffs_map.count(fs_block.header.id)) {
                    throw Exception("Duplicate id {}", fs_block.header.id);
                }



                ffs_map[fs_block.header.id] = fs_block;

                // print_fit_header(fs_block.header);

                // fmt::print("Data:\n");

                // for (size_t i = 0; i < fs_block.data.get_size(); ++i) {
                //     fmt::print("{:02X} ", (unsigned char) *(fs_block.data.get_data().get() + i));

                //     if ((i + 1) % 32 == 0) {
                //         fmt::print("\n");
                //     }
                // }
                // fmt::print("\n");
                // fmt::print("=============================\n");
            }
        }

        for (const auto &block_pair : ffs_map) {
            const auto &fs_block = block_pair.second;

            // if (fs_block.data.get_size() > 30 && fs_block.data.get_size() < 100) {

            // } else {
            //     continue;
            // }

            print_fit_header(fs_block.header);
            fmt::print("Data:\n");

            for (size_t i = 0; i < fs_block.data.get_size(); ++i) {
                fmt::print("{:02X} ", (unsigned char) *(fs_block.data.get_data().get() + i));

                if ((i + 1) % 32 == 0) {
                    fmt::print("\n");
                }
            }
            fmt::print("\n");
            fmt::print("=============================\n");
        }

        // const FFSBlock &    root_block      = ffs_map.at(0x14);
        // FileHeader          root_header     = read_file_header(root_block.data);

        // print_fit_header(root_block.header);
        // print_file_header(root_header);

        // const FFSBlock &    root_block      = ffs_map.at(3686);
        const FFSBlock &    root_block      = ffs_map.at(10);
        FileHeader          root_header     = read_file_header(root_block.data);
        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_map, root, root_header);

        fs_map[ffs_block_name] = root;
    }
}

void NewSGOLD::read_recurse(FSBlocksMap &ffs_map, RawData &data, uint16_t next_id) {
    if (!ffs_map.count(next_id)) {
        throw Exception("Reading part. Couldn't find block with id: {}", next_id);
    }

    const FFSBlock &block      = ffs_map.at(next_id);
    FilePart        part       = read_file_part(block.data);
    uint16_t        data_id    = part.id + 1;

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading part data. Couldn't find block with id: {}", data_id);
    }

    FFSBlock & block_data = ffs_map.at(data_id);

    data.add(block_data.data);

    if (part.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, data, part.next_part);
    }
}

RawData NewSGOLD::read_full_data(FSBlocksMap &ffs_map, const FileHeader &header) {
    uint32_t data_id = header.id + 1;

    if (!ffs_map.count(data_id)) {
        throw Exception("Reading file data. Couldn't find block with id: {}", data_id);
    }

    const FFSBlock &block = ffs_map.at(data_id);

    RawData data_full(block.data);

    if (header.next_part != 0xFFFFFFFF) {
        read_recurse(ffs_map, data_full, header.next_part);
    }

    return data_full;
}

void NewSGOLD::scan(FSBlocksMap &ffs_map, Directory::Ptr dir, const FileHeader &header, std::string path) {
    RawData data    = read_full_data(ffs_map, header);
    size_t  offset  = 0;

    while (offset < data.get_size()) {
        fflush(stdout);

        uint32_t raw;

        data.read<uint32_t>(offset, reinterpret_cast<char *>(&raw), 1);
        offset += 4;

        uint16_t    id  = raw & 0xFFFF;
        uint16_t    unk = (raw >> 16) & 0xFFFF;

        if (id == 0xFFFF) {
            break;
        }
        
        if (id == 0) {
            continue;
        }

        const FFSBlock &    file_block      = ffs_map.at(id);
        FileHeader          file_header     = read_file_header(file_block.data);
        
        fmt::print("{:5d} {}\n", id, path + file_header.name);

        if (file_header.unknown6 & 0x10) {
            Directory::Ptr dir_next = Directory::build(file_header.name);

            dir->add_subdir(dir_next);

            scan(ffs_map, dir_next, file_header, path + file_header.name + "/");
        } else {
            try {
                RawData     file_data;
                uint32_t    data_id = file_header.id + 1;

                if (ffs_map.count(data_id)) {
                    file_data = read_full_data(ffs_map, file_header);
                }
            
                File::Ptr file = File::build(file_header.name, file_data);

                dir->add_file(file);
            } catch (const Exception &e) {
                fmt::print("Warning! Broken file: {}\n", e.what());
            }
        }
    }
}

void NewSGOLD::unpack(Directory::Ptr dir, std::string path) {
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

};
};
