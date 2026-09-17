#pragma once
#include <stdexcept>
#include <string>

namespace mt {

struct MtError : std::runtime_error { using std::runtime_error::runtime_error; };
struct ConfigError    : MtError { using MtError::MtError; };
struct DataError      : MtError { using MtError::MtError; };
struct CudaError      : MtError { using MtError::MtError; };
struct NumericError   : MtError { using MtError::MtError; };
struct ContractError  : MtError { using MtError::MtError; };  // evolution / determinism violation

}  // namespace mt
