#ifndef KET_MPI_GATE_PAGE_EXPONENTIAL_PAULI_Z_HPP
# define KET_MPI_GATE_PAGE_EXPONENTIAL_PAULI_Z_HPP

# include <boost/config.hpp>

# include <cassert>
# include <cmath>

# include <ket/qubit.hpp>
# include <ket/utility/integer_exp2.hpp>
# include <ket/utility/imaginary_unit.hpp>
# include <ket/mpi/permutated.hpp>
# include <ket/mpi/page/is_on_page.hpp>
# include <ket/mpi/gate/page/detail/two_page_qubits_gate.hpp>
# include <ket/mpi/gate/page/detail/one_page_qubit_gate.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace page
      {
        // 1_p: the qubit of eY is on page
        // eZ_i(s) = exp(is Z_i) = I cos s + i Z_i sin s
        // eZ_1(s) (a_0 |0> + a_1 |1>) = e^{is} a_0 |0> + e^{-is} a_1 |1>
        namespace exponential_pauli_z_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Complex, typename StateInteger>
          struct exponential_pauli_z_coeff1
          {
            Real cos_theta_;
            Real sin_theta_;

            exponential_pauli_z_coeff1(
              Complex const& phase_coefficient, Complex const& conj_phase_coefficient) noexcept
              : phase_coefficient_{phase_coefficient}, conj_phase_coefficient_{conj_phase_coefficient}
            { }

            template <typename Iterator>
            void operator()(Iterator const zero_first, Iterator const one_first, StateInteger const index, int const) const
            {
              *(zero_first + index) *= phase_coefficient_;
              *(one_first + index) *= conj_phase_coefficient_;
            }
          }; // struct exponential_pauli_z_coeff1<Complex, StateInteger>

          template <typename Complex, typename StateInteger>
          inline ::ket::mpi::gate::page::exponential_pauli_z_detail::exponential_pauli_z_coeff1<Complex, StateInteger>
          make_exponential_pauli_z_coeff1(Complex const& phase_coefficient, Complex const& conj_phase_coefficient)
          { return {phase_coefficient, conj_phase_coefficient}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace exponential_pauli_z_detail

        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename Complex, typename StateInteger, typename BitInteger>
        inline RandomAccessRange& exponential_pauli_z_coeff1(
          ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          Complex const& phase_coefficient, // exp(i theta) = cos(theta) + i sin(theta)
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit)
        {
          assert(::ket::mpi::page::is_on_page(permutated_qubit, local_state));

          using std::conj;
          auto const conj_phase_coefficient = conj(phase_coefficient);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
            parallel_policy, local_state, permutated_qubit,
            [&phase_coefficient, &conj_phase_coefficient](
              auto const zero_first, auto const one_first, StateInteger const index, int const)
            {
              *(zero_first + index) *= phase_coefficient;
              *(one_first + index) *= conj_phase_coefficient;
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<1u>(
            parallel_policy, local_state, page_permutated_qubit,
            ::ket::mpi::gate::page::exponential_pauli_z_detail::make_exponential_pauli_z_coeff1_p(
              phase_coefficient, conj_phase_coefficient));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }

        // 2_2p: both of qubits of eYY are on page
        // eZZ_{ij}(s) = exp(is Z_i Z_j) = I cos s + i Z_i Z_j sin s
        // eZZ_{1,2}(s) (a_{00} |00> + a_{01} |01> + a_{10} |10> + a{11} |11>)
        //   = e^{is} |00> + e^{-is} |01> + e^{-is} |10> + e^{is} |11>
        namespace exponential_pauli_z_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Complex>
          struct exponential_pauli_z_coeff2_2p
          {
            Complex phase_coefficient_;
            Complex conj_phase_coefficient_;

            exponential_pauli_z_coeff2_2p(Complex const& phase_coefficient, Complex const& conj_phase_coefficient) noexcept
              : phase_coefficient_{phase_coefficient}, conj_phase_coefficient_{conj_phase_coefficient}
            { }

            template <typename Iterator, typename StateInteger>
            void operator()(
              Iterator const first_00, Iterator const first_01,
              Iterator const first_10, Iterator const first_11,
              StateInteger const index, int const) const
            {
              *(first_00 + index) *= phase_coefficient_;
              *(first_01 + index) *= conj_phase_coefficient_;
              *(first_10 + index) *= conj_phase_coefficient_;
              *(first_11 + index) *= phase_coefficient_;
            }
          }; // struct exponential_pauli_z_coeff2_2p<Complex>

          template <typename Complex>
          inline ::ket::mpi::gate::page::exponential_pauli_z_detail::exponential_pauli_z_coeff2_2p<Complex>
          make_exponential_pauli_z_coeff2_2p(Complex const& phase_coefficient, Complex const& conj_phase_coefficient)
          { return {phase_coefficient, conj_phase_coefficient}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace exponential_pauli_z_detail

        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename Complex, typename StateInteger, typename BitInteger>
        inline RandomAccessRange& exponential_pauli_z_coeff2_2p(
          ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          Complex const& phase_coefficient, // exp(i theta) = cos(theta) + i sin(theta)
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const page_permutated_qubit1,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const page_permutated_qubit2)
        {
          using std::conj;
          auto const conj_phase_coefficient = conj(phase_coefficient);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::two_page_qubits_gate<0u>(
            parallel_policy, local_state, page_permutated_qubit1, page_permutated_qubit2,
            [&phase_coefficient, &conj_phase_coefficient](
              auto const first_00, auto const first_01, auto const first_10, auto const first_11,
              StateInteger const index, int const)
            {
              *(first_00 + index) *= phase_coefficient;
              *(first_01 + index) *= conj_phase_coefficient;
              *(first_10 + index) *= conj_phase_coefficient;
              *(first_11 + index) *= phase_coefficient;
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::two_page_qubits_gate<0u>(
            parallel_policy, local_state, page_permutated_qubit1, page_permutated_qubit2,
            ::ket::mpi::gate::page::exponential_pauli_z_detail::make_exponential_pauli_z_coeff2_2p(
              phase_coefficient, conj_phase_coefficient));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }

        // 2_p: only one qubit of eYY is on page
        // eZZ_{ij}(s) = exp(is Z_i Z_j) = I cos s + i Z_i Z_j sin s
        // eZZ_{1,2}(s) (a_{00} |00> + a_{01} |01> + a_{10} |10> + a{11} |11>)
        //   = e^{is} |00> + e^{-is} |01> + e^{-is} |10> + e^{is} |11>
        namespace exponential_pauli_z_detail
        {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
          template <typename Complex, typename StateInteger>
          struct exponential_pauli_z_coeff2_p
          {
            Complex phase_coefficient_;
            Complex conj_phase_coefficient_;
            StateInteger nonpage_permutated_qubit_mask_;
            StateInteger nonpage_lower_bits_mask_;
            StateInteger nonpage_upper_bits_mask_;

            exponential_pauli_z_coeff2_p(
              Complex const& phase_coefficient, Complex const& conj_phase_coefficient,
              StateInteger const nonpage_permutated_qubit_mask,
              StateInteger const nonpage_lower_bits_mask,
              StateInteger const nonpage_upper_bits_mask) noexcept
              : phase_coefficient_{phase_coefficient}, conj_phase_coefficient_{conj_phase_coefficient},
                nonpage_permutated_qubit_mask_{nonpage_permutated_qubit_mask},
                nonpage_lower_bits_mask_{nonpage_lower_bits_mask},
                nonpage_upper_bits_mask_{nonpage_upper_bits_mask}
            { }

            template <typename Iterator>
            void operator()(
              Iterator const zero_first, Iterator const one_first,
              StateInteger const index_wo_nonpage_qubit, int const) const
            {
              auto const zero_index
                = ((index_wo_nonpage_qubit bitand nonpage_upper_bits_mask_) << 1u)
                  bitor (index_wo_nonpage_qubit bitand nonpage_lower_bits_mask_);
              auto const one_index = zero_index bitor nonpage_permutated_qubit_mask_;

              auto const iter_00 = zero_first + zero_index;
              auto const iter_01_or_10 = zero_first + one_index;
              auto const iter_10_or_01 = one_first + zero_index;
              auto const iter_11 = one_first + one_index;

              *iter_00 *= phase_coefficient_;
              *iter_01_or_10 *= conj_phase_coefficient_;
              *iter_10_or_01 *= conj_phase_coefficient_;
              *iter_11 *= phase_coefficient_;
            }
          }; // struct exponential_pauli_z_coeff2_p<Complex, StateInteger>

          template <typename Complex, typename StateInteger>
          inline ::ket::mpi::gate::page::exponential_pauli_z_detail::exponential_pauli_z_coeff2_p<Complex, StateInteger>
          make_exponential_pauli_z_coeff2_p(
            Complex const& phase_coefficient, Complex const& conj_phase_coefficient,
            StateInteger const nonpage_permutated_qubit_mask,
            StateInteger const nonpage_lower_bits_mask,
            StateInteger const nonpage_upper_bits_mask)
          { return {phase_coefficient, conj_phase_coefficient, nonpage_permutated_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        } // namespace exponential_pauli_z_detail

        template <
          typename ParallelPolicy,
          typename RandomAccessRange, typename Complex, typename StateInteger, typename BitInteger>
        inline RandomAccessRange& exponential_pauli_z_coeff2_p(
          ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          Complex const& phase_coefficient, // exp(i theta) = cos(theta) + i sin(theta)
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const page_permutated_qubit,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const nonpage_permutated_qubit)
        {
          assert(::ket::mpi::page::is_on_page(page_permutated_qubit, local_state));
          assert(not ::ket::mpi::page::is_on_page(nonpage_permutated_qubit, local_state));

          auto const nonpage_permutated_qubit_mask
            = ::ket::utility::integer_exp2<StateInteger>(nonpage_permutated_qubit);
          auto const nonpage_lower_bits_mask = nonpage_permutated_qubit_mask - StateInteger{1u};
          auto const nonpage_upper_bits_mask = compl nonpage_lower_bits_mask;

          using std::conj;
          auto const conj_phase_coefficient = conj(phase_coefficient);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<1u>(
            parallel_policy, local_state, page_permutated_qubit,
            [&phase_coefficient, &conj_phase_coefficient, nonpage_permutated_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask](
              auto const zero_first, auto const one_first, StateInteger const index_wo_nonpage_qubit, int const)
            {
              auto const zero_index
                = ((index_wo_nonpage_qubit bitand nonpage_upper_bits_mask) << 1u)
                  bitor (index_wo_nonpage_qubit bitand nonpage_lower_bits_mask);
              auto const one_index = zero_index bitor nonpage_permutated_qubit_mask;

              auto const iter_00 = zero_first + zero_index;
              auto const iter_01_or_10 = zero_first + one_index;
              auto const iter_10_or_01 = one_first + zero_index;
              auto const iter_11 = one_first + one_index;

              *iter_00 *= phase_coefficient;
              *iter_01_or_10 *= conj_phase_coefficient;
              *iter_10_or_01 *= conj_phase_coefficient;
              *iter_11 *= phase_coefficient;
            });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          return ::ket::mpi::gate::page::detail::one_page_qubit_gate<1u>(
            parallel_policy, local_state, page_permutated_qubit,
            ::ket::mpi::gate::page::exponential_pauli_z_detail::make_exponential_pauli_z_coeff2_p(
              phase_coefficient, conj_phase_coefficient, nonpage_permutated_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
        }
      } // namespace page
    } // namespace gate
  } // namespace mpi
} // namespace ket


#endif // KET_MPI_GATE_PAGE_EXPONENTIAL_PAULI_Z_HPP
