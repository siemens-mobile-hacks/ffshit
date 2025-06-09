#include "blocks.h"
#include "ex.h"
#include "filesystem/ex.h"
#include "filesystem/sgold.h"
#include "filesystem/newsgold.h"
#include "filesystem/newsgold_x85.h"

#include <spdlog/spdlog.h>
#include "thirdparty/cxxopts.hpp"

int main(int argc, char *argv[]) {
    spdlog::set_pattern("\033[30;1m[%H:%M:%S.%e]\033[0;39m %^[%=8l]%$ \033[1;37m%v\033[0;39m");

    cxxopts::Options options(argv[0], "Siemens filesystem extractor");

    std::string ff_path;
    std::string dst_path_override;

    try {
        options.add_options()
            ("d,debug", "Enable debugging")
            ("p,path", "Destination path. Data_<IMEI> by default", cxxopts::value<std::string>())
            ("ffpath", "fullflash path", cxxopts::value<std::string>())
            ("h,help", "Help");

        options.parse_positional({"ffpath"});

        auto parsed = options.parse(argc, argv);

        if (parsed.count("h")) {
            fmt::print("{}", options.help());

            return EXIT_SUCCESS;
        }

        if (!parsed.count("ffpath")) {
            spdlog::error("Please specify fullflash path");

            return EXIT_FAILURE;
        }

        if (parsed.count("d")) {
            spdlog::set_level(spdlog::level::debug);
        }

        ff_path = parsed["ffpath"].as<std::string>();

        if (parsed.count("p")) {
            dst_path_override = parsed["p"].as<std::string>();
        }

    } catch (const cxxopts::exceptions::exception &e) {
        spdlog::error("{}", e.what());
    }
    
    try {
        FULLFLASH::Blocks   blocks(ff_path);
        // FULLFLASH::Blocks   blocks(argv[1], FULLFLASH::Platform::X85);

        FULLFLASH::Platform platform    = blocks.get_platform();
        std::string         imei        = blocks.get_imei();

        spdlog::info("Detected platform: {}", FULLFLASH::PlatformString.at(platform));
        spdlog::info("IMEI: {}", imei);

        std::string         data_path = fmt::format("./Data_{}", imei);

        if (dst_path_override.length() != 0) {
            spdlog::warn("Destination path override '{}' -> '{}'", data_path, dst_path_override);

            data_path = dst_path_override;
        }

        blocks.print();

        switch(platform) {
            case FULLFLASH::Platform::X65: {
                auto fs_sgold = FULLFLASH::Filesystem::SGOLD::build(blocks);

                fs_sgold->load();
                fs_sgold->extract(data_path);

                break;
            }
            case FULLFLASH::Platform::X75: {
                auto fs_newsgold = FULLFLASH::Filesystem::NewSGOLD::build(blocks);

                fs_newsgold->load();
                fs_newsgold->extract(data_path);

                break;
            }
            case FULLFLASH::Platform::X85: {
                auto fs_newsgold_x85 = FULLFLASH::Filesystem::NewSGOLD_X85::build(blocks);

                fs_newsgold_x85->load(); 
                fs_newsgold_x85->extract(data_path);

                break;
            }
            case FULLFLASH::Platform::UNK: {
                throw FULLFLASH::Exception("Unknown platform");
            }
        }

        // auto fs_sgold = FULLFLASH::Filesystem::SGOLD::build(blocks);

        // fs_sgold->load();
        // fs_sgold->extract("./Data");

        // auto fs_newsgold = FULLFLASH::Filesystem::NewSGOLD::build(blocks);
        // fs_newsgold->load();
        // fs_newsgold->extract("./Data");

        // auto fs_newsgold_x85 = FULLFLASH::Filesystem::NewSGOLD_X85::build(blocks);
        // fs_newsgold_x85->load(); 
        // fs_newsgold_x85->extract("./Data");

    } catch (const FULLFLASH::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        spdlog::error("{}", e.what());
    }

    return 0;
}
