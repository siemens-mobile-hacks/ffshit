#include "filesystem/newsgold_x85.h"
#include "filesystem/ex.h"
#include "filesystem/extract.h"

#include <iconv.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FULLFLASH {
namespace Filesystem {

NewSGOLD_X85::NewSGOLD_X85(Blocks &blocks) : blocks(blocks) {
    parse_FIT();
}

void NewSGOLD_X85::load() {

}

static bool is_directory_exists(std::string path) {
    struct stat sb;

    if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) { 
        return true;
    }

    return false;
}

void NewSGOLD_X85::extract(std::string path) {
    FULLFLASH::Filesystem::extract(path, [&](std::string dst_path) {
        for (const auto &fs : fs_map) {
            std::string fs_name = fs.first;
            auto root           = fs.second;

            fmt::print("Extracting {}\n", fs_name);

            unpack(root, dst_path + "/" + fs_name);
        }
    });

    // fmt::print("Extracting filesystem\n");

    // if (is_directory_exists(path)) {
    //     fmt::print("Directory {} already exsits\n", path);
    //     fmt::print("Deleting...\n");

    //     int r = rmdir(path.c_str());

    //     if (r == -1) {
    //         throw Exception("Couldn't delete directory '{}': {}", path, strerror(errno));
    //     }
    // }

    // int r = mkdir(path.c_str(), 0755);

    // if (r == -1) {
    //     throw Exception("Couldn't create directory '{}': {}", path, strerror(errno));
    // }

    // for (const auto &fs : fs_map) {
    //     std::string fs_name = fs.first;
    //     auto root           = fs.second;

    //     fmt::print("Extracting {}\n", fs_name);

    //     unpack(root, path + "/" + fs_name);
    // }
}

void NewSGOLD_X85::print_fit_header(const NewSGOLD_X85::FITHeader &header) {
    fmt::print("    FIT:\n");
    fmt::print("      Flags:      {:08X}\n",      header.flags);
    fmt::print("      ID:         {:08X} {:d}\n", header.id, header.id);
    fmt::print("      Size:       {:08X} {:d}\n", header.size, header.size);
    fmt::print("      Offset:     {:08X}\n",      header.offset);
    fmt::print("    ===========================\n");
}

void NewSGOLD_X85::print_file_header(const NewSGOLD_X85::FileHeader &header) {
    fmt::print("File:\n");
    fmt::print("  ID:           {:04X} {}\n",     header.id, header.id);
    fmt::print("  Parent ID:    {:04X} {}\n",     header.parent_id, header.parent_id);
    fmt::print("  Next part ID: {:04X} {}\n",     header.next_part, header.next_part);
    fmt::print("  Size:         {:08X} {}\n",     header.unknown2, header.unknown2);
    fmt::print("  Unknown4:     {:04X} {}\n",     header.unknown4, header.unknown4);
    fmt::print("  Unknown5:     {:04X} {}\n",     header.unknown5, header.unknown5);
    fmt::print("  Unknown6:     {:04X} {}\n",     header.unknown6, header.unknown6);
    fmt::print("  Unknown7:     {:04X} {}\n",     header.unknown7, header.unknown7);
    fmt::print("  Name:         {}\n",            header.name);
    fmt::print("===========================\n");
}

void NewSGOLD_X85::print_file_part(const FilePart &part) {
    fmt::print("Part:\n");
    fmt::print("  ID:             {:04X} {}\n", part.id, part.id);
    fmt::print("  Parent ID:      {:04X} {}\n", part.parent_id, part.parent_id);
    fmt::print("  Next part ID:   {:04X} {}\n", part.next_part, part.next_part);
    fmt::print("===========================\n");
}

RawData NewSGOLD_X85::read_aligned(const FFSBlock &block) {
    RawData         data_ret;

    const auto &    data    = block.data;
    const auto &    fit     = block.header;

    char *          data_ptr    = data.get_data().get() + data.get_size();
    size_t          to_read     = fit.size;

    while(to_read != 0) {
        size_t read_size    = 16;
        size_t skip         = 0;

        if (to_read > 16) {
            // read_size = 16;
        } else {
            // read_size   = to_read;
            skip        = 16 - to_read;
        }

        data_ptr    -= 32;
        data_ret.add_top(data_ptr + skip, read_size - skip);
        to_read     -= read_size - skip;
    }

    return data_ret;
}

NewSGOLD_X85::FileHeader NewSGOLD_X85::read_file_header(const FFSBlock &block) {
    RawData                     header_data = read_aligned(block);
    NewSGOLD_X85::FileHeader    header;
    size_t                      offset = 0;

    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.id), 1);
    offset += 4;
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.next_part), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.parent_id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&header.unknown2), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown4), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown5), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown6), 1);
    header_data.read<uint16_t>(offset, reinterpret_cast<char *>(&header.unknown7), 1);

    // ======================================================================

    offset = 28;
    size_t  str_size    = header_data.get_size() - 28;
    size_t  from_n      = str_size;
    char *  from        = new char[str_size];

    size_t  to_n        = str_size;
    char *  to          = new char[str_size];

    header_data.read<char>(offset, from, from_n);

    iconv_t iccd = iconv_open("UTF-8", "UTF-16LE");

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

NewSGOLD_X85::FilePart NewSGOLD_X85::read_file_part(const FFSBlock &block) {
   RawData         header_data;

    const auto &    data    = block.data;
    const auto &    fit     = block.header;
    
    char *          data_ptr    = data.get_data().get() + data.get_size();
    size_t          to_read     = fit.size;

    while(to_read != 0) {
        size_t read_size    = 16;
        size_t skip         = 0;

        if (to_read > 16) {
            // read_size = 16;
        } else {
            // read_size   = to_read;
            skip        = 16 - to_read;
        }

        data_ptr    -= 32;
        header_data.add_top(data_ptr + skip, read_size - skip);
        to_read     -= read_size - skip;
    }

    // ======================================================================

    FilePart    part;
    size_t      offset = 0;

    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.parent_id), 1);
    header_data.read<uint32_t>(offset, reinterpret_cast<char *>(&part.next_part), 1);

    return part;
}

void NewSGOLD_X85::dump_data(const RawData &raw_data) {
    bool is_aligned = false;

    fmt::print("    ");

    for (size_t i = 0; i < raw_data.get_size(); ++i) {
        is_aligned = false;

        fmt::print("{:02X} ", (unsigned char) *(raw_data.get_data().get() + i));

        if ((i + 1) % 32 == 0) {
            fmt::print("\n    ");

            is_aligned = true;
        }
    }

    if (!is_aligned) {
        fmt::print("\n");
    }
}

void NewSGOLD_X85::dump_block(const NewSGOLD_X85::FFSBlock &block) {
    print_fit_header(block.header);

    fmt::print("Data:\n");

    dump_data(block.data);
}

void NewSGOLD_X85::parse_FIT() {
    Blocks::Map &bl = blocks.get_blocks();

    for (const auto &block : blocks.get_blocks()) {
        const auto &ffs_block_name  = block.first;
        const auto &ffs             = block.second;

        FSBlocksMap ffs_map;
        FSBlocksMap ffs_map_dub;

        if (ffs_block_name.find("FFS") != std::string::npos) {
            fmt::print("{} Blocks: {}\n", ffs_block_name, ffs.size());
        } else {
            continue;
        }

        for (auto &block : ffs) {
             fmt::print("  Block {:08X} {:08X} {:08X} {:08x} Size: {}\n",
                block.offset,
                block.header.unknown_1,
                block.header.unknown_2,
                block.header.unknown_3,
                block.data.get_size());

            const RawData & block_data = block.data;
            size_t          block_size = block_data.get_size();
            size_t          fit_size = 0;

            std::vector<uint32_t> id_list;

            // for (ssize_t offset = block_size - 64; offset > 0; offset -= 32) {
            // for (ssize_t offset = block_size - 64; offset > 0;) {

            //     FFSBlock    fs_block;
            //     size_t      offset_header = offset;

            //     uint32_t unk1;
            //     uint32_t unk2;
            //     uint32_t unk3;
            //     uint32_t unk4;

            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&unk1), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&unk2), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&unk3), 1);
            //     block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&unk4), 1);

            //     if (fs_block.header.flags != 0xFFFFFFC0) {
            //         offset -= 32;

            //         continue;
            //     }

            //     fmt::print("    ==== Offset: {:08X} ====\n", block.offset + offset);
            //     print_fit_header(fs_block.header);

            //     size_t size_data    = ((fs_block.header.size / 16) + 1) * 32;
            //     size_t offset_data1  = offset - size_data;

            //     auto data1 = RawData(block_data, offset_data1, size_data);

            //     dump_data(data1);

            //     offset = offset_data1;

            //     // offset = offset_data;
            // }

            for (ssize_t offset = block_size - 64; offset > 0; offset -= 32) {
                FFSBlock    fs_block;    
                size_t      offset_header = offset;

                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                fmt::print("    =================================\n");
                fmt::print("    ID:     {:08X}\n", fs_block.header.id);
                fmt::print("    Size:   {:08X} {}\n", fs_block.header.size, fs_block.header.size);
                fmt::print("    Offset: {:08X}\n", fs_block.header.offset);
                fmt::print("    Flags:  {:08X}\n", fs_block.header.flags);

                // Конец fit табитцы. Корявое определение. Не очень понятно
                // как определять конец, или где находится её размер
                uint32_t end_marker;
                
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&end_marker), 1);

                if (end_marker != 0xFFFFFFFF) {
                    fit_size = (block.offset + block_size) - (block.offset + offset) - 32;

                    break;
                }

                // fmt::print("{:08X}\n", fs_block.header.flags);
                // fmt::print("  {:08X}\n", fs_block.header.id);

                if (fs_block.header.flags != 0xFFFFFFC0) {
                    continue;
                }

                fs_block.ff_boffset = block.offset;
                fs_block.ff_offset  = block.offset + offset;

                size_t size_data    = ((fs_block.header.size / 16) + 1) * 32;
                size_t offset_data  = offset - size_data;

                fs_block.data = RawData(block_data, offset_data, size_data);
                fs_block.block_ptr = &block;

                if (ffs_map.count(fs_block.header.id)) {
                    throw Exception("Duplicate id {}", fs_block.header.id);
                } else {
                    ffs_map[fs_block.header.id] = fs_block;
                }

                id_list.push_back(fs_block.header.id);
            }

            // for (const auto &id : id_list) {
            //     ffs_map[id].fit_size = fit_size;
            // }

            // fmt::print("FIT table size: {:08x}\n", fit_size);
        }

        for (const auto &block : ffs_map_dub) {
            dump_block(block.second);
        }

        // for (const auto &block_pair : ffs_map) {
        //     const auto &fs_block = block_pair.second;

        //     print_fit_header(fs_block.header);
        //     fmt::print("FF Offset: {:08X}\n", fs_block.ff_offset);
        //     fmt::print("B  Offset: {:08X}\n", fs_block.ff_boffset);
        //     fmt::print("Data:\n");

        //     for (size_t i = 0; i < fs_block.data.get_size(); ++i) {
        //         fmt::print("{:02X} ", (unsigned char) *(fs_block.data.get_data().get() + i));

        //         if ((i + 1) % 32 == 0) {
        //             fmt::print("\n");
        //         }
        //     }
        //     fmt::print("\n");
        //     fmt::print("=============================\n");
        // }
        // const FFSBlock &    block1      = ffs_map.at(20);
        // print_fit_header(block1.header);
        // FileHeader          header1     = read_file_header(block1);
        // print_file_header(header1);

        // const FFSBlock &    block2      = ffs_map.at(627);
        // print_fit_header(block2.header);
        // dump_block(block2);
        // RawData aligned = read_aligned(block2);
        // fmt::print("\n");
        // dump_data(aligned);

        // return;

        const FFSBlock &    root_block      = ffs_map.at(10);

        FileHeader          root_header     = read_file_header(root_block);
        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_map, root, root_block);

        fs_map[ffs_block_name] = root;

    }
}

std::vector<uint16_t> NewSGOLD_X85::get_directory(FSBlocksMap &ffs_map, const FFSBlock &block) {
    const FFSBlock &dir_block  = ffs_map.at(block.header.id + 1);

    const auto &    dir_data = dir_block.data;
    const size_t    dir_data_size = dir_block.data.get_size();

    RawData data;

    size_t offset = 0;

    while (offset < dir_data_size) {
        data.add(dir_data.get_data().get() + offset, 16);
        offset += 32;
    }

    // =======================================================
    std::vector<uint16_t> id_list;

    offset = 0;

    while (offset < data.get_size()) {
        uint16_t id;
        uint16_t unk;
        uint16_t unk2;
        uint16_t unk3;

        data.read<uint16_t>(offset, reinterpret_cast<char *>(&id), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&unk), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&unk2), 1);
        data.read<uint16_t>(offset, reinterpret_cast<char *>(&unk3), 1);

        fmt::print("{:04X} {:04X} {:04X} {:04X}\n", id, unk, unk2, unk3);

        if (id == 0xFFFF) {
            continue;;
        }
        
        if (id == 0) {
            continue;
        }

        if (unk3 != 0xFFFF) {
            continue;
        }

        id_list.push_back(id);

        // fmt::print("ID: {:04X}, Unk: {:04X} Unk: {:04X} Unk: {:04X}\n", id, unk, unk2, unk3);
    }

    return id_list;
}

uint32_t NewSGOLD_X85::read_part(const FileHeader &file_header, const FSBlocksMap &ffs_map, uint32_t next_part, uint32_t last_offset, RawData &data, size_t start_offset) {
    const FFSBlock &part_block = ffs_map.at(next_part);
    FilePart        part = read_file_part(part_block);

    print_file_part(part);

    const FFSBlock &part_data = ffs_map.at(next_part + 1);

    fmt::print("ID: {} Full size: {}, data size: {}, maybe size: {}\n", next_part, file_header.unknown2, data.get_size(), data.get_size() + part_data.header.size);
    fmt::print("ID: {} Last offset: {:08X} part_data.header.offset: {:08X}\n", next_part, last_offset, part_data.header.offset);

    if (last_offset == part_data.header.offset) {
        fmt::print("Add end. Last offset: {:08X} part_data.header.offset: {:08X}\n", last_offset, part_data.header.offset);
        auto end = read_aligned(part_data);

        data.add(end);

        dump_data(end);

        return next_part;
    }

    // size_t data_addr = part_block.ff_boffset + part_data.header.offset + ((part_block.header.offset + part_block.fit_size) & 0x40000);

    size_t data_addr = start_offset + part_data.header.offset;

    if (part_block.header.offset != part_data.header.offset) {
        data_addr += part_block.header.offset + part_block.fit_size + 0x800;
    }

    RawData file_data(this->blocks.get_data(), data_addr, part_data.header.size);
    data.add(file_data);

    fmt::print("ID: {}, FF Block offset: {:08X} Part block offset {:08X} Size: {:08X} Part data offset {:08X} Size: {:08X} - Data addr: {:08X}\n", 
        next_part, part_block.ff_boffset, 
        part_block.header.offset, part_block.header.size, 
        part_data.header.offset, part_data.header.size, 
        data_addr);

    // dump_data(file_data);

    if (part.next_part != 0xFFFFFFFF) {
        return read_part(file_header, ffs_map, part.next_part, last_offset, data, start_offset);
    }

    return next_part;
}

void NewSGOLD_X85::read_file(const FSBlocksMap &ffs_map, const FFSBlock &file_block, RawData &data) {
    const FITHeader &   fit_header  = file_block.header;
    FileHeader          file_header = read_file_header(file_block);;

    if (!ffs_map.count(file_header.id + 1)) {
        throw Exception("Block {} not found", file_header.id + 1);
    }

    dump_block(file_block);

    const FFSBlock &file_data_block = ffs_map.at(file_header.id + 1);

    fmt::print("  {:08X} Start: {:04X}, {:08X}\n", file_data_block.ff_boffset, file_data_block.header.offset, file_data_block.ff_boffset + file_data_block.header.offset);
    // fmt::print("{:08X} {:04X} {:04X} Data offset: {:04X}\n", file_data.ff_boffset, file_data.header.offset, file_data.header.offset, file_data.ff_boffset + file_data.header.offset - file_data.header.offset);
    RawData file_data(this->blocks.get_data(), file_data_block.ff_boffset + file_data_block.header.offset, file_data_block.header.size);

    data.add(file_data);

    if (file_header.next_part != 0xFFFFFFFF) {
        uint32_t last_part = read_part(file_header, ffs_map, file_header.next_part, fit_header.offset, data, file_data_block.ff_boffset);
    }

    if (file_header.next_part != 0xFFFFFFFF) {
        // const FFSBlock &    block2      = ffs_map.at(last_part + 1);
        // dump_block(block2);
        // RawData aligned = read_aligned(block2);
        // fmt::print("\n");
        // dump_data(aligned);
    }

}

void NewSGOLD_X85::scan(FSBlocksMap &ffs_map, Directory::Ptr dir, const FFSBlock &block, std::string path) {
    auto id_list = get_directory(ffs_map, block);

    for (const auto &id : id_list) {
        const FFSBlock &    file_block  = ffs_map.at(id);
        const FITHeader &   fit_header  = file_block.header;
        FileHeader          file_header = read_file_header(file_block);;

        fmt::print("{:5d} {}\n", id, path + file_header.name);
        fmt::print("Block: {:08X}\n", file_block.ff_boffset);
        fmt::print("FF Offset: {:08X}\n", file_block.ff_offset);

        print_fit_header(file_block.header);
        print_file_header(file_header);
        
        if (file_header.unknown6 & 0x10) {
            Directory::Ptr dir_next = Directory::build(file_header.name);

            dir->add_subdir(dir_next);

            scan(ffs_map, dir_next, file_block, path + file_header.name + "/");
        } else {
            try {
                RawData                 file_data;
                size_t                  size    = file_header.unknown2;

                if (file_header.unknown2 != 0) {
                    read_file(ffs_map, file_block, file_data);
                }                

                File::Ptr file = File::build(file_header.name, file_data);

                dir->add_file(file);
            } catch (const Exception &e) {
                fmt::print("Warning! Broken file: {}\n", e.what());

                exit(0);
            }
        }

    }
}

void NewSGOLD_X85::unpack(Directory::Ptr dir, std::string path) {
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
