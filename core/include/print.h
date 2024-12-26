///--- The Helix Project ------------------------------------------------------------------------///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
///------------------------------------------------------------------------------------ Helix ---///

#ifndef __$LIBHELIX_PRINT__
#define __$LIBHELIX_PRINT__

#include "concepts.h"
#include "config.h"
#include "types.h"
#include "libc.h"
#include "refs.h"
#include "traits.h"
#include "primitives.h"

H_NAMESPACE_BEGIN
H_STD_NAMESPACE_BEGIN

class endl {
  public:
    endl &operator=(const endl &) = delete;
    endl &operator=(endl &&)      = delete;
    endl(const endl &)            = delete;
    endl(endl &&)                 = delete;

    endl()  = default;
    ~endl() = default;

    explicit endl(string end)
        : end_l(std::ref::move(end)) {}

    explicit endl(const char *end)
        : end_l((end != nullptr) ? end : "\n") {}

    explicit endl(char end)
        : end_l(1, end) {}

    friend LIBCXX_NAMESPACE::ostream &operator<<(LIBCXX_NAMESPACE::ostream &oss, const endl &end) {
        oss << end.end_l;
        return oss;
    }

  private:
    string end_l = "\n";
};

/// \include belongs to the helix standard library.
/// \brief convert any type to a string
///
/// This function will try to convert the argument to a string using the following methods:
/// - if the argument has a to_string method, it will use that
/// - if the argument has a ostream operator, it will use that
/// - if the argument is an arithmetic type, it will use std::to_string
/// - if all else fails, it will convert the address of the argument to a string
///
template <typename Ty>
constexpr string to_string(Ty &&t) {
    if constexpr (std::concepts::HasToString<Ty>) {
        return t.to_string();
    } else if constexpr (std::concepts::SupportsOStream<Ty>) {
        LIBCXX_NAMESPACE::stringstream ss;
        ss << t;
        return ss.str();
    } else if constexpr (std::concepts::SafelyCastable<Ty, string>) {
        return t.operator$cast(static_cast<string *>(nullptr));
    } else if constexpr (std::traits::is_same_v<Ty, bool>) {
        return t ? "true" : "false";
    } else if constexpr (LIBCXX_NAMESPACE::is_arithmetic_v<Ty>) {
        return LIBCXX_NAMESPACE::to_string(t);
    } else {
        LIBCXX_NAMESPACE::stringstream ss;

#ifdef _MSC_VER
        ss << "[" << typeid(t).name() << " at 0x" << LIBCXX_NAMESPACE::hex << &t << "]";
#else
        int   st;
        char *rn = libc::abi::__cxa_demangle(typeid(t).name(), 0, 0, &st);
        ss << "[" << rn << " at 0x" << LIBCXX_NAMESPACE::hex << &t << "]";
        free(rn);
#endif

        return ss.str();
    }
}

/// \include belongs to the helix standard library.
/// \brief format a string with arguments
///
/// TODO: = is not yet suppoted
///
/// the following calls can happen in helix and becomes the following c++:
///
/// f"hi: {var}"   -> stringf("hi: \{\}", var)
/// f"hi: {var1=}" -> stringf("hi: var1=\{\}", var1)
///
/// f"hi: {(some_expr() + 12)=}" -> stringf("hi: (some_expr() + 12)=\{\}", some_expr())
/// f"hi: {some_expr() + 12}"    -> stringf("hi: \{\}", some_expr() + 12)
///
template <typename... Ty>
constexpr string stringf(string s, Ty &&...t) {
    const array<string, sizeof...(t)> EAS = {to_string(std::forward<Ty>(t))...};

    usize pos = 0;

#ifdef __GNUG__
#pragma unroll
#endif

    for (auto &&arg : EAS) {
        pos = s.find("\\{\\}", pos);

        if (pos == string::npos) [[unlikely]] {
            throw LIBCXX_NAMESPACE::runtime_error(
                "error: [f-stirng engine]: format argument count mismatch, this should not "
                "happen, please open a issue on github");
        }

        s.replace(pos, 4, arg);
        pos += arg.size();
    }

    return s;
}

H_STD_NAMESPACE_END

template <typename... Args>
inline constexpr void print(Args &&...t) {
    if constexpr (sizeof...(t) == 0) {
        printf("\n");
        return;
    }

    ((printf("%s", std::to_string(std::forward<Args>(t)).c_str())), ...);

    if constexpr (sizeof...(t) > 0) {
        if constexpr (!std::traits::same_as_v<
                          std::ref::remove_cv_t<std::ref::remove_ref_t<
                              decltype(LIBCXX_NAMESPACE::get<sizeof...(t) - 1>(tuple<Args...>(t...)))>>,
                          std::endl>) {
            printf("\n");
        }
    }
}

H_NAMESPACE_END
#endif