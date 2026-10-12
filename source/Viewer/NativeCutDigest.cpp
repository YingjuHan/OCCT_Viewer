#include <Viewer/NativeCutDigest.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <limits>
namespace {
uint32_t rotate(uint32_t x,int n) { return (x>>n)|(x<<(32-n)); }
constexpr uint32_t k[]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
}
namespace Viewer {
void NativeCutDigest::block(const unsigned char* data)
{
    uint32_t w[64]; for(int i=0;i<16;++i) { w[i]=((uint32_t)data[4*i]<<24)|((uint32_t)data[4*i+1]<<16)|((uint32_t)data[4*i+2]<<8)|data[4*i+3]; }
    for(int i=16;i<64;++i) { const auto a=w[i-15],b=w[i-2]; w[i]=w[i-16]+(rotate(a,7)^rotate(a,18)^(a>>3))+w[i-7]+(rotate(b,17)^rotate(b,19)^(b>>10)); }
    auto a=myState[0],b=myState[1],c=myState[2],d=myState[3],e=myState[4],f=myState[5],g=myState[6],h=myState[7];
    for(int i=0;i<64;++i) { const auto t=h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+w[i],s=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c)); h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+s; }
    const uint32_t values[]={a,b,c,d,e,f,g,h}; for(int i=0;i<8;++i) { myState[i]+=values[i]; }
}
void NativeCutDigest::Update(const void* bytes,size_t size)
{
    if(myFinished||size>std::numeric_limits<uint64_t>::max()/8-myBytes) { throw std::length_error("Invalid SHA-256 stream length/state"); }
    myBytes+=size; const auto* data=static_cast<const unsigned char*>(bytes);
    while(size) { const size_t n=std::min(size,64-myUsed); std::memcpy(myBuffer.data()+myUsed,data,n); data+=n; size-=n; myUsed+=n; if(myUsed==64) { block(myBuffer.data()); myUsed=0; } }
}
std::string NativeCutDigest::Finish()
{
    if(myFinished) { throw std::logic_error("SHA-256 already finalized"); }
    const uint64_t bits=myBytes*8; myBuffer[myUsed++]=0x80;
    if(myUsed>56) { std::fill(myBuffer.begin()+myUsed,myBuffer.end(),0); block(myBuffer.data()); myUsed=0; }
    std::fill(myBuffer.begin()+myUsed,myBuffer.begin()+56,0); for(int i=0;i<8;++i) { myBuffer[63-i]=(unsigned char)(bits>>(8*i)); } block(myBuffer.data()); myFinished=true;
    std::ostringstream out; out<<std::hex<<std::setfill('0'); for(auto v:myState) { out<<std::setw(8)<<v; } return out.str();
}
std::string NativeCutDigest::Text(const std::string& text) { NativeCutDigest digest; digest.Update(text.data(),text.size()); return digest.Finish(); }
std::string NativeCutDigest::File(const std::string& path,uint64_t maximumBytes)
{
    std::ifstream in(std::filesystem::u8path(path),std::ios::binary); if(!in) { throw std::runtime_error("Cannot open session source for digest: "+path); }
    NativeCutDigest digest; std::array<char,65536> bytes; uint64_t read=0;
    while(in) { in.read(bytes.data(),bytes.size()); const auto n=in.gcount(); if((uint64_t)n>maximumBytes-read) { throw std::length_error("Session source digest byte limit exceeded"); } read+=(uint64_t)n; digest.Update(bytes.data(),(size_t)n); }
    if(!in.eof()) { throw std::runtime_error("Session source digest read failed"); } return digest.Finish();
}
}
