// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include <sstream>
#include "turbo/times/stop_watcher.h"
#include "turbo/var/var.h"
#include <gtest/gtest.h>

namespace {
    class StatusTest : public testing::Test {
    protected:
        void SetUp() {
        }

        void TearDown() {
            ASSERT_EQ(0UL, turbo::Variable::count_exposed());
        }
    };

    TEST_F(StatusTest, status) {
        turbo::StatusVar<std::string> st1;
        st1.set_value("hello {}", 9);
        ASSERT_EQ(0, st1.expose("var1"));
        ASSERT_EQ("hello 9", turbo::Variable::describe_exposed("var1"));
        ASSERT_EQ("\"hello 9\"", turbo::Variable::describe_exposed("var1", true));
        std::vector<std::string> vars;
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());

        turbo::StatusVar<std::string> st2;
        st2.set_value("world {}", 10);
        ASSERT_EQ(-1, st2.expose("var1"));
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());
        ASSERT_EQ("world 10", st2.get_description());
        ASSERT_EQ("hello 9", turbo::Variable::describe_exposed("var1"));
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());

        ASSERT_TRUE(st1.hide());
        ASSERT_EQ(0UL, turbo::Variable::count_exposed());
        ASSERT_EQ("", turbo::Variable::describe_exposed("var1"));
        ASSERT_EQ(0, st1.expose("var1"));
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());
        ASSERT_EQ("hello 9",
                  turbo::Variable::describe_exposed("var1"));

        ASSERT_EQ(0, st2.expose("var2"));
        ASSERT_EQ(2UL, turbo::Variable::count_exposed());
        ASSERT_EQ("hello 9", turbo::Variable::describe_exposed("var1"));
        ASSERT_EQ("world 10", turbo::Variable::describe_exposed("var2"));
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var2", vars[1]);

        ASSERT_TRUE(st2.hide());
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());
        ASSERT_EQ("", turbo::Variable::describe_exposed("var2"));
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var1", vars[0]);

        st2.expose("var2 again");
        ASSERT_EQ("world 10", turbo::Variable::describe_exposed("var2_again"));
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var2_again", vars[1]);
        ASSERT_EQ(2UL, turbo::Variable::count_exposed());

        turbo::StatusVar<std::string> st3("var3", "foobar");
        ASSERT_EQ("var3", st3.name());
        ASSERT_EQ(3UL, turbo::Variable::count_exposed());
        ASSERT_EQ("foobar", turbo::Variable::describe_exposed("var3"));
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(3UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var3", vars[1]);
        ASSERT_EQ("var2_again", vars[2]);
        ASSERT_EQ(3UL, turbo::Variable::count_exposed());

        turbo::StatusVar<int> st4("var4", 9);
        ASSERT_EQ("var4", st4.name());
        ASSERT_EQ(4UL, turbo::Variable::count_exposed());
        ASSERT_EQ("9", turbo::Variable::describe_exposed("var4"));
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(4UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ("var3", vars[1]);
        ASSERT_EQ("var4", vars[2]);
        ASSERT_EQ("var2_again", vars[3]);

        turbo::StatusVar<void *> st5((void *) 19UL);
        std::cout<<st5<<std::endl;
        ASSERT_EQ("0x13", st5.get_description());
    }

    void print1(std::ostream &os, void *arg) {
        os << arg;
    }

    int64_t print2(void *arg) {
        return *(int64_t *) arg;
    }

    TEST_F(StatusTest, passive_status) {
        turbo::BasicPassiveStatus<std::string> st1("var11", print1, (void *) 9UL);
        std::cout<<st1<<std::endl;
        std::ostringstream ss;
        ASSERT_EQ(0, turbo::Variable::describe_exposed("var11", ss));
        ASSERT_EQ("0x9", ss.str());
        std::vector<std::string> vars;
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var11", vars[0]);
        ASSERT_EQ(1UL, turbo::Variable::count_exposed());

        int64_t tmp2 = 9;
        turbo::BasicPassiveStatus<int64_t> st2("var12", print2, &tmp2);
        ss.str("");
        ASSERT_EQ(0, turbo::Variable::describe_exposed("var12", ss));
        ASSERT_EQ("9", ss.str());
        turbo::Variable::list_exposed(&vars);
        ASSERT_EQ(2UL, vars.size());
        ASSERT_EQ("var11", vars[0]);
        ASSERT_EQ("var12", vars[1]);
        ASSERT_EQ(2UL, turbo::Variable::count_exposed());
    }

    struct Foo {
        int x;

        Foo() : x(0) {}

        explicit Foo(int x2) : x(x2) {}

        Foo operator+(const Foo &rhs) const {
            return Foo(x + rhs.x);
        }
    };

    std::ostream &operator<<(std::ostream &os, const Foo &f) {
        return os << "Foo{" << f.x << "}";
    }

    TEST_F(StatusTest, non_primitive) {
        turbo::StatusVar<Foo> st;
        ASSERT_EQ(0, st.get_value().x);
        st.set_value(Foo(1));
        ASSERT_EQ(1, st.get_value().x);
    }
} // namespace
