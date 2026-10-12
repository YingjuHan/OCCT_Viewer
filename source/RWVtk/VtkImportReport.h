#ifndef RWVTK_IMPORTREPORT_HEADERFILE
#define RWVTK_IMPORTREPORT_HEADERFILE

#include <cstddef>
#include <string>
#include <vector>

namespace RWVtk {
//! One backend attempt; rejected attempts are retained separately from the final outcome.
struct VtkImportAttempt {
    std::string backend;
    bool succeeded = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct VtkImportReport {
    size_t points = 0;
    size_t cells = 0;
    size_t fields = 0;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    //! Populated by MeshLoader for the overall import, including successful fallback.
    std::string source;
    std::string format;
    std::string backend;
    bool succeeded = false;
    std::vector<VtkImportAttempt> attempts;

    bool IsComplete() const { return errors.empty() && warnings.empty(); }
};
}
#endif
