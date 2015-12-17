#include "UnitTest.hh"
#include "nua.hh"

TEST_CASE(selector_call)
{
    nua::Context ctx;
    ctx(R"(
        function test_add(a, b, c, d)
            sum = 0.0
            if(a) then
                sum = sum + a
            end
            if(b) then
                sum = sum + b
            end
            if(c) then
                sum = sum + c
            end
            if(d) then
                sum = sum + d
            end
            return math.ceil(sum)
        end
    )");

    TEST_CHECK(ctx["test_add"](1).get<int>() == 1);
    TEST_CHECK(ctx["test_add"](1, 2).get<int>() == 3);
    TEST_CHECK(ctx["test_add"](1, 2, 3).get<int>() == 6);
    TEST_CHECK(ctx["test_add"](1, 2, 3, 4).get<int>() == 10);
    TEST_CHECK(ctx["test_add"](1, 2, 3, 4, 5).get<int>() == 10);
}

TEST_CASE(tuple_test)
{
    nua::Context ctx;
    ctx(R"(
        function test_tuple()
            return 233, "hello", "nua"
        end
    )");

    int num;
    std::string s1, s2;
    std::tie(num, s1, s2) = ctx["test_tuple"](1).get<int, std::string, std::string>();
    TEST_CHECK(num == 233);
    TEST_CHECK(s1 == "hello");
    TEST_CHECK(s2 == "nua");
}

TEST_CASE(store_test)
{
    nua::Context ctx;

    ctx["test_value"] = 123;

    ctx(R"(
        print(test_value)
    )");

    ctx["test_func"] = std::function<std::tuple<int, int>()>([]
    { 
        return std::make_tuple(1, 2); 
    });
    auto x = ctx["test_func"]().get<int, double>();
    std::cout << std::get<0>(x) << std::endl;
    std::cout << std::get<1>(x) << std::endl;
}