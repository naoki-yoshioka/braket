#ifndef BRA_GATE_X_ROTATION_HALF_PI_HPP
# define BRA_GATE_X_ROTATION_HALF_PI_HPP

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
    class x_rotation_half_pi final
      : public ::bra::gate::gate
    {
     public:
      typedef ::bra::state::qubit_type qubit_type;

     private:
      qubit_type qubit_;

      static std::string const name_;

     public:
      explicit x_rotation_half_pi(qubit_type const qubit);
# ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
      ~x_rotation_half_pi() = default;
# else
      ~x_rotation_half_pi() { }
# endif

     private:
# ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
      x_rotation_half_pi(x_rotation_half_pi const&) = delete;
      x_rotation_half_pi& operator=(x_rotation_half_pi const&) = delete;
#   ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
      x_rotation_half_pi(x_rotation_half_pi&&) = delete;
      x_rotation_half_pi& operator=(x_rotation_half_pi&&) = delete;
#   endif // BOOST_NO_CXX11_RVALUE_REFERENCES
# else // BOOST_NO_CXX11_DELETED_FUNCTIONS
      x_rotation_half_pi(x_rotation_half_pi const&);
      x_rotation_half_pi& operator=(x_rotation_half_pi const&);
#   ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
      x_rotation_half_pi(x_rotation_half_pi&&);
      x_rotation_half_pi& operator=(x_rotation_half_pi&&);
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

