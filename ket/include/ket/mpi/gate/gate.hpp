#ifndef KET_MPI_GATE_GATE_HPP
# define KET_MPI_GATE_GATE_HPP

# include <tuple>
# include <array>
# include <vector>
# include <iterator>
# include <utility>

# include <boost/range/value_type.hpp>

# include <yampi/datatype_base.hpp>
# include <yampi/communicator.hpp>
# include <yampi/environment.hpp>

# include <ket/qubit.hpp>
# include <ket/gate/gate.hpp>
# include <ket/mpi/permutated.hpp>
# include <ket/mpi/qubit_permutation.hpp>
# include <ket/mpi/utility/simple_mpi.hpp>


namespace ket
{
  namespace mpi
  {
    namespace gate
    {
      namespace gate_detail
      {
        template <
          typename MpiPolicy, typename ParallelPolicy,
          typename RandomAccessRange, typename StateInteger, typename BitInteger,
          typename Function, typename Qubit, typename... Qubits>
        inline RandomAccessRange& gate(
          MpiPolicy const& mpi_policy, ParallelPolicy const parallel_policy,
          RandomAccessRange& local_state, yampi::communicator const& communicator, yampi::environment const& environment,
          Function&& function, ::ket::permutated<Qubit> const permutated_qubit, ::ket::permutated<Qubits> const... permutated_qubits)
        {
          auto const data_block_size
            = ::ket::mpi::utility::policy::data_block_size(mpi_policy, local_state, communicator, environment);
          auto const num_data_blocks
            = ::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment);

          auto const first = std::begin(local_state);
          for (auto data_block_index = decltype(num_data_blocks){0u}; data_block_index < num_data_blocks; ++data_block_index)
            ::ket::gate::gate(
              parallel_policy,
              first + data_block_index * data_block_size,
              first + (data_block_index + 1u) * data_block_size,
              function, permutated_qubit.qubit(), permutated_qubits.qubit()...);

          return local_state;
        }
      } // namespace gate_detail

      template <
        typename MpiPolicy, typename ParallelPolicy,
        typename RandomAccessRange, typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator,
        typename Function, typename Qubit, typename... Qubits>
      inline RandomAccessRange& gate(
        MpiPolicy const& mpi_policy, ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::communicator const& communicator, yampi::environment const& environment,
        Function&& function, Qubit&& qubit, Qubits&&... qubits)
      {
        using qubit_type = ::ket::qubit<StateInteger, BitInteger>;
        auto qubit_array = std::array<qubit_type, sizeof...(Qubits) + 1u>{::ket::remove_control(qubit), ::ket::remove_control(qubits)...};
        ::ket::mpi::utility::maybe_interchange_qubits(
          mpi_policy, parallel_policy,
          local_state, qubit_array, permutation, buffer, communicator, environment);

        return ::ket::mpi::gate::gate_detail::gate(
          mpi_policy, parallel_policy, local_state, communicator, environment,
          std::forward<Function>(function), permutation[std::forward<Qubit>(qubit)], permutation[std::forward<Qubits>(qubits)]...);
      }

      template <
        typename MpiPolicy, typename ParallelPolicy,
        typename RandomAccessRange, typename StateInteger, typename BitInteger,
        typename Allocator, typename BufferAllocator, typename DerivedDatatype,
        typename Function, typename Qubit, typename... Qubits>
      inline RandomAccessRange& gate(
        MpiPolicy const& mpi_policy, ParallelPolicy const parallel_policy,
        RandomAccessRange& local_state,
        ::ket::mpi::qubit_permutation<StateInteger, BitInteger, Allocator>& permutation,
        std::vector<typename boost::range_value<RandomAccessRange>::type, BufferAllocator>& buffer,
        yampi::datatype_base<DerivedDatatype> const& datatype,
        yampi::communicator const& communicator, yampi::environment const& environment,
        Function&& function, Qubit&& qubit, Qubits&&... qubits)
      {
        using qubit_type = ::ket::qubit<StateInteger, BitInteger>;
        auto qubit_array = std::array<qubit_type, sizeof...(Qubits) + 1u>{::ket::remove_control(qubit), ::ket::remove_control(qubits)...};
        ::ket::mpi::utility::maybe_interchange_qubits(
          mpi_policy, parallel_policy,
          local_state, qubit_array, permutation, buffer, datatype, communicator, environment);

        return ::ket::mpi::gate::gate_detail::gate(
          mpi_policy, parallel_policy, local_state, communicator, environment,
          std::forward<Function>(function), permutation[std::forward<Qubit>(qubit)], permutation[std::forward<Qubits>(qubits)]...);
      }
    } // namespace gate
  } // namespace mpi
} // namespace ket


#endif // KET_MPI_GATE_GATE_HPP
