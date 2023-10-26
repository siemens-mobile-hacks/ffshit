#include "blocks.h"
#include "ex.h"

#include "filesystem/ex.h"
#include "filesystem/sgold.h"
#include "filesystem/newsgold.h"
#include "filesystem/newsgold_x85.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fmt::print("Please specify fullflash path\n");

        return EXIT_FAILURE;
    }

    try {
        FULLFLASH::Blocks blocks;

        blocks.load(argv[1]);
        blocks.print();

        auto fs_sgold = FULLFLASH::Filesystem::SGOLD::build(blocks);

        fs_sgold->load();
        fs_sgold->extract("./Data");

        // auto fs_newsgold = FULLFLASH::Filesystem::NewSGOLD::build(blocks);
        // fs_newsgold->load();
        // fs_newsgold->extract("./Data");

        // auto fs_newsgold_x85 = FULLFLASH::Filesystem::NewSGOLD_X85::build(blocks);
        // fs_newsgold_x85->load(); 

    } catch (const FULLFLASH::Exception &e) {
        fmt::print("error: {}\n", e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        fmt::print("error: {}\n", e.what());
    }

    return 0;
}
