#include "filesystem/newsgold_x85.h"
#include "filesystem/ex.h"

#include <iconv.h>
#include <sys/stat.h>

namespace FULLFLASH {
namespace Filesystem {

NewSGOLD_X85::NewSGOLD_X85(Blocks &blocks) : blocks(blocks) {
    parse_FIT();
}

void NewSGOLD_X85::load() {

}

void NewSGOLD_X85::extract(std::string path) {

}

void NewSGOLD_X85::print_fit_header(const NewSGOLD_X85::FITHeader &header) {
    fmt::print("FIT:\n");
    fmt::print("Flags:      {:08X}\n",  header.flags);
    fmt::print("ID:         {:d}\n",    header.id);
    fmt::print("Size:       {:d}\n",    header.size);
    fmt::print("Offset:     {:04X}\n",  header.offset);
    fmt::print("===========================\n");
}

void NewSGOLD_X85::print_file_header(const NewSGOLD_X85::FileHeader &header) {
    fmt::print("File:\n");
    fmt::print("ID:           {}\n",      header.id);
    fmt::print("Parent ID:    {}\n",      header.parent_id);
    fmt::print("Next part ID: {}\n",      header.next_part);
    fmt::print("Unknown2:     {:08X} {}\n",      header.unknown2, header.unknown2);
    fmt::print("Unknown4:     {:04X} {}\n",      header.unknown4, header.unknown4);
    fmt::print("Unknown5:     {:04X} {}\n",      header.unknown5, header.unknown5);
    fmt::print("Unknown6:     {:04X} {}\n",      header.unknown6, header.unknown6);
    fmt::print("Unknown7:     {:04X} {}\n",      header.unknown7, header.unknown7);
    fmt::print("Name:         {}\n",      header.name);
    fmt::print("===========================\n");
}

void NewSGOLD_X85::print_file_part(const FilePart &part) {
    fmt::print("Part:\n");
    fmt::print("ID:             {}\n", part.id);
    fmt::print("Parent ID:      {}\n", part.parent_id);
    fmt::print("Next part ID:   {}\n", part.next_part);
    fmt::print("===========================\n");
}

NewSGOLD_X85::FileHeader NewSGOLD_X85::read_file_header(const FFSBlock &block) {
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

void NewSGOLD_X85::dump_block(const NewSGOLD_X85::FFSBlock &block) {
    print_fit_header(block.header);

    fmt::print("Data:\n");

    for (size_t i = 0; i < block.data.get_size(); ++i) {
        fmt::print("{:02X} ", (unsigned char) *(block.data.get_data().get() + i));

        if ((i + 1) % 32 == 0) {
            fmt::print("\n");
        }
    }

    fmt::print("\n");
}

void NewSGOLD_X85::parse_FIT() {
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

            for (ssize_t offset = block_size - 64; offset > 0; offset -= 32) {
                FFSBlock    fs_block;
                size_t      offset_header = offset;


                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.flags), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.id), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.size), 1);
                block_data.read<uint32_t>(offset_header, reinterpret_cast<char *>(&fs_block.header.offset), 1);

                if (fs_block.header.flags != 0xFFFFFFC0) {
                    continue;
                }

                fs_block.ff_boffset = block.offset;
                fs_block.ff_offset  = block.offset + offset;


                // print_fit_header(fs_block.header);

                // fmt::print("{:08X}\n", block.offset + offset);
                // print_fit_header(fs_block.header);
                size_t size_data    = ((fs_block.header.size / 16) + 1) * 32;
                size_t offset_data  = offset - size_data;

                fs_block.data = RawData(block_data, offset_data, size_data);
                fs_block.block_ptr = &block;

                // for (size_t i = 0; i < 32; ++i) {
                //     char *c = block_data.get_data().get() + offset + i;

                //     fmt::print("{:02X} ", (unsigned char) *c);

                //     if ((i + 1) % 4 == 0) {
                //         fmt::print("\n");
                //     }
                // }

                // fs_block.data = RawData(block_data, fs_block.header.offset, fs_block.header.size);

                if (ffs_map.count(fs_block.header.id)) {
                    throw Exception("Duplicate id {}", fs_block.header.id);
                }

                ffs_map[fs_block.header.id] = fs_block;
            }
        }

        // for (const auto &block_pair : ffs_map) {
        //     const auto &fs_block = block_pair.second;

        //     print_fit_header(fs_block.header);
        //     fmt::print("FF Offset: {:08X}\n", fs_block.ff_offset);
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

        const FFSBlock &    root_block      = ffs_map.at(10);

        FileHeader          root_header     = read_file_header(root_block);
        Directory::Ptr      root            = Directory::build(root_header.name);

        scan(ffs_map, root, root_block);
        
        return;
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

void NewSGOLD_X85::read_part(const FSBlocksMap &ffs_map, std::vector<uint32_t> &offsets, uint32_t next_part) {
    const FFSBlock &part_block = ffs_map.at(next_part);
    FilePart        part = read_file_part(part_block);

    // offsets.push_back(part_block.header.offset);

    const FFSBlock &part_data = ffs_map.at(next_part + 1);

    offsets.push_back(part_data.header.offset);

    // fmt::print("{:08X}\n", part_block.ff_boffset);
    fmt::print("{:08X} {:04X} {:04X}\n", part_block.ff_boffset, part_block.header.offset, part_data.header.offset);
    // fmt::print("{:08X} {:04X} {:04X} Data offset: {:04X}\n", part_block.ff_boffset, part_block.header.offset, part_data.header.offset, part_block.ff_boffset + part_block.header.offset - part_data.header.offset);
    // print_fit_header(part_block.header);
    // print_file_part(part);

    if (part.next_part != 0xFFFFFFFF) {
        read_part(ffs_map, offsets, part.next_part);
    }
}

void NewSGOLD_X85::read_parts(const FSBlocksMap &ffs_map, const NewSGOLD_X85::FileHeader &header, std::vector<uint32_t> &offsets) {
    const FFSBlock &file_data = ffs_map.at(header.id + 1);

    offsets.push_back(file_data.header.offset);

    fmt::print("{:08X} {:04X} {:04X}\n", file_data.ff_boffset, file_data.header.offset, file_data.header.offset);
    // fmt::print("{:08X} {:04X} {:04X} Data offset: {:04X}\n", file_data.ff_boffset, file_data.header.offset, file_data.header.offset, file_data.ff_boffset + file_data.header.offset - file_data.header.offset);

    if (header.next_part != 0xFFFFFFFF) {
        read_part(ffs_map, offsets, header.next_part);
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
                std::vector<uint32_t>   offsets;

                read_parts(ffs_map, file_header, offsets);

                fmt::print("Offsets:\n");
                for (const auto &offs : offsets) {
                    fmt::print("  {:04X}\n", offs);
                }

                // size_t      offset  = fit_header.offset - (fit_header.offset - file_header.unknown2);

                // fmt::print("Data offset: {:04X}\n", offset);
                // uint32_t    data_id = file_header.id + 1;

                // if (ffs_map.count(data_id)) {
                //     file_data = read_full_data(ffs_map, file_header);
                // }
            
                // File::Ptr file = File::build(file_header.name, file_data);

                // dir->add_file(file);
            } catch (const Exception &e) {
                fmt::print("Warning! Broken file: {}\n", e.what());
            }
        }

    }
}

};
};
