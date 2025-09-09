#ifndef _EMSCRIPTENHELPER_HEADERFILE
#define _EMSCRIPTENHELPER_HEADERFILE

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>

namespace Viewer {

//! Class contains emscripten-related functions.
class EmscriptenHelper {
public:
    //! Convert std::vector to JavaScript array.
    template<typename T>
    static emscripten::val ToJSArray(const std::vector<T>& theVec)
    {
        std::vector<emscripten::val> valVec(theVec.begin(), theVec.end());
        return emscripten::val::array(valVec);
    }

    //! Convert JavaScript array to std::vector.
    template<typename T>
    static void FromJSArray(std::vector<T>& theVec, emscripten::val theVal)
    {
        if (theVal.isArray()) {
            theVec = emscripten::vecFromJSArray<T>(theVal);
        }
    }

    //! Convert std::map to JavaScript map.
    template<typename K, typename T>
    static emscripten::val ToJSMap(const std::map<K, T>& theMap)
    {
        emscripten::val aRes = emscripten::val::global("Map").new_();
        for (const std::pair<K, T>& aPair : theMap) {
            aRes.call<void>("set", aPair.first, aPair.second);
        }

        return aRes;
    }

    //! Convert JavaScript map to std::map.
    template<typename K, typename T>
    static void FromJSMap(std::map<K, T>& theMap, emscripten::val theVal)
    {
        theMap.clear();

        if (!theVal.instanceof (emscripten::val::global("Map"))) {
            return;
        }

        const size_t aSize = theVal["size"].as<size_t>();
        if (aSize == 0) {
            return;
        }

        emscripten::val anIter = theVal.call<emscripten::val>("entries");

        for (;;) {
            emscripten::val aValue = anIter.call<emscripten::val>("next");
            if (aValue["done"].as<bool>()) {
                return;
            }

            K aKey = aValue["value"][0].as<K>();
            T aVal = aValue["value"][1].as<T>();
            theMap.emplace(aKey, aVal);
        }
    }
};

}
#endif

#endif // _EMSCRIPTENHELPER_HEADERFILE