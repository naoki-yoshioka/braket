#ifndef KET_MPI_GATE_X_ROTATION_HALF_PI_HPP
# define KET_MPI_GATE_X_ROTATION_HALF_PI_HPP

# include <boost/config.hpp>

# ifndef BOOST_NO_CXX11_HDR_ARRAY
#   include <array>
# else
#   include <boost/array.hpp>
# endif

# include <boost/range/value_type.hpp>

# include <yampi/environment.hpp>
# include <yampi/datatype.hpp>
# include <yampi/communicator.hpp>

# include <ket/qubit.hpp>
# include <ket/gate/x_rotation_half_pi.hpp>
# include <ket/mpi/qubit_permutation.hpp>
# include <ket/mpi/utility/general_mpi.hpp>
# include <ket/mpi/utility/logger.hpp>
# include <ket/mpi/gate/page/x_rotation_half_pi.hpp>
# include <ket/mpi/page/is_on_page.hpp>

# ifndef BOOST_NO_CXX11_HDR_ARRAY
#   define KET_array std::array
# else
#   define KET_array boost::array
# endif


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace x_rotation_half_pi_detail
      {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
        template <typename ParallelPolicy, typename Qubit>
        struct call_x_rotation_half_pi
        {
          ParallelPolicy parallel_policy_;
          Qubit qubit_;

          call_x_rotation_half_pi(ParallelPolicy const parallel_policy, Qubit const qubit)
            : parallel_policy_(parallel_policy),
              qubit_(qubit)
          { }

          template <typename RandomAccessIterator>
          void operator()(
            RandomAccessIterator const first,
            RandomAccessIterator const last) const
          { ::ket::gate::x_rotation_half_pi(parallel_policy_, first, last, qubit_); }
        };

        template <typename ParallelPolicy, typename Qubit>
        inline call_x_rotation_half_pi<ParallelPolicy, Qubit> make_call_x_rotation_half_pi(
          ParallelPolicy const parallel_policy, Qubit const qubit)
        {
          return call_x_rotation_half_pi<ParallelPolicy, Qubit>(
            parallel_policy, qubit);
        }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      } // namespace x_rotation_half_pi_detail

      template <
        typename MpiPolicy, typename ParallelPolicy,
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& x_rotation_half_pi(
        MpiPolicy const mpi_policy, ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        ::ket::mpi::utility::log_with_time_guard<char> print("Xpi", environment);

        typedef ::ket::qubit<StateInteger, BitInteger> qubit_type;
        KET_array<qubit_type, 1u> qubits = { qubit };
        ::ket::mpi::utility::maybe_interchange_qubits(
          mpi_policy, parallel_policy,
          local_state, qubits, permutation,
          buffer, datatype, communicator, environment);

        if (::ket::mpi::page::is_on_page(qubit, local_state, permutation))
          return ::ket::mpi::gate::page::x_rotation_half_pi(
            mpi_policy, parallel_policy, local_state, qubit, permutation);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          [parallel_policy, qubit, &permutation](auto const first, auto const last)
          { ::ket::gate::x_rotation_half_pi(parallel_policy, first, last, permutation[qubit]); });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          ::ket::mpi::gate::x_rotation_half_pi_detail::make_call_x_rotation_half_pi(
            parallel_policy, permutation[qubit]));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      }

      template <
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& x_rotation_half_pi(
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(),
          ::ket::utility::policy::make_sequential(),
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }

      template <
        typename ParallelPolicy, typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& x_rotation_half_pi(
        ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(), parallel_policy,
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }


      namespace x_rotation_half_pi_detail
      {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
        template <typename ParallelPolicy, typename Qubit>
        struct call_conj_x_rotation_half_pi
        {
          ParallelPolicy parallel_policy_;
          Qubit qubit_;

          call_conj_x_rotation_half_pi(ParallelPolicy const parallel_policy, Qubit const qubit)
            : parallel_policy_(parallel_policy),
              qubit_(qubit)
          { }

          template <typename RandomAccessIterator>
          void operator()(
            RandomAccessIterator const first,
            RandomAccessIterator const last) const
          { ::ket::gate::conj_x_rotation_half_pi(parallel_policy_, first, last, qubit_); }
        };

        template <typename ParallelPolicy, typename Qubit>
        inline call_conj_x_rotation_half_pi<ParallelPolicy, Qubit> make_call_conj_x_rotation_half_pi(
          ParallelPolicy const parallel_policy, Qubit const qubit)
        {
          return call_conj_x_rotation_half_pi<ParallelPolicy, Qubit>(
            parallel_policy, qubit);
        }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      } // namespace x_rotation_half_pi_detail

      template <
        typename MpiPolicy, typename ParallelPolicy,
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& conj_x_rotation_half_pi(
        MpiPolicy const mpi_policy, ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        ::ket::mpi::utility::log_with_time_guard<char> print("Conj(Xpi)", environment);

        typedef ::ket::qubit<StateInteger, BitInteger> qubit_type;
        KET_array<qubit_type, 1u> qubits = { qubit };
        ::ket::mpi::utility::maybe_interchange_qubits(
          mpi_policy, parallel_policy,
          local_state, qubits, permutation,
          buffer, datatype, communicator, environment);

        if (::ket::mpi::page::is_on_page(qubit, local_state, permutation))
          return ::ket::mpi::gate::page::conj_x_rotation_half_pi(
            mpi_policy, parallel_policy, local_state, qubit, permutation);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          [parallel_policy, qubit, &permutation](auto const first, auto const last)
          { ::ket::gate::conj_x_rotation_half_pi(parallel_policy, first, last, permutation[qubit]); });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          ::ket::mpi::gate::x_rotation_half_pi_detail::make_call_conj_x_rotation_half_pi(
            parallel_policy, permutation[qubit]));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      }

      template <
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& conj_x_rotation_half_pi(
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::conj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(),
          ::ket::utility::policy::make_sequential(),
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }

      template <
        typename ParallelPolicy, typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& conj_x_rotation_half_pi(
        ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::conj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(), parallel_policy,
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }


      namespace x_rotation_half_pi_detail
      {
# ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
        template <typename ParallelPolicy, typename Qubit>
        struct call_adj_x_rotation_half_pi
        {
          ParallelPolicy parallel_policy_;
          Qubit qubit_;

          call_adj_x_rotation_half_pi(ParallelPolicy const parallel_policy, Qubit const qubit)
            : parallel_policy_(parallel_policy),
              qubit_(qubit)
          { }

          template <typename RandomAccessIterator>
          void operator()(
            RandomAccessIterator const first,
            RandomAccessIterator const last) const
          { ::ket::gate::adj_x_rotation_half_pi(parallel_policy_, first, last, qubit_); }
        };

        template <typename ParallelPolicy, typename Qubit>
        inline call_adj_x_rotation_half_pi<ParallelPolicy, Qubit> make_call_adj_x_rotation_half_pi(
          ParallelPolicy const parallel_policy, Qubit const qubit)
        {
          return call_adj_x_rotation_half_pi<ParallelPolicy, Qubit>(
            parallel_policy, qubit);
        }
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      } // namespace x_rotation_half_pi_detail

      template <
        typename MpiPolicy, typename ParallelPolicy,
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& adj_x_rotation_half_pi(
        MpiPolicy const mpi_policy, ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        ::ket::mpi::utility::log_with_time_guard<char> print("Adj(Xpi)", environment);

        typedef ::ket::qubit<StateInteger, BitInteger> qubit_type;
        KET_array<qubit_type, 1u> qubits = { qubit };
        ::ket::mpi::utility::maybe_interchange_qubits(
          mpi_policy, parallel_policy,
          local_state, qubits, permutation,
          buffer, datatype, communicator, environment);

        if (::ket::mpi::page::is_on_page(qubit, local_state, permutation))
          return ::ket::mpi::gate::page::adj_x_rotation_half_pi(
            mpi_policy, parallel_policy, local_state, qubit, permutation);

# ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          [parallel_policy, qubit, &permutation](auto const first, auto const last)
          { ::ket::gate::adj_x_rotation_half_pi(parallel_policy, first, last, permutation[qubit]); });
# else // BOOST_NO_CXX14_GENERIC_LAMBDAS
        return ::ket::mpi::utility::for_each_local_range(
          mpi_policy, local_state,
          ::ket::mpi::gate::x_rotation_half_pi_detail::make_call_adj_x_rotation_half_pi(
            parallel_policy, permutation[qubit]));
# endif // BOOST_NO_CXX14_GENERIC_LAMBDAS
      }

      template <
        typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& adj_x_rotation_half_pi(
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::adj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(),
          ::ket::utility::policy::make_sequential(),
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }

      template <
        typename ParallelPolicy, typename RandomAccessRange,
        typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator>
      inline RandomAccessRange& adj_x_rotation_half_pi(
        ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::qubit<StateInteger, BitInteger> const qubit,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype const datatype,
        yampi::communicator const communicator,
        yampi::environment const& environment)
      {
        return ::ket::mpi::gate::adj_x_rotation_half_pi(
          ::ket::mpi::utility::policy::make_general_mpi(), parallel_policy,
          local_state, qubit, permutation,
          buffer, datatype, communicator, environment);
      }
    } // namespace gate
  } // namespace mpi
} // namespace ket


# undef KET_array

#endif

