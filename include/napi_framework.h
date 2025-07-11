#ifndef OHOS_NAPI_FRAMEWORK_H
#define OHOS_NAPI_FRAMEWORK_H

#include <cstdint>
#include <napi/native_api.h>
#include <shared_mutex>
#include <string>

#if defined(CAPABLE_WITH_AKI)
#include <aki/jsbind.h>
#endif

#define NAPI_CHECK_STATUS(env, status, message)                                                                        \
    if ((status) != napi_ok)                                                                                           \
    throw OHOS::napi::Exception(env, message)

namespace OHOS {
namespace napi {

// native层的C++异常，不是JS的异常
class Exception : public std::runtime_error {
public:
    explicit Exception(napi_env env, const std::string &message) : env_(env), std::runtime_error(message) {
        const napi_extended_error_info *errorInfo;
        napi_status stat = napi_get_last_error_info(env_, &errorInfo);
        if (stat == napi_ok) {
            message_ += errorInfo->error_message;
        } else {
            message_ += "unknown";
        }
        message_ += ": ";
        message_ += std::runtime_error::what();
    }

    const char *what() const noexcept override { return message_.c_str(); }

private:
    const napi_env env_;
    std::string message_;
};

class Value;

// NAPI环境包装类
// 禁止跨线程使用
class Env {
public:
    explicit Env(napi_env env) : env_(env) {}

    napi_env env() const { return env_; }
    operator napi_env() const { return env_; }

    // 获取global对象
    Value global() const;
    // 获取undefined对象
    Value undefined() const;
    // 获取null对象
    Value null() const;

private:
    // napi_env 禁止缓存，因此设置此类为仅栈上创建使用
    void *operator new(std::size_t size);
    void operator delete(void *p);

    const napi_env env_;
};

// forward declarations
class Boolean;
class Number;
class BigInt;
class String;
class Object;
class Function;
class Array;

class Value {
public:
    Value(napi_env env) : env_(env), value_(nullptr) {}
    Value(napi_env env, napi_value value) : env_(env), value_(value) {}

    napi_env env() const { return env_; }
    napi_value value() const { return value_; }
    operator napi_value() const { return value_; }

    bool strictEquals(const Value &other) const;

    bool isEmpty() const { return value_ == nullptr; };
    napi_valuetype type() const;
    //    // 判断是否是指定的JS值类型
    //    template <JSValueType T> bool is() const;
    bool isUndefined() const { return type() == napi_undefined; }
    bool isNull() const { return type() == napi_null; }
    bool isBoolean() const { return type() == napi_boolean; }
    bool isNumber() const { return type() == napi_number; }
    bool isString() const { return type() == napi_string; }
    bool isSymbol() const { return type() == napi_symbol; }
    bool isObject() const { return type() == napi_object || isFunction(); }
    bool isFunction() const { return type() == napi_function; }
    bool isExternal() const { return type() == napi_external; }
    bool isBigInt() const { return type() == napi_bigint; }
    bool isArray() const;

    /// Creates a JS value from a C++ primitive.
    ///
    /// `value` may be any of:
    /// - bool
    /// - Any integer type
    /// - Any floating point type
    /// - const char* (encoded using UTF-8, null-terminated)
    /// - const char16_t* (encoded using UTF-16-LE, null-terminated)
    /// - std::string (encoded using UTF-8)
    /// - std::u16string
    /// - napi::Value
    /// - napi_value
    template <typename T> static Value From(napi_env env, const T &value);
    template <typename NapiValue>
    typename std::enable_if<std::is_base_of<Value, NapiValue>::value, NapiValue>::type as() const;

    Boolean toBoolean() const; ///< Coerces a value to a JavaScript boolean.
    Number toNumber() const;   ///< Coerces a value to a JavaScript number.
    String toString() const;   ///< Coerces a value to a JavaScript string.
    Object toObject() const;   ///< Coerces a value to a JavaScript object.

protected:
    const napi_env env_;
    const napi_value value_;
};
using Any = Value; // Value相当于js中的any类型

class Boolean : public Value {
public:
    static Boolean Create(napi_env env, bool value);

    Boolean(napi_env env) : Value(env) {}
    Boolean(napi_env env, napi_value value) : Value(env, value) {}

    operator bool() const;
    bool asBool() const;
};

class Number : public Value {
public:
    static Number Create(napi_env env, double value);
    static Number Create(napi_env env, float value);
    static Number Create(napi_env env, std::uint32_t value);
    static Number Create(napi_env env, std::int32_t value);
    static Number Create(napi_env env, std::int64_t value);

    Number(napi_env env) : Value(env) {}
    Number(napi_env env, napi_value value) : Value(env, value) {}

    operator std::uint32_t() const; ///< Converts a Number value to a 32-bit unsigned integer value.
    std::uint32_t asUint32() const;
    operator std::int32_t() const; ///< Converts a Number value to a 32-bit signed integer value.
    std::int32_t asInt32() const;
    operator std::int64_t() const; ///< Converts a Number value to a 64-bit signed integer value.
    std::int64_t asInt64() const;
    operator float() const; ///< Converts a Number value to a 32-bit floating-point value.
    float asFloat() const;
    operator double() const; ///< Converts a Number value to a 64-bit floating-point value.
    double asDouble() const;
};

class BigInt : public Value {
public:
    static BigInt Create(napi_env env, std::int64_t value);
    static BigInt Create(napi_env env, std::uint64_t value);
    static BigInt Create(napi_env env, int signBit, std::size_t wordCount, const std::uint64_t *words);

    BigInt(napi_env env) : Value(env) {}
    BigInt(napi_env env, napi_value value) : Value(env, value) {}

    std::int64_t asInt64(bool *lossless) const;
    std::uint64_t asUint64(bool *lossless) const;

    std::size_t wordCount() const;
    /// Writes the contents of this BigInt to a specified memory location.
    /// `sign_bit` must be provided and will be set to 1 if this BigInt is
    /// negative.
    /// `*word_count` has to be initialized to the length of the `words` array.
    /// Upon return, it will be set to the actual number of words that would
    /// be needed to store this BigInt (i.e. the return value of `WordCount()`).
    void toWords(int *signBit, std::size_t *wordCount, std::uint64_t *words) const;
};

class Named : public Value {
public:
    Named(napi_env env) : Value(env) {}
    Named(napi_env env, napi_value value) : Value(env, value) {}
};

class String : public Named {
public:
    static String Create(napi_env env, const char *str, std::size_t length);
    static String Create(napi_env env, const std::string &str);

    template <typename T> static String From(napi_env env, const T &value);

    String(napi_env env) : Named(env) {}
    String(napi_env env, napi_value value) : Named(env, value) {}

    //    operator const char *() const;
    //    const char *asCString() const;
    operator std::string() const; ///< Converts a String value to a UTF-8 encoded C++ string.
    std::string asString() const; ///< Converts a String value to a UTF-8 encoded C++ string.
private:
    std::string result_;
};

class Object : public Value {
public:
    /*
     * Enables property and element assignments using indexing syntax.
     * 实现类似 C++ 原生对象的“左值引用”语法，让你可以用 object["foo"] = value;
     * 这种方式既能读也能写属性，而且赋值表达式还能链式调用。
     * 这里不将operator[]写到Value类中是因为不想污染了Value的其他子类。
     * Example:
     *   Value propertyValue = object1['A']; 调用PropertyLValue::operator Value()发生隐式类型转换
     *   object2['A'] = propertyValue; 调用PropertyLValue::operator[](Value)进而调用Object::set来调用到napi_set_property
     *   Value elementValue = array[0u];
     *   array[1] = elementValue;
     */
    template <typename Key> class PropertyLValue {
        friend class Object;

    public:
        operator Value() const { return Object(env_, object_).get(key_); }
        /// Assigns a value to the property. The type of value can be anything supported by `Object::set`.
        template <typename ValueType> PropertyLValue &operator=(ValueType value) {
            Object(env_, object_).set(key_, value);
            return *this;
        }
        Value asValue() const { return Value(*this); }

    private:
        PropertyLValue(Object object, Key key) : env_(object.env()), object_(object), key_(key) {}

        const napi_env env_;
        napi_value object_;
        Key key_;
    };

    static Object Create(napi_env env);

    Object(napi_env env) : Value(env) {}
    Object(napi_env env, napi_value value) : Value(env, value) {}

    /// Checks whether a property is present.
    bool has(napi_value key) const;
    /// Checks whether a named property is present.
    bool has(const char *utf8name) const;
    /// Checks whether a named property is present.
    bool has(const std::string &utf8name) const;
    bool has(std::uint32_t index) const;
    /// Checks whether a own property is present.
    bool hasOwnProperty(napi_value key) const;
    /// Checks whether a own property is present.
    bool hasOwnProperty(const char *utf8name) const;
    /// Checks whether a own property is present.
    bool hasOwnProperty(const std::string &utf8name) const;

    /// Gets or sets a named property.
    PropertyLValue<std::string> operator[](const char *utf8name);
    /// Gets or sets a named property.
    PropertyLValue<std::string> operator[](const std::string &utf8name);
    /// Gets or sets an indexed property or array element.
    PropertyLValue<std::uint32_t> operator[](std::uint32_t index);
    /// Gets or sets an indexed property or array element.
    PropertyLValue<Value> operator[](Value index) const;

    //    /// Gets a property.
    //    Value operator[](napi_value key) const; // NOTE
    //    这个工作应该由PropertyLValue类来完成，这样才能做到“左值引用”的效果。
    /// Gets a property.
    Value get(napi_value key) const;
    /// Gets a named property.
    Value operator[](const char *utf8name) const;
    /// Gets a named property.
    Value operator[](const std::string &utf8name) const;
    /// Gets a named property.
    Value get(const char *utf8name) const;
    /// Gets a named property.
    Value get(const std::string &utf8name) const;
    /// Gets an indexed property or array element.
    Value operator[](std::uint32_t index) const;
    /// Gets an indexed property or array element.
    Value get(std::uint32_t index) const;

    /// Sets a property.
    template <typename ValueType> bool set(napi_value key, const ValueType &value) const;
    /// Sets a named property.
    template <typename ValueType> bool set(const char *utf8name, const ValueType &value) const;
    /// Sets a named property.
    template <typename ValueType> bool set(const std::string &utf8name, const ValueType &value) const;
    /// Sets an indexed property or array element.
    template <typename ValueType> bool set(std::uint32_t index, const ValueType &value) const;

    /// Delete property.
    bool del(napi_value key) const;
    /// Delete property.
    bool del(const char *utf8name) const;
    /// Delete property.
    bool del(const std::string &utf8name) const;
    /// Deletes an indexed property or array element.
    bool del(std::uint32_t index) const;

    /// Get all property names
    Array getPropertyNames() const;

    /// Checks if an object is an instance created by a constructor function.
    bool instanceof (const Function &constructor) const;

    class const_iterator;
    const_iterator begin() const;
    const_iterator end() const;
    class iterator;
    iterator begin();
    iterator end();

    bool freeze() const;
    bool seal() const;
};

class Function : public Object {
public:
    static Function Create(napi_env env, const char *utf8name, napi_callback callback, void *data = nullptr);
    static Function Create(napi_env env, const std::string &utf8name, napi_callback callback, void *data = nullptr);

    Function(napi_env env) : Object(env) {}
    Function(napi_env env, napi_value value) : Object(env, value) {}

    Value call(Value recv, const std::initializer_list<Value> &args) const;
    Value call(const std::initializer_list<napi_value> &args) const;
};

class Array : public Object {
public:
    static Array Create(napi_env env);
    static Array Create(napi_env env, std::size_t length);

    Array(napi_env env) : Object(env) {}
    Array(napi_env env, napi_value value) : Object(env, value) {}

    std::uint32_t length() const;
};

// TODO 其他JS类型暂时用不到

// 函数元信息包装类
class CallbackInfo {
    void operator=(const CallbackInfo &) = delete;
    CallbackInfo(const CallbackInfo &) = delete;

public:
    CallbackInfo(napi_env env, napi_callback_info info, std::size_t argc) : env_(env), info_(info) {
        // 初始化参数列表
        NAPI_CHECK_STATUS(env_, napi_get_cb_info(env, info_, &argc, nullptr, nullptr, nullptr),
                          "get callback argc info failed");
        // 当传入napi_get_cb_info的argv不为nullptr时，argv的长度必须大于等于传入argc声明的大小
        if (argc > 0) {
            args_.reserve(argc);
            //            napi_value *argv = (napi_value*)calloc(argc, sizeof(napi_value));
            napi_value argv[argc];
            std::memset(argv, 0, sizeof(argv));
            NAPI_CHECK_STATUS(env_, napi_get_cb_info(env, info_, &argc, argv, nullptr, nullptr),
                              "get callback argv info failed");

            for (size_t i = 0; i < argc; i++) {
                args_.emplace_back(Value(env, argv[i]));
            }

            //            free(argv);
        }
    }

    Env env() const { return Env(env_); }
    napi_callback_info info() const { return info_; }
    operator napi_callback_info() const { return info_; }
    const std::vector<Value> &args() const { return args_; }
    std::size_t argCount() const { return args_.size(); }
    Value argAt(std::size_t index) const { return args_.at(index); }
    Value operator[](std::size_t index) const { return index < args_.size() ? args_[index] : env().undefined(); }

private:
    const napi_env env_;
    napi_callback_info info_; // 从中可以提取出参数信息
    std::vector<Value> args_;
};

namespace tools {

class HandleScope {
public:
    HandleScope(napi_env env, napi_handle_scope scope) : env_(env), scope_(scope) {}
    explicit HandleScope(napi_env env) : env_(env) {
        NAPI_CHECK_STATUS(env, napi_open_handle_scope(env, &scope_), "open handle scope failed");
    }
    ~HandleScope() { NAPI_CHECK_STATUS(env_, napi_close_handle_scope(env_, scope_), "close handle scope failed"); }

    // Disallow copying to prevent double close of napi_handle_scope
    HandleScope(const HandleScope &) = delete;
    HandleScope &operator=(const HandleScope &) = delete;

    operator napi_handle_scope() const { return scope_; }

    Env env() const { return Env(env_); }

private:
    const napi_env env_;
    napi_handle_scope scope_;
};

template <typename T>
class Reference {
    static_assert(std::is_base_of<Value, T>::value, "T must derived from Value");

public:
    static Reference Create(const T &value, std::uint32_t initial);

    Reference(napi_env env, napi_ref ref);
    ~Reference();

    // A reference can be moved but cannot be copied.
    Reference(Reference<T> &&other);
    Reference<T> &operator=(Reference<T> &&other);
    Reference(const Reference<T> &);
    void operator=(const Reference<T>) = delete;

    operator napi_ref() const { return ref_; }
    bool operator==(const Reference<T> &other) const;
    bool operator!=(const Reference<T> &other) const;

    std::uint32_t ref() const;
    std::uint32_t unref() const;
    void reset();
    void reset(const T &value, std::uint32_t refCount = 0);

    Env env() const { return Env(env_); };
    T value() const;

protected:
    napi_env env_;
    napi_ref ref_;
};

/**
 * Reflector C++运行时反射调用JS函数
 * @note TODO 这里的加锁还要斟酌一下，目前是不支持非NAPI线程操作这些API的。
 */
class Reflector {
    Reflector() = default;
    Reflector(const Reflector &) = delete;
    Reflector &operator=(const Reflector &) = delete;

public:
    using JSFuncsMap = std::unordered_map<std::string, Reference<Function>>;

    static Reflector &Instance() {
        static Reflector inst;
        return inst;
    }

    const JSFuncsMap &boundJSFuncs() const { return jsFuncsMap_; }

    /**
     * 绑定JS函数
     * @note 如果函数已存在，则替换
     * @param alias
     * @param func
     */
    void bindFunc(const std::string &alias, Function func) {
        std::unique_lock lck(mtx_);
        auto it = jsFuncsMap_.find(alias);
        if (it != jsFuncsMap_.end()) {
            it->second.unref();
        }
        jsFuncsMap_.emplace(alias, Reference<Function>::Create(func, 1));
    }

    void unbindFunc(const std::string &alias) {
        std::unique_lock lck(mtx_);
        auto it = jsFuncsMap_.find(alias);
        if (it != jsFuncsMap_.end()) {
            it->second.unref();
        }
        jsFuncsMap_.erase(alias);
    }

    Value callBoundFunc(const std::string &alias, const std::initializer_list<napi_value> &args) {
        std::shared_lock lck(mtx_);
        auto it = jsFuncsMap_.find(alias);
        if (it != jsFuncsMap_.end()) {
            it->second.ref();
            auto ret = it->second.value().call(std::move(args));
            it->second.unref();
            return ret;
        } else {
            throw std::runtime_error("Function not found");
        }
    }

private:
    std::shared_mutex mtx_;
    JSFuncsMap jsFuncsMap_;
};

} // namespace tools

} // namespace napi
} // namespace OHOS

#if defined(CAPABLE_WITH_AKI)
#define APPEND_AKI_SYMBOLS(env, exports) exports = aki::JSBind::BindSymbols(env, exports)
#else
#define APPEND_AKI_SYMBOLS(env, exports)
#endif

#define NAPI_FUNC(name, argc, body)                                                                                    \
    static napi_value napi__##name(napi_env environment, napi_callback_info information) {                             \
        OHOS::napi::Env env(environment);                                                                              \
        OHOS::napi::CallbackInfo cbInfo(environment, information, argc);                                               \
        body;                                                                                                          \
    }

#define NAPI_BIND_FUNC(utf8name, name, method, getter, setter, value, attributes, data)                                \
    { #utf8name, name, napi__##method, getter, setter, value, attributes, data }

#define NAPI_REGISTER_FUNCS_MODULE(modname, version, flags, filename, priv, funcs_bind...)                             \
    EXTERN_C_START                                                                                                     \
    static napi_value module_init(napi_env env, napi_value exports) {                                                  \
        napi_property_descriptor desc[] = {funcs_bind};                                                                \
        NAPI_CHECK_STATUS(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc),             \
                          "define properties failed");                                                                 \
        APPEND_AKI_SYMBOLS(env, exports);                                                                              \
        return exports;                                                                                                \
    }                                                                                                                  \
    EXTERN_C_END                                                                                                       \
    static napi_module modname##_module = {                                                                            \
        .nm_version = version,                                                                                         \
        .nm_flags = flags,                                                                                             \
        .nm_filename = filename,                                                                                       \
        .nm_register_func = module_init,                                                                               \
        .nm_modname = #modname,                                                                                        \
        .nm_priv = ((void *)priv),                                                                                     \
        .reserved = {0},                                                                                               \
    };                                                                                                                 \
    extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&modname##_module); }

#include "napi_framework-inl.h"

#endif // OHOS_NAPI_FRAMEWORK_H
