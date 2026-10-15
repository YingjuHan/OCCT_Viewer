#ifndef JSONHELPER_HEADERFILE
#define JSONHELPER_HEADERFILE

#ifndef HAVE_RAPIDJSON
    #define HAVE_RAPIDJSON
#endif

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <sstream>

namespace RWBase {
//! 针对 rapidjson 提供的常用辅助工具集合。
class JsonHelper {
public:
    typedef rapidjson::Writer<rapidjson::OStreamWrapper> Writer;
    typedef rapidjson::Document::ValueType               Value;

    //! 在对象中安全查找指定字段，不存在时返回空指针。
    static const Value* FindMember(const Value& theObject, const char* theName)
    {
        if (!theObject.IsObject()) {
            return nullptr;
        }

        rapidjson::Document::ConstMemberIterator anIter = theObject.FindMember(theName);
        return anIter != theObject.MemberEnd() ? &anIter->value : nullptr;
    }

    //! 查找布尔字段，确保类型匹配。
    static const Value* FindBoolMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsBool() ? aRes : nullptr;
    }

    //! 查找整数字段，确保类型匹配。
    static const Value* FindIntMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsInt() ? aRes : nullptr;
    }

    //! 查找双精度字段，确保类型匹配。
    static const Value* FindDoubleMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsDouble() ? aRes : nullptr;
    }

    //! 查找字符串字段，确保类型匹配。
    static const Value* FindStringMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsString() ? aRes : nullptr;
    }

    //! 查找子对象字段，确保类型匹配。
    static const Value* FindObjectMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsObject() ? aRes : nullptr;
    }

    //! 查找数组字段，确保类型匹配。
    static const Value* FindArrayMember(const Value& theObject, const char* theName)
    {
        const Value* aRes = FindMember(theObject, theName);
        return aRes != nullptr && aRes->IsArray() ? aRes : nullptr;
    }

    //! 将字符串解析为 rapidjson::Document。
    static bool ParseFromString(const std::string& theJsonStr, rapidjson::Document& theReader)
    {
        std::istringstream aStr(theJsonStr);

        rapidjson::IStreamWrapper aStrStream(aStr);
        rapidjson::ParseResult    aParseRes =
            theReader.ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                                  rapidjson::UTF8<>,
                                  rapidjson::IStreamWrapper>(aStrStream);
        return !aParseRes.IsError();
    }
};

}

#endif // JSONHELPER_HEADERFILE
