#ifndef RWVTK_IMPORTREPORT_HEADERFILE
#define RWVTK_IMPORTREPORT_HEADERFILE

#include <cstddef>
#include <string>
#include <vector>

namespace RWVtk {
struct VtkImportReport {
    size_t points = 0;
    size_t cells = 0;
    size_t fields = 0;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    bool IsComplete() const { return errors.empty() && warnings.empty(); }
};
}
#endif
