#ifndef KET_MPI_GATE_PAGE_Y_ROTATION_HALF_PI_HPP
# define KET_MPI_GATE_PAGE_Y_ROTATION_HALF_PI_HPP

# include <boost/config.hpp>

# include <boost/math/constants/constants.hpp>
# include <boost/range/value_type.hpp>

# include <ket/qubit.hpp>
# include <ket/utility/meta/real_of.hpp>
# include <ket/mpi/permutated.hpp>
# include <ket/mpi/gate/page/detail/one_page_qubit_gate.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace page
      {
        namespace y_rotation_half_pi_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Real>
          struct y_rotation_half_pi
          {
            template <typename Iterator, typename StateInteger>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              auto const zero_iter = zero_first + index;
              auto const one_iter = one_first + index;
              auto const zero_iter_value = *zero_iter;

              using boost::math::constants::one_div_root_two;
              *zero_iter += *one_iter;
              *zero_iter *= one_div_root_two<Real>();
              *one_iter -= zero_iter_value;
              *one_iter *= one_div_root_two<Real>();
            }
          }; // struct y_rotation_half_pi<Real>
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace y_rotation_half_pi_detail

        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename StateInteger, typename BitInteger>
        inline RandomAccessRange& y_rotation_half_pi(
          ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit)
        {
          using real_type = typename ::ket::utility::meta::real_of<typename boost::range_value<RandomAccessRange>::type>::type;

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [](auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              auto const zero_iter = zero_first + index;
              auto const one_iter = one_first + index;
              auto const zero_iter_value = *zero_iter;

              using boost::math::constants::one_div_root_two;
              *zero_iter += *one_iter;
              *zero_iter *= one_div_root_two<real_type>();
              *one_iter -= zero_iter_value;
              *one_iter *= one_div_root_two<real_type>();
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            ::ket::mpi::gate::page::y_rotation_half_pi_detail::y_rotation_half_pi<real_type>{});
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }

        namespace y_rotation_half_pi_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Real>
          struct adj_y_rotation_half_pi
          {
            template <typename Iterator, typename StateInteger>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              auto const zero_iter = zero_first + index;
              auto const one_iter = one_first + index;
              auto const zero_iter_value = *zero_iter;

              using boost::math::constants::one_div_root_two;
              *zero_iter -= *one_iter;
              *zero_iter *= one_div_root_two<Real>();
              *one_iter += zero_iter_value;
              *one_iter *= one_div_root_two<Real>();
            }
          }; // struct adj_y_rotation_half_pi<Real>
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace y_rotation_half_pi_detail

        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename StateInteger, typename BitInteger>
        inline RandomAccessRange& adj_y_rotation_half_pi(
          ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit)
        {
          using real_type = typename ::ket::utility::meta::real_of<typename boost::range_value<RandomAccessRange>::type>::type;

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [](auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              auto const zero_iter = zero_first + index;
              auto const one_iter = one_first + index;
              auto const zero_iter_value = *zero_iter;

              using boost::math::constants::one_div_root_two;
              *zero_iter -= *one_iter;
              *zero_iter *= one_div_root_two<real_type>();
              *one_iter += zero_iter_value;
              *one_iter *= one_div_root_two<real_type>();
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            ::ket::mpi::gate::page::y_rotation_half_pi_detail::adj_y_rotation_half_pi<real_type>{});
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }
      } // namespace page
    } // namespace gate
  } // namespace mpi
} // namespace ket


#endif // KET_MPI_GATE_PAGE_Y_ROTATION_HALF_PI_HPP
