#pragma once
#include <string_view>

namespace mt {

enum class AuthorityRegime {
    FiniteClosedRegime,
    OpenButAuthoritativeIntersticeRegime,
    RejectedNonAuthoritativeRegime
};

inline std::string_view authority_regime_name(AuthorityRegime r) {
    switch (r) {
        case AuthorityRegime::FiniteClosedRegime:
            return "finite_closed_regime";
        case AuthorityRegime::OpenButAuthoritativeIntersticeRegime:
            return "open_but_authoritative_interstice_regime";
        case AuthorityRegime::RejectedNonAuthoritativeRegime:
        default:
            return "rejected_non_authoritative_regime";
    }
}

}  // namespace mt
