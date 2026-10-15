#ifndef RWCAE_MESHCELLTOPOLOGY_HEADERFILE
#define RWCAE_MESHCELLTOPOLOGY_HEADERFILE

#include <cstddef>
#include <cstdint>
#include <vector>

namespace RWCae {
//! VTK file-format cell IDs. This table has no dependency on the VTK SDK.
struct MeshCellTopology {
    static int Dimension(int type)
    {
        switch (type) {
        case 0: case 1: case 2: return 0;
        case 3: case 4: case 21: case 35: case 68: case 75: return 1;
        case 5: case 6: case 7: case 8: case 9: case 22: case 23: case 28:
        case 30: case 34: case 36: case 69: case 70: case 76: case 77: return 2;
        case 10: case 11: case 12: case 13: case 14: case 15: case 16:
        case 24: case 25: case 26: case 27: case 29: case 31: case 32:
        case 33: case 37: case 41: case 42: case 71: case 72: case 73:
        case 78: case 79: case 80: return 3;
        default: return -1;
        }
    }
    static int FixedSize(int type)
    {
        switch (type) {
        case 0: return 0; case 1: return 1; case 3: return 2; case 5: return 3;
        case 8: case 9: case 10: return 4; case 11: case 12: return 8;
        case 13: return 6; case 14: return 5; case 15: return 10; case 16: return 12;
        case 21: return 3; case 22: return 6; case 23: return 8; case 24: return 10;
        case 25: return 20; case 26: return 15; case 27: return 13; case 28: return 9;
        case 29: return 27; case 30: return 6; case 31: return 12; case 32: return 18;
        case 33: return 24; case 34: return 7; case 35: return 4; case 37: return 19;
        default: return -1;
        }
    }
    static bool ValidSize(int type, size_t count, const std::vector<int>& degrees = {})
    {
        if (Dimension(type) < 0) { return false; }
        const int fixed = FixedSize(type);
        if (fixed >= 0) { return count == (size_t)fixed; }
        if (type < 68) {
            return type == 2 ? count >= 1 : type == 4 ? count >= 2
                : type == 6 || type == 7 ? count >= 3 : type == 36 ? count >= 6 && count%2 == 0 : count >= 4;
        }
        const int family = type >= 75 ? type-75 : type-68;
        auto size = [family](int p, int q, int r) -> uint64_t {
            switch (family) {
            case 0: return p+1;
            case 1: return (uint64_t)(p+1)*(p+2)/2;
            case 2: return (uint64_t)(p+1)*(q+1);
            case 3: return (uint64_t)(p+1)*(p+2)*(p+3)/6;
            case 4: return (uint64_t)(p+1)*(q+1)*(r+1);
            case 5: return (uint64_t)(p+1)*(p+2)*(r+1)/2;
            default: return 0;
            }
        };
        auto match = [&](int p, int q, int r) {
            if (p < 1 || p > 1024 || q < 0 || q > 1024 || r < 0 || r > 1024
                || ((family == 2 || family == 4 || family == 5) && q < 1)
                || ((family == 4 || family == 5) && r < 1) || (family == 5 && p != q)) { return false; }
            return size(p,q,r) == count || (type < 75 && p == 2 && ((family == 1 && count == 7)
                || (family == 3 && count == 15))) || (type < 75 && family == 5 && p == 2 && r == 2 && count == 21);
        };
        if (!degrees.empty()) { return degrees.size() == 3 && match(degrees[0],degrees[1],degrees[2]); }
        for (int p = 1; p <= 1024; ++p) {
            if (match(p,p,p)) { return true; }
            if (size(p,p,p) > count) { break; }
        }
        return false;
    }
};
}
#endif
