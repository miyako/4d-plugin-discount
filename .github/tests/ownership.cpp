#include "4DPlugin-Discount.h"
#include "4DPlugin-JSON.h"
#include "ARRAY_TEXT.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>

#define CHECK(condition, message) do { if (!(condition)) { \
    std::fprintf(stderr, "%s\n", message); std::abort(); } } while (0)

using Text = std::basic_string<PA_Unichar>;
struct Reference {
    int owners = 1;
    std::map<Text, PA_Variable> properties;
};
static std::map<PA_Unichar*, size_t> strings;
static std::map<PA_Handle, size_t> handles;
static std::map<void*, Reference*> references;
static std::map<PA_Picture, int> pictures;
static bool nullEmpty = false;
static bool failGet = false;
static int failedOutputClears = 0;
static int propertyGets = 0;

static Text text(const PA_Unistring& value) {
    PA_Unichar* pointer = value.fString;
    if (!pointer) {
        CHECK(value.fLength == 0, "null string has nonzero length");
        return {};
    }
    CHECK(strings.count(pointer), "borrowed string no longer alive");
    CHECK(strings.at(pointer) == size_t(value.fLength), "wrong UTF16 length");
    CHECK(pointer[value.fLength] == 0, "missing UTF16 terminator");
    return Text(pointer, value.fLength);
}

static PA_Unistring makeString(const PA_Unichar* input) {
    size_t size = 0;
    while (input && input[size]) ++size;
    PA_Unistring value = {};
    if (size || !nullEmpty) {
        value.fString = new PA_Unichar[size + 1];
        if (size) std::memcpy(value.fString, input, size * sizeof(PA_Unichar));
        value.fString[size] = 0;
        value.fLength = size;
        PA_Unichar* pointer = value.fString;
        strings[pointer] = size;
    }
    return value;
}

static void disposeString(PA_Unistring& value) {
    if (value.fString) {
        PA_Unichar* pointer = value.fString;
        CHECK(strings.erase(pointer) == 1, "double disposal or failed getter output read");
        delete[] value.fString;
    }
    value = {};
}

static void clear(PA_Variable& value);

static PA_Variable copy(PA_Variable value) {
    if (value.fType == eVK_Unistring) {
        Text content = text(value.uValue.fString);
        value.uValue.fString = makeString(content.c_str());
    } else if (value.fType == eVK_Object || value.fType == eVK_Collection) {
        void* ref = value.fType == eVK_Object
            ? value.uValue.fObject : value.uValue.fCollection;
        if (ref) {
            CHECK(references.count(ref), "dead reference copied");
            ++references.at(ref)->owners;
        }
    } else if (value.fType == eVK_Picture && value.uValue.fPicture) {
        PA_Picture picture = value.uValue.fPicture;
        CHECK(pictures.count(picture), "dead picture copied");
        ++pictures.at(picture);
    }
    return value;
}

static void release(void* ref) {
    if (!ref) return;
    CHECK(references.count(ref), "dead object/collection released");
    Reference* owned = references.at(ref);
    if (--owned->owners == 0) {
        for (auto& item : owned->properties) clear(item.second);
        references.erase(ref);
        delete owned;
    }
}

static void clear(PA_Variable& value) {
    switch (value.fType) {
    case eVK_Unistring:
        if (value.uValue.fString.fString == reinterpret_cast<PA_Unichar*>(1))
            ++failedOutputClears;
        disposeString(value.uValue.fString);
        break;
    case eVK_Object: release(value.uValue.fObject); break;
    case eVK_Collection: release(value.uValue.fCollection); break;
    case eVK_Picture:
        if (value.uValue.fPicture) {
            PA_Picture picture = value.uValue.fPicture;
            CHECK(pictures.count(picture), "dead picture released");
            if (--pictures.at(picture) == 0) {
                pictures.erase(picture);
                delete static_cast<int*>(value.uValue.fPicture);
            }
        }
        break;
    case eVK_ArrayUnicode: {
        PA_Handle h = value.uValue.fArray.fData;
        CHECK(handles.count(h), "dead array cleared");
        auto* elements = reinterpret_cast<PA_Unistring*>(*h);
        CHECK(handles.at(h) == (value.uValue.fArray.fNbElements + 1) * sizeof(*elements),
              "array capacity mismatch");
        for (int i = 0; i <= value.uValue.fArray.fNbElements; ++i)
            disposeString(elements[i]);
        PA_DisposeHandle(h);
        break;
    }
    default: break;
    }
    value = {};
    value.fType = eVK_Undefined;
}

static void* newReference() {
    Reference* ref = new Reference;
    references[ref] = ref;
    return ref;
}

static void engine(short selector, EngineBlock* eb) {
    switch (selector) {
    case EX_CREATE_UNISTRING:
        eb->fUniString1 = makeString(reinterpret_cast<PA_Unichar*>(eb->fHandle));
        break;
    case EX_DISPOSE_UNISTRING: disposeString(eb->fUniString1); break;
    case EX_SET_UNISTRING: {
        PA_Unistring replacement = makeString(reinterpret_cast<PA_Unichar*>(eb->fHandle));
        disposeString(eb->fUniString1);
        eb->fUniString1 = replacement;
        break;
    }
    case EX_CLEAR_VARIABLE: clear(*static_cast<PA_Variable*>(eb->fPtr1)); break;
    case EX_VARIABLE_TO_STRING: {
        auto* variable = reinterpret_cast<PA_Variable*>(eb->fHandle);
        CHECK(variable->fType == eVK_Unistring, "only string identity conversion supported");
        eb->fUniString1 = variable->uValue.fString;
        break;
    }
    case EX_CREATE_PICTURE: {
        CHECK(eb->fParam1 == 0, "only synthetic empty pictures supported");
        PA_Picture p = new int(0);
        pictures[p] = 1;
        eb->fPicture = p;
        break;
    }
    case EX_HANDLE_MANAGER: {
        PA_Handle h = eb->fHandle;
        switch (eb->fParam1) {
        case 1:
            h = new char*;
            *h = new char[eb->fParam2]();
            handles[h] = eb->fParam2;
            eb->fHandle = h;
            break;
        case 2:
            CHECK(handles.erase(h) == 1, "handle double free");
            delete[] *h;
            delete h;
            break;
        case 3: {
            CHECK(handles.count(h), "dead handle resized");
            auto size = size_t(eb->fParam2);
            char* bytes = new char[size]();
            std::memcpy(bytes, *h, std::min(size, handles.at(h)));
            delete[] *h;
            *h = bytes;
            handles[h] = size;
            break;
        }
        case 4: eb->fParam2 = handles.at(h); break;
        case 5: eb->fParam3 = reinterpret_cast<sLONG_PTR>(*h); break;
        case 6: break;
        default: CHECK(false, "unexpected handle operation");
        }
        break;
    }
    case EX_EXECUTE_COMMAND_BY_ID: {
        auto** params = static_cast<PA_Variable**>(eb->fPtr1);
        auto* result = static_cast<PA_Variable*>(eb->fPtr2);
        CHECK(eb->fParam1 == 1231, "unexpected engine command");
        CHECK(eb->fParam2 == 2, "wrong command argument count");
        void* object = params[0]->uValue.fObject;
        CHECK(references.count(object), "dead command object");
        Text key = text(params[1]->uValue.fString);
        PA_SetBooleanVariable(result,
            references.at(object)->properties.count(key) != 0);
        break;
    }
    case EX_SET_OBJ_VALUE: {
        void* ptr1 = eb->fPtr1;
        CHECK(references.count(ptr1), "dead setter object");
        Text key = text(eb->fUniString1);
        auto& property = references.at(ptr1)->properties[key];
        clear(property);
        property = copy(*static_cast<PA_Variable*>(eb->fPtr2));
        break;
    }
    case EX_GET_OBJ_VALUE: {
        void* ptr1 = eb->fPtr1;
        ++propertyGets;
        auto* result = static_cast<PA_Variable*>(eb->fPtr2);
        if (failGet) {
            // A failed getter does not promise a valid output descriptor.
            result->fType = eVK_Unistring;
            result->uValue.fString = {};
            result->uValue.fString.fString = reinterpret_cast<PA_Unichar*>(1);
            eb->fError = eER_IncompatibleTypes;
        } else {
            Text key = text(eb->fUniString1);
            *result = copy(references.at(ptr1)->properties.at(key));
        }
        break;
    }
    default:
        std::fprintf(stderr, "unexpected engine selector: %d\n", selector);
        std::abort();
    }
}

static void balanced() {
    CHECK(strings.empty(), "owning string leak");
    CHECK(handles.empty(), "owning handle leak");
    CHECK(references.empty(), "owning object/collection reference leak");
    CHECK(pictures.empty(), "owning picture leak");
}

static const char* utf8[] = {"ASCII", "caf\xc3\xa9", "\xf0\x9f\x98\x80", ""};
static const wchar_t* wideText[] = {L"ASCII", L"caf\u00e9", L"\U0001f600", L""};
static const Text expected[] = {
    { 'A','S','C','I','I' }, { 'c','a','f',0xe9 }, { 0xd83d,0xde00 }, {}
};

static void setters() {
    void* obj = newReference();
    for (int i = 0; i < 4; ++i) {
        CUTF16String value = expected[i];
        for (int overload = 0; overload < 4; ++overload) {
            switch (overload) {
            case 0: ob_set_s(obj, "key", utf8[i]); break;
            case 1: ob_set_s(obj, L"key", utf8[i]); break;
            case 2: ob_set_a(obj, L"key", wideText[i]); break;
            case 3: ob_set_a(obj, L"key", &value); break;
            }
            CHECK(text(references.at(obj)->properties.at(Text{'k','e','y'}).uValue.fString)
                  == expected[i], "setter changed Unicode content");
            CHECK(strings.size() == ((!nullEmpty || i != 3) ? 1u : 0u),
                  "string setter leaked initial owner");
        }
    }
    release(obj);
    balanced();
}

static void pictureSetter() {
    void* obj = newReference();
    ob_set_p(obj, L"key", PA_CreatePicture(nullptr, 0));
    release(obj);
    balanced();
}

static void getters() {
    void* obj = newReference();
    CUTF8String unchanged(reinterpret_cast<const uint8_t*>("unchanged"));
    CHECK(!ob_get_a(obj, L"missing", &unchanged), "missing string key");
    CHECK(!ob_get_b(nullptr, L"k") && ob_get_n(nullptr, L"k") == 0 &&
          !ob_get_c(nullptr, L"k"), "null object defaults changed");
    for (int i = 0; i < 4; ++i) {
        PA_Variable source = {};
        source.fType = eVK_Unistring;
        source.uValue.fString = makeString(expected[i].c_str());
        auto& property = references.at(obj)->properties[Text{'k'}];
        clear(property);
        property = source;
        size_t stringOwners = strings.size();
        CUTF8String output;
        CHECK(ob_get_a(obj, L"k", &output), "existing key reported missing");
        CHECK(output == CUTF8String(reinterpret_cast<const uint8_t*>(utf8[i])),
              "getter changed Unicode content");
        CHECK(strings.size() == stringOwners, "string getter leaked owning result");
        CHECK(!ob_get_b(obj, L"k"), "boolean mismatch result");
        CHECK(strings.size() == stringOwners, "boolean mismatch leaked owning result");
        CHECK(ob_get_n(obj, L"k") == 0, "real mismatch result");
        CHECK(strings.size() == stringOwners, "real mismatch leaked owning result");
        CHECK(!ob_get_c(obj, L"k"), "collection mismatch result");
        CHECK(strings.size() == stringOwners, "collection mismatch leaked owning result");
        clear(property);
        CHECK(output == CUTF8String(reinterpret_cast<const uint8_t*>(utf8[i])),
              "copied output died with its source");
    }
    ob_set_b(obj, L"k", true);
    CHECK(ob_get_b(obj, L"k"), "boolean getter result");
    ob_set_n(obj, L"k", 3.25);
    CHECK(ob_get_n(obj, L"k") == 3.25, "real getter result");
    auto& property = references.at(obj)->properties[Text{'k'}];
    clear(property);
    void* nested = newReference();
    PA_SetObjectVariable(&property, nested);
    ob_get_a(obj, L"k", &unchanged);
    ob_get_b(obj, L"k");
    ob_get_n(obj, L"k");
    ob_get_c(obj, L"k");
    CHECK(references.at(nested)->owners == 1, "object mismatch owners leaked");
    clear(property);
    PA_Picture picture = PA_CreatePicture(nullptr, 0);
    PA_SetPictureVariable(&property, picture);
    ob_get_a(obj, L"k", &unchanged);
    ob_get_b(obj, L"k");
    ob_get_n(obj, L"k");
    ob_get_c(obj, L"k");
    CHECK(pictures.at(picture) == 1, "picture mismatch owners leaked");
    CHECK(unchanged == CUTF8String(reinterpret_cast<const uint8_t*>("unchanged")),
          "missing/mismatched string changed output");
    void* collection = newReference();
    PA_Variable collectionValue = {};
    PA_SetCollectionVariable(&collectionValue, collection);
    clear(property);
    property = collectionValue;
    PA_CollectionRef handoff = ob_get_c(obj, L"k");
    CHECK(handoff == collection, "collection identity changed");
    CUTF8String mismatch;
    ob_get_a(obj, L"k", &mismatch);
    ob_get_b(obj, L"k");
    ob_get_n(obj, L"k");
    release(obj);
    CHECK(references.count(handoff) && references.at(handoff)->owners == 1,
          "collection handoff missing or mismatch owners leaked");
    PA_DisposeCollection(handoff);
    balanced();
}

static void failures() {
    void* obj = newReference();
    ob_set_b(obj, L"k", true);
    failGet = true;
    for (int i = 0; i < 4; ++i) {
        bool reported = false;
        try {
            CUTF8String value;
            switch (i) {
            case 0: ob_get_a(obj, L"k", &value); break;
            case 1: ob_get_b(obj, L"k"); break;
            case 2: ob_get_n(obj, L"k"); break;
            case 3: ob_get_c(obj, L"k"); break;
            }
        } catch (const std::runtime_error&) { reported = true; }
        CHECK(reported, "failed property getter was not reported");
    }
    CHECK(propertyGets == 4 && failedOutputClears == 0, "failed output was inspected/cleared");
    release(obj);
    balanced();
}

static void arrays() {
    PA_Variable array = PA_CreateVariable(eVK_ArrayUnicode);
    PA_ResizeArray(&array, 4);
    for (int i = 0; i < 4; ++i) {
        PA_Unistring source = makeString(expected[i].c_str());
        PA_SetStringInArray(array, i, &source);
    }
    PA_Unistring borrowed = PA_GetStringInArray(array, 0);
    Text ownedCopy = text(borrowed);
    PA_Unistring replacement = makeString(expected[1].c_str());
    PA_SetStringInArray(array, 0, &replacement);
    CHECK(ownedCopy == expected[0], "borrowed copy changed on replacement");
    for (int i = 1; i < 4; ++i)
        CHECK(text(PA_GetStringInArray(array, i)) == expected[i], "array content mismatch");
    PA_Unistring independent = makeString(expected[1].c_str());
    PA_SetStringInArray(array, 4, &independent);
    CHECK(PA_GetStringInArray(array, 0).fString != PA_GetStringInArray(array, 4).fString,
          "two transferred outputs share ownership");
    for (int invalid = 0; invalid < 4; ++invalid) {
        PA_Variable destination = array;
        int index = 0;
        if (invalid == 0) destination.fType = eVK_Longint;
        if (invalid == 1) destination.uValue.fArray.fData = nullptr;
        if (invalid == 2) index = -1;
        if (invalid == 3) index = 5;
        PA_Unistring rejected = makeString(expected[0].c_str());
        PA_SetStringInArray(destination, index, &rejected);
        CHECK(text(rejected) == expected[0], "rejected transfer consumed input");
        PA_DisposeUnistring(&rejected);
    }
    PA_SetStringInArray(array, 0, nullptr);
    PA_ResizeArray(&array, 1);
    PA_ClearVariable(&array);
    balanced();
}

static void arrayWrapper() {
    PA_Variable array = PA_CreateVariable(eVK_Undefined);
    BytePtr args[] = {reinterpret_cast<BytePtr>(&array)};
    {
        ARRAY_TEXT output;
        for (const auto& fixture : expected)
            output.appendUTF16String(fixture.c_str(), fixture.size());
        output.toParamAtIndex(args, 1);
        ARRAY_TEXT copied;
        copied.fromParamAtIndex(args, 1);
        for (int i = 0; i < 4; ++i)
            CHECK(text(PA_GetStringInArray(array, i)) == expected[i], "wrapper array content");
        output.toParamAtIndex(args, 1);
        output.setSize(2);
        output.toParamAtIndex(args, 1);
        PA_ClearVariable(&array);
        for (int i = 0; i < 4; ++i) {
            Text actual;
            copied.copyUTF16StringAtIndex(&actual, i);
            CHECK(actual == expected[i], "wrapper failed to copy borrowed input");
        }
        PA_SetLongintVariable(&array, 1);
        output.toParamAtIndex(args, 1);
        CHECK(PA_GetLongintVariable(array) == 1, "wrapper modified rejected type");
    }
    balanced();
}

static void live() {
    for (int i = 0; i < 4; ++i) {
        PA_Unistring input = makeString(expected[i].c_str());
        PA_Unistring output = makeString(expected[0].c_str());
        PA_long32 flags = 0, options = 0, result = -99;
        void* args[] = {&input, &output, &flags, &options};
        PluginBlock params = {args, &result, nullptr};
        PluginMain(1, &params);
        CHECK(result == 0, "Markdown dispatch failed");
        Text html{'\n'};
        if (i != 3) {
            html = Text{'<','p','>'} + expected[i] + Text{'<','/','p','>','\n'};
        }
        Text actual = text(output);
        if (actual != html) {
            std::fprintf(stderr, "Markdown fixture %d output:", i);
            for (auto unit : actual) std::fprintf(stderr, " %04x", unit);
            std::fprintf(stderr, "\n");
        }
        CHECK(actual == html, "Markdown changed exact Unicode output");
        CHECK(text(input) == expected[i], "Markdown mutated borrowed input");
        PA_DisposeUnistring(&input);
        PA_DisposeUnistring(&output);
        balanced();
    }
}

int main(int argc, char** argv) {
    CHECK(argc == 3, "expected case and empty representation");
    gCall4D = engine;
    nullEmpty = std::strcmp(argv[2], "null") == 0;
    if (!std::strcmp(argv[1], "setters")) setters();
    else if (!std::strcmp(argv[1], "picture")) pictureSetter();
    else if (!std::strcmp(argv[1], "getters")) getters();
    else if (!std::strcmp(argv[1], "failures")) failures();
    else if (!std::strcmp(argv[1], "arrays")) arrays();
    else if (!std::strcmp(argv[1], "array-wrapper")) arrayWrapper();
    else if (!std::strcmp(argv[1], "live")) live();
    else CHECK(false, "unknown test case");
    std::puts("PASS: exact content and balanced ownership");
}
