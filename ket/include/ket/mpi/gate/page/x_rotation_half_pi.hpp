#ifndef KET_MPI_GATE_PAGE_X_ROTATION_HALF_PI_HPP
# define KET_MPI_GATE_PAGE_X_ROTATION_HALF_PI_HPP

# include <boost/config.hpp>

# include <cassert>
# include <iterator>

# include <boost/math/constants/constants.hpp>
# include <boost/range/begin.hpp>
# include <boost/range/size.hpp>
# include <boost/range/iterator.hpp>

# include <ket/qubit.hpp>
# include <ket/utility/loop_n.hpp>
# include <ket/utility/integer_exp2.hpp>
# include <ket/utility/imaginary_unit.hpp>
# include <ket/utility/meta/real_of.hpp>
# include <ket/mpi/qubit_permutation.hpp>
# include <ket/mpi/state.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace page
      {
        template <
          typename MpiPolicy, typename ParallelPolicy,
          typename RandomAccessRange,
          typename StateInteger, typename BitInteger, typename Allocator>
        inline RandomAccessRange& x_rotation_half_pi(
          MpiPolicy const, ParallelPolicy const,
          RandomAccessRange& local_state,
          ::ket::qubit<StateInteger, BitInteger> const,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, Allocator> const&)
        { return local_state; }

        template <
          typename ParallelPolicy,
          typename Complex, typename StateAllocator,
          typename StateInteger, typename BitInteger, typename PermutationAllocator>
        inline ::ket::mpi::state<Complex, 0, StateAllocator>& x_rotation_half_pi(
          ::ket::mpi::utility::policy::general_mpi const, ParallelPolicy const,
          ::ket::mpi::state<Complex, 0, StateAllocator>& local_state,
          ::ket::qubit<StateInteger, BitInteger> const,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, PermutationAllocator> const&)
        { return local_state; }


        namespace x_rotation_half_pi_detail
        {
# ifdef BOOST_NO_CXX11_LAMBDAS
          template <typename RandomAccessIterator>
          struct x_rotation_half_pi_loop_inside
          {
            RandomAccessIterator zero_first_;
            RandomAccessIterator one_first_;

            x_rotation_half_pi_loop_inside(
              RandomAccessIterator const zero_first,
              RandomAccessIterator const one_first)
              : zero_first_(zero_first), one_first_(one_first)
            { }

            template <typename StateInteger>
            void operator()(StateInteger const index, int const) const
            {
              typedef
                typename std::iterator_traits<RandomAccessIterator>::value_type
                complex_type;

              RandomAccessIterator const zero_iter = zero_first_+index;
              RandomAccessIterator const one_iter = one_first_+index;
              complex_type const zero_iter_value = *zero_iter;

              typedef
                typename ::ket::utility::meta::real_of<complex_type>::type real_type;
              using boost::math::constants::one_div_root_two;
              *zero_iter += ::ket::utility::imaginary_unit<complex_type>() * (*one_iter);
              *zero_iter *= one_div_root_two<real_type>();
              *one_iter += ::ket::utility::imaginary_unit<complex_type>() * zero_iter_value;
              *one_iter *= one_div_root_two<real_type>();
            }
          };

          template <typename RandomAccessIterator>
          inline x_rotation_half_pi_loop_inside<RandomAccessIterator>
          make_x_rotation_half_pi_loop_inside(
            RandomAccessIterator const zero_first,
            RandomAccessIterator const one_first)
          {
            typedef
              ::ket::mpi::gate::page::x_rotation_half_pi_detail::x_rotation_half_pi_loop_inside<RandomAccessIterator>
              result_type;

            return result_type(zero_first, one_first);
          }
# endif // BOOST_NO_CXX11_LAMBDAS
        }

        template <
          typename ParallelPolicy,
          typename Complex, int num_page_qubits_, typename StateAllocator,
          typename StateInteger, typename BitInteger, typename PermutationAllocator>
        inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& x_rotation_half_pi(
          ::ket::mpi::utility::policy::general_mpi const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
          ::ket::qubit<StateInteger, BitInteger> const qubit,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, PermutationAllocator> const&
            permutation)
        {
          assert(local_state.is_page_qubit(permutation[qubit]));

          typedef ::ket::qubit<StateInteger, BitInteger> qubit_type;

          BitInteger const num_nonpage_qubits
            = static_cast<BitInteger>(local_state.num_local_qubits()-num_page_qubits_);
          StateInteger const qubit_mask
            = ::ket::utility::integer_exp2<StateInteger>(
                permutation[qubit] - static_cast<qubit_type>(num_nonpage_qubits));
          StateInteger const lower_bits_mask = qubit_mask-static_cast<StateInteger>(1u);
          StateInteger const upper_bits_mask = compl lower_bits_mask;

          typedef ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator> local_state_type;
          for (std::size_t base_page_id = 0u;
               base_page_id < local_state_type::num_pages/2u; ++base_page_id)
          {
            // x0x
            StateInteger const zero_page_id
              = ((base_page_id bitand upper_bits_mask) << 1u)
                bitor (base_page_id bitand lower_bits_mask);
            // x1x
            StateInteger const one_page_id = zero_page_id bitor qubit_mask;

            typedef typename local_state_type::page_range_type page_range_type;
            page_range_type zero_page_range
              = local_state.page_range(zero_page_id);
            page_range_type one_page_range
              = local_state.page_range(one_page_id);
            assert(boost::size(zero_page_range) == boost::size(one_page_range));

            using ::ket::utility::loop_n;
# ifndef BOOST_NO_CXX11_LAMBDAS
            loop_n(
              parallel_policy,
              boost::size(zero_page_range),
              [&zero_page_range, &one_page_range](StateInteger const index, int const)
              {
                typedef typename boost::range_iterator<page_range_type>::type page_iterator;
                page_iterator const zero_iter = boost::begin(zero_page_range)+index;
                page_iterator const one_iter = boost::begin(one_page_range)+index;
                Complex const zero_iter_value = *zero_iter;

                typedef
                  typename ::ket::utility::meta::real_of<Complex>::type real_type;
                using boost::math::constants::one_div_root_two;
                *zero_iter += ::ket::utility::imaginary_unit<Complex>() * (*one_iter);
                *zero_iter *= one_div_root_two<real_type>();
                *one_iter += ::ket::utility::imaginary_unit<Complex>() * zero_iter_value;
                *one_iter *= one_div_root_two<real_type>();
              });
# else // BOOST_NO_CXX11_LAMBDAS
            loop_n(
              parallel_policy,
              boost::size(zero_page_range),
              ::ket::mpi::gate::page::x_rotation_half_pi_detail::make_x_rotation_half_pi_loop_inside(
                boost::begin(zero_page_range), boost::begin(one_page_range)));
# endif // BOOST_NO_CXX11_LAMBDAS
          }

          return local_state;
        }


        template <
          typename MpiPolicy, typename ParallelPolicy,
          typename RandomAccessRange,
          typename StateInteger, typename BitInteger, typename Allocator>
        inline RandomAccessRange& conj_x_rotation_half_pi(
          MpiPolicy const, ParallelPolicy const,
          RandomAccessRange& local_state,
          ::ket::qubit<StateInteger, BitInteger> const,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, Allocator> const&)
        { return local_state; }

        template <
          typename ParallelPolicy,
          typename Complex, typename StateAllocator,
          typename StateInteger, typename BitInteger, typename PermutationAllocator>
        inline ::ket::mpi::state<Complex, 0, StateAllocator>& conj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::general_mpi const, ParallelPolicy const,
          ::ket::mpi::state<Complex, 0, StateAllocator>& local_state,
          ::ket::qubit<StateInteger, BitInteger> const,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, PermutationAllocator> const&)
        { return local_state; }


        namespace x_rotation_half_pi_detail
        {
# ifdef BOOST_NO_CXX11_LAMBDAS
          template <typename RandomAccessIterator>
          struct conj_x_rotation_half_pi_loop_inside
          {
            RandomAccessIterator zero_first_;
            RandomAccessIterator one_first_;

            conj_x_rotation_half_pi_loop_inside(
              RandomAccessIterator const zero_first,
              RandomAccessIterator const one_first)
              : zero_first_(zero_first), one_first_(one_first)
            { }

            template <typename StateInteger>
            void operator()(StateInteger const index, int const) const
            {
              typedef
                typename std::iterator_traits<RandomAccessIterator>::value_type
                complex_type;

              RandomAccessIterator const zero_iter = zero_first_+index;
              RandomAccessIterator const one_iter = one_first_+index;
              complex_type const zero_iter_value = *zero_iter;

              typedef
                typename ::ket::utility::meta::real_of<complex_type>::type real_type;
              using boost::math::constants::one_div_root_two;
              *zero_iter -= ::ket::utility::imaginary_unit<complex_type>() * (*one_iter);
              *zero_iter *= one_div_root_two<real_type>();
              *one_iter -= ::ket::utility::imaginary_unit<complex_type>() * zero_iter_value;
              *one_iter *= one_div_root_two<real_type>();
            }
          };

          template <typename RandomAccessIterator>
          inline conj_x_rotation_half_pi_loop_inside<RandomAccessIterator>
          make_conj_x_rotation_half_pi_loop_inside(
            RandomAccessIterator const zero_first,
            RandomAccessIterator const one_first)
          {
            typedef
              ::ket::mpi::gate::page::x_rotation_half_pi_detail::conj_x_rotation_half_pi_loop_inside<RandomAccessIterator>
              result_type;

            return result_type(zero_first, one_first);
          }
# endif // BOOST_NO_CXX11_LAMBDAS
        }

        template <
          typename ParallelPolicy,
          typename Complex, int num_page_qubits_, typename StateAllocator,
          typename StateInteger, typename BitInteger, typename PermutationAllocator>
        inline ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& conj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::general_mpi const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator>& local_state,
          ::ket::qubit<StateInteger, BitInteger> const qubit,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, PermutationAllocator> const&
            permutation)
        {
          assert(local_state.is_page_qubit(permutation[qubit]));

          typedef ::ket::qubit<StateInteger, BitInteger> qubit_type;

          BitInteger const num_nonpage_qubits
            = static_cast<BitInteger>(local_state.num_local_qubits()-num_page_qubits_);
          StateInteger const qubit_mask
            = ::ket::utility::integer_exp2<StateInteger>(
                permutation[qubit] - static_cast<qubit_type>(num_nonpage_qubits));
          StateInteger const lower_bits_mask = qubit_mask-static_cast<StateInteger>(1u);
          StateInteger const upper_bits_mask = compl lower_bits_mask;

          typedef ::ket::mpi::state<Complex, num_page_qubits_, StateAllocator> local_state_type;
          for (std::size_t base_page_id = 0u;
               base_page_id < local_state_type::num_pages/2u; ++base_page_id)
          {
            // x0x
            StateInteger const zero_page_id
              = ((base_page_id bitand upper_bits_mask) << 1u)
                bitor (base_page_id bitand lower_bits_mask);
            // x1x
            StateInteger const one_page_id = zero_page_id bitor qubit_mask;

            typedef typename local_state_type::page_range_type page_range_type;
            page_range_type zero_page_range
              = local_state.page_range(zero_page_id);
            page_range_type one_page_range
              = local_state.page_range(one_page_id);
            assert(boost::size(zero_page_range) == boost::size(one_page_range));

            using ::ket::utility::loop_n;
# ifndef BOOST_NO_CXX11_LAMBDAS
            loop_n(
              parallel_policy,
              boost::size(zero_page_range),
              [&zero_page_range, &one_page_range](StateInteger const index, int const)
              {
                typedef typename boost::range_iterator<page_range_type>::type page_iterator;
                page_iterator const zero_iter = boost::begin(zero_page_range)+index;
                page_iterator const one_iter = boost::begin(one_page_range)+index;
                Complex const zero_iter_value = *zero_iter;

                typedef
                  typename ::ket::utility::meta::real_of<Complex>::type real_type;
                using boost::math::constants::one_div_root_two;
                *zero_iter -= ::ket::utility::imaginary_unit<Complex>() * (*one_iter);
                *zero_iter *= one_div_root_two<real_type>();
                *one_iter -= ::ket::utility::imaginary_unit<Complex>() * zero_iter_value;
                *one_iter *= one_div_root_two<real_type>();
              });
# else // BOOST_NO_CXX11_LAMBDAS
            loop_n(
              parallel_policy,
              boost::size(zero_page_range),
              ::ket::mpi::gate::page::x_rotation_half_pi_detail::make_conj_x_rotation_half_pi_loop_inside(
                boost::begin(zero_page_range), boost::begin(one_page_range)));
# endif // BOOST_NO_CXX11_LAMBDAS
          }

          return local_state;
        }


        template <
          typename MpiPolicy, typename ParallelPolicy,
          typename RandomAccessRange,
          typename StateInteger, typename BitInteger, typename Allocator>
        inline RandomAccessRange& adj_x_rotation_half_pi(
          MpiPolicy const mpi_policy, ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state,
          ::ket::qubit<StateInteger, BitInteger> const qubit,
          ::ket::mpi::qubit_permutation<
            StateInteger, BitInteger, Allocator> const& permutation)
        {
          return ::ket::mpi::gate::page::conj_x_rotation_half_pi(
            mpi_policy, parallel_policy, local_state, qubit, permutation);
        }
      }
    }
  }
}


#endif

