#ifndef BRA_GATE_ADJ_TOFFOLI_HPP
# define BRA_GATE_ADJ_TOFFOLI_HPP

# include <boost/config.hpp>

# include <string>
# include <iosfwd>

# include <bra/gate/gate.hpp>
# include <bra/state.hpp>

# ifdef BOOST_NO_CXX11_FINAL
#   define final 
#   define override 
# endif // BOOST_NO_CXX11_FINAL


namespace bra
{
  namespace gate
  {
    class adj_toffoli final
      : public ::bra::gate::gate
    {
     public:
      typedef ::bra::state::qubit_type qubit_type;
      typedef ::bra::state::control_qubit_type control_qubit_type;

     private:
      qubit_type target_qubit_;
      control_qubit_type control_qubit1_;
      control_qubit_type control_qubit2_;

      static std::string const name_;

     public:
      adj_toffoli(
        qubit_type const target_qubit,
        control_qubit_type const control_qubit1, control_qubit_type const control_qubit2);
# ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
      ~adj_toffoli() = default;
# else
      ~adj_toffoli() { }
# endif

     private:
# ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
      adj_toffoli(adj_toffoli const&) = delete;
      adj_toffoli& operator=(adj_toffoli const&) = delete;
#   ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
      adj_toffoli(adj_toffoli&&) = delete;
      adj_toffoli& operator=(adj_toffoli&&) = delete;
#   endif // BOOST_NO_CXX11_RVALUE_REFERENCES
# else // BOOST_NO_CXX11_DELETED_FUNCTIONS
      adj_toffoli(adj_toffoli const&);
      adj_toffoli& operator=(adj_toffoli const&);
#   ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
      adj_toffoli(adj_toffoli&&);
      adj_toffoli& operator=(adj_toffoli&&);
#   endif // BOOST_NO_CXX11_RVALUE_REFERENCES
# endif // BOOST_NO_CXX11_DELETED_FUNCTIONS

     private:
      ::bra::state& do_apply(::bra::state& state) const override;
      std::string const& do_name() const override;
      std::string do_representation(
        std::ostringstream& repr_stream, int const parameter_width) const override;
    };
  }
}


# ifdef BOOST_NO_CXX11_FINAL
#   undef final 
#   undef override 
# endif // BOOST_NO_CXX11_FINAL

#endif

