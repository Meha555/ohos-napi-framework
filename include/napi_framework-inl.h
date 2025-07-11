// NOTE: DO NOT INCLUDE THIS FILE. INCLUDE napi_framework.h INSTEAD.
#ifndef OHOS_NAPI_FRAMEWORK_INL_H
#define OHOS_NAPI_FRAMEWORK_INL_H

#include "napi_framework.h"

namespace OHOS {
namespace napi {

/* ---------------------------------- Env --------------------------------- */

inline Value Env::global() const {
    napi_value value;
    NAPI_CHECK_STATUS(*this, napi_get_global(*this, &value), "Get global object failed");
    return Value(*this, value);
}

inline Value Env::undefined() const {
    napi_value value;
    NAPI_CHECK_STATUS(*this, napi_get_undefined(*this, &value), "Get undefined failed");
    return Value(*this, value);
}

inline Value Env::null() const {
    napi_value value;
    NAPI_CHECK_STATUS(*this, napi_get_null(*this, &value), "Get null failed");
    return Value(*this, value);
}

/* ---------------------------------- Value --------------------------------- */

inline bool Value::strictEquals(const Value &other) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_strict_equals(env_, *this, other, &result), "napi_strict_equals failed");
    return result;
}

inline napi_valuetype Value::type() const {
    if (isEmpty()) {
        return napi_undefined;
    }
    napi_valuetype result;
    NAPI_CHECK_STATUS(env_, napi_typeof(env_, value_, &result), "Get value type failed");
    return result;
}

// template <JSValueType T> inline bool Value::is() const {
//     napi_valuetype result;
//     NAPI_CHECK_STATUS(env_, napi_typeof(env_, value_, &result), "Get value type failed");
//     return result == static_cast<napi_valuetype>(T);
// }

inline bool Value::isArray() const {
    if (isEmpty()) {
        return false;
    }
    bool result;
    NAPI_CHECK_STATUS(env_, napi_is_array(env_, value_, &result), "napi_is_array failed");
    return result;
}

namespace details {
template <typename T> struct vf_number {
    static Number From(napi_env env, T value) { return Number::Create(env, static_cast<double>(value)); }
};

template <> struct vf_number<bool> {
    static Boolean From(napi_env env, bool value) { return Boolean::Create(env, value); }
};

struct vf_utf8_charp {
    static String From(napi_env env, const char *value) { return String::Create(env, value); }
};

struct vf_utf8_string {
    static String From(napi_env env, const std::string &value) { return String::Create(env, value); }
};

template <typename T> struct vf_fallback {
    static Value From(napi_env env, const T &value) { return Value(env, value); }
};

template <typename...> struct disjunction : std::false_type {};
template <typename B> struct disjunction<B> : B {};
template <typename B, typename... Bs>
struct disjunction<B, Bs...> : std::conditional<bool(B::value), B, disjunction<Bs...>>::type {};

template <typename T>
struct can_make_string
    : disjunction<
          typename std::is_convertible<T, const char *>::type, typename std::is_convertible<T, const char16_t *>::type,
          typename std::is_convertible<T, std::string>::type, typename std::is_convertible<T, std::u16string>::type> {};
} // namespace details

// clang-format off
template <typename T> inline Value Value::From(napi_env env, const T &value) {
  using Helper = typename std::conditional<
      std::is_integral<T>::value || std::is_floating_point<T>::value,
      details::vf_number<T>,
      typename std::conditional<details::can_make_string<T>::value,
                                String,
                                details::vf_fallback<T>>::type>::type;
    return Helper::From(env, value);
}

template <typename T> inline String String::From(napi_env env, const T& value) {
  struct Dummy {};
  using Helper = typename std::conditional<
      std::is_convertible<T, const char*>::value,
      details::vf_utf8_charp,
      typename std::conditional<
          std::is_convertible<T, std::string>::value,
          details::vf_utf8_string,
          Dummy>::type>::type;
  return Helper::From(env, value);
}
// clang-format on

template <typename NapiValue>
inline typename std::enable_if<std::is_base_of<Value, NapiValue>::value, NapiValue>::type Value::as() const {
    return NapiValue(env_, value_);
}

inline Boolean Value::toBoolean() const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_coerce_to_bool(env_, value_, &result), "napi_coerce_to_bool failed");
    return Boolean(env_, result);
}

inline Number Value::toNumber() const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_coerce_to_number(env_, value_, &result), "napi_coerce_to_number failed");
    return Number(env_, result);
}

inline String Value::toString() const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_coerce_to_string(env_, value_, &result), "napi_coerce_to_string failed");
    return String(env_, result);
}

inline Object Value::toObject() const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_coerce_to_object(env_, value_, &result), "napi_coerce_to_object failed");
    return Object(env_, result);
}

/* --------------------------------- Boolean -------------------------------- */

inline Boolean Boolean::Create(napi_env env, bool value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_get_boolean(env, value, &result), "napi_get_boolean");
    return Boolean(env, result);
}

inline Boolean::operator bool() const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_get_value_bool(env_, value_, &result), "napi_get_value_bool");
    return result;
}

/* --------------------------------- Number --------------------------------- */

inline Number Number::Create(napi_env env, double value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_double(env, value, &result), "Failed to create double");
    return Number(env, result);
}

inline Number Number::Create(napi_env env, float value) { return Create(env, static_cast<double>(value)); }

inline Number Number::Create(napi_env env, std::uint32_t value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_uint32(env, value, &result), "Failed to create uint32");
    return Number(env, result);
}

inline Number Number::Create(napi_env env, std::int32_t value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_int32(env, value, &result), "Failed to create int32");
    return Number(env, result);
}

inline Number Number::Create(napi_env env, std::int64_t value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_int64(env, value, &result), "Failed to create int64");
    return Number(env, result);
}

inline Number::operator double() const { return asDouble(); }
inline double Number::asDouble() const {
    double result;
    NAPI_CHECK_STATUS(env_, napi_get_value_double(env_, value_, &result), "Convert napi_value to double failed");
    return result;
}
inline Number::operator float() const { return asFloat(); }
inline float Number::asFloat() const { return static_cast<float>(asDouble()); }
inline Number::operator std::uint32_t() const { return asUint32(); }
inline std::uint32_t Number::asUint32() const {
    std::uint32_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_uint32(env_, value_, &result), "Convert napi_value to uint32_t failed");
    return result;
}
inline Number::operator std::int32_t() const { return asInt32(); }
inline std::int32_t Number::asInt32() const {
    std::int32_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_int32(env_, value_, &result), "Convert napi_value to int32_t failed");
    return result;
}
inline Number::operator std::int64_t() const { return asInt64(); }
inline std::int64_t Number::asInt64() const {
    std::int64_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_int64(env_, value_, &result), "Convert napi_value to int64_t failed");
    return result;
}

/* --------------------------------- BigInt --------------------------------- */

inline BigInt BigInt::Create(napi_env env, std::int64_t value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_bigint_int64(env, value, &result), "Failed to create bigint");
    return BigInt(env, result);
}

inline BigInt BigInt::Create(napi_env env, std::uint64_t value) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_bigint_uint64(env, value, &result), "Failed to create bigint");
    return BigInt(env, result);
}

inline BigInt BigInt::Create(napi_env env, int signBit, std::size_t wordCount, const std::uint64_t *words) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_bigint_words(env, signBit, wordCount, words, &result),
                      "Failed to create bigint");
    return BigInt(env, result);
}

inline std::int64_t BigInt::asInt64(bool *lossless) const {
    std::int64_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_bigint_int64(env_, value_, &result, lossless),
                      "Failed to get bigint as int64");
    return result;
}

inline std::uint64_t BigInt::asUint64(bool *lossless) const {
    std::uint64_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_bigint_uint64(env_, value_, &result, lossless),
                      "Failed to get bigint as uint64");
    return result;
}

inline std::size_t BigInt::wordCount() const {
    std::size_t result;
    NAPI_CHECK_STATUS(env_, napi_get_value_bigint_words(env_, value_, nullptr, &result, nullptr),
                      "Failed to get bigint word count");
    return result;
}

inline void BigInt::toWords(int *signBit, std::size_t *wordCount, std::uint64_t *words) const {
    NAPI_CHECK_STATUS(env_, napi_get_value_bigint_words(env_, value_, signBit, wordCount, words),
                      "Failed to get bigint words");
}

/* --------------------------------- Object --------------------------------- */

inline Object Object::Create(napi_env env) {
    napi_value value;
    NAPI_CHECK_STATUS(env, napi_create_object(env, &value), "Create object failed");
    return Object(env, value);
}

inline bool Object::has(napi_value key) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_has_property(env_, value_, key, &result), "napi_has_property failed");
    return result;
}

inline bool Object::has(const char *utf8name) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_has_named_property(env_, value_, utf8name, &result), "napi_has_named_property failed");
    return result;
}
inline bool Object::has(const std::string &utf8name) const { return has(utf8name.c_str()); }

inline bool Object::has(std::uint32_t index) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_has_element(env_, value_, index, &result), "napi_has_element failed");
    return result;
}

inline bool Object::hasOwnProperty(napi_value key) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_has_own_property(env_, value_, key, &result), "napi_has_own_property failed");
    return result;
}

inline bool Object::hasOwnProperty(const char *utf8name) const {
    return hasOwnProperty(String::Create(env_, utf8name, std::strlen(utf8name)).value());
}
inline bool Object::hasOwnProperty(const std::string &utf8name) const {
    return hasOwnProperty(String::Create(env_, utf8name).value());
}

inline Object::PropertyLValue<std::string> Object::operator[](const char *utf8name) {
    return PropertyLValue<std::string>(*this, utf8name);
}

inline Object::PropertyLValue<std::string> Object::operator[](const std::string &utf8name) {
    return PropertyLValue<std::string>(*this, utf8name);
}

inline Object::PropertyLValue<std::uint32_t> Object::operator[](std::uint32_t index) {
    return PropertyLValue<std::uint32_t>(*this, index);
}

inline Object::PropertyLValue<Value> Object::operator[](Value index) const {
    return PropertyLValue<Value>(*this, index);
}

inline Value Object::get(napi_value key) const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_get_property(env_, value_, key, &result), "napi_get_property failed");
    return Value(env_, result);
}
// inline Value Object::operator[](napi_value key) const { return get(key); }

inline Value Object::get(const char *utf8name) const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_get_named_property(env_, value_, utf8name, &result), "napi_get_named_property failed");
    return Value(env_, result);
}
inline Value Object::get(const std::string &utf8name) const { return get(utf8name.c_str()); }
inline Value Object::operator[](const char *utf8name) const { return get(utf8name); }
inline Value Object::operator[](const std::string &utf8name) const { return get(utf8name); }

inline Value Object::get(std::uint32_t index) const {
    napi_value value;
    NAPI_CHECK_STATUS(env_, napi_get_element(env_, value_, index, &value), "napi_get_element failed");
    return Value(env_, value);
}
inline Value Object::operator[](std::uint32_t index) const { return get(index); }

template <typename ValueType> inline bool Object::set(napi_value key, const ValueType &value) const {
    NAPI_CHECK_STATUS(env_, napi_set_property(env_, value_, key, Value::From(env_, value)), "napi_set_property failed");
    return true;
}

template <typename ValueType> inline bool Object::set(const char *utf8name, const ValueType &value) const {
    NAPI_CHECK_STATUS(env_, napi_set_named_property(env_, value_, utf8name, Value::From(env_, value)),
                      "napi_set_named_property failed");
    return true;
}
template <typename ValueType> inline bool Object::set(const std::string &name, const ValueType &value) const {
    return set(name.c_str(), value);
}

template <typename ValueType> inline bool Object::set(std::uint32_t index, const ValueType &value) const {
    NAPI_CHECK_STATUS(env_, napi_set_element(env_, value_, index, Value::From(env_, value)), "napi_set_element failed");
    return true;
}

inline bool Object::del(napi_value key) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_delete_property(env_, value_, key, &result), "napi_delete_property failed");
    return result;
}

inline bool Object::del(const char *utf8name) const {
    return del(String::Create(env_, utf8name, std::strlen(utf8name)).value());
}
inline bool Object::del(const std::string &utf8name) const { return del(String::Create(env_, utf8name).value()); }

inline bool Object::del(std::uint32_t index) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_delete_element(env_, value_, index, &result), "napi_delete_element failed");
    return result;
}

class Object::const_iterator {
    friend class Object;
    enum class Type { Begin, End };

    const_iterator(const Object *object, const Type type)
        : object_(object), keys_(object->getPropertyNames()), index_(type == Type::Begin ? 0 : keys_.length()) {}

public:
    const_iterator &operator++() {
        ++index_;
        return *this;
    }
    bool operator==(const const_iterator &other) const { return index_ == other.index_; }
    bool operator!=(const const_iterator &other) const { return index_ != other.index_; }
    const std::pair<Value, Object::PropertyLValue<Value>> operator*() const {
        const Value key = keys_[index_];                     // 属性的索引（napi_string类型）
        const PropertyLValue<Value> value = (*object_)[key]; // 属性的值
        return {key, value};
    }

private:
    const Object *object_;
    Array keys_;
    uint32_t index_;
};

class Object::iterator {
    friend class Object;
    enum class Type { Begin, End };

    iterator(Object *object, const Type type)
        : object_(object), keys_(object->getPropertyNames()), index_(type == Type::Begin ? 0 : keys_.length()) {}

public:
    iterator &operator++() {
        ++index_;
        return *this;
    }
    bool operator==(const iterator &other) const { return index_ == other.index_; }
    bool operator!=(const iterator &other) const { return index_ != other.index_; }
    std::pair<Value, Object::PropertyLValue<Value>> operator*() {
        Value key = keys_[index_];                     // 属性的索引（napi_string类型）
        PropertyLValue<Value> value = (*object_)[key]; // 属性的值
        return {key, value};
    }

private:
    Object *object_;
    Array keys_;
    uint32_t index_;
};

inline Object::const_iterator Object::begin() const {
    return const_iterator(this, Object::const_iterator::Type::Begin);
}

inline Object::const_iterator Object::end() const { return const_iterator(this, Object::const_iterator::Type::End); }

inline Object::iterator Object::begin() { return iterator(this, Object::iterator::Type::Begin); }

inline Object::iterator Object::end() { return iterator(this, Object::iterator::Type::End); }

inline Array Object::getPropertyNames() const {
    napi_value result;
    NAPI_CHECK_STATUS(env_, napi_get_property_names(env_, value_, &result), "napi_get_property_names failed");
    return Array(env_, result);
}

inline bool Object:: instanceof (const Function &constructor) const {
    bool result;
    NAPI_CHECK_STATUS(env_, napi_instanceof(env_, value_, constructor, &result), "napi_instanceof failed");
    return result;
}

inline bool Object::freeze() const {
    NAPI_CHECK_STATUS(env_, napi_object_freeze(env_, value_), "napi_object_freeze failed");
    return true;
}

inline bool Object::seal() const {
    NAPI_CHECK_STATUS(env_, napi_object_seal(env_, value_), "napi_object_seal failed");
    return true;
}

/* --------------------------------- String --------------------------------- */

inline String String::Create(napi_env env, const char *str, std::size_t length) {
    napi_value value;
    NAPI_CHECK_STATUS(env, napi_create_string_utf8(env, str, length, &value), "napi_create_string_utf8 failed");
    return String(env, value);
}

inline String String::Create(napi_env env, const std::string &str) {
    return String::Create(env, str.c_str(), str.size());
}

// inline String::operator const char *() const {
//     return asCString();
// }
// inline const char *String::asCString() const {
//     return asString().c_str();
// }

inline String::operator std::string() const { return asString(); }

inline std::string String::asString() const {
    std::size_t length;
    NAPI_CHECK_STATUS(env_, napi_get_value_string_utf8(env_, value_, nullptr, 0, &length), "Get string length failed");
    std::string result;
    result.reserve(length + 1);
    result.resize(length);
    NAPI_CHECK_STATUS(env_, napi_get_value_string_utf8(env_, value_, &result[0], result.capacity(), nullptr),
                      "Convert napi_value to string failed");
    return result;
}

/* -------------------------------- Function -------------------------------- */

inline Function Function::Create(napi_env env, const char *utf8name, napi_callback callback, void *data) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_function(env, utf8name, NAPI_AUTO_LENGTH, callback, data, &result),
                      "Failed to create function");
    return Function(env, result);
}

inline Function Function::Create(napi_env env, const std::string &utf8name, napi_callback callback, void *data) {
    napi_value result;
    NAPI_CHECK_STATUS(env, napi_create_function(env, utf8name.c_str(), NAPI_AUTO_LENGTH, callback, data, &result),
                      "Failed to create function");
    return Function(env, result);
}

inline Value Function::call(Value recv, const std::initializer_list<Value> &args) const {
    napi_value result;
    napi_value argv[args.size()];
    std::size_t i = 0;
    for (const auto &arg : args) {
        argv[i++] = arg;
    }
    NAPI_CHECK_STATUS(env_, napi_call_function(env_, recv.value(), value_, args.size(), argv, &result),
                      "napi_call_function failed");
    return Value(env_, result);
}

inline Value Function::call(const std::initializer_list<napi_value> &args) const {
    napi_value result;
    napi_value argv[args.size()];
    std::size_t i = 0;
    for (const auto &arg : args) {
        argv[i++] = arg;
    }
    NAPI_CHECK_STATUS(env_, napi_call_function(env_, nullptr, value_, args.size(), argv, &result),
                      "napi_call_function failed");
    return Value(env_, result);
}

/* ---------------------------------- Array --------------------------------- */

inline Array Array::Create(napi_env env) {
    napi_value value;
    NAPI_CHECK_STATUS(env, napi_create_array(env, &value), "Create array failed");
    return Array(env, value);
}

inline Array Array::Create(napi_env env, std::size_t length) {
    napi_value value;
    NAPI_CHECK_STATUS(env, napi_create_array_with_length(env, length, &value), "Create array failed");
    return Array(env, value);
}

inline std::uint32_t Array::length() const {
    std::uint32_t result;
    NAPI_CHECK_STATUS(env_, napi_get_array_length(env_, value_, &result), 0);
    return result;
}

/* -------------------------------- Reference ------------------------------- */

namespace tools {

template <typename T> inline Reference<T> Reference<T>::Create(const T &value, std::uint32_t initial) {
    napi_env env = value.env();
    napi_ref ref;
    NAPI_CHECK_STATUS(env, napi_create_reference(env, value, initial, &ref), "napi_create_reference failed");
    return Reference<T>(env, ref);
}

template <typename T> inline Reference<T>::Reference(napi_env env, napi_ref ref) : env_(env), ref_(ref) {}

template <typename T> inline Reference<T>::~Reference() {
    // 这里是否要判断引用计数，否则是否可能发生delete了外部传入的napi_ref，而外部也delete了这个ref而导致该对象double free的问题
    // 不，直接要求外部传入的ref会直接被当前类获得所有权。这样和其他的类的风格统一了
    if (ref_ != nullptr) {
        napi_delete_reference(env_, ref_);
    }
    ref_ = nullptr;
}

template <typename T>
inline Reference<T>::Reference(Reference<T> &&other)
    : env_(std::exchange(other.env_, nullptr)), ref_(std::exchange(other.ref_, nullptr)) {}

template <typename T> inline Reference<T> &Reference<T>::operator=(Reference<T> &&other) {
    if (this != &other) {
        reset();
        env_ = std::exchange(other.env_, nullptr);
        ref_ = std::exchange(other.ref_, nullptr);
    }
    return *this;
}

template <typename T> inline Reference<T>::Reference(const Reference<T> &other) : env_(other.env_), ref_(nullptr) {
    HandleScope scope(env_);

    napi_value value = other.value();
    if (value != nullptr) {
        // Copying is a limited scenario (currently only used for Error object) and
        // always creates a strong reference to the given value even if the incoming
        // reference is weak.
        NAPI_CHECK_STATUS(env_, napi_create_reference(env_, value, 1, &ref_), "napi_create_reference failed");
    }
}

template <typename T> inline bool Reference<T>::operator==(const Reference<T> &other) const {
    HandleScope scope(env_);
    return this->value().strictEquals(other.value());
}

template <typename T> inline bool Reference<T>::operator!=(const Reference<T> &other) const {
    return !this->operator==(other);
}

template <typename T> inline std::uint32_t Reference<T>::ref() const {
    std::uint32_t result;
    NAPI_CHECK_STATUS(env_, napi_reference_ref(env_, ref_, &result), "napi_reference_ref failed");
    return result;
}

template <typename T> inline std::uint32_t Reference<T>::unref() const {
    std::uint32_t result;
    NAPI_CHECK_STATUS(env_, napi_reference_unref(env_, ref_, &result), "napi_reference_unref failed");
    return result;
}

template <typename T> inline void Reference<T>::reset() {
    if (ref_ != nullptr) {
        NAPI_CHECK_STATUS(env_, napi_delete_reference(env_, ref_), "napi_delete_reference failed");
        ref_ = nullptr;
    }
}

template <typename T> inline void Reference<T>::reset(const T &value, uint32_t refcount) {
    reset();
    env_ = value.env();

    napi_value val = value;
    if (val != nullptr) {
        NAPI_CHECK_STATUS(env_, napi_create_reference(env_, value, refcount, &ref_), "napi_create_reference failed");
    }
}

template <typename T> inline T Reference<T>::value() const {
    if (ref_ == nullptr) {
        return T(env_, nullptr);
    }

    napi_value value;
    NAPI_CHECK_STATUS(env_, napi_get_reference_value(env_, ref_, &value), "napi_get_reference_value failed");
    return T(env_, value);
}

} // namespace tools
} // namespace napi
} // namespace OHOS

#endif // OHOS_NAPI_FRAMEWORK_INL_H
