#ifndef RWVTK_BOUNDEDTEXTBUFFER_HEADERFILE
#define RWVTK_BOUNDEDTEXTBUFFER_HEADERFILE
#include <streambuf>
#include <string>
#include <stdexcept>
#include <cstdint>
namespace RWVtk { namespace detail {
//! Bound Legacy's generated XML before any output string growth.
class BoundedTextBuffer : public std::streambuf {
public:
    explicit BoundedTextBuffer(uint64_t limit) : myLimit(limit) {}
    const std::string& Text() const { return myText; }
protected:
    std::streamsize xsputn(const char* data,std::streamsize count) override
    {
        if(count<0 || myText.size()>myLimit || (uint64_t)count>myLimit-myText.size()) {throw std::runtime_error("VTK resource limit: normalized XML bytes");}
        myText.append(data,(size_t)count);return count;
    }
    int_type overflow(int_type value) override
    {
        if(traits_type::eq_int_type(value,traits_type::eof())) {return traits_type::not_eof(value);}
        const char c=traits_type::to_char_type(value);xsputn(&c,1);return value;
    }
private:
    uint64_t myLimit;
    std::string myText;
};
} }
#endif
