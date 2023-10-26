#ifndef BLOCKS_H
#define BLOCKS_H

#define FMT_HEADER_ONLY

#include <fmt/format.h>
#include <fmt/printf.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <map>

#include "rawdata.h"

namespace FULLFLASH {

class Blocks {
    public:
        typedef struct {
            char        name[8];
            uint16_t    unknown_1;
            uint16_t    unknown_2;
            uint32_t    unknown_3;
        } BlockHeader;

        typedef struct {
            BlockHeader             header;
            size_t                  offset;
            size_t                  count;
            RawData                 data;
        } Block;

        using Map   = std::map<std::string, std::vector<Block>>;

        Blocks();

        void                        load(std::string fullflash_path);
        void                        print();
        Map &                       get_blocks();
        RawData &                   get_data();

        static const uint32_t      block_size = 0x10000;

    private:
        void                        search_blocks();
        void                        search_blocks_x85();

        RawData                     data;

        // Data                data;
        // size_t              data_size;

        Map                         blocks_map;

};
};

#endif
