#ifndef DYNAMICXX_DYNAMICXX_H
#define DYNAMICXX_DYNAMICXX_H

#include <type_traits>
#ifdef _MSVC_LANG
#define DCXX_LANG (_MSVC_LANG)
#else
#define DCXX_LANG (__cplusplus)
#endif

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define HAS_CXX_14 (DCXX_LANG >= 201402UL)
#define HAS_CXX_17 (DCXX_LANG >= 201703UL)
#define HAS_CXX_20 (DCXX_LANG >= 202002UL)

#if HAS_CXX_14
#define DCONSTEXPR_14 constexpr
#else
#define DCONSTEXPR_14
#endif

#if HAS_CXX_20
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#else
#define LIKELY
#define UNLIKELY
#endif

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

    static_assert(std::is_integral<Integer>::value,
                  "Integer type provided must be an integral type");
    static_assert(std::is_floating_point<Number>::value,
                  "Number type provided must be a floating point type");

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
    static constexpr Tag TagOf() noexcept {
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

        constexpr bool HoldsInteger() const noexcept {
            return Holds<Integer>();
        }
        constexpr bool HoldsNumber() const noexcept { return Holds<Number>(); }
        constexpr bool HoldsString() const noexcept { return Holds<String>(); }
        constexpr bool HoldsArray() const noexcept { return Holds<Array>(); }
        constexpr bool HoldsObject() const noexcept { return Holds<Object>(); }
        constexpr bool HoldsNull() const noexcept { return Holds<Null>(); }

        DCONSTEXPR_14 Integer GetInteger() const {
            if (HoldsInteger()) LIKELY {
                    return payload_.integer;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DCONSTEXPR_14 Number GetNumber() const {
            if (HoldsNumber()) LIKELY {
                    return payload_.number;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DCONSTEXPR_14 String& GetString() {
            if (HoldsString()) LIKELY {
                    return payload_.string;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DCONSTEXPR_14 const String& GetString() const {
            if (HoldsString()) LIKELY {
                    return payload_.string;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DCONSTEXPR_14 Array& GetArray() {
            if (HoldsArray()) LIKELY {
                    return payload_.array;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DCONSTEXPR_14 const Array& GetArray() const {
            if (HoldsArray()) LIKELY {
                    return payload_.array;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DCONSTEXPR_14 Object& GetObject() {
            if (HoldsObject()) LIKELY {
                    return payload_.object;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }
        DCONSTEXPR_14 const Object& GetObject() const {
            if (HoldsObject()) LIKELY {
                    return payload_.object;
                }
            else
                UNLIKELY { InvalidAccess(); }
        }

        DCONSTEXPR_14 Null GetNull() const {
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
            static Integer& As(Impl& impl) { return impl.payload_.integer; }
            static const Integer& As(const Impl& impl) {
                return impl.payload_.integer;
            }
        };
        template <>
        struct Caster<Number> {
            static Number& As(Impl& impl) { return impl.payload_.number; }
            static const Number& As(const Impl& impl) {
                return impl.payload_.number;
            }
        };
        template <>
        struct Caster<String> {
            static String& As(Impl& impl) { return impl.payload_.string; }
            static const String& As(const Impl& impl) {
                return impl.payload_.string;
            }
        };
        template <>
        struct Caster<Array> {
            static Array& As(Impl& impl) { return impl.payload_.array; }
            static const Array& As(const Impl& impl) {
                return impl.payload_.array;
            }
        };
        template <>
        struct Caster<Object> {
            static Object& As(Impl& impl) { return impl.payload_.object; }
            static const Object& As(const Impl& impl) {
                return impl.payload_.object;
            }
        };

       public:
        template <class CastType>
        CastType& As() {
            if (Holds<CastType>()) {
                return Caster<CastType>::As(*this);
            } else {
                InvalidAccess();
            }
        }

        template <class CastType>
        const CastType& As() const {
            if (Holds<CastType>()) {
                return Caster<CastType>::As(*this);
            } else {
                InvalidAccess();
            }
        }

       private:
        template <class WantedTag>
        constexpr bool Holds() const noexcept {
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

       public:
        Tag tag_ = Tag::Invalid;
        Payload payload_{};
    };

   public:
    BasicDynamic() {}

    template <class Type, class... Args>
    static BasicDynamic From(Args&&... args) {
        BasicDynamic dynamic;
        dynamic.Emplace<Type>(std::forward<Args>(args)...);
        return dynamic;
    }

    template <class Type, class... Args>
    void Emplace(Args&&... args) {
        impl_.template Emplace<Type>(std::forward<Args>(args)...);
    }

    DCONSTEXPR_14 Integer GetInteger() const { return impl_.GetInteger(); }

    DCONSTEXPR_14 Number GetNumber() const { return impl_.GetNumber(); }

    DCONSTEXPR_14 String& GetString() { return impl_.GetString(); }
    DCONSTEXPR_14 const String& GetString() const { return impl_.GetString(); }

    DCONSTEXPR_14 Array& GetArray() { return impl_.GetArray(); }
    DCONSTEXPR_14 const Array& GetArray() const { return impl_.GetArray(); }

    DCONSTEXPR_14 Object& GetObject() { return impl_.GetObject(); }
    DCONSTEXPR_14 const Object& GetObject() const { return impl_.GetObject(); }

    DCONSTEXPR_14 Null GetNull() const { return impl_.GetNull(); }

    template <class CastType>
    CastType& As() {
        return impl_.template As<CastType>();
    }

    template <class CastType>
    const CastType& As() const {
        return impl_.template As<CastType>();
    }

    template <class CastType>
    operator CastType&() {
        return As<CastType>();
    }

    template <class CastType>
    operator const CastType&() const {
        return As<CastType>();
    }

    template <class CastType>
    operator CastType() const {
        return As<CastType>();
    }

    template <class Type>
    BasicDynamic& operator=(const Type& value) {
        Emplace<typename BestFitFor<Type>::type>(value);
        return *this;
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
