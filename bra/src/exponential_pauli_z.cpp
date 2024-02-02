#include <string>
#include <ios>
#include <iomanip>
#include <sstream>

#include <ket/qubit_io.hpp>

#include <bra/gate/gate.hpp>
#include <bra/gate/exponential_pauli_z.hpp>
#include <bra/state.hpp>


namespace bra
{
  namespace gate
  {
    std::string const exponential_pauli_z::name_ = "eZ";

    exponential_pauli_z::exponential_pauli_z(real_type const phase, qubit_type const qubit)
      : ::bra::gate::gate{}, phase_{phase}, qubit_{qubit}
    { }

    ::bra::state& exponential_pauli_z::do_apply(::bra::state& state) const
    { return state.exponential_pauli_z(phase_, qubit_); }

    std::string const& exponential_pauli_z::do_name() const { return name_; }
    std::string exponential_pauli_z::do_representation(
      std::ostringstream& repr_stream, int const parameter_width) const
    {
      repr_stream
        << std::right
        << std::setw(parameter_width) << qubit_
        << std::setw(parameter_width) << phase_;
      return repr_stream.str();
    }
  } // namespace gate
} // namespace bra
