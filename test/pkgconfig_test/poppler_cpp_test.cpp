#include <iostream>
#include <poppler-document.h>
#include <memory>

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " file.pdf\n";
        return 1;
    }

    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(argv[1]));
    if (!doc) {
        std::cerr << "failed to open " << argv[1] << "\n";
        return 1;
    }

    int n_pages = doc->pages();
    std::cout << argv[1] << " has " << n_pages << " pages\n";

    return 0;
}
