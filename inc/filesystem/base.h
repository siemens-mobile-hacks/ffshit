#ifndef FULLFLASH_FILESYSTEM_BASE_H
#define FULLFLASH_FILESYSTEM_BASE_H

#include <memory>

namespace FULLFLASH {
namespace Filesystem  {

class Base {
    public:
        using Ptr = std::shared_ptr<Base>;

        Base() { }

        virtual void load() = 0;
        virtual void extract(std::string path) = 0;

    private:
};

};
};

#endif
