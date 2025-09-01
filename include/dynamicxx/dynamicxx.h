#ifndef DYNAMICXX_DYNAMICXX_H
#define DYNAMICXX_DYNAMICXX_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef _MSVC_LANG
#define DCXX_LANG (_MSVC_LANG)
#else
#define DCXX_LANG (__cplusplus)
#endif

#define HAS_CXX_14 (DCXX_LANG >= 201402UL)
#define HAS_CXX_17 (DCXX_LANG >= 201703UL)
#define HAS_CXX_20 (DCXX_LANG >= 202002UL)

#if HAS_CXX_14
#define DCONSTEXPR_14 constexpr
#else
#define DCONSTEXPR_14
#endif

#if HAS_CXX_17
#define DNODISCARD [[nodiscard]]
#else
#define DNODISCARD
#endif

#if HAS_CXX_20
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#else
#define LIKELY
#define UNLIKELY
#endif

#if __cpp_concepts >= 201907UL
#define DHAS_CONCEPTS 1
#else
#define DHAS_CONCEPTS 0
#endif

#if DHAS_CONCEPTS
#include <concepts>
#endif

#define DDO_ASSERT(...)      \
    do {                     \
        assert(__VA_ARGS__); \
    } while (false)

namespace dynamicxx {

namespace detail {

template <class Type>
struct TypeIdentity {
    using type = Type;  // NOLINT
};

template <class Type>
struct IsBool {
    static constexpr bool value = false;
};
template <>
struct IsBool<bool> {
    static constexpr bool value = true;
};

struct Null {};

struct Undefined {};

template <class Type>
using Just = Type;

template <class Type>
using NonNull = Type;

template <class Type>
auto PointerOf(Type& value) -> NonNull<Type*> {
    return std::addressof(value);
}

template <class Type>
auto PointerOf(std::shared_ptr<Type>& ptr) -> NonNull<Type*> {
    return ptr.get();
}
template <class Type>
auto PointerOf(const std::shared_ptr<Type>& ptr) -> NonNull<const Type*> {
    return ptr.get();
}

template <class Type>
struct DefaultFactory {
    template <class... Args>
    Type operator()(Args&&... args) const
        noexcept(noexcept(Type{std::forward<Args>(args)...})) {
        return Type{std::forward<Args>(args)...};
    }
};
template <class Type>
struct DefaultFactory<std::shared_ptr<Type>> {
    template <class... Args>
    std::shared_ptr<Type> operator()(Args&&... args) const noexcept(
        noexcept(std::make_shared<Type>(std::forward<Args>(args)...))) {
        return std::make_shared<Type>(std::forward<Args>(args)...);
    }
};

template <class Type>
Type DefaultOf() noexcept(noexcept(DefaultFactory<Type>{}())) {
    return DefaultFactory<Type>{}();
}

}  // namespace detail

struct DefaultToIndex {
    template <class Type>
    std::size_t Convert(Type value) const noexcept {
        return static_cast<std::size_t>(value);
    }

    std::size_t Convert(const char* str) const noexcept {
        return std::stoul(str);
    }

    std::size_t Convert(const std::string& str) const noexcept {
        return std::stoul(str);
    }
};

template <class String>
struct IntStringifier;

template <>
struct IntStringifier<std::string> {
    template <class Type>
    std::string Convert(Type value) const {
        return std::to_string(value);
    }
};

struct DefaultToString {
    template <class String, class Type>
    String Convert(Type value) const {
        return IntStringifier<String>{}.Convert(value);
    }

    template <class String>
    const String& Convert(const String& str) const {
        return str;
    }

    template <class String>
    const char* Convert(const char* str) const {
        return str;
    }
};

class InvalidAccessException : std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

using DefaultInteger = std::int64_t;
using DefaultNumber = double;
using DefaultString = std::string;
template <class... Ts>
using DefaultBlobContainer = std::vector<Ts...>;
template <class... Ts>
using DefaultArrayContainer = std::vector<Ts...>;
template <class... Ts>
using DefaultObjectContainer = std::unordered_map<Ts...>;

template <class IntegerType = DefaultInteger, class NumberType = DefaultNumber,
          class StringType = DefaultString,
          template <class...> class BlobContainerType = DefaultBlobContainer,
          template <class...> class ArrayContainerType = DefaultArrayContainer,
          template <class...> class ObjectContainerType =
              DefaultObjectContainer,
          class ToString = DefaultToString, class ToIndex = DefaultToIndex,
          template <class...> class ImplWrapper = detail::Just>
class BasicDynamic {
   public:
    using Null = detail::Null;
    using Boolean = bool;
    using Integer = IntegerType;
    using Number = NumberType;
    using String = StringType;
    using Blob = BlobContainerType<std::uint8_t>;
    using Array = ArrayContainerType<BasicDynamic>;
    using Object = ObjectContainerType<std::string, BasicDynamic>;
    using Undefined = detail::Undefined;

    static_assert(std::is_integral<Integer>::value,
                  "Integer type provided must be an integral type");
    static_assert(std::is_floating_point<Number>::value,
                  "Number type provided must be a floating point type");

    template <class... Args>
    struct BestFitFor;

    template <>
    struct BestFitFor<Boolean> : detail::TypeIdentity<Boolean> {};
    template <>
    struct BestFitFor<Integer> : detail::TypeIdentity<Integer> {};
    template <>
    struct BestFitFor<Number> : detail::TypeIdentity<Number> {};
    template <>
    struct BestFitFor<String> : detail::TypeIdentity<String> {};
    template <>
    struct BestFitFor<Blob> : detail::TypeIdentity<Blob> {};
    template <>
    struct BestFitFor<Array> : detail::TypeIdentity<Array> {};
    template <>
    struct BestFitFor<Object> : detail::TypeIdentity<Object> {};

#if DHAS_CONCEPTS
    template <class Type>
        requires(detail::IsBool<Type>::value)
    struct BestFitFor<Type> : detail::TypeIdentity<Boolean> {};
    template <std::integral Type>
    struct BestFitFor<Type> : detail::TypeIdentity<Integer> {};
    template <std::floating_point Type>
    struct BestFitFor<Type> : detail::TypeIdentity<Number> {};

    template <class... Args>
        requires(std::constructible_from<String, Args...>)
    struct BestFitFor<Args...> : detail::TypeIdentity<String> {};
    template <class... Args>
        requires(std::constructible_from<Blob, Args...>)
    struct BestFitFor<Args...> : detail::TypeIdentity<Blob> {};
    template <class... Args>
        requires(std::constructible_from<Array, Args...>)
    struct BestFitFor<Args...> : detail::TypeIdentity<Array> {};
    template <class... Args>
        requires(std::constructible_from<Object, Args...>)
    struct BestFitFor<Args...> : detail::TypeIdentity<Object> {};
#else
    template <class Type>
    struct BestFitFor<Type>
        : detail::TypeIdentity<typename std::conditional<
              detail::IsBool<Type>::value, Boolean,
              typename std::conditional<
                  std::is_integral<Type>::value, Integer,
                  typename std::conditional<
                      std::is_floating_point<Type>::value, Number,
                      typename std::conditional<
                          std::is_constructible<String, Type>::value, String,
                          typename std::conditional<
                              std::is_constructible<Blob, Type>::value, Blob,
                              typename std::conditional<
                                  std::is_constructible<Array, Type>::value,
                                  Array,
                                  typename std::conditional<
                                      std::is_constructible<Object,
                                                            Type>::value,
                                      Object, void>::type>::type>::type>::
                          type>::type>::type>::type> {};

    template <class... Args>
    struct BestFitFor
        : detail::TypeIdentity<typename std::conditional<
              // Check for multi-arg constructors in a reasonable priority order
              std::is_constructible<String, Args...>::value, String,
              typename std::conditional<
                  std::is_constructible<Blob, Args...>::value, Blob,
                  typename std::conditional<
                      std::is_constructible<Array, Args...>::value, Array,
                      typename std::conditional<
                          std::is_constructible<Object, Args...>::value, Object,
                          void>::type>::type>::type>::type> {};
#endif  // ^^^ DHAS_CONCEPTS

   private:
    using TagRepr = std::uint32_t;
    enum struct Tag : TagRepr {
        Null = 0,
        Boolean,
        Integer,
        Number,
        String,
        Blob,
        Array,
        Object,
        Undefined = ~static_cast<TagRepr>(0),
    };

    template <Tag MyTag>
    struct TagIdentity : public std::integral_constant<Tag, MyTag> {};

    template <class>
    struct TagOfHelper;

    template <>
    struct TagOfHelper<Null> : TagIdentity<Tag::Null> {};
    template <>
    struct TagOfHelper<Boolean> : TagIdentity<Tag::Boolean> {};
    template <>
    struct TagOfHelper<Integer> : TagIdentity<Tag::Integer> {};
    template <>
    struct TagOfHelper<Number> : TagIdentity<Tag::Number> {};
    template <>
    struct TagOfHelper<String> : TagIdentity<Tag::String> {};
    template <>
    struct TagOfHelper<Blob> : TagIdentity<Tag::Blob> {};
    template <>
    struct TagOfHelper<Array> : TagIdentity<Tag::Array> {};
    template <>
    struct TagOfHelper<Object> : TagIdentity<Tag::Object> {};
    template <>
    struct TagOfHelper<Undefined> : TagIdentity<Tag::Undefined> {};

    template <class Type>
    DNODISCARD static constexpr Tag TagOf() noexcept {
        return TagOfHelper<Type>::value;
    }

    union Payload {
        Payload() {}
        ~Payload() {}

        Null null;
        Boolean boolean;
        Integer integer;
        Number number;
        String string;
        Blob blob;
        Array array;
        Object object;
        Undefined undefined = {};
    };

    struct Impl {
        Impl() {}
        ~Impl() { DestroyIfNeeded(); }

        Impl(const Impl& that) { CopyRaw(that); }
        Impl& operator=(const Impl& that) {
            if (this != std::addressof(that)) {
                Copy(that);
            }
            return *this;
        }

        Impl(Impl&& that) noexcept { MoveRaw(that); }
        Impl& operator=(Impl&& that) noexcept {
            if (this != std::addressof(that)) {
                Move(that);
            }
            return *this;
        }

        DNODISCARD constexpr bool HoldsNull() const noexcept {
            return Holds<Null>();
        }
        DNODISCARD constexpr bool HoldsBoolean() const noexcept {
            return Holds<Boolean>();
        }
        DNODISCARD constexpr bool HoldsInteger() const noexcept {
            return Holds<Integer>();
        }
        DNODISCARD constexpr bool HoldsNumber() const noexcept {
            return Holds<Number>();
        }
        DNODISCARD constexpr bool HoldsString() const noexcept {
            return Holds<String>();
        }
        DNODISCARD constexpr bool HoldsBlob() const noexcept {
            return Holds<Blob>();
        }
        DNODISCARD constexpr bool HoldsArray() const noexcept {
            return Holds<Array>();
        }
        DNODISCARD constexpr bool HoldsObject() const noexcept {
            return Holds<Object>();
        }
        DNODISCARD constexpr bool HoldsUndefined() const noexcept {
            return Holds<Undefined>();
        }

        DNODISCARD DCONSTEXPR_14 Boolean GetBoolean() const {
            if (HoldsBoolean()) {
                LIKELY { return payload_.boolean; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Integer GetInteger() const {
            if (HoldsInteger()) {
                LIKELY { return payload_.integer; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Number GetNumber() const {
            if (HoldsNumber()) {
                LIKELY { return payload_.number; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 String& GetString() {
            if (HoldsString()) {
                LIKELY { return payload_.string; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }
        DNODISCARD DCONSTEXPR_14 const String& GetString() const {
            if (HoldsString()) {
                LIKELY { return payload_.string; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Blob& GetBlob() {
            if (HoldsBlob()) {
                LIKELY { return payload_.blob; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }
        DNODISCARD DCONSTEXPR_14 const Blob& GetBlob() const {
            if (HoldsBlob()) {
                LIKELY { return payload_.blob; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Array& GetArray() {
            if (HoldsArray()) {
                LIKELY { return payload_.array; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }
        DNODISCARD DCONSTEXPR_14 const Array& GetArray() const {
            if (HoldsArray()) {
                LIKELY { return payload_.array; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Object& GetObject() {
            if (HoldsObject()) {
                LIKELY { return payload_.object; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }
        DNODISCARD DCONSTEXPR_14 const Object& GetObject() const {
            if (HoldsObject()) {
                LIKELY { return payload_.object; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        DNODISCARD DCONSTEXPR_14 Null GetNull() const {
            if (HoldsNull()) {
                LIKELY { return payload_.null; }
            } else {
                UNLIKELY { InvalidAccess(); }
            }
        }

        void Move(Impl& that) noexcept {
            DestroyIfNeeded();
            MoveRaw(that);
        }

        void Copy(const Impl& that) {
            DestroyIfNeeded();
            CopyRaw(that);
        }

        template <class Type, class... Args>
        void Emplace(Args&&... args) {
            DestroyIfNeeded();
            EmplaceRaw<Type>(std::forward<Args>(args)...);
        }

       private:
        template <class CastType>
        struct Caster;

        friend Caster<Boolean>;
        friend Caster<Integer>;
        friend Caster<Number>;
        friend Caster<String>;
        friend Caster<Blob>;
        friend Caster<Array>;
        friend Caster<Object>;

        template <>
        struct Caster<Boolean> {
            DNODISCARD static Boolean& As(Impl& impl) {
                return impl.payload_.boolean;
            }
            DNODISCARD static const Boolean& As(const Impl& impl) {
                return impl.payload_.boolean;
            }
        };
        template <>
        struct Caster<Integer> {
            DNODISCARD static Integer& As(Impl& impl) {
                return impl.payload_.integer;
            }
            DNODISCARD static const Integer& As(const Impl& impl) {
                return impl.payload_.integer;
            }
        };
        template <>
        struct Caster<Number> {
            DNODISCARD static Number& As(Impl& impl) {
                return impl.payload_.number;
            }
            DNODISCARD static const Number& As(const Impl& impl) {
                return impl.payload_.number;
            }
        };
        template <>
        struct Caster<String> {
            DNODISCARD static String& As(Impl& impl) {
                return impl.payload_.string;
            }
            DNODISCARD static const String& As(const Impl& impl) {
                return impl.payload_.string;
            }
        };
        template <>
        struct Caster<Array> {
            DNODISCARD static Array& As(Impl& impl) {
                return impl.payload_.array;
            }
            DNODISCARD static const Array& As(const Impl& impl) {
                return impl.payload_.array;
            }
        };
        template <>
        struct Caster<Blob> {
            DNODISCARD static Blob& As(Impl& impl) {
                return impl.payload_.blob;
            }
            DNODISCARD static const Blob& As(const Impl& impl) {
                return impl.payload_.blob;
            }
        };
        template <>
        struct Caster<Object> {
            DNODISCARD static Object& As(Impl& impl) {
                return impl.payload_.object;
            }
            DNODISCARD static const Object& As(const Impl& impl) {
                return impl.payload_.object;
            }
        };

        template <>
        struct Caster<Null> {
            DNODISCARD static Null As(const Impl& impl) {
                return impl.payload_.null;
            }
        };

       public:
        template <class CastType>
        DNODISCARD CastType& As() {
            if (Holds<CastType>()) {
                return Caster<CastType>::As(*this);
            } else {
                InvalidAccess();
            }
        }

        template <class CastType>
        DNODISCARD const CastType& As() const {
            if (Holds<CastType>()) {
                return Caster<CastType>::As(*this);
            } else {
                InvalidAccess();
            }
        }

        DNODISCARD Impl Clone() const {
            Impl impl;
            switch (tag_) {
                case Tag::Null: {
                    impl.EmplaceRaw<Null>(payload_.null);
                    return impl;
                }
                case Tag::Boolean: {
                    impl.EmplaceRaw<Boolean>(payload_.boolean);
                    return impl;
                }
                case Tag::Integer: {
                    impl.EmplaceRaw<Integer>(payload_.integer);
                    return impl;
                }
                case Tag::Number: {
                    impl.EmplaceRaw<Number>(payload_.number);
                    return impl;
                }
                case Tag::String: {
                    impl.EmplaceRaw<String>(payload_.string);
                    return impl;
                }
                case Tag::Blob: {
                    impl.EmplaceRaw<Blob>(payload_.blob);
                    return impl;
                }
                case Tag::Array: {
                    impl.EmplaceRaw<Array>();
                    auto& array = impl.As<Array>();
                    array.reserve(payload_.array.size());
                    for (const auto& value : payload_.array) {
                        array.emplace_back(value.Clone());
                    }
                    return impl;
                }
                case Tag::Object: {
                    impl.EmplaceRaw<Object>();
                    auto& object = impl.As<Object>();
                    object.reserve(payload_.object.size());
                    for (const auto& value : payload_.object) {
                        object[value.first] = value.second.Clone();
                    }
                    return impl;
                }
                case Tag::Undefined: {
                    impl.EmplaceRaw<Undefined>(payload_.undefined);
                    return impl;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
        }

        DNODISCARD DCONSTEXPR_14 bool Equals(const Impl& that) const {
            DDO_ASSERT(tag_ == that.tag_);
            switch (tag_) {
                case Tag::Null: {
                    return true;
                }
                case Tag::Boolean: {
                    return payload_.boolean == that.payload_.boolean;
                }
                case Tag::Integer: {
                    return payload_.integer == that.payload_.integer;
                }
                case Tag::Number: {
                    return payload_.number == that.payload_.number;
                }
                case Tag::String: {
                    return payload_.string == that.payload_.string;
                }
                case Tag::Blob: {
                    return payload_.blob == that.payload_.blob;
                }
                case Tag::Array: {
                    return payload_.array == that.payload_.array;
                }
                case Tag::Object: {
                    return payload_.object == that.payload_.object;
                }
                case Tag::Undefined: {
                    return true;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
        }

       private:
        template <class WantedTag>
        DNODISCARD constexpr bool Holds() const noexcept {
            return tag_ == TagOf<WantedTag>();
        }

        void MoveRaw(Impl&& that) noexcept { return MoveRaw(that); }

        void MoveRaw(Impl& that) noexcept {
            tag_ = that.tag_;
            switch (that.tag_) {
                case Tag::Null: {
                    break;
                }
                case Tag::Boolean: {
                    EmplaceRaw<Boolean>(std::move(that.payload_.boolean));
                    break;
                }
                case Tag::Integer: {
                    EmplaceRaw<Integer>(std::move(that.payload_.integer));
                    break;
                }
                case Tag::Number: {
                    EmplaceRaw<Number>(std::move(that.payload_.number));
                    break;
                }
                case Tag::String: {
                    EmplaceRaw<String>(std::move(that.payload_.string));
                    break;
                }
                case Tag::Blob: {
                    EmplaceRaw<Blob>(std::move(that.payload_.blob));
                    break;
                }
                case Tag::Array: {
                    EmplaceRaw<Array>(std::move(that.payload_.array));
                    break;
                }
                case Tag::Object: {
                    EmplaceRaw<Object>(std::move(that.payload_.object));
                    break;
                }
                case Tag::Undefined: {
                    break;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
            that.DestroyIfNeeded();
        }

        void CopyRaw(const Impl& that) noexcept {
            tag_ = that.tag_;
            switch (that.tag_) {
                case Tag::Null: {
                    break;
                }
                case Tag::Boolean: {
                    EmplaceRaw<Boolean>(that.payload_.boolean);
                    break;
                }
                case Tag::Integer: {
                    EmplaceRaw<Integer>(that.payload_.integer);
                    break;
                }
                case Tag::Number: {
                    EmplaceRaw<Number>(that.payload_.number);
                    break;
                }
                case Tag::String: {
                    EmplaceRaw<String>(that.payload_.string);
                    break;
                }
                case Tag::Blob: {
                    EmplaceRaw<Blob>(that.payload_.blob);
                    break;
                }
                case Tag::Array: {
                    EmplaceRaw<Array>(that.payload_.array);
                    break;
                }
                case Tag::Object: {
                    EmplaceRaw<Object>(that.payload_.object);
                    break;
                }
                case Tag::Undefined: {
                    break;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
        }

        [[noreturn]] static void ThrowInvalidTerminatingTag() {
            throw std::runtime_error("Invalid tag. Terminating now");
        }

        void DestroyIfNeeded() noexcept {
            switch (tag_) {
                case Tag::Null: {
                    payload_.null.~Null();
                    break;
                }
                case Tag::Boolean: {
                    payload_.boolean.~Boolean();
                    break;
                }
                case Tag::Integer: {
                    payload_.integer.~Integer();
                    break;
                }
                case Tag::Number: {
                    payload_.number.~Number();
                    break;
                }
                case Tag::String: {
                    payload_.string.~String();
                    break;
                }
                case Tag::Array: {
                    payload_.array.~Array();
                    break;
                }
                case Tag::Blob: {
                    payload_.blob.~Blob();
                    break;
                }
                case Tag::Object: {
                    payload_.object.~Object();
                    break;
                }
                case Tag::Undefined: {
                    payload_.undefined.~Undefined();
                    break;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
            tag_ = Tag::Undefined;
        }

        template <class Type, class... Args>
        void EmplaceRaw(Args&&... args) {
            new (std::addressof(payload_)) Type{std::forward<Args>(args)...};
            tag_ = TagOf<Type>();
        }

        Tag tag_ = Tag::Undefined;
        Payload payload_{};

       private:
        friend BasicDynamic;
    };

   protected:
    BasicDynamic(ImplWrapper<Impl> impl) : impl_(std::move(impl)) {}

   public:
    BasicDynamic() : BasicDynamic(detail::DefaultOf<ImplWrapper<Impl>>()) {}

    template <class Type, class... Args>
    DNODISCARD static BasicDynamic From(Args&&... args) {
        BasicDynamic dynamic;
        dynamic.Emplace<Type>(std::forward<Args>(args)...);
        return dynamic;
    }

    template <class Type, class... Args>
    void Emplace(Args&&... args) {
        GetImpl().template Emplace<Type>(std::forward<Args>(args)...);
    }

    DNODISCARD constexpr bool IsNull() const noexcept {
        return GetImpl().HoldsNull();
    }
    DNODISCARD constexpr bool IsBoolean() const noexcept {
        return GetImpl().HoldsBool();
    }
    DNODISCARD constexpr bool IsInteger() const noexcept {
        return GetImpl().HoldsInteger();
    }
    DNODISCARD constexpr bool IsNumber() const noexcept {
        return GetImpl().HoldsNumber();
    }
    DNODISCARD constexpr bool IsString() const noexcept {
        return GetImpl().HoldsString();
    }
    DNODISCARD constexpr bool IsBlob() const noexcept {
        return GetImpl().HoldsBlob();
    }
    DNODISCARD constexpr bool IsArray() const noexcept {
        return GetImpl().HoldsArray();
    }
    DNODISCARD constexpr bool IsObject() const noexcept {
        return GetImpl().HoldsObject();
    }
    DNODISCARD constexpr bool IsUndefined() const noexcept {
        return GetImpl().HoldsUndefined();
    }

    DNODISCARD DCONSTEXPR_14 Null GetNull() const {
        return GetImpl().GetNull();
    }

    DNODISCARD DCONSTEXPR_14 Boolean GetBoolean() const {
        return GetImpl().GetBoolean();
    }

    DNODISCARD DCONSTEXPR_14 Integer GetInteger() const {
        return GetImpl().GetInteger();
    }

    DNODISCARD DCONSTEXPR_14 Number GetNumber() const {
        return GetImpl().GetNumber();
    }

    DNODISCARD DCONSTEXPR_14 String& GetString() {
        return GetImpl().GetString();
    }
    DNODISCARD DCONSTEXPR_14 const String& GetString() const {
        return GetImpl().GetString();
    }

    DNODISCARD DCONSTEXPR_14 Blob& GetBlob() { return GetImpl().GetBlob(); }
    DNODISCARD DCONSTEXPR_14 const Blob& GetBlob() const {
        return GetImpl().GetBlob();
    }

    DNODISCARD DCONSTEXPR_14 Array& GetArray() { return GetImpl().GetArray(); }
    DNODISCARD DCONSTEXPR_14 const Array& GetArray() const {
        return GetImpl().GetArray();
    }

    DNODISCARD DCONSTEXPR_14 Object& GetObject() {
        return GetImpl().GetObject();
    }
    DNODISCARD DCONSTEXPR_14 const Object& GetObject() const {
        return GetImpl().GetObject();
    }

    DNODISCARD DCONSTEXPR_14 Undefined GetUndefined() const {
        return GetImpl().GetUndefined();
    }

    template <class CastType>
    DNODISCARD CastType& As() {
        return GetImpl().template As<CastType>();
    }

    template <class CastType>
    DNODISCARD const CastType& As() const {
        return GetImpl().template As<CastType>();
    }

    template <class CastType>
    DNODISCARD operator CastType&() {
        return As<CastType>();
    }

    template <class CastType>
    DNODISCARD operator const CastType&() const {
        return As<CastType>();
    }

    template <class CastType>
    DNODISCARD operator CastType() const {
        return As<CastType>();
    }

    DNODISCARD BasicDynamic Clone() const {
        return BasicDynamic{
            detail::DefaultFactory<ImplWrapper<Impl>>{}(GetImpl().Clone())};
    }

    DNODISCARD DCONSTEXPR_14 bool Equals(
        const BasicDynamic& rhs) const noexcept {
        if (GetImpl().tag_ != rhs.GetImpl().tag_) {
            return false;
        }
        return GetImpl().Equals(rhs.GetImpl());
    }

    template <class Type>
    DNODISCARD DCONSTEXPR_14 bool Equals(const Type& that) const noexcept {
        using CastType = typename BestFitFor<typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type>::type;
        if (!GetImpl().template Holds<CastType>()) {
            return false;
        }
        return As<CastType>() == that;
    }

    DNODISCARD friend DCONSTEXPR_14 bool operator==(
        const BasicDynamic& lhs, const BasicDynamic& rhs) noexcept {
        return lhs.Equals(rhs);
    }
    template <class Type>
    DNODISCARD friend DCONSTEXPR_14 bool operator==(const BasicDynamic& lhs,
                                                    const Type& rhs) noexcept {
        return lhs.Equals(rhs);
    }
    template <class Type>
    DNODISCARD friend DCONSTEXPR_14 bool operator==(
        const Type& lhs, const BasicDynamic& rhs) noexcept {
        return rhs == lhs;
    }

    template <class Type>
    BasicDynamic& operator=(Type&& value) {
        Emplace<typename BestFitFor<typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type>::type>(
            std::forward<Type>(value));
        return *this;
    }

    template <class Key>
    DNODISCARD BasicDynamic& operator[](const Key& k) {
        if (IsArray()) {
            decltype(auto) index = ToIndex{}.Convert(k);
            return AtIndex(index);
        } else if (IsObject()) {
            decltype(auto) key = ToString{}.template Convert<std::string>(k);
            return AtKey(key);
        } else {
            InvalidAccess();
        }
    }
    template <class Key>
    DNODISCARD const BasicDynamic& operator[](const Key& k) const {
        if (IsArray()) {
            decltype(auto) index = ToIndex{}.Convert(k);
            return AtIndex(index);
        } else if (IsObject()) {
            decltype(auto) key = ToString{}.template Convert<std::string>(k);
            return AtKey(key);
        } else {
            InvalidAccess();
        }
    }

    template <class Key>
    DNODISCARD BasicDynamic& At(const Key& key) {
        return As<Object>().at(key);
    }

    template <class Key>
    DNODISCARD const BasicDynamic& At(const Key& key) const {
        return As<Object>().at(key);
    }

   private:
    template <class Key>
    DNODISCARD BasicDynamic& AtKey(const Key& key) {
        return As<Object>()[key];
    }
    template <class Key>
    DNODISCARD const BasicDynamic& AtKey(const Key& key) const {
        return As<Object>().at(key);
    }

   public:
    DNODISCARD BasicDynamic& AtIndex(const std::size_t index) {
        return As<Array>().at(index);
    }
    DNODISCARD const BasicDynamic& AtIndex(const std::size_t index) const {
        return As<Array>().at(index);
    }

    template <class Type>
    void Push(Type&& value) {
        auto& array = As<Array>();
        array.emplace_back(
            BasicDynamic::From<typename BestFitFor<typename std::remove_cv<
                typename std::remove_reference<Type>::type>::type>::type>(
                std::forward<Type>(value)));
    }

    DNODISCARD BasicDynamic Pop() {
        auto& array = As<Array>();
        auto back = std::move(array.back());
        array.pop_back();
        return back;
    }

    template <class Key>
    DNODISCARD bool Contains(const Key& key) const {
        const auto& self = As<Object>();
        return self.find(key) != self.end();
    }

   private:
    [[noreturn]]
    static void InvalidAccess() {
        throw InvalidAccessException("Invalid access attempted");
    }

    auto GetImpl() noexcept -> Impl& { return *detail::PointerOf(impl_); }

    auto GetImpl() const noexcept -> const Impl& {
        return *detail::PointerOf(impl_);
    }

   private:
    ImplWrapper<Impl> impl_;
};

using Dynamic = BasicDynamic<DefaultInteger, DefaultNumber, DefaultString,
                             DefaultBlobContainer, DefaultArrayContainer,
                             DefaultObjectContainer, DefaultToString,
                             DefaultToIndex, detail::Just>;

using DynamicManaged =
    BasicDynamic<DefaultInteger, DefaultNumber, DefaultString,
                 DefaultBlobContainer, DefaultArrayContainer,
                 DefaultObjectContainer, DefaultToString, DefaultToIndex,
                 std::shared_ptr>;

}  // namespace dynamicxx

#endif  // DYNAMICXX_DYNAMICXX_H
