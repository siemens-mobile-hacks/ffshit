#ifndef FULLFLASH_FILESYSTEM_EXTRACT_H
#define FULLFLASH_FILESYSTEM_EXTRACT_H

#include <string>
#include <functional>

namespace FULLFLASH {
namespace Filesystem  {

void extract(const std::string &path, std::function<void(std::string)> extractor); 

};
};

#endif
