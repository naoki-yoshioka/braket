#ifndef KET_MPI_GATE_PAGE_PROJECTIVE_MEASUREMENT_HPP
# define KET_MPI_GATE_PAGE_PROJECTIVE_MEASUREMENT_HPP

# include <boost/config.hpp>

# include <cmath>
# include <utility>

# include <boost/math/constants/constants.hpp>
# include <boost/range/value_type.hpp>

# include <ket/qubit.hpp>
# include <ket/utility/meta/real_of.hpp>
# include <ket/mpi/permutated.hpp>
# include <ket/mpi/state.hpp>
# include <ket/mpi/gate/page/unsupported_page_gate_operation.hpp>
# include <ket/mpi/gate/page/detail/one_page_qubit_gate.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace page
      {
        // zero_one_probabilities
        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename StateInteger, typename BitInteger>
        [[noreturn]] inline
        std::pair<
          typename ::ket::utility::meta::real_of<
            typename boost::range_value<RandomAccessRange>::type>::type,
          typename ::ket::utility::meta::real_of<
            typename boost::range_value<RandomAccessRange>::type>::type>
        zero_one_probabilities(
          ParallelPolicy const,
          RandomAccessRange const& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"zero_one_probabilities"}; }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger>
        [[noreturn]] inline
        std::pair<
          typename ::ket::utility::meta::real_of<Complex>::type,
          typename ::ket::utility::meta::real_of<Complex>::type>
        zero_one_probabilities(
          ParallelPolicy const,
          ::ket::mpi::state<Complex, false, Allocator> const& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"zero_one_probabilities"}; }

        namespace projective_measurement_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          struct zero_one_probabilities
          {
            long double& zero_probability_;
            long double& one_probability_;

            zero_one_probabilities(
              long double& zero_probability, long double& one_probability)
              : zero_probability_{zero_probability},
                one_probability_{one_probability}
            { }

            template <typename Iterator, typename StateInteger>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              using std::norm;
              zero_probability_ += static_cast<long double>(norm(*(zero_first + index)));
              one_probability_ += static_cast<long double>(norm(*(one_first + index)));
            }
          }; // struct zero_one_probabilities
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace projective_measurement_detail

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger>
        inline
        std::pair<
          typename ::ket::utility::meta::real_of<Complex>::type,
          typename ::ket::utility::meta::real_of<Complex>::type>
        zero_one_probabilities(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, true, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit)
        {
          auto zero_probability = 0.0l;
          auto one_probability = 0.0l;

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [&zero_probability, &one_probability](
              auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              using std::norm;
              zero_probability += static_cast<long double>(norm(*(zero_first + index)));
              one_probability += static_cast<long double>(norm(*(one_first + index)));
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            ::ket::mpi::gate::page::projective_measurement_detail::zero_one_probabilities{
              zero_probability, one_probability});
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

          using real_type = typename ::ket::utility::meta::real_of<Complex>::type;
          return std::make_pair(
            static_cast<real_type>(zero_probability), static_cast<real_type>(one_probability));
        }

        // change_state_after_measuring_zero
        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename StateInteger, typename BitInteger, typename Real>
        [[noreturn]] inline void change_state_after_measuring_zero(
          ParallelPolicy const,
          RandomAccessRange& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const,
          Real const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"change_state_after_measuring_zero"}; }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger, typename Real>
        [[noreturn]] inline void change_state_after_measuring_zero(
          ParallelPolicy const,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const,
          Real const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"change_state_after_measuring_zero"}; }

        namespace projective_measurement_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Complex, typename Real>
          struct change_state_after_measuring_zero
          {
            Real multiplier_;

            explicit change_state_after_measuring_zero(Real const& multiplier)
              : multiplier_{multiplier}
            { }

            template <typename Iterator, typename StateInteger>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              *(zero_first + index) *= multiplier_;
              *(one_first + index) = Complex{Real{0}};
            }
          }; // struct change_state_after_measuring_zero<Complex, Real>

          template <typename Complex, typename Real>
          inline ::ket::mpi::gate::page::projective_measurement_detail::change_state_after_measuring_zero<Complex, Real>
          make_change_state_after_measuring_zero(Real const& multiplier)
          { return ::ket::mpi::gate::page::projective_measurement_detail::change_state_after_measuring_zero<Complex, Real>{multiplier}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace projective_measurement_detail

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger, typename Real>
        inline void change_state_after_measuring_zero(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, true, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit,
          Real const zero_probability)
        {
          using std::pow;
          using boost::math::constants::half;
          auto const multiplier = pow(zero_probability, -half<Real>());

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [multiplier](auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              *(zero_first + index) *= multiplier;
              *(one_first + index) = Complex{Real{0}};
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            ::ket::mpi::gate::page::projective_measurement_detail::make_change_state_after_measuring_zero<Complex>(multiplier));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }

        // change_state_after_measuring_one
        template <
          typename ParallelPolicy,
          typename RandomAccessRange,
          typename StateInteger, typename BitInteger, typename Real>
        [[noreturn]] inline void change_state_after_measuring_one(
          ParallelPolicy const,
          RandomAccessRange& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const,
          Real const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"change_state_after_measuring_one"}; }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger, typename Real>
        [[noreturn]] inline void change_state_after_measuring_one(
          ParallelPolicy const,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const,
          Real const)
        { throw ::ket::mpi::gate::page::unsupported_page_gate_operation{"change_state_after_measuring_one"}; }

        namespace projective_measurement_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Complex, typename Real>
          struct change_state_after_measuring_one
          {
            Real multiplier_;

            explicit change_state_after_measuring_one(Real const& multiplier)
              : multiplier_{multiplier}
            { }

            template <typename Iterator, typename StateInteger>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              *(zero_first + index) = Complex{Real{0}};
              *(one_first + index) *= multiplier_;
            }
          }; // struct change_state_after_measuring_one<Complex, Real>

          template <typename Complex, typename Real>
          inline ::ket::mpi::gate::page::projective_measurement_detail::change_state_after_measuring_one<Complex, Real>
          make_change_state_after_measuring_one(Real const& multiplier)
          { return ::ket::mpi::gate::page::projective_measurement_detail::change_state_after_measuring_one<Complex, Real>{multiplier}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace projective_measurement_detail

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename StateInteger, typename BitInteger, typename Real>
        inline void change_state_after_measuring_one(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, true, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit,
          Real const one_probability)
        {
          using std::pow;
          using boost::math::constants::half;
          auto const multiplier = pow(one_probability, -half<Real>());

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [multiplier](auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              *(zero_first + index) = Complex{Real{0}};
              *(one_first + index) *= multiplier;
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            ::ket::mpi::gate::page::projective_measurement_detail::make_change_state_after_measuring_one<Complex>(multiplier));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }
      } // namespace page
    } // namespace gate
  } // namespace mpi
} // namespace ket


#endif // KET_MPI_GATE_PAGE_PROJECTIVE_MEASUREMENT_HPP
