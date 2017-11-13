#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <boost/move/unique_ptr.hpp>

#include <yampi/environment.hpp>
#include <yampi/communicator.hpp>
#include <yampi/rank.hpp>
#include <yampi/wall_clock.hpp>

#include <ket/utility/integer_exp2.hpp>
#include <ket/utility/integer_log2.hpp>

#include <bra/gates.hpp>
#include <bra/state.hpp>
#include <bra/make_general_mpi_state.hpp>


#include <ket/mpi/state.hpp>
#include <ket/mpi/qubit_permutation.hpp>
#include <ket/mpi/measure.hpp>
#include <ket/mpi/utility/general_mpi.hpp>
#include <ket/mpi/utility/logger.hpp>
#include <ket/mpi/gate/hadamard.hpp>
#include <ket/mpi/gate/phase_shift.hpp>
#include <ket/mpi/gate/x_rotation_half_pi.hpp>
#include <ket/mpi/gate/y_rotation_half_pi.hpp>
#include <ket/mpi/gate/controlled_not.hpp>
#include <ket/mpi/gate/controlled_phase_shift.hpp>
//#include <ket/mpi/gate/toffoli.hpp>


int main(int argc, char* argv[])
{
  std::ios::sync_with_stdio(false);

  typedef unsigned int bit_integer_type;
  typedef boost::mt19937_64 rng_type;
  typedef rng_type::result_type seed_type;

  yampi::environment environment(argc, argv);
  yampi::communicator world_communicator(yampi::world_communicator());
  yampi::rank const rank = world_communicator.rank(environment);
  BOOST_CONSTEXPR_OR_CONST yampi::rank root_rank(0);
  bool const is_io_root_rank = rank == root_rank and yampi::is_io_process(root_rank, environment);

  bit_integer_type const num_gqubits
    = ket::utility::integer_log2<bit_integer_type>(world_communicator.size(environment));

  if (ket::utility::integer_exp2<bit_integer_type>(num_gqubits)
      != static_cast<bit_integer_type>(world_communicator.size(environment)))
  {
    if (is_io_root_rank)
      std::cerr << "wrong number of MPI processes" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc < 2 or argc > 3)
  {
    if (is_io_root_rank)
      std::cerr << "wrong number of arguments: bra qcxfile [seed]" << std::endl;
    return EXIT_FAILURE;
  }

  std::string const filename(argv[1]);
  seed_type const seed
    = argc == 3
      ? boost::lexical_cast<seed_type>(argv[2])
      : static_cast<seed_type>(1);

  std::ifstream file_stream(filename.c_str());
  if (!file_stream)
  {
    if (is_io_root_rank)
      std::cerr << "cannot open an input file " << filename << std::endl;
    return EXIT_FAILURE;
  }

  bra::gates gates(file_stream, environment, root_rank, yampi::world_communicator());
  //boost::movelib::unique_ptr<bra::general_mpi_state> state_ptr
  //  = bra::make_general_mpi_state(2u, gates.initial_state_value(), gates.num_lqubits(), gates.initial_permutation(), seed, yampi::world_communicator(), environment);
  boost::movelib::unique_ptr<bra::state> state_ptr
    = bra::make_general_mpi_state(
        2u, gates.initial_state_value(), gates.num_lqubits(), gates.initial_permutation(),
        seed, yampi::world_communicator(), environment);

  yampi::wall_clock::time_point const start_time = yampi::wall_clock::now(environment);

  *state_ptr << gates;

  if (is_io_root_rank)
  {
    std::cout
      << "Operations finished: "
      << (state_ptr->operations_finish_time() - start_time).count()
      << std::endl;

    std::cout
      << boost::format("Expectation values of spins:\n%|=8s| %|=8s| %|=8s|\n")
         % "<Qx>" % "<Qy>" % "<Qz>";
# ifndef BOOST_NO_CXX11_RANGE_BASED_FOR
    typedef bra::state::spin_type spin_type;
    for (spin_type const& spin: *(state_ptr->maybe_expectation_values()))
      std::cout
        << boost::format("%|=8.3f| %|=8.3f| %|=8.3f|\n")
           % (0.5-spin[0u]) % (0.5-spin[1u]) % (0.5-spin[2u]);
# else // BOOST_NO_CXX11_RANGE_BASED_FOR
    typedef bra::state::spins_type::const_iterator const_iterator;
    const_iterator const last = state_ptr->maybe_expectation_values()->end();
    for (const_iterator iter = state_ptr->maybe_expectation_values()->begin();
         iter != last; ++iter)
      std::cout
        << boost::format("%|=8.3f| %|=8.3f| %|=8.3f|\n")
           % (0.5-(*iter)[0u]) % (0.5-(*iter)[1u]) % (0.5-(*iter)[2u]);
# endif // BOOST_NO_CXX11_RANGE_BASED_FOR
    std::cout << std::flush;

    std::cout
      << "Expectation values finished: "
      << (state_ptr->expectation_values_finish_time() - start_time).count()
      << " ("
      << (state_ptr->expectation_values_finish_time()
          - state_ptr->operations_finish_time()).count()
      << ')'
      << std::endl;

    std::cout
      << "Measurement result: " << state_ptr->measured_value()
      << "\nMeasurement finished: "
      << (state_ptr->measurement_finish_time() - start_time).count()
      << " ("
      << (state_ptr->measurement_finish_time()
          - state_ptr->expectation_values_finish_time()).count()
      << ')'
      << std::endl;
  }

  return EXIT_SUCCESS;
}
