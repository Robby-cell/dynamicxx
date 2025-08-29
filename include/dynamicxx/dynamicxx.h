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

class InvalidAccessException : std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

class Dynamic {
   public:
    struct Null {};
    using Integer = std::int64_t;
    using Number = double;
    using String = std::string;
    using Array = std::vector<Dynamic>;
    using Object = std::unordered_map<std::string, Dynamic>;
    struct Invalid {};

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
                case Tag::Integer:
                case Tag::Number: {
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
        Payload payload_{};
        Tag tag_ = Tag::Invalid;
    };

   public:
    Dynamic() {}

    template <class Type, class... Args>
    static Dynamic From(Args&&... args) {
        Dynamic dynamic;
        dynamic.Emplace<Type>(std::forward<Args>(args)...);
        return dynamic;
    }

    template <class Type, class... Args>
    void Emplace(Args&&... args) {
        impl_.Emplace<Type>(std::forward<Args>(args)...);
    }

    Null null;
    Integer integer;
    Number number;
    String string;
    Array array;
    Object object;
    Invalid invalid = {};

    DCONSTEXPR_14 Integer GetInteger() const { return impl_.GetInteger(); }

    DCONSTEXPR_14 Number GetNumber() const { return impl_.GetNumber(); }

    DCONSTEXPR_14 String& GetString() { return impl_.GetString(); }
    DCONSTEXPR_14 const String& GetString() const { return impl_.GetString(); }

    DCONSTEXPR_14 Array& GetArray() { return impl_.GetArray(); }
    DCONSTEXPR_14 const Array& GetArray() const { return impl_.GetArray(); }

    DCONSTEXPR_14 Object& GetObject() { return impl_.GetObject(); }
    DCONSTEXPR_14 const Object& GetObject() const { return impl_.GetObject(); }

    DCONSTEXPR_14 Null GetNull() const { return impl_.GetNull(); }

   private:
    [[noreturn]]
    static void InvalidAccess() {
        throw InvalidAccessException("Invalid access attempted");
    }

   private:
    Impl impl_;
};

}  // namespace dynamicxx

#endif  // DYNAMICXX_DYNAMICXX_H
