#ifndef KET_MPI_STATE_HPP
# define KET_MPI_STATE_HPP

# include <boost/config.hpp>

# include <cstddef>
# include <cassert>
# include <iterator>
# include <vector>
# include <algorithm>
# include <memory>
# include <utility>
# include <tuple>
# include <type_traits>
# if __cplusplus < 201703L
#   include <boost/type_traits/is_nothrow_swappable.hpp>
# endif
# include <array>
# include <initializer_list>

# include <boost/iterator/iterator_facade.hpp>

# include <boost/range/sub_range.hpp>
# include <boost/range/iterator_range.hpp>
# include <boost/range/iterator.hpp>

# include <yampi/environment.hpp>
# include <yampi/buffer.hpp>
# include <yampi/datatype_base.hpp>
# include <yampi/communicator.hpp>
# include <yampi/rank.hpp>
# include <yampi/status.hpp>
# include <yampi/algorithm/swap.hpp>

# include <ket/qubit.hpp>
# include <ket/control.hpp>
# include <ket/utility/integer_exp2.hpp>
# include <ket/utility/loop_n.hpp>
# include <ket/mpi/permutated.hpp>
# include <ket/mpi/qubit_permutation.hpp>
# include <ket/mpi/page/is_on_page.hpp>
# include <ket/mpi/utility/simple_mpi.hpp>
# include <ket/mpi/utility/for_each_local_range.hpp>
# include <ket/mpi/utility/buffer_range.hpp>
# include <ket/mpi/utility/transform_inclusive_scan.hpp>
# include <ket/mpi/utility/transform_inclusive_scan_self.hpp>
# include <ket/mpi/utility/upper_bound.hpp>
# include <ket/mpi/utility/detail/swap_permutated_local_qubits.hpp>
# include <ket/mpi/utility/detail/for_each_in_diagonal_loop.hpp>
# include <ket/mpi/utility/detail/swap_local_data.hpp>

# if __cplusplus >= 201703L
#   define KET_is_nothrow_swappable std::is_nothrow_swappable
# else
#   define KET_is_nothrow_swappable boost::is_nothrow_swappable
# endif


namespace ket
{
  namespace mpi
  {
    namespace state_detail
    {
      template <typename State>
      class state_iterator
        : public boost::iterators::iterator_facade<
            state_iterator<State>,
            typename State::value_type,
            std::random_access_iterator_tag>
      {
        State* state_ptr_;
        int index_;

        friend class boost::iterators::iterator_core_access;

       public:
        constexpr state_iterator() noexcept
          : state_ptr_{nullptr}, index_{}
        { }

        constexpr state_iterator(State& state, int const index) noexcept
          : state_ptr_{std::addressof(state)}, index_{index}
        { }


        typename State::value_type& dereference() const
        { return const_cast<typename State::value_type&>((*state_ptr_)[index_]); }

        bool equal(state_iterator const& other) const
        { return state_ptr_ == other.state_ptr_ and index_ == other.index_; }

        state_iterator& increment() { ++index_; return *this; }
        state_iterator& decrement() { --index_; return *this; }
        state_iterator& advance(typename State::difference_type const n)
        { index_ += n; return *this; }
        typename State::difference_type distance_to(state_iterator const& other) const
        { return other.index_-index_; }

        void swap(state_iterator& other) noexcept
        {
          using std::swap;
          swap(state_ptr_, other.state_ptr_);
          swap(index_, other.index_);
        }
      }; // class state_iterator<State>

      template <typename State>
      inline void swap(
        ::ket::mpi::state_detail::state_iterator<State>& lhs,
        ::ket::mpi::state_detail::state_iterator<State>& rhs) noexcept
      { lhs.swap(rhs); }
    } // namespace state_detail


    template <typename Complex, bool has_page_qubits = true, typename Allocator = std::allocator<Complex>>
    class state
    {
     public:
      using value_type = Complex;
      using allocator_type = typename Allocator::template rebind<value_type>::other;

     private:
      using data_type = std::vector<value_type, allocator_type>;
      data_type data_;

     public:
      using page_range_type
        = boost::iterator_range<typename boost::range_iterator<data_type>::type>;

     private:
      std::size_t num_local_qubits_;
      std::size_t num_page_qubits_;
      std::size_t num_pages_; // 1u << num_page_qubits_
      std::size_t num_data_blocks_;
      std::vector<page_range_type> page_ranges_;
      page_range_type buffer_range_;

     public:
      using size_type = typename data_type::size_type;
      using difference_type = typename data_type::difference_type;
      using reference = typename data_type::reference;
      using const_reference = typename data_type::const_reference;
      using pointer = typename data_type::pointer;
      using const_pointer = typename data_type::const_pointer;
      using iterator = ::ket::mpi::state_detail::state_iterator<state>;
      using const_iterator = ::ket::mpi::state_detail::state_iterator<state const>;
      using reverse_iterator = std::reverse_iterator<iterator>;
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;

      state() = delete;
      ~state() noexcept = default;
      state(state const&) = default;
      state& operator=(state const&) = default;
      state(state&&) = default;
      state& operator=(state&&) = default;

      state(state const& other, allocator_type const& allocator)
        : data_{other.data_, allocator},
          num_local_qubits_{other.num_local_qubits_},
          num_page_qubits_{other.num_page_qubits_},
          num_pages_{other.num_pages_},
          num_data_blocks_{other.num_data_blocks_},
          page_ranges_{other.page_ranges_},
          buffer_range_{other.buffer_range_}
      { }

      state(state&& other, allocator_type const& allocator)
        : data_{std::move(other.data_), allocator},
          num_local_qubits_{std::move(other.num_local_qubits_)},
          num_page_qubits_{std::move(other.num_page_qubits_)},
          num_pages_{std::move(other.num_pages_)},
          num_data_blocks_{std::move(other.num_data_blocks_)},
          page_ranges_{std::move(other.page_ranges_)},
          buffer_range_{std::move(other.buffer_range_)}
      { }

      state(std::initializer_list<value_type> initializer_list, allocator_type const& allocator = allocator_type())
        : data_{generate_initial_data(initializer_list, std::size_t{2u}, std::size_t{1u}, allocator)},
          num_local_qubits_{::ket::utility::integer_log2(initializer_list.size())},
          num_page_qubits_{std::size_t{1u}},
          num_pages_{std::size_t{2u}},
          num_data_blocks_{std::size_t{1u}},
          page_ranges_{generate_initial_page_ranges(data_, num_pages_, num_data_blocks_)},
          buffer_range_{generate_initial_buffer_range(data_, num_pages_, num_data_blocks_)}
      {
        assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) == initializer_list.size());
        assert(num_local_qubits_ > num_page_qubits_);
      }

      template <typename BitInteger>
      state(std::initializer_list<value_type> initializer_list, BitInteger const num_page_qubits, allocator_type const& allocator = allocator_type())
        : data_{generate_initial_data(initializer_list, std::size_t{1u} << num_page_qubits, std::size_t{1u}, allocator)},
          num_local_qubits_{::ket::utility::integer_log2(initializer_list.size())},
          num_page_qubits_{static_cast<std::size_t>(num_page_qubits)},
          num_pages_{std::size_t{1u} << num_page_qubits},
          num_data_blocks_{std::size_t{1u}},
          page_ranges_{generate_initial_page_ranges(data_, num_pages_, num_data_blocks_)},
          buffer_range_{generate_initial_buffer_range(data_, num_pages_, num_data_blocks_)}
      {
        assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) == initializer_list.size());
        assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_);
      }

      template <typename BitInteger, typename StateInteger>
      state(std::initializer_list<value_type> initializer_list, BitInteger const num_page_qubits, StateInteger const num_data_blocks, allocator_type const& allocator = allocator_type())
        : data_{generate_initial_data(initializer_list, std::size_t{1u} << num_page_qubits, static_cast<std::size_t>(num_data_blocks), allocator)},
          num_local_qubits_{::ket::utility::integer_log2(initializer_list.size() / num_data_blocks)},
          num_page_qubits_{static_cast<std::size_t>(num_page_qubits)},
          num_pages_{std::size_t{1u} << num_page_qubits},
          num_data_blocks_{static_cast<std::size_t>(num_data_blocks)},
          page_ranges_{generate_initial_page_ranges(data_, num_pages_, num_data_blocks_)},
          buffer_range_{generate_initial_buffer_range(data_, num_pages_, num_data_blocks_)}
      {
        assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) * num_data_blocks_ == initializer_list.size());
        assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_);
      }

      template <typename BitInteger, typename StateInteger, typename PermutationAllocator>
      state(
        BitInteger const num_local_qubits, BitInteger const num_page_qubits,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
        : data_{generate_initial_data(
            ::ket::mpi::utility::policy::make_simple_mpi(),
            num_local_qubits, StateInteger{1u} << num_page_qubits,
            initial_integer, permutation, communicator, environment)},
          num_local_qubits_{static_cast<std::size_t>(num_local_qubits)},
          num_page_qubits_{static_cast<std::size_t>(num_page_qubits)},
          num_pages_{std::size_t{1u} << num_page_qubits},
          num_data_blocks_{std::size_t{1u}},
          page_ranges_{generate_initial_page_ranges(data_, num_pages_, num_data_blocks_)},
          buffer_range_{generate_initial_buffer_range(data_, num_pages_, num_data_blocks_)}
      { assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_); }

      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      state(
        MpiPolicy const& mpi_policy,
        BitInteger const num_local_qubits, BitInteger const num_page_qubits,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const& permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
        : data_{generate_initial_data(
            mpi_policy, num_local_qubits, StateInteger{1u} << num_page_qubits, initial_integer, permutation, communicator, environment)},
          num_local_qubits_{static_cast<std::size_t>(num_local_qubits)},
          num_page_qubits_{static_cast<std::size_t>(num_page_qubits)},
          num_pages_{std::size_t{1u} << num_page_qubits},
          num_data_blocks_{static_cast<std::size_t>(::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment))},
          page_ranges_{generate_initial_page_ranges(data_, num_pages_, num_data_blocks_)},
          buffer_range_{generate_initial_buffer_range(data_, num_pages_, num_data_blocks_)}
      { assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_); }

      void assign(std::initializer_list<value_type> initializer_list)
      { assign(initializer_list, std::size_t{1u}, std::size_t{1u}); }

      template <typename BitInteger>
      void assign(std::initializer_list<value_type> initializer_list, BitInteger const num_page_qubits)
      { assign(initializer_list, num_page_qubits, std::size_t{1u}); }

      template <typename BitInteger, typename StateInteger>
      void assign(std::initializer_list<value_type> initializer_list, BitInteger const num_page_qubits, StateInteger const num_data_blocks)
      {
        initialize_data(data_, initializer_list, std::size_t{1u} << num_page_qubits, static_cast<std::size_t>(num_data_blocks));

        num_local_qubits_ = ::ket::utility::integer_log2(initializer_list.size() / num_data_blocks);
        num_page_qubits_ = static_cast<std::size_t>(num_page_qubits);
        num_pages_ = std::size_t{1u} << num_page_qubits;
        num_data_blocks_ = static_cast<std::size_t>(num_data_blocks);

        assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) * num_data_blocks_ == initializer_list.size());
        assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_);

        page_ranges_ = generate_initial_page_ranges(data_, num_pages_, num_data_blocks_);
        buffer_range_ = generate_initial_buffer_range(data_, num_pages_, num_data_blocks_);
      }

      template <typename BitInteger, typename StateInteger, typename PermutationAllocator>
      void assign(
        BitInteger const num_local_qubits, BitInteger const num_page_qubits,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
      { assign(::ket::mpi::utility::policy::make_simple_mpi(), num_local_qubits, num_page_qubits, initial_integer, permutation, communicator, environment); }

      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      void assign(
        MpiPolicy const& mpi_policy,
        BitInteger const num_local_qubits, BitInteger const num_page_qubits,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const& permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
      {
        initialize_data(data_, mpi_policy, num_local_qubits, StateInteger{1u} << num_page_qubits, initial_integer, permutation, communicator, environment);

        num_local_qubits_ = static_cast<std::size_t>(num_local_qubits);
        num_page_qubits_ = static_cast<std::size_t>(num_page_qubits);
        num_pages_ = std::size_t{1u} << num_page_qubits;
        num_data_blocks_ = static_cast<std::size_t>(::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment));

        assert(num_page_qubits_ >= BitInteger{1u} and num_local_qubits_ > num_page_qubits_);

        page_ranges_ = generate_initial_page_ranges(data_, num_pages_, num_data_blocks_);
        buffer_range_ = generate_initial_buffer_range(data_, num_pages_, num_data_blocks_);
      }

      template <typename PairOrTuple>
      size_type page_range_index(PairOrTuple const& data_block_page_etc_indices) const
      {
        auto const data_block_index = std::get<0u>(data_block_page_etc_indices);
        auto const page_index = std::get<1u>(data_block_page_etc_indices);
        assert(data_block_index >= decltype(data_block_index){0u} and data_block_index < num_data_blocks_);
        assert(page_index >= decltype(page_index){0u} and page_index < num_pages_);
        return static_cast<size_type>(data_block_index * num_pages_ + page_index);
      }

      std::pair<size_type, size_type> data_block_page_indices(size_type const page_range_index) const
      {
        assert(page_range_index >= size_type{0u} and page_range_index < num_data_blocks_ * num_pages_);
        return {page_range_index / num_pages_, page_range_index % num_pages_};
      }

      template <typename DataBlockIndex, typename PageIndex>
      void swap_pages(
        std::pair<DataBlockIndex, PageIndex> const& data_block_page_indices1,
        std::pair<DataBlockIndex, PageIndex> const& data_block_page_indices2)
      {
        assert(data_block_page_indices1 != data_block_page_indices2);
        using std::swap;
        swap(
          page_ranges_[page_range_index(data_block_page_indices1)],
          page_ranges_[page_range_index(data_block_page_indices2)]);
      }

      template <typename DataBlockIndex, typename PageIndex>
      void swap_buffer_and_page(
        std::pair<DataBlockIndex, PageIndex> const& data_block_page_indices)
      {
        using std::swap;
        swap(buffer_range_, page_ranges_[page_range_index(data_block_page_indices)]);
      }

      template <typename DataBlockIndex, typename PageIndex, typename NonpageIndex>
      void swap_values(
        std::tuple<DataBlockIndex, PageIndex, NonpageIndex> const& data_block_page_nonpage_indices1,
        std::tuple<DataBlockIndex, PageIndex, NonpageIndex> const& data_block_page_nonpage_indices2)
      {
        assert(
          std::get<0u>(data_block_page_nonpage_indices1) != std::get<0u>(data_block_page_nonpage_indices2)
          or std::get<1u>(data_block_page_nonpage_indices1) != std::get<1u>(data_block_page_nonpage_indices2));

        auto const nonpage_index1 = std::get<2u>(data_block_page_nonpage_indices1);
        auto const nonpage_index2 = std::get<2u>(data_block_page_nonpage_indices2);
        assert(
          nonpage_index1 >= decltype(nonpage_index1){0u}
          and nonpage_index1 < ::ket::utility::integer_exp2<size_type>(num_local_qubits_ - num_page_qubits_));
        assert(
          nonpage_index2 >= decltype(nonpage_index2){0u}
          and nonpage_index2 < ::ket::utility::integer_exp2<size_type>(num_local_qubits_ - num_page_qubits_));

        using std::swap;
        swap(
          std::begin(page_ranges_[page_range_index(data_block_page_nonpage_indices1)])[nonpage_index1],
          std::begin(page_ranges_[page_range_index(data_block_page_nonpage_indices2)])[nonpage_index2]);
      }

      template <typename DataBlockIndex, typename PageIndex>
      page_range_type const& page_range(std::pair<DataBlockIndex, PageIndex> const& data_block_page_indices) const
      { return page_ranges_[page_range_index(data_block_page_indices)]; }

      page_range_type const& buffer_range() const
      { return buffer_range_; }

      std::size_t num_local_qubits() const noexcept { assert(num_local_qubits_ > num_page_qubits_); return num_local_qubits_; }
      std::size_t num_page_qubits() const noexcept { assert(num_page_qubits_ >= std::size_t{1u}); return num_page_qubits_; }
      std::size_t num_pages() const noexcept { assert(num_pages_ >= std::size_t{2u}); return num_pages_; }
      std::size_t num_data_blocks() const noexcept { assert(num_data_blocks_ >= std::size_t{1u}); return num_data_blocks_; }

      bool operator==(state const& other) const noexcept
      { return num_local_qubits_ == other.num_local_qubits_ and num_data_blocks_ == other.num_data_blocks_ and std::equal(begin(), end(), other.begin()); }
      bool operator<(state const& other) const noexcept { return std::lexicographical_compare(begin(), end(), other.begin(), other.end()); }

      // Element access
      reference at(size_type const index)
      {
        return data_.at(
          (std::begin(page_ranges_[page_range_index(get_data_block_page_indices(index))]) - std::begin(data_))
          + get_nonpage_index(index));
      }

      const_reference at(size_type const index) const
      {
        return data_.at(
          (std::begin(page_ranges_[page_range_index(get_data_block_page_indices(index))]) - std::begin(data_))
          + get_nonpage_index(index));
      }

      reference operator[](size_type const index)
      {
        assert(index < ::ket::utility::integer_exp2<size_type>(num_local_qubits_) * num_data_blocks_);
        return std::begin(page_ranges_[page_range_index(get_data_block_page_indices(index))])[get_nonpage_index(index)];
      }

      const_reference operator[](size_type const index) const
      {
        assert(index < ::ket::utility::integer_exp2<size_type>(num_local_qubits_) * num_data_blocks_);
        return std::begin(page_ranges_[page_range_index(get_data_block_page_indices(index))])[get_nonpage_index(index)];
      }

      reference front() { return *std::begin(page_ranges_[page_range_index(get_data_block_page_indices(0u))]); }
      const_reference front() const { return *std::begin(page_ranges_[page_range_index(get_data_block_page_indices(0u))]); }

      reference back() { return *--std::end(page_ranges_[page_range_index(get_data_block_page_indices((1u << num_local_qubits_) - 1u))]); }
      const_reference back() const { return *--std::end(page_ranges_[page_range_index(get_data_block_page_indices((1u << num_local_qubits_) - 1u))]); }

      // Iterators
      iterator begin() noexcept { return iterator{*this, 0}; }
      const_iterator begin() const noexcept { return const_iterator{*this, 0}; }
      const_iterator cbegin() const noexcept { return const_iterator{*this, 0}; }
      iterator end() noexcept
      { return iterator{*this, static_cast<int>(1u << num_local_qubits_)}; }
      const_iterator end() const noexcept
      { return const_iterator{*this, static_cast<int>(1u << num_local_qubits_)}; }
      const_iterator cend() const noexcept
      { return const_iterator{*this, static_cast<int>(1u << num_local_qubits_)}; }
      reverse_iterator rbegin() noexcept { return reverse_iterator{this->end()}; }
      const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{this->end()}; }
      const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{this->cend()}; }
      reverse_iterator rend() noexcept { return reverse_iterator{this->begin()}; }
      const_reverse_iterator rend() const noexcept { return const_reverse_iterator{this->begin()}; }
      const_reverse_iterator crend() const noexcept { return const_reverse_iterator{this->cbegin()}; }

      // Capacity
      size_type size() const noexcept { return data_.size() - boost::size(buffer_range_); }
      size_type max_size() const noexcept { return data_.max_size() - boost::size(buffer_range_); }
      void reserve(size_type const new_capacity) { data_.reserve(new_capacity + boost::size(buffer_range_)); }
      size_type capacity() const noexcept { return data_.capacity() - boost::size(buffer_range_); }
      void shrink_to_fit() { data_.shrink_to_fit(); }

      // Modifiers
      void swap(state& other)
        noexcept(
          KET_is_nothrow_swappable<data_type>::value
          and KET_is_nothrow_swappable<std::size_t>::value
          and KET_is_nothrow_swappable<std::vector<page_range_type>>::value
          and KET_is_nothrow_swappable<page_range_type>::value )
      {
        using std::swap;
        swap(data_, other.data_);

        swap(num_local_qubits_, other.num_local_qubits_);
        swap(num_page_qubits_, other.num_page_qubits_);
        swap(num_pages_, other.num_pages_);
        swap(num_data_blocks_, other.num_data_blocks_);
        swap(page_ranges_, other.page_ranges_);
        swap(buffer_range_, other.buffer_range_);
      }

     private:
      void initialize_data(
        data_type& data,
        std::initializer_list<value_type> initializer_list,
        std::size_t const num_pages, std::size_t const num_data_blocks) const
      {
        auto const state_size = initializer_list.size();
        auto const data_size = state_size + state_size / num_pages / num_data_blocks;

        assert(state_size % (num_pages * num_data_blocks) == 0);

        data.clear();
        data.reserve(data_size);
        data.assign(initializer_list);
        data.resize(data_size);
      }

      template <
        typename MpiPolicy, typename BitInteger, typename StateInteger,
        typename PermutationAllocator>
      void initialize_data(
        data_type& data,
        MpiPolicy const& mpi_policy,
        BitInteger const num_local_qubits, StateInteger const num_pages,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment) const
      {
        auto const data_block_size = ::ket::utility::integer_exp2<std::size_t>(num_local_qubits);
        auto const num_data_blocks = ::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment);
        auto const state_size = data_block_size * static_cast<std::size_t>(num_data_blocks);
        auto const data_size = state_size + data_block_size / static_cast<std::size_t>(num_pages);

        assert(state_size % (num_pages * num_data_blocks) == 0);

        data.clear();
        data.reserve(data_size);
        data.assign(state_size, value_type{0});

        using ::ket::mpi::permutate_bits;
        auto const rank_index
          = ::ket::mpi::utility::qubit_value_to_rank_index(
              mpi_policy, data, permutate_bits(permutation, initial_integer),
              communicator, environment);

        if (communicator.rank(environment) == rank_index.first)
          data[rank_index.second] = value_type{1};

        data.resize(data_size);
      }

      data_type generate_initial_data(
        std::initializer_list<value_type> initializer_list,
        std::size_t const num_pages, std::size_t const num_data_blocks,
        allocator_type const& allocator) const
      {
        auto result = data_type{allocator};
        initialize_data(result, initializer_list, num_pages, num_data_blocks);
        return result;
      }

      template <
        typename MpiPolicy, typename BitInteger, typename StateInteger,
        typename PermutationAllocator>
      data_type generate_initial_data(
        MpiPolicy const& mpi_policy,
        BitInteger const num_local_qubits, StateInteger const num_pages,
        StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment) const
      {
        auto result = data_type{};
        initialize_data(result, mpi_policy, num_local_qubits, num_pages, initial_integer, permutation, communicator, environment);
        return result;
      }

      std::vector<page_range_type>
      generate_initial_page_ranges(data_type& data, std::size_t const num_pages, std::size_t const num_data_blocks) const
      {
        assert(data.size() % (num_pages * num_data_blocks + 1u) == 0u);
        auto const page_size = static_cast<size_type>(data.size() / (num_pages * num_data_blocks + 1u));

        auto result = std::vector<page_range_type>{};
        result.reserve(num_pages * num_data_blocks);
        for (auto page_range_index = std::size_t{0u}; page_range_index < num_pages * num_data_blocks; ++page_range_index)
          result.push_back(
            boost::make_iterator_range(
              std::begin(data) + page_range_index * page_size,
              std::begin(data) + (page_range_index + 1u) * page_size));

        return result;
      }

      page_range_type generate_initial_buffer_range(data_type& data, std::size_t const num_pages, std::size_t const num_data_blocks) const
      {
        assert(data.size() % (num_pages * num_data_blocks + 1u) == 0u);
        auto const page_size = static_cast<size_type>(data.size() / (num_pages * num_data_blocks + 1u));

        return boost::make_iterator_range(
          std::begin(data) + num_pages * num_data_blocks * page_size,
          std::begin(data) + (num_pages * num_data_blocks + 1u) * page_size);
      }

     public:
      std::pair<size_type, size_type> get_data_block_page_indices(size_type const index) const
      {
        auto const data_block_size = ::ket::utility::integer_exp2<size_type>(num_local_qubits_);
        assert(index < data_block_size * num_data_blocks_);

        auto const num_nonpage_local_qubits = num_local_qubits_ - num_page_qubits_;
        return std::make_pair(
          index / data_block_size,
          (((num_pages_ - 1u) << num_nonpage_local_qubits) bitand (index % data_block_size)) >> num_nonpage_local_qubits);
      }

      size_type get_nonpage_index(size_type const index) const
      {
        auto const data_block_size = ::ket::utility::integer_exp2<size_type>(num_local_qubits_);
        assert(index < data_block_size * num_data_blocks_);

        auto const num_nonpage_local_qubits = num_local_qubits_ - num_page_qubits_;
        return (compl ((num_pages_ - 1u) << num_nonpage_local_qubits)) bitand (index % data_block_size);
      }
    }; // class state<Complex, has_page_qubits, Allocator>

    template <typename StateInteger, typename BitInteger, typename Complex, bool has_page_qubits, typename Allocator>
    inline bool is_page_qubit(
      ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state)
    {
      auto const num_local_qubits = static_cast<BitInteger>(local_state.num_local_qubits());
      auto const num_page_qubits = static_cast<BitInteger>(local_state.num_page_qubits());
      return
        permutated_qubit >= ::ket::mpi::make_permutated(::ket::make_qubit<StateInteger>(num_local_qubits - num_page_qubits))
        and permutated_qubit < ::ket::mpi::make_permutated(::ket::make_qubit<StateInteger>(num_local_qubits));
    }

    template <typename StateInteger, typename BitInteger, typename Complex, bool has_page_qubits, typename Allocator>
    inline bool is_page_qubit(
      ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > > const permutated_control_qubit,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state)
    { return ::ket::mpi::is_page_qubit(::ket::mpi::remove_control(permutated_control_qubit), local_state); }

    namespace state_detail
    {
      template <
        typename Complex, bool has_page_qubits, typename Allocator,
        typename StateInteger, typename BitInteger>
      void interpage_swap(
        ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2)
      {
        assert(local_state.num_page_qubits() >= 2u);
        assert(::ket::mpi::is_page_qubit(permutated_qubit1, local_state) and ::ket::mpi::is_page_qubit(permutated_qubit2, local_state));
        assert(permutated_qubit1 != permutated_qubit2);

        auto const num_nonpage_local_qubits
          = static_cast<BitInteger>(local_state.num_local_qubits() - local_state.num_page_qubits());
        auto const minmax_permutated_qubits = std::minmax(permutated_qubit1, permutated_qubit2);
        auto const lower_bits_mask
          = ::ket::utility::integer_exp2<StateInteger>(minmax_permutated_qubits.first - num_nonpage_local_qubits)
            - StateInteger{1u};
        auto const middle_bits_mask
          = (::ket::utility::integer_exp2<StateInteger>(
               minmax_permutated_qubits.second - (num_nonpage_local_qubits + BitInteger{1u}))
             - StateInteger{1u})
            xor lower_bits_mask;
        auto const upper_bits_mask = compl (lower_bits_mask bitor middle_bits_mask);

        for (auto value_wo_qubits = StateInteger{0u};
             value_wo_qubits < ::ket::utility::integer_exp2<StateInteger>(static_cast<StateInteger>(local_state.num_page_qubits() - 2u));
             ++value_wo_qubits)
        {
          auto const base_page_index
            = ((value_wo_qubits bitand upper_bits_mask) << 2u)
              bitor ((value_wo_qubits bitand middle_bits_mask) << 1u)
              bitor (value_wo_qubits bitand lower_bits_mask);
          auto const page_index1
            = base_page_index bitor (StateInteger{1u} << (permutated_qubit1 - num_nonpage_local_qubits));
          auto const page_index2
            = base_page_index bitor (StateInteger{1u} << (permutated_qubit2 - num_nonpage_local_qubits));

          for (auto data_block_index = StateInteger{0u};
               data_block_index < local_state.num_data_blocks(); ++data_block_index)
            local_state.swap_pages(
              std::make_pair(data_block_index, page_index1),
              std::make_pair(data_block_index, page_index2));
        }
      }

      template <
        typename ParallelPolicy, typename Complex, bool has_page_qubits, typename Allocator,
        typename StateInteger, typename BitInteger>
      void swap_page_and_nonpage_qubits(
        ParallelPolicy const parallel_policy,
        ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2)
      {
        assert(
          (::ket::mpi::is_page_qubit(permutated_qubit1, local_state) and (not ::ket::mpi::is_page_qubit(permutated_qubit2, local_state)))
          or ((not ::ket::mpi::is_page_qubit(permutated_qubit1, local_state)) and ::ket::mpi::is_page_qubit(permutated_qubit2, local_state)));
# ifndef NDEBUG
        using permutated_qubit_type = ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> >;
# endif
        assert(permutated_qubit1 < permutated_qubit_type{local_state.num_local_qubits()});
        assert(permutated_qubit2 < permutated_qubit_type{local_state.num_local_qubits()});

        auto const num_nonpage_local_qubits
          = static_cast<BitInteger>(local_state.num_local_qubits() - local_state.num_page_qubits());
        auto const minmax_permutated_qubits = std::minmax(permutated_qubit1, permutated_qubit2);
        auto const nonpage_lower_bits_mask
          = ::ket::utility::integer_exp2<StateInteger>(minmax_permutated_qubits.first) - StateInteger{1u};
        auto const nonpage_upper_bits_mask
          = (::ket::utility::integer_exp2<StateInteger>(num_nonpage_local_qubits - 1u) - StateInteger{1u})
            xor nonpage_lower_bits_mask;
        auto const page_lower_bits_mask
          = ::ket::utility::integer_exp2<StateInteger>(minmax_permutated_qubits.second - num_nonpage_local_qubits)
            - StateInteger{1u};
        auto const page_upper_bits_mask
          = (::ket::utility::integer_exp2<StateInteger>(local_state.num_page_qubits() - 1u) - StateInteger{1u})
            xor page_lower_bits_mask;

        for (auto page_value_wo_qubits = StateInteger{0u};
             page_value_wo_qubits < ::ket::utility::integer_exp2<StateInteger>(static_cast<StateInteger>(local_state.num_page_qubits() - 1u));
             ++page_value_wo_qubits)
        {
          auto const page_index0
            = ((page_value_wo_qubits bitand page_upper_bits_mask) << 1u)
              bitor (page_value_wo_qubits bitand page_lower_bits_mask);
          auto const page_index1
            = (StateInteger{1u} << (minmax_permutated_qubits.second - num_nonpage_local_qubits))
              bitor page_index0;

          for (auto nonpage_value_wo_qubits = StateInteger{0u};
               nonpage_value_wo_qubits < ::ket::utility::integer_exp2<StateInteger>(static_cast<StateInteger>(num_nonpage_local_qubits - 1u));
               ++nonpage_value_wo_qubits)
          {
            auto const nonpage_index0
              = ((nonpage_value_wo_qubits bitand nonpage_upper_bits_mask) << 1u)
                bitor (nonpage_value_wo_qubits bitand nonpage_lower_bits_mask);
            auto const nonpage_index1
              = nonpage_index0 bitor (StateInteger{1u} << minmax_permutated_qubits.first);

            for (auto data_block_index = StateInteger{0u};
                 data_block_index < local_state.num_data_blocks(); ++data_block_index)
              local_state.swap_values(
                std::make_tuple(data_block_index, page_index0, nonpage_index1),
                std::make_tuple(data_block_index, page_index1, nonpage_index0));
          }
        }
      }

      template <
        typename ParallelPolicy, typename Complex, bool has_page_qubits, typename Allocator,
        typename StateInteger, typename BitInteger>
      void swap_nonpage_qubits(
        ParallelPolicy const parallel_policy,
        ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2,
        yampi::communicator const& communicator, yampi::environment const& environment)
      {
        assert((not ::ket::mpi::is_page_qubit(permutated_qubit1, local_state)) and (not ::ket::mpi::is_page_qubit(permutated_qubit2, local_state)));
# ifndef NDEBUG
        using permutated_qubit_type = ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> >;
# endif
        assert(permutated_qubit1 < permutated_qubit_type{local_state.num_local_qubits()});
        assert(permutated_qubit2 < permutated_qubit_type{local_state.num_local_qubits()});

        auto const num_pages = local_state.num_pages();
        auto const num_data_blocks = local_state.num_data_blocks();
        for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
          for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
          {
            auto const data_block_page_indices = std::make_pair(data_block_index, page_index);
            using page_range_type
              = typename std::remove_reference<
                  typename std::remove_cv<
                    decltype(local_state.page_range(data_block_page_indices))>::type>::type;
            using swap_permutated_local_qubits_type
              = ::ket::mpi::utility::dispatch::swap_permutated_local_qubits<page_range_type>;
            swap_permutated_local_qubits_type::call(
              parallel_policy, local_state.page_range(data_block_page_indices),
              permutated_qubit1, permutated_qubit2,
              StateInteger{1u}, static_cast<StateInteger>(boost::size(local_state.page_range(data_block_page_indices))),
              communicator, environment);
          }
      }

      template <bool has_page_qubits>
      struct swap_permutated_local_qubits
      {
        template <
          typename ParallelPolicy, typename Complex, typename Allocator,
          typename StateInteger, typename BitInteger>
        static void call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2,
          StateInteger const, StateInteger const,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
# ifndef NDEBUG
          using permutated_qubit_type = ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> >;
# endif
          assert(permutated_qubit1 < permutated_qubit_type{local_state.num_local_qubits()});
          assert(permutated_qubit2 < permutated_qubit_type{local_state.num_local_qubits()});

          if (::ket::mpi::is_page_qubit(permutated_qubit1, local_state))
          {
            if (local_state.num_page_qubits() >= 2u and ::ket::mpi::is_page_qubit(permutated_qubit2, local_state))
              ::ket::mpi::state_detail::interpage_swap(
                local_state, permutated_qubit1, permutated_qubit2);
            else
              ::ket::mpi::state_detail::swap_page_and_nonpage_qubits(
                parallel_policy, local_state, permutated_qubit1, permutated_qubit2);
          }
          else if (::ket::mpi::is_page_qubit(permutated_qubit2, local_state))
            ::ket::mpi::state_detail::swap_page_and_nonpage_qubits(
              parallel_policy, local_state, permutated_qubit2, permutated_qubit1);
          else
            ::ket::mpi::state_detail::swap_nonpage_qubits(
              parallel_policy, local_state, permutated_qubit2, permutated_qubit1,
              communicator, environment);
        }
      }; // struct swap_permutated_local_qubits<has_page_qubits>

      template <bool has_page_qubits>
      struct interchange_qubits
      {
        template <
          typename Allocator, typename Complex, typename Allocator_, typename StateInteger>
        static void call(
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          std::vector<Complex, Allocator_>&,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const source_local_first_index,
          StateInteger const source_local_last_index,
          yampi::rank const target_rank,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
          using page_range_type
            = typename ::ket::mpi::state<Complex, has_page_qubits, Allocator>::page_range_type;
          using page_iterator
            = typename boost::range_iterator<page_range_type>::type;
          do_call(
            local_state, data_block_index, data_block_size,
            source_local_first_index, source_local_last_index,
            [target_rank, &communicator, &environment](
              page_iterator const first, page_iterator const last,
              page_iterator const buffer_first, page_iterator const buffer_last)
            {
              yampi::algorithm::swap(
                yampi::ignore_status,
                yampi::make_buffer(first, last),
                yampi::make_buffer(buffer_first, buffer_last),
                target_rank, communicator, environment);
            });
        }

        template <
          typename Allocator, typename Complex, typename Allocator_, typename StateInteger,
          typename DerivedDatatype>
        static void call(
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          std::vector<Complex, Allocator_>&,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const source_local_first_index,
          StateInteger const source_local_last_index,
          yampi::datatype_base<DerivedDatatype> const& datatype, yampi::rank const target_rank,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
          using page_range_type
            = typename ::ket::mpi::state<Complex, has_page_qubits, Allocator>::page_range_type;
          using page_iterator
            = typename boost::range_iterator<page_range_type>::type;
          do_call(
            local_state, data_block_index, data_block_size,
            source_local_first_index, source_local_last_index,
            [&datatype, target_rank, &communicator, &environment](
              page_iterator const first, page_iterator const last,
              page_iterator const buffer_first, page_iterator const buffer_last)
            {
              yampi::algorithm::swap(
                yampi::ignore_status,
                yampi::make_buffer(first, last, datatype),
                yampi::make_buffer(buffer_first, buffer_last, datatype),
                target_rank, communicator, environment);
            });
        }

       private:
        template <
          typename Allocator, typename Complex, typename StateInteger, typename Function>
        static void do_call(
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const source_local_first_index, StateInteger const source_local_last_index,
          Function&& yampi_swap)
        {
          assert(data_block_index >= StateInteger{0u} and data_block_index < local_state.num_data_blocks());
          assert(data_block_size == ::ket::utility::integer_exp2<std::size_t>(local_state.num_local_qubits()));

          assert(source_local_last_index >= source_local_first_index);

          auto const front_data_block_page_indices
            = local_state.get_data_block_page_indices(data_block_index * data_block_size + source_local_first_index);
          auto const back_data_block_page_indices
            = local_state.get_data_block_page_indices(data_block_index * data_block_size + source_local_last_index - 1u);
          assert(front_data_block_page_indices.first == data_block_index);
          assert(back_data_block_page_indices.first == data_block_index);

          auto const front_page_index
            = static_cast<StateInteger>(front_data_block_page_indices.second);
          auto const back_page_index
            = static_cast<StateInteger>(back_data_block_page_indices.second);

          for (auto page_index = front_page_index; page_index <= back_page_index; ++page_index)
          {
            auto const page_range = local_state.page_range(std::make_pair(data_block_index, page_index));
            auto const page_size = boost::size(page_range);

            auto const page_first = std::begin(page_range);
            auto const page_last = std::end(page_range);
            auto const buffer_first = std::begin(local_state.buffer_range());

            auto const first_index
              = page_index == front_page_index
                ? static_cast<StateInteger>(local_state.get_nonpage_index(data_block_index * data_block_size + source_local_first_index))
                : StateInteger{0u};
            auto const last_index
              = page_index == back_page_index
                ? static_cast<StateInteger>(local_state.get_nonpage_index(data_block_index * data_block_size + source_local_last_index - 1u) + 1u)
                : static_cast<StateInteger>(page_size);

            auto const the_first = page_first + first_index;
            auto const the_last = page_first + last_index;
            auto const the_buffer_first = buffer_first + first_index;
            auto const the_buffer_last = buffer_first + last_index;

            std::copy(page_first, the_first, buffer_first);
            std::copy(the_last, page_last, the_buffer_last);

            yampi_swap(the_first, the_last, the_buffer_first, the_buffer_last);

            local_state.swap_buffer_and_page(std::make_pair(data_block_index, page_index));
          }
        }
      }; // struct interchange_qubits<has_page_qubits>

      template <bool has_page_qubits>
      struct for_each_local_range
      {
        template <typename MpiPolicy, typename Complex, typename Allocator, typename Function>
        static ::ket::mpi::state<Complex, has_page_qubits, Allocator>& call(
          MpiPolicy const&,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          yampi::communicator const&, yampi::environment const&,
          Function&& function)
        {
          // Gates should not be on page qubits
          auto const num_data_blocks = local_state.num_data_blocks();
          auto const num_pages = local_state.num_pages();
          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
              function(
                std::begin(local_state.page_range(std::make_pair(data_block_index, page_index))),
                std::end(local_state.page_range(std::make_pair(data_block_index, page_index))));
          return local_state;
        }

        template <typename MpiPolicy, typename Complex, typename Allocator, typename Function>
        static ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& call(
          MpiPolicy const&,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          yampi::communicator const&, yampi::environment const&,
          Function&& function)
        {
          // Gates should not be on page qubits
          auto const num_data_blocks = local_state.num_data_blocks();
          auto const num_pages = local_state.num_pages();
          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
              function(
                std::begin(local_state.page_range(std::make_pair(data_block_index, page_index))),
                std::end(local_state.page_range(std::make_pair(data_block_index, page_index))));
          return local_state;
        }
      }; // struct for_each_local_range<has_page_qubits>

      template <bool has_page_qubits>
      struct swap_local_data
      {
        template <typename Complex, typename Allocator, typename StateInteger>
        static void call(
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          StateInteger const data_block_index1, StateInteger const local_first_index1, StateInteger const local_last_index1,
          StateInteger const data_block_index2, StateInteger const local_first_index2,
          StateInteger const data_block_size)
        {
          auto const first_index1 = data_block_index1 * data_block_size + local_first_index1;
          auto const last_index1 = first_index1 + (local_last_index1 - local_first_index1);
          auto const first_index2 = data_block_index2 * data_block_size + local_first_index2;
          auto const last_index2 = first_index2 + (local_last_index1 - local_first_index1);

          auto const front_data_block_page_indices1 = local_state.get_data_block_page_indices(first_index1);
          auto const back_data_block_page_indices1 = local_state.get_data_block_page_indices(last_index1 - StateInteger{1u});
          auto const front_data_block_page_indices2 = local_state.get_data_block_page_indices(first_index2);
          auto const back_data_block_page_indices2 = local_state.get_data_block_page_indices(last_index2 - StateInteger{1u});
          assert(static_cast<StateInteger>(front_data_block_page_indices1.first) == data_block_index1);
          assert(static_cast<StateInteger>(back_data_block_page_indices1.first) == data_block_index1);
          assert(static_cast<StateInteger>(front_data_block_page_indices2.first) == data_block_index2);
          assert(static_cast<StateInteger>(back_data_block_page_indices2.first) == data_block_index2);

          auto const front_page_index1 = static_cast<StateInteger>(front_data_block_page_indices1.second);
          auto const back_page_index1 = static_cast<StateInteger>(back_data_block_page_indices1.second);
          auto const front_page_index2 = static_cast<StateInteger>(front_data_block_page_indices2.second);
          auto const back_page_index2 = static_cast<StateInteger>(back_data_block_page_indices2.second);

          auto const nonpage_first_index1 = static_cast<StateInteger>(local_state.get_nonpage_index(first_index1));
          auto const nonpage_last_index1 = static_cast<StateInteger>(local_state.get_nonpage_index(last_index1 - StateInteger{1u})) + StateInteger{1u};
          auto const nonpage_first_index2 = static_cast<StateInteger>(local_state.get_nonpage_index(first_index2));
          auto const nonpage_last_index2 = static_cast<StateInteger>(local_state.get_nonpage_index(last_index2 - StateInteger{1u})) + StateInteger{1u};

          auto const first1 = std::begin(local_state.page_range(front_data_block_page_indices1)) + nonpage_first_index1;
          auto const last1 = std::begin(local_state.page_range(back_data_block_page_indices1)) + nonpage_last_index1;
          auto const last2 = std::begin(local_state.page_range(back_data_block_page_indices2)) + nonpage_last_index2;

          if (nonpage_first_index1 == nonpage_first_index2)
          {
            assert(nonpage_last_index1 == nonpage_last_index2);

            for (auto page_index1 = front_page_index1, page_index2 = front_page_index2;
                 page_index1 <= back_page_index1; ++page_index1, ++page_index2)
            {
              assert(page_index2 <= back_page_index2);

              auto const data_block_page_indices1 = std::make_pair(data_block_index1, page_index1);
              auto const data_block_page_indices2 = std::make_pair(data_block_index2, page_index2);

              auto const page_range1 = local_state.page_range(data_block_page_indices1);
              auto const page_range2 = local_state.page_range(data_block_page_indices2);

              if (page_index1 == front_page_index1)
                std::swap_ranges(std::begin(page_range1), first1, std::begin(page_range2));

              if (page_index1 == back_page_index1)
              {
                assert(page_index2 == back_page_index2);
                std::swap_ranges(last1, std::end(page_range1), last2);
              }

              local_state.swap_pages(data_block_page_indices1, data_block_page_indices2);
            }
          }
          else // nonpage_first_index1 != nonpage_first_index2
          {
            auto page_index1 = front_page_index1;
            auto page_index2 = front_page_index2;
            auto the_first1 = first1;
            auto the_first2 = std::begin(local_state.page_range(front_data_block_page_indices2)) + nonpage_first_index2;

            while (true)
            {
              auto const page_range1 = local_state.page_range(std::make_pair(data_block_index1, page_index1));
              auto const page_range2 = local_state.page_range(std::make_pair(data_block_index2, page_index2));

              auto const the_last1 = page_index1 == back_page_index1 ? last1 : std::end(page_range1);
              auto const the_last2 = page_index2 == back_page_index2 ? last2 : std::end(page_range2);

              auto const size1 = the_last1 - the_first1;
              auto const size2 = the_last2 - the_first2;
              if (size1 == size2)
              {
                assert(the_last1 == last1 and the_last2 == last2);
                std::swap_ranges(the_first1, the_last1, the_first2);
                break;
              }

              else if (size1 > size2)
              {
                assert(the_last2 != last2);
                std::swap_ranges(the_first2, the_last2, the_first1);

                std::advance(the_first1, size2);
                the_first2 = the_last2;

                ++page_index2;
                the_first2 = std::begin(local_state.page_range(std::make_pair(data_block_index2, page_index2)));
              }
              else // size1 < size2
              {
                assert(the_last1 != last1);
                std::swap_ranges(the_first1, the_last1, the_first2);

                std::advance(the_first2, size1);
                the_first1 = the_last1;

                ++page_index1;
                the_first1 = std::begin(local_state.page_range(std::make_pair(data_block_index1, page_index1)));
              }
            }
          }
        }
      }; // struct swap_local_data<has_page_qubits>

# ifdef KET_USE_DIAGONAL_LOOP
      template <bool has_page_qubits>
      struct for_each_in_diagonal_loop
      {
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename StateInteger, typename BitInteger,
          std::size_t num_local_control_qubits, typename Function>
        static void call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          StateInteger const data_block_index, StateInteger const,
          StateInteger const last_local_qubit_value,
          std::array<
            ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > >,
            num_local_control_qubits> local_permutated_control_qubits,
          Function&& function)
        {
          std::sort(
            std::begin(local_permutated_control_qubits),
            std::end(local_permutated_control_qubits));

          impl(
            parallel_policy, local_state,
            data_block_index, last_local_qubit_value, local_permutated_control_qubits,
            std::forward<Function>(function));
        }

       private:
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename StateInteger, typename BitInteger,
          std::size_t num_local_control_qubits, typename Function>
        static void impl(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          StateInteger const data_block_index,
          StateInteger const last_local_qubit_value,
          std::array<
            ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > >,
            num_local_control_qubits> const& sorted_local_permutated_control_qubits,
          Function&& function)
        {
          static constexpr auto zero_state_integer = StateInteger{0u};

          using permutated_control_qubit_type
            = ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > >;
          // 000101000100
          auto const mask
            = std::accumulate(
                std::begin(sorted_local_permutated_control_qubits),
                std::end(sorted_local_permutated_control_qubits),
                zero_state_integer,
                [](StateInteger const& partial_mask, permutated_control_qubit_type const& permutated_control_qubit)
                {
                  static constexpr auto one_state_integer = StateInteger{1u};
                  return partial_mask bitor (one_state_integer << permutated_control_qubit);
                });

          auto const last_integer
            = (last_local_qubit_value >> local_state.num_page_qubits()) >> num_local_control_qubits;

          auto const num_pages = local_state.num_pages();
          for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
          {
            auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
            using ::ket::utility::loop_n;
            loop_n(
              parallel_policy, last_integer,
              [&function, &sorted_local_permutated_control_qubits, mask, first](StateInteger state_integer, int const)
              {
                static constexpr auto one_state_integer = StateInteger{1u};

                // xxx0x0xxx0xx
                for (permutated_control_qubit_type const& permutated_control_qubit: sorted_local_permutated_control_qubits)
                {
                  auto const lower_mask = (one_state_integer << permutated_control_qubit) - one_state_integer;
                  auto const upper_mask = compl lower_mask;
                  state_integer
                    = (state_integer bitand lower_mask)
                      bitor ((state_integer bitand upper_mask) << 1u);
                }

                // xxx1x1xxx1xx
                state_integer |= mask;

                function(first + state_integer, state_integer);
              });
          }
        }
      }; // struct for_each_in_diagonal_loop<has_page_qubits>
# endif // KET_USE_DIAGONAL_LOOP

      template <bool has_page_qubits>
      struct transform_inclusive_scan
      {
# ifdef KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
#   ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
        template <
          typename PartialSums, typename ParallelPolicy,
          typename LocalState, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        class process_in_execute
        {
          unsigned int num_threads_;
          PartialSums& partial_sums_;
          ParallelPolicy parallel_policy_;
          LocalState& local_state_;
          ForwardIterator d_first_;
          BinaryOperation binary_operation_;
          UnaryOperation unary_operation_;

         public:
          process_in_execute(
            unsigned int num_threads, PartialSums& partial_sums,
            ParallelPolicy parallel_policy,
            LocalState& local_state, ForwardIterator d_first,
            BinaryOperation binary_operation, UnaryOperation unary_operation)
            : num_threads_{num_threads},
              partial_sums_{partial_sums},
              parallel_policy_{parallel_policy},
              local_state_{local_state},
              d_first_{d_first},
              binary_operation_{binary_operation},
              unary_operation_{unary_operation}
          { }

          template <typename Executor>
          void operator()(int const thread_index, Executor& executor)
          {
            auto d_page_first = d_first_;

            for (auto data_block_index = std::size_t{0u}; data_block_index < local_state_.data_block_index(); ++data_block_index)
              for (auto page_index = std::size_t{0u}; page_index < local_state.num_pages(); ++page_index)
              {
                auto const first = std::begin(local_state_.page_range(std::make_pair(data_block_index, page_index)));
                auto d_iter = d_page_first;
                auto is_called = false;
                auto const page_range_index = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                using difference_type
                  = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                ::ket::utility::loop_n_in_execute(
                  parallel_policy_,
                  boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                  [page_range_index, first, &d_iter, &is_called, this](
                    difference_type const n, int const thread_index)
                  {
                    auto const partial_sums_index = this->num_threads_ * page_range_index + thread_index;
                    if (not is_called)
                    {
                      std::advance(d_iter, n);
                      this->partial_sums_[partial_sums_index] = this->unary_operation_(first[n]);
                      is_called = true;
                    }
                    else
                      this->partial_sums_[partial_sums_index]
                        = this->binary_operation_(
                            this->partial_sums_[partial_sums_index], this->unary_operation_(first[n]));

                    *d_iter++ = this->partial_sums_[patial_sums_index];
                  });

                std::advance(d_page_first, boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))));
              }

            post_process(
              parallel_policy_, local_state_, d_first_, binary_operation_,
              partial_sums_, thread_index, executor);
          }
        }; // struct process_in_execute<PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation>

        template <
          typename PartialSums, typename ParallelPolicy,
          typename LocalState, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static process_in_execute<PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation>
        make_process_in_execute(
          unsigned int num_threads, PartialSums& partial_sums,
          ParallelPolicy parallel_policy,
          LocalState& local_state, ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation)
        {
          using result_type
            = process_in_execute<PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation>;
          return result_type{
            num_threads, partial_sums, parallel_policy, local_state, d_first, binary_operation, unary_operation};
        }

        template <
          typename PartialSums, typename ParallelPolicy,
          typename LocalState, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation, typename Complex>
        class process_in_execute_with_initial_value
        {
          unsigned int num_threads_;
          PartialSums& partial_sums_;
          ParallelPolicy parallel_policy_;
          LocalState& local_state_;
          ForwardIterator d_first_;
          BinaryOperation binary_operation_;
          UnaryOperation unary_operation_;
          Complex initial_value_;

         public:
          process_in_execute_with_initial_value(
            unsigned int num_threads, PartialSums& partial_sums,
            ParallelPolicy parallel_policy,
            LocalState& local_state, ForwardIterator d_first,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            Complex initial_value)
            : num_threads_{num_threads},
              partial_sums_{partial_sums},
              parallel_policy_{parallel_policy},
              local_state_{local_state},
              d_first_{d_first},
              binary_operation_{binary_operation},
              unary_operation_{unary_operation},
              initial_value_{initial_value}
          { }

          template <typename Executor>
          void operator()(int const thread_index, Executor& executor)
          {
            auto d_page_first = d_first_;

            for (auto data_block_index = std::size_t{0u}; data_block_index < local_state_.num_data_blocks(); ++data_block_index)
              for (auto page_index = std::size_t{0u}; page_index < local_state.num_pages(); ++page_index)
              {
                auto const first = std::begin(local_state_.page_range(std::make_pair(data_block_index, page_index)));
                auto d_iter = d_page_first;
                auto is_called = false;
                auto const page_range_index = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                using difference_type
                  = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                ::ket::utility::loop_n_in_execute(
                  parallel_policy_,
                  boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                  [page_range_index, first, &d_iter, &is_called, this](
                    difference_type const n, int const thread_index)
                  {
                    auto const partial_sums_index = this->num_threads_ * page_range_index + thread_index;
                    if (not is_called)
                    {
                      std::advance(d_iter, n);
                      this->partial_sums_[partial_sums_index]
                        = thread_index == 0
                          ? this->binary_operation_(this->initial_value_, this->unary_operation_(first[n]))
                          : this->unary_operation_(first[n]);
                      is_called = true;
                    }
                    else
                      this->partial_sums_[partial_sums_index]
                        = this->binary_operation_(this->partial_sums_[partial_sums_index], this->unary_operation_(first[n]));

                    *d_iter++ = this->partial_sums_[partial_sums_index];
                  });

                std::advance(d_page_first, boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))));
              }

            post_process(
              parallel_policy_, local_state_, d_first_, binary_operation_,
              partial_sums_, thread_index, executor);
          }
        }; // struct process_in_execute_with_initial_value<PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation, Complex>

        template <
          typename PartialSums, typename ParallelPolicy,
          typename LocalState, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation, typename Complex>
        static process_in_execute_with_initial_value<
          PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation, Complex>
        make_process_in_execute_with_initial_value(
          unsigned int num_threads, PartialSums& partial_sums,
          ParallelPolicy parallel_policy,
          LocalState& local_state, ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex initial_value)
        {
          using result_type
            = process_in_execute_with_initial_value<
                PartialSums, ParallelPolicy, LocalState, ForwardIterator, BinaryOperation, UnaryOperation, Complex>;
          return result_type{
            num_threads, partial_sums, parallel_policy,
            local_state, d_first, binary_operation, unary_operation, initial_value};
        }
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const&)
        {
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto partial_sums = std::vector<Complex>(num_threads * num_data_blocks * num_pages);

#   ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            [num_data_blocks, num_threads, &partial_sums,
             parallel_policy, &local_state, d_first, binary_operation, unary_operation](
              int const thread_index, auto& executor)
            {
              auto d_page_first = d_first;

              for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
                for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
                {
                  auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
                  auto d_iter = d_page_first;
                  auto is_called = false;
                  auto const page_range_index
                    = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                  using difference_type
                    = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                  ::ket::utility::loop_n_in_execute(
                    parallel_policy,
                    boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                    [page_range_index, first, &d_iter, &is_called, num_threads, &partial_sums,
                     binary_operation, unary_operation](
                      difference_type const n, int const thread_index)
                    {
                      auto const partial_sums_index = num_threads * page_range_index + thread_index;
                      if (not is_called)
                      {
                        std::advance(d_iter, n);
                        partial_sums[partial_sums_index] = unary_operation(first[n]);
                        is_called = true;
                      }
                      else
                        partial_sums[partial_sums_index]
                          = binary_operation(partial_sums[partial_sums_index], unary_operation(first[n]));

                      *d_iter++ = partial_sums[partial_sums_index];
                    });

                  std::advance(d_page_first, boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))));
                }

              post_process(
                parallel_policy, local_state, d_first, binary_operation,
                partial_sums, thread_index, executor);
            });
#   else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            make_process_in_execute(
              num_threads, partial_sums, parallel_policy, local_state, d_first,
              binary_operation, unary_operation));
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

          return partial_sums.back();
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const&)
        {
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto partial_sums = std::vector<Complex>(num_threads * num_data_blocks * num_pages);

#   ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            [num_data_blocks, num_threads, &partial_sums,
             parallel_policy, &local_state, d_first, binary_operation, unary_operation,
             initial_value](
              int const thread_index, auto& executor)
            {
              auto d_page_first = d_first;

              for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
                for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
                {
                  auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
                  auto d_iter = d_page_first;
                  auto is_called = false;
                  auto const page_range_index
                    = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                  using difference_type
                    = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                  ::ket::utility::loop_n_in_execute(
                    parallel_policy,
                    boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                    [page_range_index, first, &d_iter, &is_called, num_threads, &partial_sums,
                     binary_operation, unary_operation, initial_value](
                      difference_type const n, int const thread_index)
                    {
                      auto const partial_sums_index = num_threads * page_range_index + thread_index;
                      if (not is_called)
                      {
                        std::advance(d_iter, n);
                        partial_sums[partial_sums_index]
                          = thread_index == 0
                            ? binary_operation(initial_value, unary_operation(first[n]))
                            : unary_operation(first[n]);
                        is_called = true;
                      }
                      else
                        partial_sums[partial_sums_index]
                          = binary_operation(partial_sums[partial_sums_index], unary_operation(first[n]));

                      *d_iter++ = partial_sums[partial_sums_index];
                    });

                  std::advance(d_page_first, boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))));
                }

              post_process(
                parallel_policy, local_state, d_first, binary_operation,
                partial_sums, thread_index, executor);
            });
#   else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            make_process_in_execute_with_initial_value(
              num_threads, partial_sums, parallel_policy, local_state, d_first,
              binary_operation, unary_operation, initial_value));
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

          return partial_sums.back();
        }

       private:
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename Executor>
        static void post_process(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          ForwardIterator d_first, BinaryOperation binary_operation,
          std::vector<Complex>& partial_sums, int const thread_index,
          Executor& executor)
        {
          ::ket::utility::barrier(parallel_policy, executor);

          ::ket::utility::single_execute(
            parallel_policy, executor,
            [&partial_sums, binary_operation]
            {
              std::partial_sum(
                std::begin(partial_sums), std::end(partial_sums),
                std::begin(partial_sums), binary_operation);
            });

          auto d_page_first = d_first;

          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();

          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              auto d_iter = d_page_first;
              auto is_called = false;
              auto const page_range_index
                = local_state.page_range_index(std::make_pair(data_block_index, page_index));

              using page_range_type = typename local_state_type::page_range_type;
              using page_iterator = typename boost::range_iterator<page_range_type>::type;
              using difference_type = typename std::iterator_traits<page_iterator>::difference_type;
              ::ket::utility::loop_n_in_execute(
                parallel_policy,
                boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                [page_range_index, &d_iter, &is_called,
                 num_threads, &partial_sums, binary_operation](
                  difference_type const n, int const thread_index)
                {
                  if (thread_index == 0u and page_range_index == 0u)
                    return;

                  auto const partial_sums_index = num_threads * page_range_index + thread_index;

                  if (not is_called)
                  {
                    std::advance(d_iter, n);
                    is_called = true;
                  }

                  *d_iter = binary_operation(partial_sums[partial_sums_index - 1u], *d_iter);
                  ++d_iter;
                });

              std::advance(d_page_first, boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))));
            }
        }
# else // KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const& environment)
        {
          return impl(
            typename std::iterator_traits<ForwardIterator>::iterator_category(),
            parallel_policy, local_state, d_first, binary_operation, unary_operation,
            environment);
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const& environment)
        {
          return impl(
            typename std::iterator_traits<ForwardIterator>::iterator_category(),
            parallel_policy, local_state, d_first, binary_operation, unary_operation,
            initial_value, environment);
        }

       private:
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex impl(
          std::forward_iterator_tag const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const&)
        {
          auto prev_d_first = d_first;
          d_first
            = ::ket::utility::ranges::transform_inclusive_scan(
                parallel_policy,
                local_state.page_range(std::make_pair(0u, 0u)), d_first, binary_operation, unary_operation);
          std::advance(prev_d_first, boost::size(local_state.page_range(std::make_pair(0u, 0u))) - 1);
          auto partial_sum = *prev_d_first;

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto page_range_index = std::size_t{1u}; page_range_index < num_pages * num_data_blocks; ++page_range_index)
          {
            auto const data_block_page_indices = local_state.data_block_page_indices(page_range_index);

            prev_d_first = d_first;
            d_first
              = ::ket::utility::ranges::transform_inclusive_scan(
                  parallel_policy,
                  local_state.page_range(data_block_page_indices), d_first,
                  binary_operation, unary_operation, partial_sum);
            std::advance(prev_d_first, boost::size(local_state.page_range(data_block_page_indices)) - 1);
            partial_sum = *prev_d_first;
          }

          return partial_sum;
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex impl(
          std::forward_iterator_tag const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          ForwardIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const&)
        {
          auto partial_sum = initial_value;

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              auto prev_d_first = d_first;
              d_first
                = ::ket::utility::ranges::transform_inclusive_scan(
                    parallel_policy,
                    local_state.page_range(std::make_pair(data_block_index, page_index)), d_first,
                    binary_operation, unary_operation, partial_sum);
              std::advance(prev_d_first, boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))) - 1);
              partial_sum = *prev_d_first;
            }

          return partial_sum;
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename BidirectionalIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex impl(
          std::bidirectional_iterator_tag const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          BidirectionalIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const&)
        {
          d_first
            = ::ket::utility::ranges::transform_inclusive_scan(
                parallel_policy,
                local_state.page_range(std::make_pair(0u, 0u)), d_first, binary_operation, unary_operation);
          auto partial_sum = *std::prev(d_first);

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto page_range_index = std::size_t{1u}; page_range_index < num_pages * num_data_blocks; ++page_range_index)
          {
            auto const data_block_page_indices = local_state.data_block_page_indices(page_range_index);

            d_first
              = ::ket::utility::ranges::transform_inclusive_scan(
                  parallel_policy,
                  local_state.page_range(data_block_page_indices), d_first,
                  binary_operation, unary_operation, partial_sum);
            partial_sum = *std::prev(d_first);
          }

          return partial_sum;
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename BidirectionalIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex impl(
          std::bidirectional_iterator_tag const,
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          BidirectionalIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const&)
        {
          auto partial_sum = initial_value;

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              d_first
                = ::ket::utility::ranges::transform_inclusive_scan(
                    parallel_policy,
                    local_state.page_range(std::make_pair(data_block_index, page_index)), d_first,
                    binary_operation, unary_operation, partial_sum);
              partial_sum = *std::prev(d_first);
            }

          return partial_sum;
        }
# endif // KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
      }; // struct transform_inclusive_scan<has_page_qubits>

      template <bool has_page_qubits>
      struct transform_inclusive_scan_self
      {
# ifdef KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
#   ifdef BOOST_NO_CXX14_GENERIC_LAMBDAS
        template <
          typename PartialSums, typename ParallelPolicy, typename LocalState,
          typename BinaryOperation, typename UnaryOperation>
        class process_in_execute
        {
          unsigned int num_threads_;
          PartialSums& partial_sums_;
          ParallelPolicy parallel_policy_;
          LocalState& local_state_;
          BinaryOperation binary_operation_;
          UnaryOperation unary_operation_;

         public:
          process_in_execute(
            unsigned int num_threads, PartialSums& partial_sums,
            ParallelPolicy parallel_policy, LocalState& local_state,
            BinaryOperation binary_operation, UnaryOperation unary_operation)
            : num_threads_{num_threads},
              partial_sums_{partial_sums},
              parallel_policy_{parallel_policy},
              local_state_{local_state},
              binary_operation_{binary_operation},
              unary_operation_{unary_operation}
          { }

          template <typename Executor>
          void operator()(int const thread_index, Executor& executor)
          {
            for (auto data_block_index = std::size_t{0u}; data_block_index < local_state_.num_data_blocks(); ++data_block_index)
              for (auto page_index = std::size_t{0u}; page_index < local_state.num_pages(); ++page_index)
              {
                auto const first = std::begin(local_state_.page_range(std::make_pair(data_block_index, page_index)));
                auto is_called = false;
                auto const page_range_index
                  = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                using difference_type
                  = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                ::ket::utility::loop_n_in_execute(
                  parallel_policy_,
                  boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                  [page_range_index, first, &is_called, this](difference_type const n, int const thread_index)
                  {
                    auto const partial_sums_index = this->num_threads_ * page_range_index + thread_index;
                    if (not is_called)
                    {
                      this->partial_sums_[partial_sums_index] = this->unary_operation_(first[n]);
                      is_called = true;
                    }
                    else
                      this->partial_sums_[partial_sums_index]
                        = this->binary_operation_(this->partial_sums_[partial_sums_index], this->unary_operation_(first[n]));

                    first[n] = this->partial_sums_[partial_sums_index];
                  });
              }

            post_process(
              parallel_policy_, local_state_, binary_operation_,
              partial_sums_, thread_index, executor);
          }
        }; // struct process_in_execute<PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation>

        template <
          typename PartialSums, typename ParallelPolicy, typename LocalState,
          typename BinaryOperation, typename UnaryOperation>
        static process_in_execute<PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation>
        make_process_in_execute(
          unsigned int num_threads, PartialSums& partial_sums,
          ParallelPolicy parallel_policy, LocalState& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation)
        {
          using result_type
            = process_in_execute<PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation>;
          return result_type{
            num_threads, partial_sums, parallel_policy, local_state, binary_operation, unary_operation};
        }

        template <
          typename PartialSums, typename ParallelPolicy, typename LocalState,
          typename BinaryOperation, typename UnaryOperation, typename Complex>
        class process_in_execute_with_initial_value
        {
          unsigned int num_threads_;
          PartialSums& partial_sums_;
          ParallelPolicy parallel_policy_;
          LocalState& local_state_;
          BinaryOperation binary_operation_;
          UnaryOperation unary_operation_;
          Complex initial_value_;

         public:
          process_in_execute_with_initial_value(
            unsigned int num_threads, PartialSums& partial_sums,
            ParallelPolicy parallel_policy, LocalState& local_state,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            Complex initial_value)
            : num_threads_{num_threads},
              partial_sums_{partial_sums},
              parallel_policy_{parallel_policy},
              local_state_{local_state},
              binary_operation_{binary_operation},
              unary_operation_{unary_operation},
              initial_value_{initial_value}
          { }

          template <typename Executor>
          void operator()(int const thread_index, Executor& executor)
          {
            for (auto data_block_index = std::size_t{0u}; data_block_index < local_state_.num_data_blocks(); ++data_block_index)
              for (auto page_index = std::size_t{0u}; page_index < local_state.num_pages(); ++page_index)
              {
                auto const first = std::begin(local_state_.page_range(std::make_pair(data_block_index, page_index)));
                auto is_called = false;
                auto const page_range_index
                  = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                using difference_type
                  = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                ::ket::utility::loop_n_in_execute(
                  parallel_policy_,
                  boost::size(local_state_.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                  [page_range_index, first, &is_called, this](difference_type const n, int const thread_index)
                  {
                    auto const partial_sums_index = this->num_threads_ * page_range_index + thread_index;
                    if (not is_called)
                    {
                      this->partial_sums_[partial_sums_index]
                        = page_index == 0 && thread_index == 0
                          ? this->binary_operation_(this->initial_value_, this->unary_operation_(first[n]))
                          : this->unary_operation_(first[n]);
                      is_called = true;
                    }
                    else
                      this->partial_sums_[partial_sums_index]
                        = this->binary_operation_(this->partial_sums_[partial_sums_index], this->unary_operation_(first[n]));

                    first[n] = this->partial_sums_[partial_sums_index];
                  });
              }

            post_process(
              parallel_policy_, local_state_, binary_operation_,
              partial_sums_, thread_index, executor);
          }
        }; // struct process_in_execute_with_initial_value<PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation, Complex>

        template <
          typename PartialSums, typename ParallelPolicy, typename LocalState,
          typename BinaryOperation, typename UnaryOperation, typename Complex>
        static process_in_execute_with_initial_value<
          PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation, Complex>
        make_process_in_execute_with_initial_value(
          unsigned int num_threads, PartialSums& partial_sums,
          ParallelPolicy parallel_policy, LocalState& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation, Complex initial_value)
        {
          using result_type
            = process_in_execute_with_initial_value<
                PartialSums, ParallelPolicy, LocalState, BinaryOperation, UnaryOperation, Complex>;
          return result_type{
            num_threads, partial_sums, parallel_policy,
            local_state, binary_operation, unary_operation, initial_value};
        }
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const&)
        {
          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          auto partial_sums = std::vector<Complex>(num_threads * num_data_blocks * num_pages);

#   ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            [num_threads, num_data_blocks, &partial_sums,
             parallel_policy, &local_state, binary_operation, unary_operation](
              int const thread_index, auto& executor)
            {
              for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
                for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
                {
                  auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
                  bool is_called = false;
                  auto const page_range_index
                    = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                  using difference_type
                    = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                  ::ket::utility::loop_n_in_execute(
                    parallel_policy,
                    boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                    [page_range_index, first, &is_called, num_threads, &partial_sums,
                     binary_operation, unary_operation](
                      difference_type const n, int const thread_index)
                    {
                      auto const partial_sums_index = num_threads * page_range_index + thread_index;
                      if (not is_called)
                      {
                        partial_sums[partial_sums_index] = unary_operation(first[n]);
                        is_called = true;
                      }
                      else
                        partial_sums[partial_sums_index]
                          = binary_operation(partial_sums[partial_sums_index], unary_operation(first[n]));

                      first[n] = partial_sums[partial_sums_index];
                    });
                }

              post_process(
                parallel_policy, local_state, binary_operation,
                partial_sums, thread_index, executor);
            });
#   else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            make_process_in_execute(
              num_threads, partial_sums, parallel_policy, local_state,
              binary_operation, unary_operation));
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

          return partial_sums.back();
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const&)
        {
          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          auto partial_sums = std::vector<Complex>(num_threads * num_data_blocks * num_pages);

#   ifndef BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            [num_threads, num_data_blocks, &partial_sums,
             parallel_policy, &local_state, binary_operation, unary_operation,
             initial_value](
              int const thread_index, auto& executor)
            {
              for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
                for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
                {
                  auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
                  auto is_called = false;
                  auto const page_range_index
                    = local_state.page_range_index(std::make_pair(data_block_index, page_index));

                  using difference_type
                    = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
                  ::ket::utility::loop_n_in_execute(
                    parallel_policy,
                    boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                    [page_range_index, first, &is_called, num_threads, &partial_sums,
                     binary_operation, unary_operation, initial_value](
                      difference_type const n, int const thread_index)
                    {
                      auto const partial_sums_index = num_threads * page_range_index + thread_index;
                      if (not is_called)
                      {
                        partial_sums[partial_sums_index]
                          = page_index == 0 && thread_index == 0
                            ? binary_operation(initial_value, unary_operation(first[n]))
                            : unary_operation(first[n]);
                        is_called = true;
                      }
                      else
                        partial_sums[partial_sums_index]
                          = binary_operation(partial_sums[partial_sums_index], unary_operation(first[n]));

                      first[n] = partial_sums[partial_sums_index];
                    });
                }

              post_process(
                parallel_policy, local_state, binary_operation,
                partial_sums, thread_index, executor);
            });
#   else // BOOST_NO_CXX14_GENERIC_LAMBDAS
          ::ket::utility::execute(
            parallel_policy,
            make_process_in_execute_with_initial_value(
              num_threads, partial_sums, parallel_policy, local_state,
              binary_operation, unary_operation, initial_value));
#   endif // BOOST_NO_CXX14_GENERIC_LAMBDAS

          return partial_sums.back();
        }

       private:
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename BinaryOperation,
          typename Executor>
        static void post_process(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          BinaryOperation binary_operation,
          std::vector<Complex>& partial_sums, int const thread_index,
          Executor& executor)
        {
          ::ket::utility::barrier(parallel_policy, executor);

          ::ket::utility::single_execute(
            parallel_policy, executor,
            [&partial_sums, binary_operation]
            {
              std::partial_sum(
                std::begin(partial_sums), std::end(partial_sums),
                std::begin(partial_sums), binary_operation);
            });

          auto const num_threads = static_cast<unsigned int>(::ket::utility::num_threads(parallel_policy));
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();

          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              auto const first = std::begin(local_state.page_range(std::make_pair(data_block_index, page_index)));
              auto const page_range_index
                = local_state.page_range_index(std::make_pair(data_block_index, page_index));

              using difference_type
                = typename std::iterator_traits<typename std::remove_cv<decltype(first)>::type>::difference_type;
              ::ket::utility::loop_n_in_execute(
                parallel_policy,
                boost::size(local_state.page_range(std::make_pair(data_block_index, page_index))), thread_index,
                [page_range_index, first, num_threads, &partial_sums, binary_operation](
                  difference_type const n, int const thread_index)
                {
                  if (thread_index == 0u and page_range_index == 0u)
                    return;

                  auto const partial_sums_index = num_threads * page_range_index + thread_index;
                  first[n] = binary_operation(partial_sums[partial_sums_index - 1u], first[n]);
                });
            }
        }
# else // KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const&)
        {
          ::ket::utility::ranges::transform_inclusive_scan(
            parallel_policy,
            local_state.page_range(std::make_pair(0u, 0u)),
            std::begin(local_state.page_range(std::make_pair(0u, 0u))),
            binary_operation, unary_operation);
          auto partial_sum = *std::prev(std::end(local_state.page_range(std::make_pair(0u, 0u))));

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto page_range_index = std::size_t{1u}; page_range_index < num_pages * num_data_blocks; ++page_range_index)
          {
            auto const data_block_page_indices = local_state.data_block_page_indices(page_range_index);

            ::ket::utility::ranges::transform_inclusive_scan(
              parallel_policy,
              local_state.page_range(data_block_page_indices),
              std::begin(local_state.page_range(data_block_page_indices)),
              binary_operation, unary_operation, partial_sum);
            partial_sum = *std::prev(std::end(local_state.page_range(data_block_page_indices)));
          }

          return partial_sum;
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const&)
        {
          auto partial_sum = initial_value;

          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              ::ket::utility::ranges::transform_inclusive_scan(
                parallel_policy,
                local_state.page_range(std::make_pair(data_block_index, page_index)),
                std::begin(local_state.page_range(std::make_pair(data_block_index, page_index))),
                binary_operation, unary_operation, partial_sum);
              partial_sum = *std::prev(std::end(local_state.page_range(std::make_pair(data_block_index, page_index))));
            }

          return partial_sum;
        }
# endif // KET_USE_PARALLEL_EXECUTE_FOR_TRANSFORM_INCLUSIVE_SCAN
      }; // struct transform_inclusive_scan_self<has_page_qubits>

      // Usually num_page_qubits is small, so linear search for page is probably not bad.
      template <bool has_page_qubits>
      struct upper_bound
      {
        template <typename Complex, typename Allocator, typename Compare>
        static typename ::ket::mpi::state<Complex, has_page_qubits, Allocator>::difference_type call(
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
          Complex const& value, Compare compare, yampi::environment const&)
        {
          auto const num_pages = local_state.num_pages();
          auto const num_data_blocks = local_state.num_data_blocks();
          using difference_type = typename ::ket::mpi::state<Complex, has_page_qubits, Allocator>::difference_type;

          for (auto data_block_index = std::size_t{0u}; data_block_index < num_data_blocks; ++data_block_index)
            for (auto page_index = std::size_t{0u}; page_index < num_pages; ++page_index)
            {
              auto const page_range = local_state.page_range(std::make_pair(data_block_index, page_index));
              if (not compare(value, *std::prev(std::end(page_range))))
                continue;

              auto const index_in_page
                = std::upper_bound(std::begin(page_range), std::end(page_range), value, compare)
                  - std::begin(page_range);

              auto const num_nonpage_local_qubits = local_state.num_local_qubits() - local_state.num_page_qubits();
              return static_cast<difference_type>((data_block_index << local_state.num_local_qubits()) bitor (page_index << num_nonpage_local_qubits) bitor index_in_page);
            }

          return static_cast<difference_type>(local_state.size());
        }
      }; // struct upper_bound<has_page_qubits>
    } // namespace state_detail


    template <typename Complex, typename Allocator>
    class state<Complex, false, Allocator>
    {
     public:
      using value_type = Complex;
      using allocator_type = typename Allocator::template rebind<value_type>::other;

     private:
      using data_type = std::vector<value_type, allocator_type>;
      data_type data_;

      std::size_t num_local_qubits_;
      std::size_t num_data_blocks_;

     public:
      using size_type = typename data_type::size_type;
      using difference_type = typename data_type::difference_type;
      using reference = typename data_type::reference;
      using const_reference = typename data_type::const_reference;
      using pointer = typename data_type::pointer;
      using const_pointer = typename data_type::const_pointer;
      using iterator = typename data_type::iterator;
      using const_iterator = typename data_type::const_iterator;
      using reverse_iterator = typename data_type::reverse_iterator;
      using const_reverse_iterator = typename data_type::const_reverse_iterator;

      state() = delete;
      ~state() noexcept = default;
      state(state const&) = default;
      state& operator=(state const&) = default;
      state(state&&) = default;
      state& operator=(state&&) = default;

      state(state const& other, allocator_type const& allocator)
        : data_{other.data_, allocator},
          num_local_qubits_{other.num_local_qubits_},
          num_data_blocks_{other.num_data_blocks_}
      { }

      state(state&& other, allocator_type const& allocator)
        : data_{std::move(other.data_), allocator},
          num_local_qubits_{std::move(other.num_local_qubits_)},
          num_data_blocks_{std::move(other.num_data_blocks_)}
      { }

      state(std::initializer_list<value_type> initializer_list, allocator_type const& allocator = allocator_type())
        : data_{initializer_list, allocator},
          num_local_qubits_{::ket::utility::integer_log2(initializer_list.size())},
          num_data_blocks_{1u}
      { }

      template <typename StateInteger>
      state(std::initializer_list<value_type> initializer_list, StateInteger const num_data_blocks, allocator_type const& allocator = allocator_type())
        : data_{initializer_list, allocator},
          num_local_qubits_{::ket::utility::integer_log2(initializer_list.size() / num_data_blocks)},
          num_data_blocks_{static_cast<std::size_t>(num_data_blocks)}
      { assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) * num_data_blocks_ == initializer_list.size()); }

      template <typename BitInteger, typename StateInteger, typename PermutationAllocator>
      state(
        BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
        : data_{generate_initial_data(
            ::ket::mpi::utility::policy::make_simple_mpi(),
            num_local_qubits, initial_integer, permutation, communicator, environment)},
          num_local_qubits_{num_local_qubits},
          num_data_blocks_{1u}
      { }

      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      state(
        MpiPolicy const& mpi_policy, BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
        : data_{generate_initial_data(
            mpi_policy, num_local_qubits, initial_integer, permutation, communicator, environment)},
          num_local_qubits_{num_local_qubits},
          num_data_blocks_{::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment)}
      { }

      void assign(std::initializer_list<value_type> initializer_list)
      { assign(initializer_list, 1u); }

      template <typename StateInteger>
      void assign(std::initializer_list<value_type> initializer_list, StateInteger const num_data_blocks)
      {
        data_.assign(initializer_list);
        num_local_qubits_ = ::ket::utility::integer_log2(initializer_list.size() / num_data_blocks);
        num_data_blocks_ = static_cast<std::size_t>(num_data_blocks);

        assert(::ket::utility::integer_exp2<std::size_t>(num_local_qubits_) * num_data_blocks_ == initializer_list.size());
      }

      template <typename BitInteger, typename StateInteger, typename PermutationAllocator>
      void assign(
        BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
      { assign(::ket::mpi::utility::policy::make_simple_mpi(), num_local_qubits, initial_integer, permutation, communicator, environment); }

      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      void assign(
        MpiPolicy const& mpi_policy, BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment)
      {
        initialize_data(data_, mpi_policy, num_local_qubits, initial_integer, permutation, communicator, environment);
        num_local_qubits_ = num_local_qubits;
        num_data_blocks_ = ::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment);
      }

      std::size_t num_local_qubits() const { return num_local_qubits_; }
      std::size_t num_data_blocks() const { return num_data_blocks_; }

      bool operator==(state const& other) const { return data_ == other.data_; }
      bool operator<(state const& other) const { return data_ < other.data_; }

      // Element access
      reference at(size_type const position) { return data_.at(position); }
      const_reference at(size_type const position) const { return data_.at(position); }
      reference operator[](size_type const position) { return data_[position]; }
      const_reference operator[](size_type const position) const { return data_[position]; }
      reference front() { return data_.front(); }
      const_reference front() const { return data_.front(); }
      reference back() { return data_.back(); }
      const_reference back() const { return data_.back(); }

      // Iterators
      iterator begin() noexcept { return data_.begin(); }
      const_iterator begin() const noexcept { return data_.begin(); }
      const_iterator cbegin() const noexcept { return data_.cbegin(); }
      iterator end() noexcept { return data_.end(); }
      const_iterator end() const noexcept { return data_.end(); }
      const_iterator cend() const noexcept { return data_.cend(); }
      reverse_iterator rbegin() noexcept { return data_.rbegin(); }
      const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
      const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }
      reverse_iterator rend() noexcept { return data_.rend(); }
      const_reverse_iterator rend() const noexcept { return data_.rend(); }
      const_reverse_iterator crend() const noexcept { return data_.crend(); }

      // Capacity
      size_type size() const noexcept { return data_.size(); }
      size_type max_size() const noexcept { return data_.max_size(); } 
      void reserve(size_type const new_capacity) { data_.reserve(new_capacity); }
      size_type capacity() const noexcept { return data_.capacity(); }
      void shrink_to_fit() { data_.shrink_to_fit(); }

      // Modifiers
      void swap(state& other)
        noexcept(KET_is_nothrow_swappable<data_type>::value)
      {
        using std::swap;
        swap(data_, other.data_);
        swap(num_local_qubits_, other.num_local_qubits_);
        swap(num_data_blocks_, other.num_data_blocks_);
      }

      data_type& data() { return data_; }
      data_type const& data() const { return data_; }

     private:
      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      void initialize_data(
        data_type& data,
        MpiPolicy const& mpi_policy, BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment) const
      {
        data.assign(
          ::ket::utility::integer_exp2<std::size_t>(num_local_qubits)
            * ::ket::mpi::utility::policy::num_data_blocks(mpi_policy, communicator, environment),
          value_type{0});

        using ::ket::mpi::permutate_bits;
        auto const rank_index
          = ::ket::mpi::utility::qubit_value_to_rank_index(
              mpi_policy, data, permutate_bits(permutation, initial_integer),
              communicator, environment);

        if (communicator.rank(environment) == rank_index.first)
          data[rank_index.second] = value_type{1};
      }

      template <typename MpiPolicy, typename BitInteger, typename StateInteger, typename PermutationAllocator>
      data_type generate_initial_data(
        MpiPolicy const& mpi_policy, BitInteger const num_local_qubits, StateInteger const initial_integer,
        ::ket::mpi::qubit_permutation<
          StateInteger, BitInteger, PermutationAllocator> const&
          permutation,
        yampi::communicator const& communicator,
        yampi::environment const& environment) const
      {
        auto result = data_type{};
        initialize_data(result, mpi_policy, num_local_qubits, initial_integer, permutation, communicator, environment);
        return result;
      }
    }; // class state<Complex, false, Allocator>

    template <typename Complex, bool has_page_qubits, typename Allocator>
    inline bool operator!=(
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& lhs,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& rhs)
    { return not(lhs == rhs); }

    template <typename Complex, bool has_page_qubits, typename Allocator>
    inline bool operator>(
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& lhs,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& rhs)
    { return rhs < lhs; }

    template <typename Complex, bool has_page_qubits, typename Allocator>
    inline bool operator<=(
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& lhs,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& rhs)
    { return not(lhs > rhs); }

    template <typename Complex, bool has_page_qubits, typename Allocator>
    inline bool operator>=(
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& lhs,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& rhs)
    { return not(lhs < rhs); }

    template <typename Complex,  bool has_page_qubits, typename Allocator>
    inline void swap(
      ::ket::mpi::state<Complex, has_page_qubits, Allocator>& lhs,
      ::ket::mpi::state<Complex, has_page_qubits, Allocator>& rhs)
      noexcept(
        KET_is_nothrow_swappable<
          ::ket::mpi::state<Complex, has_page_qubits, Allocator>>::value)
    { lhs.swap(rhs); }

    namespace state_detail
    {
      template <>
      struct swap_permutated_local_qubits<false>
      {
        template <
          typename ParallelPolicy, typename Complex, typename Allocator,
          typename StateInteger, typename BitInteger>
        static void call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
          ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2,
          StateInteger const num_data_blocks, StateInteger const data_block_size,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
          ::ket::mpi::utility::detail::swap_permutated_local_qubits(
            parallel_policy, local_state.data(), permutated_qubit1, permutated_qubit2,
            num_data_blocks, data_block_size, communicator, environment);
        }
      }; // struct swap_permutated_local_qubits<false>

      template <>
      struct interchange_qubits<false>
      {
        template <
          typename Allocator, typename Complex, typename Allocator_, typename StateInteger>
        static void call(
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          std::vector<Complex, Allocator_>& buffer,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const source_local_first_index,
          StateInteger const source_local_last_index,
          yampi::rank const target_rank,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
          assert(data_block_index < local_state.num_data_blocks());
          assert(data_block_size == ::ket::utility::integer_exp2<std::size_t>(local_state.num_local_qubits()));

          ::ket::mpi::utility::detail::interchange_qubits(
            local_state.data(), buffer, data_block_index, data_block_size,
            source_local_first_index, source_local_last_index,
            target_rank, communicator, environment);
        }

        template <
          typename Allocator, typename Complex, typename Allocator_, typename StateInteger,
          typename DerivedDatatype>
        static void call(
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          std::vector<Complex, Allocator_>& buffer,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const source_local_first_index,
          StateInteger const source_local_last_index,
          yampi::datatype_base<DerivedDatatype> const& datatype, yampi::rank const target_rank,
          yampi::communicator const& communicator, yampi::environment const& environment)
        {
          assert(data_block_index < local_state.num_data_blocks());
          assert(data_block_size == ::ket::utility::integer_exp2<std::size_t>(local_state.num_local_qubits()));

          ::ket::mpi::utility::detail::interchange_qubits(
            local_state.data(), buffer, data_block_index, data_block_size,
            source_local_first_index, source_local_last_index,
            datatype, target_rank, communicator, environment);
        }
      }; // struct interchange_qubits<false>

      template <>
      struct for_each_local_range<false>
      {
        template <
          typename MpiPolicy,
          typename Complex, typename Allocator,
          typename Function>
        static ::ket::mpi::state<Complex, false, Allocator>& call(
          MpiPolicy const& mpi_policy,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          yampi::communicator const& communicator, yampi::environment const& environment,
          Function&& function)
        {
          ::ket::mpi::utility::for_each_local_range(
            mpi_policy, local_state.data(),
            communicator, environment, std::forward<Function>(function));

          return local_state;
        }

        template <
          typename MpiPolicy,
          typename Complex, typename Allocator,
          typename Function>
        static ::ket::mpi::state<Complex, false, Allocator> const& call(
          MpiPolicy const& mpi_policy,
          ::ket::mpi::state<Complex, false, Allocator> const& local_state,
          yampi::communicator const& communicator, yampi::environment const& environment,
          Function&& function)
        {
          ::ket::mpi::utility::for_each_local_range(
            mpi_policy, local_state.data(),
            communicator, environment, std::forward<Function>(function));

          return local_state;
        }
      }; // struct for_each_local_range<false>

      template <>
      struct swap_local_data<false>
      {
        template <typename Complex, typename Allocator, typename StateInteger>
        static void call(
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          StateInteger const data_block_index1, StateInteger const local_first_index1, StateInteger const local_last_index1,
          StateInteger const data_block_index2, StateInteger const local_first_index2,
          StateInteger const data_block_size)
        {
          ::ket::mpi::utility::detail::swap_local_data(
            local_state.data(),
            data_block_index1, local_first_index1, local_last_index1,
            data_block_index2, local_first_index2, data_block_size);
        }
      }; // struct swap_local_data<false>

# ifdef KET_USE_DIAGONAL_LOOP
      template <>
      struct for_each_in_diagonal_loop<false>
      {
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename StateInteger, typename BitInteger,
          std::size_t num_local_control_qubits, typename Function>
        static void call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          StateInteger const data_block_index, StateInteger const data_block_size,
          StateInteger const last_local_qubit_value,
          std::array<
            ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > >,
            num_local_control_qubits> local_permutated_control_qubits,
          Function&& function)
        {
          using data_type = typename std::remove_cv<typename std::remove_reference<decltype(local_state.data())>::type>::type;
          using for_each_in_diagonal_loop_type
            = ::ket::mpi::utility::dispatch::for_each_in_diagonal_loop<data_type>;
          for_each_in_diagonal_loop_type::call(
            parallel_policy, local_state.data(),
            data_block_index, data_block_size, last_local_qubit_value,
            local_permutated_control_qubits, std::forward<Function>(function));
        }
      }; // struct for_each_in_diagonal_loop<false>
# endif // KET_USE_DIAGONAL_LOOP

      template <>
      struct transform_inclusive_scan<false>
      {
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator> const& local_state,
          ForwardIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const& environment)
        {
          return ::ket::mpi::utility::transform_inclusive_scan(
            parallel_policy,
            local_state.data(), d_first, binary_operation, unary_operation,
            environment);
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator, typename ForwardIterator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator> const& local_state,
          ForwardIterator const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const& environment)
        {
          return ::ket::mpi::utility::transform_inclusive_scan(
            parallel_policy,
            local_state.data(), d_first, binary_operation, unary_operation,
            initial_value, environment);
        }
      }; // struct transform_inclusive_scan<false>

      template <>
      struct transform_inclusive_scan_self<false>
      {
        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          yampi::environment const& environment)
        {
          return ::ket::mpi::utility::transform_inclusive_scan_self(
            parallel_policy,
            local_state.data(), binary_operation, unary_operation,
            environment);
        }

        template <
          typename ParallelPolicy,
          typename Complex, typename Allocator,
          typename BinaryOperation, typename UnaryOperation>
        static Complex call(
          ParallelPolicy const parallel_policy,
          ::ket::mpi::state<Complex, false, Allocator>& local_state,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Complex const initial_value, yampi::environment const& environment)
        {
          return ::ket::mpi::utility::transform_inclusive_scan_self(
            parallel_policy,
            local_state.data(), binary_operation, unary_operation,
            initial_value, environment);
        }
      }; // struct transform_inclusive_scan_self<false>

      template <>
      struct upper_bound<false>
      {
        template <typename Complex, typename Allocator, typename Compare>
        static typename ::ket::mpi::state<Complex, false, Allocator>::difference_type call(
          ::ket::mpi::state<Complex, false, Allocator> const& local_state,
          Complex const& value, Compare compare, yampi::environment const& environment)
        {
          return ::ket::mpi::utility::upper_bound(
            local_state.data(), value, compare, environment);
        }
      }; // struct upper_bound<false>
    } // namespace state_detail

    namespace page
    {
      template <
        typename StateInteger, typename BitInteger,
        typename Complex, typename Allocator>
      inline constexpr bool is_on_page(
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit,
        ::ket::mpi::state<Complex, false, Allocator> const& local_state)
      { return false; }

      template <
        typename StateInteger, typename BitInteger,
        typename Complex, typename Allocator>
      inline constexpr bool is_on_page(
        ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > > const permutated_control_qubit,
        ::ket::mpi::state<Complex, false, Allocator> const& local_state)
      { return false; }

      template <
        typename StateInteger, typename BitInteger, typename Complex, typename Allocator>
      inline bool is_on_page(
        ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit,
        ::ket::mpi::state<Complex, true, Allocator> const& local_state)
      { return ::ket::mpi::is_page_qubit(permutated_qubit, local_state); }

      template <
        typename StateInteger, typename BitInteger, typename Complex, typename Allocator>
      inline bool is_on_page(
        ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > > const permutated_control_qubit,
        ::ket::mpi::state<Complex, true, Allocator> const& local_state)
      { return ::ket::mpi::is_page_qubit(permutated_control_qubit, local_state); }
    } // namespace page

    namespace utility
    {
      namespace dispatch
      {
        template <typename LocalState_>
        struct swap_permutated_local_qubits;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct swap_permutated_local_qubits< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <typename ParallelPolicy, typename StateInteger, typename BitInteger>
          static void call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit1,
            ::ket::mpi::permutated< ::ket::qubit<StateInteger, BitInteger> > const permutated_qubit2,
            StateInteger const num_data_blocks, StateInteger const data_block_size,
            yampi::communicator const& communicator, yampi::environment const& environment)
          {
            ::ket::mpi::state_detail::swap_permutated_local_qubits<has_page_qubits>::call(
              parallel_policy, local_state, permutated_qubit1, permutated_qubit2,
              num_data_blocks, data_block_size, communicator, environment);
          }
        }; // struct swap_permutated_local_qubits< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename LocalState_>
        struct interchange_qubits;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct interchange_qubits< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <typename Allocator_, typename StateInteger>
          static void call(
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            std::vector<Complex, Allocator_>& buffer,
            StateInteger const data_block_index, StateInteger const data_block_size,
            StateInteger const source_local_first_index,
            StateInteger const source_local_last_index,
            yampi::rank const target_rank,
            yampi::communicator const& communicator, yampi::environment const& environment)
          {
            ::ket::mpi::state_detail::interchange_qubits<has_page_qubits>::call(
              local_state, buffer, data_block_index, data_block_size,
              source_local_first_index, source_local_last_index,
              target_rank, communicator, environment);
          }

          template <typename Allocator_, typename StateInteger, typename DerivedDatatype>
          static void call(
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            std::vector<Complex, Allocator_>& buffer,
            StateInteger const data_block_index, StateInteger const data_block_size,
            StateInteger const source_local_first_index,
            StateInteger const source_local_last_index,
            yampi::datatype_base<DerivedDatatype> const& datatype, yampi::rank const target_rank,
            yampi::communicator const& communicator, yampi::environment const& environment)
          {
            ::ket::mpi::state_detail::interchange_qubits<has_page_qubits>::call(
              local_state, buffer, data_block_index, data_block_size,
              source_local_first_index, source_local_last_index,
              datatype, target_rank, communicator, environment);
          }
        }; // struct interchange_qubits< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename LocalState_>
        struct for_each_local_range;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct for_each_local_range< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <typename MpiPolicy, typename Function>
          static ::ket::mpi::state<Complex, has_page_qubits, Allocator>& call(
            MpiPolicy const& mpi_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            yampi::communicator const& communicator, yampi::environment const& environment,
            Function&& function)
          {
            using for_each_local_range_type
              = ::ket::mpi::state_detail::for_each_local_range<has_page_qubits>;
            return for_each_local_range_type::call(
              mpi_policy, local_state, communicator, environment, std::forward<Function>(function));
          }

          template <typename MpiPolicy, typename Function>
          static ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& call(
            MpiPolicy const& mpi_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
            yampi::communicator const& communicator, yampi::environment const& environment,
            Function&& function)
          {
            using for_each_local_range_type
              = ::ket::mpi::state_detail::for_each_local_range<has_page_qubits>;
            return for_each_local_range_type::call(
              mpi_policy, local_state, communicator, environment, std::forward<Function>(function));
          }
        }; // struct for_each_local_range< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename LocalState_>
        struct swap_local_data;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct swap_local_data< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <typename StateInteger>
          static void call(
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            StateInteger const data_block_index1, StateInteger const local_first_index1, StateInteger const local_last_index1,
            StateInteger const data_block_index2, StateInteger const local_first_index2,
            StateInteger const data_block_size)
          {
            using swap_local_data_type
              = ::ket::mpi::state_detail::swap_local_data<has_page_qubits>;
            swap_local_data_type::call(
              local_state,
              data_block_index1, local_first_index1, local_last_index1,
              data_block_index2, local_first_index2, data_block_size);
          }
        }; // struct swap_local_data< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename Complex, typename Allocator>
        struct buffer_range< ::ket::mpi::state<Complex, true, Allocator> >
        {
          template <typename BufferAllocator>
          static auto call(
            ::ket::mpi::state<Complex, true, Allocator>& local_state,
            std::vector<Complex, BufferAllocator>&)
            -> decltype(local_state.buffer_range())
          { return local_state.buffer_range(); }

          template <typename BufferAllocator>
          static auto call(
            ::ket::mpi::state<Complex, true, Allocator> const& local_state,
            std::vector<Complex, BufferAllocator> const&)
            -> decltype(local_state.buffer_range())
          { return local_state.buffer_range(); }

          template <typename BufferAllocator>
          static auto call_begin(
            ::ket::mpi::state<Complex, true, Allocator>& local_state,
            std::vector<Complex, BufferAllocator>&)
            -> typename boost::range_iterator<decltype(local_state.buffer_range())>::type
          { return std::begin(local_state.buffer_range()); }

          template <typename BufferAllocator>
          static auto call_begin(
            ::ket::mpi::state<Complex, true, Allocator> const& local_state,
            std::vector<Complex, BufferAllocator> const&)
            -> typename boost::range_iterator<decltype(local_state.buffer_range())>::type
          { return std::begin(local_state.buffer_range()); }

          template <typename BufferAllocator>
          static auto call_end(
            ::ket::mpi::state<Complex, true, Allocator>& local_state,
            std::vector<Complex, BufferAllocator>&)
            -> typename boost::range_iterator<decltype(local_state.buffer_range())>::type
          { return std::end(local_state.buffer_range()); }

          template <typename BufferAllocator>
          static auto call_end(
            ::ket::mpi::state<Complex, true, Allocator> const& local_state,
            std::vector<Complex, BufferAllocator> const&)
            -> typename boost::range_iterator<decltype(local_state.buffer_range())>::type
          { return std::end(local_state.buffer_range()); }
        }; // struct buffer_range< ::ket::mpi::state<Complex, true, Allocator> >

        template <typename Complex, typename Allocator>
        struct buffer_range< ::ket::mpi::state<Complex, false, Allocator> >
        {
          template <typename BufferAllocator>
          static boost::iterator_range<typename std::vector<Complex, BufferAllocator>::iterator> call(
            ::ket::mpi::state<Complex, false, Allocator>&,
            std::vector<Complex, BufferAllocator>& buffer)
          { return boost::make_iterator_range(std::begin(buffer), std::end(buffer)); }

          template <typename BufferAllocator>
          static boost::iterator_range<typename std::vector<Complex, BufferAllocator>::const_iterator> call(
            ::ket::mpi::state<Complex, false, Allocator> const&,
            std::vector<Complex, BufferAllocator> const& buffer)
          { return boost::make_iterator_range(std::begin(buffer), std::end(buffer)); }

          template <typename BufferAllocator>
          static typename std::vector<Complex, BufferAllocator>::iterator call_begin(
            ::ket::mpi::state<Complex, false, Allocator>&,
            std::vector<Complex, BufferAllocator>& buffer)
          { return std::begin(buffer); }

          template <typename BufferAllocator>
          static typename std::vector<Complex, BufferAllocator>::const_iterator call_begin(
            ::ket::mpi::state<Complex, false, Allocator> const&,
            std::vector<Complex, BufferAllocator> const& buffer)
          { return std::begin(buffer); }

          template <typename BufferAllocator>
          static typename std::vector<Complex, BufferAllocator>::iterator call_end(
            ::ket::mpi::state<Complex, false, Allocator>&,
            std::vector<Complex, BufferAllocator> const& buffer)
          { return std::end(buffer); }

          template <typename BufferAllocator>
          static typename std::vector<Complex, BufferAllocator>::const_iterator call_end(
            ::ket::mpi::state<Complex, false, Allocator> const&,
            std::vector<Complex, BufferAllocator> const& buffer)
          { return std::end(buffer); }
        }; // struct buffer_range< ::ket::mpi::state<Complex, false, Allocator> >

# ifdef KET_USE_DIAGONAL_LOOP
        template <typename LocalState_>
        struct for_each_in_diagonal_loop;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct for_each_in_diagonal_loop< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <
            typename ParallelPolicy,
            typename StateInteger, typename BitInteger,
            std::size_t num_local_control_qubits, typename Function>
          static void call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            StateInteger const data_block_index, StateInteger const data_block_size,
            StateInteger const last_local_qubit_value,
            std::array<
              ::ket::mpi::permutated< ::ket::control< ::ket::qubit<StateInteger, BitInteger> > >,
              num_local_control_qubits> const& local_permutated_control_qubits,
            Function&& function)
          {
            ::ket::mpi::state_detail::for_each_in_diagonal_loop<has_page_qubits>::call(
              parallel_policy,
              local_state, data_block_index, data_block_size, last_local_qubit_value,
              local_permutated_control_qubits, std::forward<Function>(function));
          }
        }; // struct for_each_in_diagonal_loop< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
# endif // KET_USE_DIAGONAL_LOOP

        template <typename LocalState_>
        struct transform_inclusive_scan;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct transform_inclusive_scan<
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <
            typename ParallelPolicy, typename ForwardIterator,
            typename BinaryOperation, typename UnaryOperation>
          static Complex call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
            ForwardIterator const d_first,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            yampi::environment const& environment)
          {
            using transform_inclusive_scan_type
              = ::ket::mpi::state_detail::transform_inclusive_scan<has_page_qubits>;
            return transform_inclusive_scan_type::call(
              parallel_policy,
              local_state, d_first, binary_operation, unary_operation,
              environment);
          }

          template <
            typename ParallelPolicy, typename ForwardIterator,
            typename BinaryOperation, typename UnaryOperation>
          static Complex call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
            ForwardIterator const d_first,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            Complex const initial_value, yampi::environment const& environment)
          {
            using transform_inclusive_scan_type
              = ::ket::mpi::state_detail::transform_inclusive_scan<has_page_qubits>;
            return transform_inclusive_scan_type::call(
              parallel_policy,
              local_state, d_first, binary_operation, unary_operation,
              initial_value, environment);
          }
        }; // struct transform_inclusive_scan< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename LocalState_>
        struct transform_inclusive_scan_self;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct transform_inclusive_scan_self<
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <
            typename ParallelPolicy,
            typename BinaryOperation, typename UnaryOperation>
          static Complex call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            yampi::environment const& environment)
          {
            using transform_inclusive_scan_self_type
              = ::ket::mpi::state_detail::transform_inclusive_scan_self<has_page_qubits>;
            return transform_inclusive_scan_self_type::call(
              parallel_policy,
              local_state, binary_operation, unary_operation, environment);
          }

          template <
            typename ParallelPolicy,
            typename BinaryOperation, typename UnaryOperation>
          static Complex call(
            ParallelPolicy const parallel_policy,
            ::ket::mpi::state<Complex, has_page_qubits, Allocator>& local_state,
            BinaryOperation binary_operation, UnaryOperation unary_operation,
            Complex const initial_value, yampi::environment const& environment)
          {
            using transform_inclusive_scan_self_type
              = ::ket::mpi::state_detail::transform_inclusive_scan_self<has_page_qubits>;
            return transform_inclusive_scan_self_type::call(
              parallel_policy,
              local_state, binary_operation, unary_operation, initial_value, environment);
          }
        }; // struct transform_inclusive_scan_self< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >

        template <typename LocalState_>
        struct upper_bound;

        template <typename Complex, bool has_page_qubits, typename Allocator>
        struct upper_bound<
          ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
        {
          template <typename Compare>
          static typename ::ket::mpi::state<Complex, has_page_qubits, Allocator>::difference_type call(
            ::ket::mpi::state<Complex, has_page_qubits, Allocator> const& local_state,
            Complex const& value, Compare compare, yampi::environment const& environment)
          {
            using upper_bound_type
              = ::ket::mpi::state_detail::upper_bound<has_page_qubits>;
            return upper_bound_type::call(local_state, value, compare, environment);
          }
        }; // struct upper_bound< ::ket::mpi::state<Complex, has_page_qubits, Allocator> >
      } // namespace dispatch
    } // namespace utility
  } // namespace mpi
} // namespace ket


# undef KET_is_nothrow_swappable

#endif // KET_MPI_STATE_HPP
