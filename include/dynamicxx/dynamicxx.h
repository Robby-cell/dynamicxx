#ifndef DYNAMICXX_DYNAMICXX_H
#define DYNAMICXX_DYNAMICXX_H

#include <cassert>
#include <cstdint>
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

}  // namespace detail

class InvalidAccessException : std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

template <class IntegerType, class NumberType, class StringType,
          template <class...> class ArrayContainerType,
          template <class...> class ObjectContainerType>
class BasicDynamic {
   public:
    struct Null {};
    using Integer = IntegerType;
    using Number = NumberType;
    using String = StringType;
    using Array = ArrayContainerType<BasicDynamic>;
    using Object = ObjectContainerType<std::string, BasicDynamic>;
    struct Invalid {};

    static_assert(std::is_integral<Integer>::value,
                  "Integer type provided must be an integral type");
    static_assert(std::is_floating_point<Number>::value,
                  "Number type provided must be a floating point type");

    template <class Type>
    struct BestFitFor;

    template <>
    struct BestFitFor<Integer> : detail::TypeIdentity<Integer> {};
    template <>
    struct BestFitFor<Number> : detail::TypeIdentity<Number> {};
    template <>
    struct BestFitFor<String> : detail::TypeIdentity<String> {};
    template <>
    struct BestFitFor<Array> : detail::TypeIdentity<Array> {};
    template <>
    struct BestFitFor<Object> : detail::TypeIdentity<Object> {};

    template <class Type>
    struct BestFitFor
        : BestFitFor<typename std::conditional<
              std::is_integral<Type>::value, Integer,
              typename std::conditional<
                  std::is_floating_point<Type>::value, Number,
                  typename std::conditional<
                      std::is_constructible<String, Type>::value, String,
                      typename std::conditional<
                          std::is_constructible<Array, Type>::value, Array,
                          typename std::conditional<
                              std::is_constructible<Object, Type>::value,
                              Object, void>::type>::type>::type>::type>::type> {
    };

   private:
    using TagRepr = std::uint32_t;
    enum struct Tag : TagRepr {
        Null = 0,
        Integer,
        Number,
        String,
        Array,
        Object,
        Invalid = ~static_cast<TagRepr>(0),
    };

    template <Tag MyTag>
    struct TagIdentity : public std::integral_constant<Tag, MyTag> {};

    template <class>
    struct TagOfHelper;

    template <>
    struct TagOfHelper<Null> : TagIdentity<Tag::Null> {};
    template <>
    struct TagOfHelper<Integer> : TagIdentity<Tag::Integer> {};
    template <>
    struct TagOfHelper<Number> : TagIdentity<Tag::Number> {};
    template <>
    struct TagOfHelper<String> : TagIdentity<Tag::String> {};
    template <>
    struct TagOfHelper<Array> : TagIdentity<Tag::Array> {};
    template <>
    struct TagOfHelper<Object> : TagIdentity<Tag::Object> {};

    template <class Type>
    DNODISCARD static constexpr Tag TagOf() noexcept {
        return TagOfHelper<Type>::value;
    }

    union Payload {
        Payload() {}
        ~Payload() {}

        Null null;
        Integer integer;
        Number number;
        String string;
        Array array;
        Object object;
        Invalid invalid = {};
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

        DNODISCARD constexpr bool HoldsInteger() const noexcept {
            return Holds<Integer>();
        }
        DNODISCARD constexpr bool HoldsNumber() const noexcept {
            return Holds<Number>();
        }
        DNODISCARD constexpr bool HoldsString() const noexcept {
            return Holds<String>();
        }
        DNODISCARD constexpr bool HoldsArray() const noexcept {
            return Holds<Array>();
        }
        DNODISCARD constexpr bool HoldsObject() const noexcept {
            return Holds<Object>();
        }
        DNODISCARD constexpr bool HoldsNull() const noexcept {
            return Holds<Null>();
        }

        DNODISCARD DCONSTEXPR_14 Integer GetInteger() const {
            if (HoldsInteger()) LIKELY {
                    return payload_.integer;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DNODISCARD DCONSTEXPR_14 Number GetNumber() const {
            if (HoldsNumber()) LIKELY {
                    return payload_.number;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DNODISCARD DCONSTEXPR_14 String& GetString() {
            if (HoldsString()) LIKELY {
                    return payload_.string;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DNODISCARD DCONSTEXPR_14 const String& GetString() const {
            if (HoldsString()) LIKELY {
                    return payload_.string;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DNODISCARD DCONSTEXPR_14 Array& GetArray() {
            if (HoldsArray()) LIKELY {
                    return payload_.array;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DNODISCARD DCONSTEXPR_14 const Array& GetArray() const {
            if (HoldsArray()) LIKELY {
                    return payload_.array;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DNODISCARD DCONSTEXPR_14 Object& GetObject() {
            if (HoldsObject()) LIKELY {
                    return payload_.object;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DNODISCARD DCONSTEXPR_14 const Object& GetObject() const {
            if (HoldsObject()) LIKELY {
                    return payload_.object;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DNODISCARD DCONSTEXPR_14 Null GetNull() const {
            if (HoldsNull()) LIKELY {
                    return payload_.null;
                }
            else
                UNLIKELY { InvalidAccess(); }
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

        friend Caster<Integer>;
        friend Caster<Number>;
        friend Caster<String>;
        friend Caster<Array>;
        friend Caster<Object>;

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
        struct Caster<Object> {
            DNODISCARD static Object& As(Impl& impl) {
                return impl.payload_.object;
            }
            DNODISCARD static const Object& As(const Impl& impl) {
                return impl.payload_.object;
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

        DNODISCARD DCONSTEXPR_14 bool Equals(const Impl& that) const {
            DDO_ASSERT(tag_ == that.tag_);
            switch (tag_) {
                case Tag::Null: {
                    return true;
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
                case Tag::Array: {
                    return payload_.array == that.payload_.array;
                }
                case Tag::Object: {
                    return payload_.object == that.payload_.object;
                }
                case Tag::Invalid: {
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

        void MoveRaw(Impl& that) noexcept {
            tag_ = that.tag_;
            switch (that.tag_) {
                case Tag::Null:
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
                case Tag::Array: {
                    EmplaceRaw<Array>(std::move(that.payload_.array));
                    break;
                }
                case Tag::Object: {
                    EmplaceRaw<Object>(std::move(that.payload_.object));
                    break;
                }
                case Tag::Invalid: {
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
                case Tag::Null:
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
                case Tag::Array: {
                    EmplaceRaw<Array>(that.payload_.array);
                    break;
                }
                case Tag::Object: {
                    EmplaceRaw<Object>(that.payload_.object);
                    break;
                }
                case Tag::Invalid: {
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
                case Tag::Null:
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
                case Tag::Object: {
                    payload_.object.~Object();
                    break;
                }
                case Tag::Invalid: {
                    break;
                }

                default:
                    ThrowInvalidTerminatingTag();
            }
            tag_ = Tag::Invalid;
        }

        template <class Type, class... Args>
        void EmplaceRaw(Args&&... args) {
            new (std::addressof(payload_)) Type{std::forward<Args>(args)...};
            tag_ = TagOf<Type>();
        }

        Tag tag_ = Tag::Invalid;
        Payload payload_{};

       private:
        friend BasicDynamic;
    };

   public:
    BasicDynamic() {}

    template <class Type, class... Args>
    DNODISCARD static BasicDynamic From(Args&&... args) {
        BasicDynamic dynamic;
        dynamic.Emplace<Type>(std::forward<Args>(args)...);
        return dynamic;
    }

    template <class Type, class... Args>
    void Emplace(Args&&... args) {
        impl_.template Emplace<Type>(std::forward<Args>(args)...);
    }

    DNODISCARD constexpr bool IsInteger() const noexcept {
        return impl_.HoldsInteger();
    }
    DNODISCARD constexpr bool IsNumber() const noexcept {
        return impl_.HoldsNumber();
    }
    DNODISCARD constexpr bool IsString() const noexcept {
        return impl_.HoldsString();
    }
    DNODISCARD constexpr bool IsArray() const noexcept {
        return impl_.HoldsArray();
    }
    DNODISCARD constexpr bool IsObject() const noexcept {
        return impl_.HoldsObject();
    }

    DNODISCARD DCONSTEXPR_14 Integer GetInteger() const {
        return impl_.GetInteger();
    }

    DNODISCARD DCONSTEXPR_14 Number GetNumber() const {
        return impl_.GetNumber();
    }

    DNODISCARD DCONSTEXPR_14 String& GetString() { return impl_.GetString(); }
    DNODISCARD DCONSTEXPR_14 const String& GetString() const {
        return impl_.GetString();
    }

    DNODISCARD DCONSTEXPR_14 Array& GetArray() { return impl_.GetArray(); }
    DNODISCARD DCONSTEXPR_14 const Array& GetArray() const {
        return impl_.GetArray();
    }

    DNODISCARD DCONSTEXPR_14 Object& GetObject() { return impl_.GetObject(); }
    DNODISCARD DCONSTEXPR_14 const Object& GetObject() const {
        return impl_.GetObject();
    }

    DNODISCARD DCONSTEXPR_14 Null GetNull() const { return impl_.GetNull(); }

    template <class CastType>
    DNODISCARD CastType& As() {
        return impl_.template As<CastType>();
    }

    template <class CastType>
    DNODISCARD const CastType& As() const {
        return impl_.template As<CastType>();
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

    DNODISCARD DCONSTEXPR_14 bool Equals(
        const BasicDynamic& rhs) const noexcept {
        if (impl_.tag_ != rhs.impl_.tag_) {
            return false;
        }
        return impl_.Equals(rhs.impl_);
    }

    template <class Type>
    DNODISCARD DCONSTEXPR_14 bool Equals(const Type& that) const noexcept {
        using CastType = typename BestFitFor<typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type>::type;
        if (!impl_.template Holds<CastType>()) {
            return false;
        }
        return As<CastType>() == that;
    }

    template <class Type>
    DNODISCARD friend DCONSTEXPR_14 bool operator==(const BasicDynamic& lhs,
                                                    Type&& rhs) noexcept {
        return lhs.Equals(rhs);
    }

    template <class Type>
    BasicDynamic& operator=(Type&& value) {
        Emplace<typename BestFitFor<typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type>::type>(value);
        return *this;
    }

    template <class Key>
    DNODISCARD BasicDynamic& operator[](const Key& key) {
        return As<Object>()[key];
    }

    template <class Key>
    DNODISCARD const BasicDynamic& operator[](const Key& key) const {
        return As<Object>().at(key);
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

   private:
    Impl impl_;
};

using Dynamic = BasicDynamic<std::int64_t, double, std::string, std::vector,
                             std::unordered_map>;

}  // namespace dynamicxx

#endif  // DYNAMICXX_DYNAMICXX_H
