#ifndef BRA_GATE_ADJ_EXPONENTIAL_PAULI_ZZ_HPP
# define BRA_GATE_ADJ_EXPONENTIAL_PAULI_ZZ_HPP

# include <string>
# include <iosfwd>

# include <bra/gate/gate.hpp>
# include <bra/state.hpp>


namespace bra
{
  namespace gate
  {
    class adj_exponential_pauli_zz final
      : public ::bra::gate::gate
    {
     public:
      using qubit_type = ::bra::state::qubit_type;
      using real_type = ::bra::state::real_type;

     private:
      real_type phase_;
      qubit_type qubit1_;
      qubit_type qubit2_;

      static std::string const name_;

     public:
      explicit adj_exponential_pauli_zz(real_type const phase, qubit_type const qubit1, qubit_type const qubit2);

      ~adj_exponential_pauli_zz() = default;
      adj_exponential_pauli_zz(adj_exponential_pauli_zz const&) = delete;
      adj_exponential_pauli_zz& operator=(adj_exponential_pauli_zz const&) = delete;
      adj_exponential_pauli_zz(adj_exponential_pauli_zz&&) = delete;
      adj_exponential_pauli_zz& operator=(adj_exponential_pauli_zz&&) = delete;

     private:
      ::bra::state& do_apply(::bra::state& state) const override;
      std::string const& do_name() const override;
      std::string do_representation(
        std::ostringstream& repr_stream, int const parameter_width) const override;
    }; // class adj_exponential_pauli_zz
  } // namespace gate
} // namespace bra


#endif // BRA_GATE_ADJ_EXPONENTIAL_PAULI_ZZ_HPP
