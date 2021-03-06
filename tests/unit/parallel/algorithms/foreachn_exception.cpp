//  Copyright (c) 2014 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/hpx.hpp>
#include <hpx/include/parallel_for_each.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_for_each_n_exception(ExPolicy policy, IteratorTag)
{
    static_assert(
        hpx::parallel::execution::is_execution_policy<ExPolicy>::value,
        "hpx::parallel::execution::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try {
        hpx::parallel::for_each_n(policy,
            iterator(std::begin(c)), c.size(),
            [](std::size_t& v) { throw std::runtime_error("test"); });

        HPX_TEST(false);
    }
    catch(hpx::exception_list const& e) {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch(...) {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_n_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try {
        hpx::future<iterator> f =
            hpx::parallel::for_each_n(p,
                iterator(std::begin(c)), c.size(),
                [](std::size_t& v) { throw std::runtime_error("test"); });
        returned_from_algorithm = true;
        f.get();    // rethrow exception

        HPX_TEST(false);
    }
    catch(hpx::exception_list const& e) {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch(...) {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_for_each_n_exception()
{
    using namespace hpx::parallel;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_for_each_n_exception(execution::seq, IteratorTag());
    test_for_each_n_exception(execution::par, IteratorTag());

    test_for_each_n_exception_async(execution::seq(execution::task), IteratorTag());
    test_for_each_n_exception_async(execution::par(execution::task), IteratorTag());

#if defined(HPX_HAVE_GENERIC_EXECUTION_POLICY)
    test_for_each_n_exception(execution_policy(execution::seq), IteratorTag());
    test_for_each_n_exception(execution_policy(execution::par), IteratorTag());

    test_for_each_n_exception(execution_policy(execution::seq(execution::task)),
        IteratorTag());
    test_for_each_n_exception(execution_policy(execution::par(execution::task)),
        IteratorTag());
#endif
}

void for_each_n_exception_test()
{
    test_for_each_n_exception<std::random_access_iterator_tag>();
    test_for_each_n_exception<std::forward_iterator_tag>();
#if defined(HPX_HAVE_ALGORITHM_INPUT_ITERATOR_SUPPORT)
    test_for_each_n_exception<std::input_iterator_tag>();
#endif
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int)std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    for_each_n_exception_test();
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace boost::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run")
        ;

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {
        "hpx.os_threads=all"
    };

    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::init(desc_commandline, argc, argv, cfg), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
