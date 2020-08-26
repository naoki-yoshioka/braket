#include <string>
#include <ios>
#include <iomanip>
#include <sstream>

#include <ket/qubit_io.hpp>

#include <bra/gate/gate.hpp>
#include <bra/gate/clear.hpp>
#include <bra/state.hpp>


namespace bra
{
  namespace gate
  {
    std::string const clear::name_ = "CLEAR";

    clear::clear(qubit_type const qubit)
      : ::bra::gate::gate{}, qubit_{qubit}
    { }

    ::bra::state& clear::do_apply(::bra::state& state) const
    { return state.clear(qubit_); }

    std::string const& clear::do_name() const { return name_; }
    std::string clear::do_representation(
      std::ostringstream& repr_stream, int const parameter_width) const
    {
      repr_stream
        << std::right
        << std::setw(parameter_width) << qubit_;
      return repr_stream.str();
    }
  } // namespace gate
} // namespace bra
