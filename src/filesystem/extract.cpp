#include "filesystem/extract.h"
#include "filesystem/ex.h"

#include <iostream>

#include <spdlog/spdlog.h>

#include "system.h"

namespace FULLFLASH {
namespace Filesystem  {

void extract(const std::string &path, std::function<void(std::string)> extractor) {
    spdlog::info("Extracting filesystem");

    if (System::is_file_exists(path)) {
        std::string yes_no;

        while (yes_no != "n" && yes_no != "y") {
            spdlog::warn("'{}' is regular file. Delete? (y/n)", path);

            yes_no.clear();
            std::cin >> yes_no;

            System::to_lower(yes_no);
        }

        if (yes_no == "y") {
            bool r = System::remove_directory(path);

            if (!r) {
                throw Exception("Couldn't delete directory '{}'", path);
            }
        } else {
            return;
        }

    }

    if (System::is_directory_exists(path)) {
        std::string yes_no;

        while (yes_no != "n" && yes_no != "y") {
            spdlog::warn("Directory '{}' already exists. Delete? (y/n)", path);

            yes_no.clear();
            std::cin >> yes_no;
            std::transform(yes_no.begin(), yes_no.end(), yes_no.begin(), [](unsigned char c){ 
                return std::tolower(c); 
            });
        }

        if (yes_no == "y") {
            bool r = System::remove_directory(path);

            if (!r) {
                throw Exception("Couldn't delete directory '{}'", path);
            }
        } else {
            return;
        }
    }

    bool r = System::create_directory(path, 
                        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                        std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                        std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

    if (!r) {
        throw Exception("Couldn't create directory '{}'", path);
    }

    extractor(path);
};


};
};