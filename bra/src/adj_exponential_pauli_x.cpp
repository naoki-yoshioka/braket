#include <string>
#include <ios>
#include <iomanip>
#include <sstream>

#include <ket/qubit_io.hpp>

#include <bra/gate/gate.hpp>
#include <bra/gate/adj_exponential_pauli_x.hpp>
#include <bra/state.hpp>


namespace bra
{
  namespace gate
  {
    std::string const adj_exponential_pauli_x::name_ = "eX+";

    adj_exponential_pauli_x::adj_exponential_pauli_x(real_type const phase, qubit_type const qubit)
      : ::bra::gate::gate{}, phase_{phase}, qubit_{qubit}
    { }

    ::bra::state& adj_exponential_pauli_x::do_apply(::bra::state& state) const
    { return state.adj_exponential_pauli_x(phase_, qubit_); }

    std::string const& adj_exponential_pauli_x::do_name() const { return name_; }
    std::string adj_exponential_pauli_x::do_representation(
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
