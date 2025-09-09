#ifndef _VALUECONVERTER_HEADERFILE
#define _VALUECONVERTER_HEADERFILE

#include <NCollection_DoubleMap.hxx>
#include <TCollection_AsciiString.hxx>

namespace RWBase
{
    //! 枚举值与字符串之间的双向映射器。
    //! 本模板不应直接实例化，需由具体枚举的转换器派生并填充映射，例如：
    //! @code
    //! class SomeConverter : public ValueConverter<SomeEnum, SomeConverter> {...}
    //! @endcode
    template <typename ValueType, typename ConverterType>
    class ValueConverter
    {
    public:
        //! 为 NCollection_Map 接口提供哈希函数。
        static int HashCode(const ValueType theType, const int theUpper)
        {
            unsigned int aHashCode = (unsigned int)::HashCode((int)theType, theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! 为 NCollection_Map 接口提供相等性比较。
        static bool IsEqual(const ValueType theType1, const ValueType theType2) { return theType1 == theType2; }

        typedef NCollection_DoubleMap<ValueType, TCollection_AsciiString, ValueConverter<ValueType, ConverterType>>
            MapOfValues;

        //! 根据枚举值返回对应的字符串描述。
        static const char *GetString(const ValueType theType) { return instance()->getString(theType); }

        //! 根据字符串反查枚举值。
        static ValueType GetValue(const char *theString) { return instance()->getValue(theString); }

    protected:
        //! 受保护的默认构造，阻止直接实例化基类。
        ValueConverter() {}

        //! 提供内部枚举-字符串映射表，以便派生类填充。
        MapOfValues &valueMap() { return myMap; }

    private:
        //! 返回单例化的派生转换器。
        static ConverterType *instance()
        {
            static ConverterType *anInstance = nullptr;
            if (anInstance == nullptr)
            {
                anInstance = new ConverterType();
            }
            return anInstance;
        }

        //! 具体执行枚举值到字符串的查找。
        const char *getString(const ValueType theType) const
        {
            const TCollection_AsciiString *aResult = myMap.Seek1(theType);
            return (aResult == nullptr ? "" : aResult->ToCString());
        }

        //! 具体执行字符串到枚举值的查找。
        ValueType getValue(const char *theString) const
        {
            const ValueType *aResult = myMap.Seek2(theString);
            return (aResult == nullptr ? ValueType::Unknown : *aResult);
        }

    private:
        MapOfValues myMap; //!< 双向的“枚举 <--> 字符串”映射存储。
    };

} // namespace

#endif // _VALUECONVERTER_HEADERFILE
