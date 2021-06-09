#ifndef KET_MPI_GATE_PAGE_DETAIL_CONTROLLED_PHASE_SHIFT_COEFF_TP_DIAGONAL_HPP
# define KET_MPI_GATE_PAGE_DETAIL_CONTROLLED_PHASE_SHIFT_COEFF_TP_DIAGONAL_HPP

# include <boost/config.hpp>

# include <cassert>
# include <utility>

# include <boost/range/size.hpp>

# include <ket/qubit.hpp>
# include <ket/control.hpp>
# include <ket/utility/integer_exp2.hpp>
# include <ket/utility/integer_log2.hpp>
# include <ket/utility/begin.hpp>
# include <ket/mpi/qubit_permutation.hpp>
# include <ket/mpi/state.hpp>
# include <ket/mpi/page/is_on_page.hpp>
# include <ket/mpi/gate/page/unsupported_page_gate_operation.hpp>
# include <ket/mpi/gate/page/detail/one_page_qubit_gate.hpp>
# include <ket/mpi/utility/general_mpi.hpp>
# include <ket/mpi/utility/unit_mpi.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace page
      {
        namespace detail
        {
          namespace controlled_phase_shift_coeff_tp_detail
          {
            // tp_cl: target qubit is on page and control qubit is local
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
            template <typename Complex, typename StateInteger>
            struct do_controlled_phase_shift_coeff_tp_cl
            {
              Complex phase_coefficient_;
              StateInteger control_qubit_mask_;
              StateInteger nonpage_lower_bits_mask_;
              StateInteger nonpage_upper_bits_mask_;

              do_controlled_phase_shift_coeff_tp_cl(
                Complex const& phase_coefficient,
                StateInteger const control_qubit_mask,
                StateInteger const nonpage_lower_bits_mask,
                StateInteger const nonpage_upper_bits_mask) noexcept
                : phase_coefficient_{phase_coefficient},
                  control_qubit_mask_{control_qubit_mask},
                  nonpage_lower_bits_mask_{nonpage_lower_bits_mask},
                  nonpage_upper_bits_mask_{nonpage_upper_bits_mask}
              { }

              template <typename Iterator>
              void operator()(
                Iterator const, Iterator const one_first,
                StateInteger const index_wo_nonpage_qubit, int const) const
              {
                auto const zero_index
                  = ((index_wo_nonpage_qubit bitand nonpage_upper_bits_mask_) << 1u)
                    bitor (index_wo_nonpage_qubit bitand nonpage_lower_bits_mask_);
                auto const one_index = zero_index bitor control_qubit_mask_;
                *(one_first + one_index) *= phase_coefficient_;
              }
            }; // struct do_controlled_phase_shift_coeff_tp_cl<Complex, StateInteger>

            template <typename Complex, typename StateInteger>
            inline ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::do_controlled_phase_shift_coeff_tp_cl<Complex, StateInteger>
            make_do_controlled_phase_shift_coeff_tp_cl(
              Complex const& phase_coefficient,
              StateInteger const control_qubit_mask,
              StateInteger const nonpage_lower_bits_mask,
              StateInteger const nonpage_upper_bits_mask)
            { return {phase_coefficient, control_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

            template <
              typename ParallelPolicy,
              typename Complex, int num_page_qubits_, typename StateAllocator,
              typename StateInteger, typename BitInteger, typename PermutationAllocator>
            inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>&
            controlled_phase_shift_coeff_tp_cl(
              ParallelPolicy const parallel_policy,
              ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
              Complex const& phase_coefficient,
              ::ket::qubit<StateInteger, BitInteger> const target_qubit,
              ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
              ::ket::mpi::qubit_permutation<
                StateInteger, BitInteger, PermutationAllocator> const& permutation)
            {
              assert(not ::ket::mpi::page::is_on_page(control_qubit.qubit(), local_state, permutation));
              auto const control_qubit_mask
                = ::ket::utility::integer_exp2<StateInteger>(permutation[control_qubit.qubit()]);
              auto const nonpage_lower_bits_mask = control_qubit_mask - StateInteger{1u};
              auto const nonpage_upper_bits_mask = compl nonpage_lower_bits_mask;

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
              return ::ket::mpi::gate::page::detail::one_page_qubit_gate<1u>(
                parallel_policy, local_state, target_qubit, permutation,
                [phase_coefficient, control_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask](
                  auto const, auto const one_first, StateInteger const index_wo_nonpage_qubit, int const)
                {
                  auto const zero_index
                    = ((index_wo_nonpage_qubit bitand nonpage_upper_bits_mask) << 1u)
                      bitor (index_wo_nonpage_qubit bitand nonpage_lower_bits_mask);
                  auto const one_index = zero_index bitor control_qubit_mask;
                  *(one_first + one_index) *= phase_coefficient;
                });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
              return ::ket::mpi::gate::page::detail::one_page_qubit_gate<1u>(
                parallel_policy, local_state, target_qubit, permutation,
                ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::make_do_controlled_phase_shift_coeff_tp_cl(
                  phase_coefficient,
                  control_qubit_mask, nonpage_lower_bits_mask, nonpage_upper_bits_mask));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
            }

            // tp_cu: target qubit is on page and control qubit is unit
            template <
              typename StateInteger, typename BitInteger, typename NumProcesses,
              typename ParallelPolicy,
              typename Complex, typename StateAllocator,
              typename PermutationAllocator>
            inline ::ket::mpi::state<Complex, 0, StateAllocator>&
            controlled_phase_shift_coeff_tp_cu(
              ::ket::mpi::utility::policy::unit_mpi<StateInteger, BitInteger, NumProcesses> const& mpi_policy,
              ParallelPolicy const parallel_policy,
              ::ket::mpi::state<Complex, 0, StateAllocator>& local_state,
              Complex const& phase_coefficient,
              ::ket::qubit<StateInteger, BitInteger> const target_qubit,
              ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
              ::ket::mpi::qubit_permutation<
                StateInteger, BitInteger, PermutationAllocator> const& permutation,
              ::yampi::rank const rank,
              ::ket::qubit<StateInteger, BitInteger> const least_permutated_unit_qubit)
            { throw ::ket::mpi::gate::page::unsupported_page_gate_operation<0>{"controlled_phase_shift_coeff_tp_cu"}; }

            template <
              typename StateInteger, typename BitInteger, typename NumProcesses,
              typename ParallelPolicy,
              typename Complex, int num_page_qubits_, typename StateAllocator,
              typename PermutationAllocator>
            inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>&
            controlled_phase_shift_coeff_tp_cu(
              ::ket::mpi::utility::policy::unit_mpi<StateInteger, BitInteger, NumProcesses> const& mpi_policy,
              ParallelPolicy const parallel_policy,
              ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
              Complex const& phase_coefficient,
              ::ket::qubit<StateInteger, BitInteger> const target_qubit,
              ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
              ::ket::mpi::qubit_permutation<
                StateInteger, BitInteger, PermutationAllocator> const& permutation,
              ::yampi::rank const rank,
              ::ket::qubit<StateInteger, BitInteger> const least_permutated_unit_qubit)
            {
              assert(not ::ket::mpi::page::is_on_page(control_qubit.qubit(), local_state, permutation));

              auto const control_qubit_mask
                = StateInteger{1u}
                  << (permutation[control_qubit.qubit()] - least_permutated_unit_qubit);

              auto const num_data_blocks = static_cast<StateInteger>(local_state.num_data_blocks());
              auto const rank_in_unit = ::ket::mpi::utility::policy::rank_in_unit(mpi_policy, rank);
              for (auto data_block_index = StateInteger{0u}; data_block_index < num_data_blocks; ++data_block_index)
              {
                auto const unit_qubit_value = ::ket::mpi::utility::policy::unit_qubit_value(mpi_policy, data_block_index, rank_in_unit);

                if ((unit_qubit_value bitand control_qubit_mask) == StateInteger{0u})
                  continue;

                auto const num_nonpage_local_qubits
                  = static_cast<BitInteger>(local_state.num_local_qubits() - num_page_qubits_);
                auto const target_qubit_mask
                  = ::ket::utility::integer_exp2<StateInteger>(
                      permutation[target_qubit] - static_cast<BitInteger>(num_nonpage_local_qubits));
                auto const lower_bits_mask = target_qubit_mask - StateInteger{1u};
                auto const upper_bits_mask = compl lower_bits_mask;

                static constexpr auto num_pages
                  = ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>::num_pages;
                for (auto page_index_wo_qubit = std::size_t{0u}; page_index_wo_qubit < num_pages / 2u; ++page_index_wo_qubit)
                {
                  // x0x
                  auto const zero_page_index
                    = ((page_index_wo_qubit bitand upper_bits_mask) << 1u)
                      bitor (page_index_wo_qubit bitand lower_bits_mask);
                  // x1x
                  auto const one_page_index = zero_page_index bitor target_qubit_mask;

                  auto one_page_range = local_state.page_range(std::make_pair(data_block_index, one_page_index));
                  auto const one_first = ::ket::utility::begin(one_page_range);

                  using ::ket::utility::loop_n;
                  loop_n(
                    parallel_policy,
                    boost::size(one_page_range),
                    [one_first, phase_coefficient](StateInteger const index, int const)
                    { *(one_first + index) *= phase_coefficient; });
                }
              }

              return local_state;
            }

            // tp_cg: target qubit is on page and control qubit is global
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
            template <typename Complex>
            struct do_controlled_phase_shift_coeff_tp_cg
            {
              Complex phase_coefficient_;

              explicit do_controlled_phase_shift_coeff_tp_cg(Complex const& phase_coefficient) noexcept
                : phase_coefficient_{phase_coefficient}
              { }

              template <typename Iterator, typename StateInteger>
              void operator()(
                Iterator const, Iterator const one_first, StateInteger const index, int const) const
              { *(one_first + index) *= phase_coefficient_; }
            }; // struct do_controlled_phase_shift_coeff_tp_cg<Complex>

            template <typename Complex>
            inline ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::do_controlled_phase_shift_coeff_tp_cg<Complex>
            make_do_controlled_phase_shift_coeff_tp_cg(Complex const& phase_coefficient)
            { return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::do_controlled_phase_shift_coeff_tp_cg<Complex>{phase_coefficient}; }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

            template <
              typename ParallelPolicy,
              typename Complex, int num_page_qubits_, typename StateAllocator,
              typename StateInteger, typename BitInteger, typename PermutationAllocator>
            inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>&
            controlled_phase_shift_coeff_tp_cg(
              ParallelPolicy const parallel_policy,
              ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
              Complex const& phase_coefficient,
              ::ket::qubit<StateInteger, BitInteger> const target_qubit,
              ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
              ::ket::mpi::qubit_permutation<
                StateInteger, BitInteger, PermutationAllocator> const& permutation,
              yampi::rank const rank,
              ::ket::qubit<StateInteger, BitInteger> const least_permutated_global_qubit,
              StateInteger const global_qubit_value)
            {
              assert(not ::ket::mpi::page::is_on_page(control_qubit.qubit(), local_state, permutation));

              auto const control_qubit_mask
                = StateInteger{1u}
                  << (permutation[control_qubit.qubit()] - least_permutated_global_qubit);

              if ((global_qubit_value bitand control_qubit_mask) == StateInteger{0u})
                return local_state;

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
              return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
                parallel_policy, local_state, target_qubit, permutation,
                [phase_coefficient](
                  auto const, auto const one_first, StateInteger const index, int const)
                { *(one_first + index) *= phase_coefficient; });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
              return ::ket::mpi::gate::page::detail::one_page_qubit_gate<0u>(
                parallel_policy, local_state, target_qubit, permutation,
                ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::make_do_controlled_phase_shift_coeff_tp_cg(phase_coefficient));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
            }
          } // namespace controlled_phase_shift_coeff_tp_detail

          template <
            typename MpiPolicy, typename ParallelPolicy,
            typename RandomAccessRange, typename Complex,
            typename StateInteger, typename BitInteger, typename PermutationAllocator>
          [[noreturn]] inline RandomAccessRange& controlled_phase_shift_coeff_tp(
            MpiPolicy const&, ParallelPolicy const,
            RandomAccessRange& local_state,
            Complex const&,
            ::ket::qubit<StateInteger, BitInteger> const,
            ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const,
            ::ket::mpi::qubit_permutation<
              StateInteger, BitInteger, PermutationAllocator> const&,
            yampi::rank const)
          { throw ::ket::mpi::gate::page::unsupported_page_gate_operation<0, false>{"controlled_phase_shift_coeff_tp"}; }

          template <
            typename ParallelPolicy,
            typename Complex, typename StateAllocator,
            typename StateInteger, typename BitInteger, typename PermutationAllocator>
          [[noreturn]] inline ::ket::mpi::state<Complex, 0, StateAllocator>&
          controlled_phaes_shift_coeff_tp(
            ::ket::mpi::utility::policy::general_mpi const, ParallelPolicy const,
            ::ket::mpi::state<Complex, 0, StateAllocator>& local_state,
            Complex const&,
            ::ket::qubit<StateInteger, BitInteger> const,
            ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const,
            ::ket::mpi::qubit_permutation<
              StateInteger, BitInteger, PermutationAllocator> const&,
            yampi::rank const)
          { throw ::ket::mpi::gate::page::unsupported_page_gate_operation<0>{"controlled_phase_shift_coeff_tp"}; }

          template <
            typename StateInteger, typename BitInteger, typename NumProcesses,
            typename ParallelPolicy,
            typename Complex, typename StateAllocator,
            typename PermutationAllocator>
          [[noreturn]] inline ::ket::mpi::state<Complex, 0, StateAllocator>&
          controlled_phaes_shift_coeff_tp(
            ::ket::mpi::utility::policy::unit_mpi<StateInteger, BitInteger, NumProcesses> const&,
            ParallelPolicy const,
            ::ket::mpi::state<Complex, 0, StateAllocator>& local_state,
            Complex const&,
            ::ket::qubit<StateInteger, BitInteger> const,
            ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const,
            ::ket::mpi::qubit_permutation<
              StateInteger, BitInteger, PermutationAllocator> const&,
            yampi::rank const)
          { throw ::ket::mpi::gate::page::unsupported_page_gate_operation<0>{"controlled_phase_shift_coeff_tp"}; }

          template <
            typename ParallelPolicy,
            typename Complex, int num_page_qubits_, typename StateAllocator,
            typename StateInteger, typename BitInteger, typename PermutationAllocator>
          inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>&
          controlled_phase_shift_coeff_tp(
            ::ket::mpi::utility::policy::general_mpi const mpi_policy,
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
            Complex const& phase_coefficient,
            ::ket::qubit<StateInteger, BitInteger> const target_qubit,
            ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
            ::ket::mpi::qubit_permutation<
              StateInteger, BitInteger, PermutationAllocator> const& permutation,
            yampi::rank const rank)
          {
            assert(not ::ket::mpi::page::is_on_page(control_qubit.qubit(), local_state, permutation));

            using qubit_type = ::ket::qubit<StateInteger, BitInteger>;
            auto const least_permutated_global_qubit = qubit_type{local_state.num_local_qubits()};

            if (permutation[control_qubit.qubit()] < least_permutated_global_qubit)
              return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::controlled_phase_shift_coeff_tp_cl(
                parallel_policy, local_state, phase_coefficient, target_qubit, control_qubit, permutation);

            return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::controlled_phase_shift_coeff_tp_cg(
              parallel_policy,
              local_state, phase_coefficient, target_qubit, control_qubit,
              permutation, rank, least_permutated_global_qubit,
              ::ket::mpi::utility::policy::global_qubit_value(mpi_policy, rank));
          }

          template <
            typename StateInteger, typename BitInteger, typename NumProcesses,
            typename ParallelPolicy,
            typename Complex, int num_page_qubits_, typename StateAllocator,
            typename PermutationAllocator>
          inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>&
          controlled_phase_shift_coeff_tp(
            ::ket::mpi::utility::policy::unit_mpi<StateInteger, BitInteger, NumProcesses> const& mpi_policy,
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
            Complex const& phase_coefficient,
            ::ket::qubit<StateInteger, BitInteger> const target_qubit,
            ::ket::control< ::ket::qubit<StateInteger, BitInteger> > const control_qubit,
            ::ket::mpi::qubit_permutation<
              StateInteger, BitInteger, PermutationAllocator> const& permutation,
            yampi::rank const rank)
          {
            assert(not ::ket::mpi::page::is_on_page(control_qubit.qubit(), local_state, permutation));

            using qubit_type = ::ket::qubit<StateInteger, BitInteger>;
            auto const least_permutated_unit_qubit = qubit_type{local_state.num_local_qubits()};
            auto const least_permutated_global_qubit = least_permutated_unit_qubit + mpi_policy.num_unit_qubits();

            auto const permutated_control_qubit = permutation[control_qubit.qubit()];
            if (permutated_control_qubit < least_permutated_unit_qubit)
              return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::controlled_phase_shift_coeff_tp_cl(
                parallel_policy, local_state, phase_coefficient, target_qubit, control_qubit, permutation);

            if (permutated_control_qubit < least_permutated_global_qubit)
              return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::controlled_phase_shift_coeff_tp_cu(
                mpi_policy, parallel_policy,
                local_state, phase_coefficient, target_qubit, control_qubit,
                permutation, rank, least_permutated_unit_qubit);

            return ::ket::mpi::gate::page::detail::controlled_phase_shift_coeff_tp_detail::controlled_phase_shift_coeff_tp_cg(
              parallel_policy,
              local_state, phase_coefficient, target_qubit, control_qubit,
              permutation, rank, least_permutated_global_qubit,
              ::ket::mpi::utility::policy::global_qubit_value(mpi_policy, rank));
          }
        } // namespace detail
      } // namespace page
    } // namespace gate
  } // namespace mpi
} // namespace ket


#endif // KET_MPI_GATE_PAGE_DETAIL_CONTROLLED_PHASE_SHIFT_COEFF_TP_DIAGONAL_HPP
