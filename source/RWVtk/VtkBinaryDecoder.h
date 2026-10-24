// Internal bounded decoder. Appended offsets address raw bytes or encoded characters.
#ifndef RWVTK_BINARYDECODER_HEADERFILE
#define RWVTK_BINARYDECODER_HEADERFILE
#include <RWVtk/VtkResourceLimits.h>
#include <zlib.h>
#include <string_view>
#include <vector>
#include <cstring>
#include <cctype>
#include <limits>

namespace RWVtk { namespace detail {
using Bytes = std::vector<unsigned char>;
class BinaryCursor {
public:
    BinaryCursor(std::string_view input, bool raw) : myInput(input), myRaw(raw) {}
    uint64_t Remaining() const { return myRaw ? myInput.size()-myAt : (myInput.size()-myAt)/4*3 + mySize-myNext; }
    unsigned char Get()
    {
        if (myRaw) {
            if (myAt==myInput.size()) { throw std::runtime_error("Truncated VTK binary data"); }
            return (unsigned char)myInput[myAt++];
        }
        if (myNext==mySize) {
            char group[4]; int count=0;
            while (count<4 && myAt<myInput.size()) {
                const unsigned char c=(unsigned char)myInput[myAt++];
                if (!std::isspace(c)) { group[count++]=(char)c; }
            }
            if (count!=4) { throw std::runtime_error("Truncated base64 quartet"); }
            const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            unsigned d[4]={};
            for (int n=0;n<4;++n) {
                const char* digit=std::strchr(alphabet,group[n]);
                if (group[n]!='=' && (!group[n] || !digit)) { throw std::runtime_error("Invalid base64 digit"); }
                if (group[n]!='=') { d[n]=(unsigned)(digit-alphabet); }
            }
            if (group[0]=='=' || group[1]=='=' || (group[2]=='=' && group[3]!='=')
                || (group[2]=='=' && (d[1]&15)) || (group[3]=='=' && group[2]!='=' && (d[2]&3))) { throw std::runtime_error("Invalid base64 padding"); }
            myNext=0; mySize=1; myBytes[0]=(unsigned char)((d[0]<<2)|(d[1]>>4));
            if(group[2]!='=') {myBytes[mySize++]=(unsigned char)((d[1]<<4)|(d[2]>>2));}
            if(group[3]!='=') {myBytes[mySize++]=(unsigned char)((d[2]<<6)|d[3]);}
        }
        return myBytes[myNext++];
    }
    void Read(unsigned char* output, size_t size)
    {
        if (size>Remaining()) { throw std::runtime_error("Truncated VTK array payload"); }
        if(myRaw) {if(size) {std::memcpy(output,myInput.data()+myAt,size);} myAt+=size;return;}
        for(size_t i=0;i<size;++i) {output[i]=Get();}
    }
    uint64_t Word(size_t size,bool big)
    {
        uint64_t value=0;
        for(size_t i=0;i<size;++i) {value|=(uint64_t)Get()<<(8*(big?size-1-i:i));}
        return value;
    }
    bool End() const
    {
        if(myNext!=mySize) {return false;}
        for(size_t i=myAt;i<myInput.size();++i) {if(!std::isspace((unsigned char)myInput[i])) {return false;}}
        return true;
    }
private:
    std::string_view myInput;
    size_t myAt=0;
    bool myRaw;
    unsigned char myBytes[3]={};
    int myNext=0,mySize=0;
};
inline Bytes Decode(BinaryCursor& input,size_t word,bool big,bool compressed,VtkResourceBudget& budget)
{
    if(!compressed) {
        const uint64_t size=input.Word(word,big);
        if(size>input.Remaining()) {throw std::runtime_error("Truncated VTK binary array");}
        budget.Array(size); Bytes output((size_t)size); input.Read(output.data(),output.size()); return output;
    }
    const uint64_t blocks=input.Word(word,big), blockSize=input.Word(word,big), last=input.Word(word,big);
    if(blocks>input.Remaining()/word || blocks>budget.Limits.MaxArrayBytes/sizeof(uint64_t)
        || (blocks && (!blockSize || last>blockSize))) {throw std::runtime_error("Invalid VTK compression header");}
    uint64_t size=0;
    if(blocks) {
        if(blocks-1>budget.Limits.MaxArrayBytes/blockSize) {throw std::runtime_error("VTK resource limit: decompressed array bytes");}
        size=(blocks-1)*blockSize;
        VtkResourceBudget::Consume(size,last?last:blockSize,budget.Limits.MaxArrayBytes,"decompressed array bytes");
    }
    budget.Array(size); // Before the packed-header vector and decompressed buffer allocations.
    std::vector<uint64_t> lengths; lengths.reserve((size_t)blocks);
    uint64_t packedTotal=0;
    for(uint64_t i=0;i<blocks;++i) {
        const uint64_t packed=input.Word(word,big);
        VtkResourceBudget::Consume(packedTotal,packed,budget.Limits.MaxArrayBytes,"packed array bytes");
        lengths.push_back(packed);
    }
    if(packedTotal>input.Remaining()) {throw std::runtime_error("Truncated VTK compressed payload");}
    Bytes output((size_t)size); size_t offset=0;
    for(uint64_t i=0;i<blocks;++i) {
        const uint64_t unpacked=i+1==blocks && last?last:blockSize;
        if(unpacked>std::numeric_limits<uLongf>::max() || lengths[i]>std::numeric_limits<uLong>::max()) {throw std::runtime_error("VTK zlib block exceeds native range");}
        Bytes packed((size_t)lengths[i]); input.Read(packed.data(),packed.size());
        uLongf actual=(uLongf)unpacked;
        if(uncompress(output.data()+offset,&actual,packed.data(),(uLong)packed.size())!=Z_OK || actual!=unpacked) {throw std::runtime_error("Cannot decompress VTK zlib block");}
        offset+=(size_t)unpacked;
    }
    return output;
}
} }
#endif
