#ifndef KET_UTILITY_LOOP_N_HPP
# define KET_UTILITY_LOOP_N_HPP

# include <iterator>
# include <algorithm>
# include <numeric>
# include <utility>
# include <type_traits>

# include <ket/utility/begin.hpp>
# include <ket/utility/end.hpp>


namespace ket
{
  namespace utility
  {
    namespace policy
    {
      struct sequential { };

      inline constexpr ::ket::utility::policy::sequential make_sequential() noexcept
      { return ::ket::utility::policy::sequential{}; }

      namespace meta
      {
        template <typename T>
        struct is_loop_n_policy
          : std::false_type
        { }; // struct is_loop_n_policy<T>

        template <>
        struct is_loop_n_policy< ::ket::utility::policy::sequential >
          : std::true_type
        { }; // struct is_loop_n_policy< ::ket::utility::policy::sequential >
      } // namespace meta
    } // namespace policy


    namespace dispatch
    {
      template <typename ParallelPolicy>
      struct num_threads
      {
        static constexpr unsigned int call(ParallelPolicy const);
      }; // struct num_threads<ParallelPolicy>

      template <>
      struct num_threads< ::ket::utility::policy::sequential >
      {
        static constexpr unsigned int call(
          ::ket::utility::policy::sequential const) noexcept
        { return 1u; }
      }; // struct num_threads< ::ket::utility::policy::sequential >
    } // namespace dispatch

    template <typename ParallelPolicy>
    inline constexpr unsigned int num_threads(ParallelPolicy const policy)
      noexcept(
        noexcept(
          ::ket::utility::dispatch::num_threads<ParallelPolicy>::call(policy)))
    { return ::ket::utility::dispatch::num_threads<ParallelPolicy>::call(policy); }


    namespace dispatch
    {
      template <typename ParallelPolicy, typename Integer>
      struct loop_n
      {
        template <typename Function>
        static void call(
          ParallelPolicy const, Integer const n, Function&& function);
      }; // struct loop_n<ParallelPolicy, Integer>

      template <typename Integer>
      struct loop_n< ::ket::utility::policy::sequential, Integer >
      {
        template <typename Function>
        static void call(
          ::ket::utility::policy::sequential const,
          Integer const n, Function&& function)
        {
          if (n < Integer{1})
            return;

          for (auto count = Integer{0}; count < n - Integer{1}; ++count)
            function(count, 0);
          std::forward<Function>(function)(n - Integer{1}, 0);
        }
      }; // struct loop_n< ::ket::utility::policy::sequential, Integer >
    } // namespace dispatch

    template <typename Integer, typename Function>
    inline void loop_n(Integer const n, Function&& function)
    {
      using loop_n_type
        = ::ket::utility::dispatch::loop_n< ::ket::utility::policy::sequential, Integer >;
      loop_n_type::call(
        ::ket::utility::policy::sequential(), n, std::forward<Function>(function));
    }

    template <typename ParallelPolicy, typename Integer, typename Function>
    inline void loop_n(
      ParallelPolicy const parallel_policy,
      Integer const n, Function&& function)
    {
      ::ket::utility::dispatch::loop_n<ParallelPolicy, Integer>::call(
        parallel_policy, n, std::forward<Function>(function));
    }


    // execute
    namespace dispatch
    {
      template <typename ParallelPolicy>
      class execute
      {
       public:
        template <typename Function>
        void invoke(ParallelPolicy const, Function&& function);
      }; // class execute<ParallelPolicy>

      template <>
      class execute< ::ket::utility::policy::sequential >
      {
       public:
        template <typename Function>
        void invoke(
          ::ket::utility::policy::sequential const, Function&& function)
        { function(0, *this); }
      }; // class execute< ::ket::utility::policy::sequential >

      template <typename ParallelPolicy>
      struct loop_n_in_execute
      {
        template <typename Integer, typename Function>
        static void call(
          ParallelPolicy const, Integer const, int const, Function&& function);
      }; // struct loop_n_in_execute<ParallelPolicy>

      template <>
      struct loop_n_in_execute< ::ket::utility::policy::sequential >
      {
        template <typename Integer, typename Function>
        static void call(
          ::ket::utility::policy::sequential const,
          Integer const n, int const thread_index, Function&& function)
        {
          if (n < Integer{1})
            return;

          for (auto count = Integer{0}; count < n - Integer{1}; ++count)
            function(count, thread_index);
          std::forward<Function>(function)(n - Integer{1}, thread_index);
        }
      }; // struct loop_n_in_execute< ::ket::utility::policy::sequential >

      template <typename ParallelPolicy>
      struct barrier
      {
        template <typename Executor>
        static void call(ParallelPolicy const, Executor&);
      };

      template <>
      struct barrier< ::ket::utility::policy::sequential >
      {
        static void call(
          ::ket::utility::policy::sequential const,
          ::ket::utility::dispatch::execute< ::ket::utility::policy::sequential >&)
        { }
      }; // struct barrier< ::ket::utility::policy::sequential >

      template <typename ParallelPolicy>
      struct single_execute
      {
        template <typename Executor, typename Function>
        static void call(
          ParallelPolicy const, Executor&, Function&& function);
      }; // struct single_execute<ParallelPolicy>

      template <>
      struct single_execute< ::ket::utility::policy::sequential >
      {
        template <typename Function>
        static void call(
          ::ket::utility::policy::sequential const,
          ::ket::utility::dispatch::execute< ::ket::utility::policy::sequential >&,
          Function&& function)
        { std::forward<Function>(function)(); }
      }; // struct single_execute< ::ket::utility::policy::sequential >
    } // namespace dispatch

    template <typename Function>
    inline void execute(Function&& function)
    {
      using execute_type
        = ::ket::utility::dispatch::execute< ::ket::utility::policy::sequential >;
      execute_type().invoke(
        ::ket::utility::policy::sequential(), std::forward<Function>(function));
    }

    template <typename ParallelPolicy, typename Function>
    inline void execute(
      ParallelPolicy const parallel_policy, Function&& function)
    {
      typedef ::ket::utility::dispatch::execute<ParallelPolicy> execute_type;
      execute_type().invoke(parallel_policy, std::forward<Function>(function));
    }

    template <typename Integer, typename Function>
    inline void loop_n_in_execute(
      Integer const n, int const thread_index, Function&& function)
    {
      using loop_n_in_execute_type
        = ::ket::utility::dispatch ::loop_n_in_execute< ::ket::utility::policy::sequential >;
      loop_n_in_execute_type::call(
        ::ket::utility::policy::sequential(),
        n, thread_index, std::forward<Function>(function));
    }

    template <typename ParallelPolicy, typename Integer, typename Function>
    inline void loop_n_in_execute(
      ParallelPolicy const parallel_policy,
      Integer const n, int const thread_index, Function&& function)
    {
      using loop_n_in_execute_type
        = ::ket::utility::dispatch::loop_n_in_execute<ParallelPolicy>;
      loop_n_in_execute_type::call(
        parallel_policy, n, thread_index, std::forward<Function>(function));
    }

    template <typename Executor>
    inline void barrier(Executor& executor)
    {
      ::ket::utility::dispatch::barrier< ::ket::utility::policy::sequential >::call(
        ::ket::utility::policy::sequential(), executor);
    }

    template <typename ParallelPolicy, typename Executor>
    inline void barrier(ParallelPolicy const parallel_policy, Executor& executor)
    { ::ket::utility::dispatch::barrier<ParallelPolicy>::call(parallel_policy, executor); }

    template <typename Executor, typename Function>
    inline void single_execute(
      Executor& executor, Function&& function)
    {
      ::ket::utility::dispatch::single_execute< ::ket::utility::policy::sequential >::call(
        ::ket::utility::policy::sequential(), executor, std::forward<Function>(function));
    }

    template <typename ParallelPolicy, typename Executor, typename Function>
    inline void single_execute(
      ParallelPolicy const parallel_policy, Executor& executor, Function&& function)
    {
      ::ket::utility::dispatch::single_execute<ParallelPolicy>::call(
        parallel_policy, executor, std::forward<Function>(function));
    }


    // fill
    namespace dispatch
    {
      template <typename ParallelPolicy>
      struct fill
      {
        template <typename ForwardIterator, typename Value, typename Category>
        static void call(
          ParallelPolicy const parallel_policy,
          ForwardIterator const first, ForwardIterator const last, Value const& value,
          Category const iterator_category);
      }; // struct fill<ParallelPolicy>

      template <>
      struct fill< ::ket::utility::policy::sequential >
      {
        template <typename ForwardIterator, typename Value>
        static void call(
          ::ket::utility::policy::sequential const,
          ForwardIterator const first, ForwardIterator const last, Value const& value,
          std::forward_iterator_tag const)
        { return std::fill(first, last, value); }
      }; // struct fill< ::ket::utility::policy::sequential >
    } // namespace dispatch

    template <typename ParallelPolicy, typename ForwardIterator, typename Value>
    inline void fill(
      ParallelPolicy const parallel_policy,
      ForwardIterator const first, ForwardIterator const last, Value const& value)
    {
      ::ket::utility::dispatch::fill<ParallelPolicy>::call(
        parallel_policy, first, last, value,
        typename std::iterator_traits<ForwardIterator>::iterator_category{});
    }

    template <typename ForwardIterator, typename Value>
    inline void fill(
      ForwardIterator const first, ForwardIterator const last, Value const& value)
    { ::ket::utility::fill(::ket::utility::policy::make_sequential(), first, last, value); }

    namespace ranges
    {
      template <typename ParallelPolicy, typename ForwardRange, typename Value>
      inline ForwardRange& fill(
        ParallelPolicy const parallel_policy, ForwardRange& range, Value const& value)
      {
        ::ket::utility::fill(parallel_policy, ::ket::utility::begin(range), ::ket::utility::end(range), value);
        return range;
      }

      template <typename ForwardRange, typename Value>
      inline ForwardRange& fill(ForwardRange& range, Value const& value)
      {
        ::ket::utility::fill(::ket::utility::begin(range), ::ket::utility::end(range), value);
        return range;
      }
    } // namespace ranges


    // inclusive_scan
    namespace dispatch
    {
      template <typename ParallelPolicy>
      struct inclusive_scan
      {
        template <
          typename ForwardIterator1, typename ForwardIterator2,
          typename Category1, typename Category2>
        static ForwardIterator2 call(
          ParallelPolicy const parallel_policy,
          ForwardIterator1 const first, ForwardIterator1 const last,
          ForwardIterator2 const d_first,
          Category1 const iterator_category1, Category2 const iterator_category2);

        template <
          typename ForwardIterator1, typename ForwardIterator2,
          typename BinaryOperation,
          typename Category1, typename Category2>
        static ForwardIterator2 call(
          ParallelPolicy const parallel_policy,
          ForwardIterator1 const first, ForwardIterator1 const last,
          ForwardIterator2 const d_first, BinaryOperation binary_operation,
          Category1 const iterator_category1, Category2 const iterator_category2);

        template <
          typename ForwardIterator1, typename ForwardIterator2,
          typename BinaryOperation, typename Value,
          typename Category1, typename Category2>
        static ForwardIterator2 call(
          ParallelPolicy const parallel_policy,
          ForwardIterator1 const first, ForwardIterator1 const last,
          ForwardIterator2 const d_first,
          BinaryOperation binary_operation, Value const initial_value,
          Category1 const iterator_category1, Category2 const iterator_category2);
      }; // struct inclusive_scan<ParallelPolicy>

      template <>
      struct inclusive_scan< ::ket::utility::policy::sequential >
      {
        template <typename InputIterator, typename OutputIterator, typename Category>
        static OutputIterator call(
          ::ket::utility::policy::sequential const,
          InputIterator const first, InputIterator const last,
          OutputIterator const d_first,
          std::input_iterator_tag const, Category const)
        { return std::partial_sum(first, last, d_first); }

        template <
          typename InputIterator, typename OutputIterator, typename BinaryOperation,
          typename Category>
        static OutputIterator call(
          ::ket::utility::policy::sequential const,
          InputIterator const first, InputIterator const last,
          OutputIterator const d_first, BinaryOperation binary_operation,
          std::input_iterator_tag const, Category const)
        { return std::partial_sum(first, last, d_first, binary_operation); }

        template <
          typename InputIterator, typename OutputIterator,
          typename BinaryOperation, typename Value, typename Category>
        static OutputIterator call(
          ::ket::utility::policy::sequential const,
          InputIterator first, InputIterator const last, OutputIterator d_first,
          BinaryOperation binary_operation, Value const initial_value,
          std::input_iterator_tag const, Category const)
        {
          if (first == last)
            return d_first;

          auto partial_sum = binary_operation(initial_value, *first);
          *d_first++ = partial_sum;

          while (++first != last)
          {
            partial_sum = binary_operation(partial_sum, *first);
            *d_first++ = partial_sum;
          }

          return d_first;
        }
      }; // struct inclusive_scan< ::ket::utility::policy::sequential >
    } // namespace dispatch

    template <
      typename ParallelPolicy, typename ForwardIterator1, typename ForwardIterator2>
    inline typename std::enable_if<
      ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
      ForwardIterator2>::type
    inclusive_scan(
      ParallelPolicy const parallel_policy,
      ForwardIterator1 const first, ForwardIterator1 const last,
      ForwardIterator2 const d_first)
    {
      return ::ket::utility::dispatch::inclusive_scan<ParallelPolicy>::call(
        parallel_policy, first, last, d_first,
        typename std::iterator_traits<ForwardIterator1>::iterator_category{},
        typename std::iterator_traits<ForwardIterator2>::iterator_category{});
    }

    template <typename InputIterator, typename OutputIterator>
    inline OutputIterator inclusive_scan(
      InputIterator const first, InputIterator const last,
      OutputIterator const d_first)
    { return std::partial_sum(first, last, d_first); }

    template <
      typename ParallelPolicy,
      typename ForwardIterator1, typename ForwardIterator2, typename BinaryOperation>
    inline typename std::enable_if<
      ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
      ForwardIterator2>::type
    inclusive_scan(
      ParallelPolicy const parallel_policy,
      ForwardIterator1 const first, ForwardIterator1 const last,
      ForwardIterator2 const d_first,
      BinaryOperation binary_operation)
    {
      return ::ket::utility::dispatch::inclusive_scan<ParallelPolicy>::call(
        parallel_policy, first, last, d_first, binary_operation,
        typename std::iterator_traits<ForwardIterator1>::iterator_category{},
        typename std::iterator_traits<ForwardIterator2>::iterator_category{});
    }

    template <typename InputIterator, typename OutputIterator, typename BinaryOperation>
    inline typename std::enable_if<
      not ::ket::utility::policy::meta::is_loop_n_policy<InputIterator>::value,
      OutputIterator>::type
    inclusive_scan(
      InputIterator const first, InputIterator const last,
      OutputIterator d_first,
      BinaryOperation binary_operation)
    { return std::partial_sum(first, last, d_first, binary_operation); }

    template <
      typename ParallelPolicy,
      typename ForwardIterator1, typename ForwardIterator2, typename BinaryOperation,
      typename Value>
    inline ForwardIterator2 inclusive_scan(
      ParallelPolicy const parallel_policy,
      ForwardIterator1 const first, ForwardIterator1 const last,
      ForwardIterator2 const d_first,
      BinaryOperation binary_operation, Value const initial_value)
    {
      return ::ket::utility::dispatch::inclusive_scan<ParallelPolicy>::call(
        parallel_policy, first, last, d_first, binary_operation, initial_value,
        typename std::iterator_traits<ForwardIterator1>::iterator_category{},
        typename std::iterator_traits<ForwardIterator2>::iterator_category{});
    }

    template <
      typename InputIterator, typename OutputIterator, typename BinaryOperation,
      typename Value>
    inline typename std::enable_if<
      not ::ket::utility::policy::meta::is_loop_n_policy<InputIterator>::value,
      OutputIterator>::type
    inclusive_scan(
      InputIterator const first, InputIterator const last,
      OutputIterator d_first,
      BinaryOperation binary_operation, Value const initial_value)
    {
      return ::ket::utility::dispatch::inclusive_scan< ::ket::utility::policy::sequential >::call(
        ::ket::utility::policy::make_sequential(),
        first, last, d_first, binary_operation, initial_value,
        typename std::iterator_traits<InputIterator>::iterator_category{},
        typename std::iterator_traits<OutputIterator>::iterator_category{});
    }

    namespace ranges
    {
      template <typename ParallelPolicy, typename ForwardRange, typename ForwardIterator>
      inline typename std::enable_if<
        ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
        ForwardIterator>::type
      inclusive_scan(
        ParallelPolicy const parallel_policy,
        ForwardRange const& range, ForwardIterator const first)
      {
        return ::ket::utility::inclusive_scan(
          parallel_policy, ::ket::utility::begin(range), ::ket::utility::end(range), first);
      }

      template <typename ForwardRange, typename ForwardIterator>
      inline ForwardIterator inclusive_scan(
        ForwardRange const& range, ForwardIterator const first)
      {
        return ::ket::utility::inclusive_scan(
          ::ket::utility::begin(range), ::ket::utility::end(range), first);
      }

      template <
        typename ParallelPolicy,
        typename ForwardRange, typename ForwardIterator, typename BinaryOperation>
      inline typename std::enable_if<
        ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
        ForwardIterator>::type
      inclusive_scan(
        ParallelPolicy const parallel_policy,
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation)
      {
        return ::ket::utility::inclusive_scan(
          parallel_policy,
          ::ket::utility::begin(range), ::ket::utility::end(range), first, binary_operation);
      }

      template <
        typename ForwardRange, typename ForwardIterator, typename BinaryOperation>
      inline typename std::enable_if<
        not ::ket::utility::policy::meta::is_loop_n_policy<ForwardRange>::value,
        ForwardIterator>::type
      inclusive_scan(
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation)
      {
        return ::ket::utility::inclusive_scan(
          ::ket::utility::begin(range), ::ket::utility::end(range), first, binary_operation);
      }

      template <
        typename ParallelPolicy,
        typename ForwardRange, typename ForwardIterator, typename BinaryOperation,
        typename Value>
      inline ForwardIterator inclusive_scan(
        ParallelPolicy const parallel_policy,
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, Value const initial_value)
      {
        return ::ket::utility::inclusive_scan(
          parallel_policy,
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, initial_value);
      }

      template <
        typename ForwardRange, typename ForwardIterator, typename BinaryOperation,
        typename Value>
      inline typename std::enable_if<
        not ::ket::utility::policy::meta::is_loop_n_policy<ForwardRange>::value,
        ForwardIterator>::type
      inclusive_scan(
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, Value const initial_value)
      {
        return ::ket::utility::inclusive_scan(
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, initial_value);
      }
    } // namespace ranges


    // transform inclusive_scan
    namespace dispatch
    {
      template <typename ParallelPolicy>
      struct transform_inclusive_scan
      {
        template <
          typename ForwardIterator1, typename ForwardIterator2,
          typename BinaryOperation, typename UnaryOperation,
          typename Category1, typename Category2>
        static ForwardIterator2 call(
          ParallelPolicy const parallel_policy,
          ForwardIterator1 const first, ForwardIterator1 const last,
          ForwardIterator2 const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Category1 const iterator_category1, Category2 const iterator_category2);

        template <
          typename ForwardIterator1, typename ForwardIterator2,
          typename BinaryOperation, typename UnaryOperation, typename Value,
          typename Category1, typename Category2>
        static ForwardIterator2 call(
          ParallelPolicy const parallel_policy,
          ForwardIterator1 const first, ForwardIterator1 const last,
          ForwardIterator2 const d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Value const initial_value,
          Category1 const iterator_category1, Category2 const iterator_category2);
      }; // struct transform_inclusive_scan<ParallelPolicy>

      template <>
      struct transform_inclusive_scan< ::ket::utility::policy::sequential >
      {
        template <
          typename InputIterator, typename OutputIterator,
          typename BinaryOperation, typename UnaryOperation, typename Category>
        static OutputIterator call(
          ::ket::utility::policy::sequential const,
          InputIterator first, InputIterator const last, OutputIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          std::input_iterator_tag const, Category const)
        {
          if (first == last)
            return d_first;

          auto partial_sum = unary_operation(*first);
          *d_first++ = partial_sum;

          while (++first != last)
          {
            partial_sum
              = binary_operation(partial_sum, unary_operation(*first));
            *d_first++ = partial_sum;
          }

          return d_first;
        }

        template <
          typename InputIterator, typename OutputIterator,
          typename BinaryOperation, typename UnaryOperation, typename Value,
          typename Category>
        static OutputIterator call(
          ::ket::utility::policy::sequential const,
          InputIterator first, InputIterator const last, OutputIterator d_first,
          BinaryOperation binary_operation, UnaryOperation unary_operation,
          Value const initial_value,
          std::input_iterator_tag const, Category const)
        {
          if (first == last)
            return d_first;

          auto partial_sum = binary_operation(initial_value, unary_operation(*first));
          *d_first++ = partial_sum;

          while (++first != last)
          {
            partial_sum
              = binary_operation(partial_sum, unary_operation(*first));
            *d_first++ = partial_sum;
          }

          return d_first;
        }
      }; // struct transform_inclusive_scan< ::ket::utility::policy::sequential >
    } // namespace dispatch

    template <
      typename ParallelPolicy,
      typename ForwardIterator1, typename ForwardIterator2,
      typename BinaryOperation, typename UnaryOperation>
    inline typename std::enable_if<
      ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
      ForwardIterator2>::type
    transform_inclusive_scan(
      ParallelPolicy const parallel_policy,
      ForwardIterator1 const first, ForwardIterator1 const last,
      ForwardIterator2 const d_first,
      BinaryOperation binary_operation, UnaryOperation unary_operation)
    {
      return ::ket::utility::dispatch::transform_inclusive_scan<ParallelPolicy>::call(
        parallel_policy, first, last, d_first, binary_operation, unary_operation,
        typename std::iterator_traits<ForwardIterator1>::iterator_category{},
        typename std::iterator_traits<ForwardIterator2>::iterator_category{});
    }

    template <
      typename InputIterator, typename OutputIterator,
      typename BinaryOperation, typename UnaryOperation>
    inline OutputIterator transform_inclusive_scan(
      InputIterator const first, InputIterator const last,
      OutputIterator d_first,
      BinaryOperation binary_operation, UnaryOperation unary_operation)
    {
      using transform_inclusive_scan_type
        = ::ket::utility::dispatch::transform_inclusive_scan< ::ket::utility::policy::sequential >;
      return transform_inclusive_scan_type::call(
        ::ket::utility::policy::make_sequential(),
        first, last, d_first, binary_operation, unary_operation,
        typename std::iterator_traits<InputIterator>::iterator_category{},
        typename std::iterator_traits<OutputIterator>::iterator_category{});
    }

    template <
      typename ParallelPolicy,
      typename ForwardIterator1, typename ForwardIterator2,
      typename BinaryOperation, typename UnaryOperation, typename Value>
    inline ForwardIterator2 transform_inclusive_scan(
      ParallelPolicy const parallel_policy,
      ForwardIterator1 const first, ForwardIterator1 const last,
      ForwardIterator2 const d_first,
      BinaryOperation binary_operation, UnaryOperation unary_operation,
      Value const initial_value)
    {
      return ::ket::utility::dispatch::transform_inclusive_scan<ParallelPolicy>::call(
        parallel_policy,
        first, last, d_first, binary_operation, unary_operation, initial_value,
        typename std::iterator_traits<ForwardIterator1>::iterator_category{},
        typename std::iterator_traits<ForwardIterator2>::iterator_category{});
    }

    template <
      typename InputIterator, typename OutputIterator,
      typename BinaryOperation, typename UnaryOperation, typename Value>
    inline typename std::enable_if<
      not ::ket::utility::policy::meta::is_loop_n_policy<InputIterator>::value,
      OutputIterator>::type
    transform_inclusive_scan(
      InputIterator const first, InputIterator const last,
      OutputIterator d_first,
      BinaryOperation binary_operation, UnaryOperation unary_operation,
      Value const initial_value)
    {
      using transform_inclusive_scan_type = ::ket::utility::dispatch::transform_inclusive_scan< ::ket::utility::policy::sequential >;
      return transform_inclusive_scan_type::call(
        ::ket::utility::policy::make_sequential(),
        first, last, d_first, binary_operation, unary_operation, initial_value,
        typename std::iterator_traits<InputIterator>::iterator_category{},
        typename std::iterator_traits<OutputIterator>::iterator_category{});
    }

    namespace ranges
    {
      template <
        typename ParallelPolicy,
        typename ForwardRange, typename ForwardIterator,
        typename BinaryOperation, typename UnaryOperation>
      inline typename std::enable_if<
        ::ket::utility::policy::meta::is_loop_n_policy<ParallelPolicy>::value,
        ForwardIterator>::type
      transform_inclusive_scan(
        ParallelPolicy const parallel_policy,
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, UnaryOperation unary_operation)
      {
        return ::ket::utility::transform_inclusive_scan(
          parallel_policy,
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, unary_operation);
      }

      template <
        typename ForwardRange, typename ForwardIterator,
        typename BinaryOperation, typename UnaryOperation>
      inline ForwardIterator transform_inclusive_scan(
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, UnaryOperation unary_operation)
      {
        return ::ket::utility::transform_inclusive_scan(
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, unary_operation);
      }

      template <
        typename ParallelPolicy,
        typename ForwardRange, typename ForwardIterator,
        typename BinaryOperation, typename UnaryOperation, typename Value>
      inline ForwardIterator transform_inclusive_scan(
        ParallelPolicy const parallel_policy,
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, UnaryOperation unary_operation,
        Value const initial_value)
      {
        return ::ket::utility::transform_inclusive_scan(
          parallel_policy,
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, unary_operation, initial_value);
      }

      template <
        typename ForwardRange, typename ForwardIterator,
        typename BinaryOperation, typename UnaryOperation, typename Value>
      inline typename std::enable_if<
        not ::ket::utility::policy::meta::is_loop_n_policy<ForwardRange>::value,
        ForwardIterator>::type
      transform_inclusive_scan(
        ForwardRange const& range, ForwardIterator const first,
        BinaryOperation binary_operation, UnaryOperation unary_operation,
        Value const initial_value)
      {
        return ::ket::utility::transform_inclusive_scan(
          ::ket::utility::begin(range), ::ket::utility::end(range), first,
          binary_operation, unary_operation, initial_value);
      }
    } // namespace ranges
  } // namespace utility
} // namespace ket


#endif // KET_UTILITY_LOOP_N_HPP
