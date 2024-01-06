// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include <turbo/taskflow/taskflow.h>
#include <turbo/taskflow/algorithm/reduce.h>

// ----------------------------------------------------------------------------
// Data Type
// ----------------------------------------------------------------------------

struct MoveOnly1{

  int a {-1234};
  
  MoveOnly1() = default;

  MoveOnly1(const MoveOnly1&) = delete;
  MoveOnly1(MoveOnly1&&) = default;

  MoveOnly1& operator = (const MoveOnly1& rhs) = delete;
  MoveOnly1& operator = (MoveOnly1&& rhs) = default;

};

struct MoveOnly2{

  int b {-1234};

  MoveOnly2() = default;

  MoveOnly2(const MoveOnly2&) = delete;
  MoveOnly2(MoveOnly2&&) = default;

  MoveOnly2& operator = (const MoveOnly2& rhs) = delete;
  MoveOnly2& operator = (MoveOnly2&& rhs) = default;

};

// --------------------------------------------------------
// Testcase: reduce
// --------------------------------------------------------

template <typename P>
void reduce(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<int> vec(1000);

  for(auto& i : vec) i = ::rand() % 100 - 50;

  for(size_t n=1; n<vec.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      int smin = std::numeric_limits<int>::max();
      int pmin = std::numeric_limits<int>::max();
      auto beg = vec.end();
      auto end = vec.end();

      taskflow.clear();
      auto stask = taskflow.emplace([&](){
        beg = vec.begin();
        end = vec.begin() + n;
        for(auto itr = beg; itr != end; itr++) {
          smin = std::min(*itr, smin);
        }
      });

      turbo::Task ptask;

      ptask = taskflow.reduce(
        std::ref(beg), std::ref(end), pmin, [](int& l, int& r){
        return std::min(l, r);
      }, P(c));

      stask.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(smin != std::numeric_limits<int>::max());
      REQUIRE(pmin != std::numeric_limits<int>::max());
      REQUIRE(smin == pmin);
    }
  }
}

// guided
TEST_CASE("Reduce.Guided.1thread" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(1);
}

TEST_CASE("Reduce.Guided.2threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(2);
}

TEST_CASE("Reduce.Guided.3threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(3);
}

TEST_CASE("Reduce.Guided.4threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(4);
}

TEST_CASE("Reduce.Guided.5threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(5);
}

TEST_CASE("Reduce.Guided.6threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(6);
}

TEST_CASE("Reduce.Guided.7threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(7);
}

TEST_CASE("Reduce.Guided.8threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(8);
}

TEST_CASE("Reduce.Guided.9threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(9);
}

TEST_CASE("Reduce.Guided.10threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(10);
}

TEST_CASE("Reduce.Guided.11threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(11);
}

TEST_CASE("Reduce.Guided.12threads" * doctest::timeout(300)) {
  reduce<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("Reduce.Dynamic.1thread" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(1);
}

TEST_CASE("Reduce.Dynamic.2threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(2);
}

TEST_CASE("Reduce.Dynamic.3threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(3);
}

TEST_CASE("Reduce.Dynamic.4threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(4);
}

TEST_CASE("Reduce.Dynamic.5threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(5);
}

TEST_CASE("Reduce.Dynamic.6threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(6);
}

TEST_CASE("Reduce.Dynamic.7threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(7);
}

TEST_CASE("Reduce.Dynamic.8threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(8);
}

TEST_CASE("Reduce.Dynamic.9threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(9);
}

TEST_CASE("Reduce.Dynamic.10threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(10);
}

TEST_CASE("Reduce.Dynamic.11threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(11);
}

TEST_CASE("Reduce.Dynamic.12threads" * doctest::timeout(300)) {
  reduce<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("Reduce.Static.1thread" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(1);
}

TEST_CASE("Reduce.Static.2threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(2);
}

TEST_CASE("Reduce.Static.3threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(3);
}

TEST_CASE("Reduce.Static.4threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(4);
}

TEST_CASE("Reduce.Static.5threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(5);
}

TEST_CASE("Reduce.Static.6threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(6);
}

TEST_CASE("Reduce.Static.7threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(7);
}

TEST_CASE("Reduce.Static.8threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(8);
}

TEST_CASE("Reduce.Static.9threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(9);
}

TEST_CASE("Reduce.Static.10threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(10);
}

TEST_CASE("Reduce.Static.11threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(11);
}

TEST_CASE("Reduce.Static.12threads" * doctest::timeout(300)) {
  reduce<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("Reduce.Random.1thread" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(1);
}

TEST_CASE("Reduce.Random.2threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(2);
}

TEST_CASE("Reduce.Random.3threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(3);
}

TEST_CASE("Reduce.Random.4threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(4);
}

TEST_CASE("Reduce.Random.5threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(5);
}

TEST_CASE("Reduce.Random.6threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(6);
}

TEST_CASE("Reduce.Random.7threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(7);
}

TEST_CASE("Reduce.Random.8threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(8);
}

TEST_CASE("Reduce.Random.9threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(9);
}

TEST_CASE("Reduce.Random.10threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(10);
}

TEST_CASE("Reduce.Random.11threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(11);
}

TEST_CASE("Reduce.Random.12threads" * doctest::timeout(300)) {
  reduce<turbo::RandomPartitioner>(12);
}

// --------------------------------------------------------
// Testcase: reduce_sum
// --------------------------------------------------------

template <typename P>
void reduce_sum(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<int> vec(1000);

  for(auto& i : vec) i = ::rand() % 100 - 50;

  for(size_t n=1; n<vec.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      auto sum = 0;
      auto sol = 0;
      auto beg = vec.end();
      auto end = vec.end();

      taskflow.clear();

      auto stask = taskflow.emplace([&](){
        beg = vec.begin();
        end = vec.begin() + n;
        for(auto itr = beg; itr != end; itr++) {
          sum += *itr;
        }
      });

      turbo::Task ptask;

      ptask = taskflow.reduce(
        std::ref(beg), std::ref(end), sol, [](int l, int r){
        return l + r;
      }, P(c));

      stask.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(sol == sum);
    }
  }
}

// guided
TEST_CASE("ReduceSum.Guided.1thread" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(1);
}

TEST_CASE("ReduceSum.Guided.2threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(2);
}

TEST_CASE("ReduceSum.Guided.3threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(3);
}

TEST_CASE("ReduceSum.Guided.4threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(4);
}

TEST_CASE("ReduceSum.Guided.5threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(5);
}

TEST_CASE("ReduceSum.Guided.6threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(6);
}

TEST_CASE("ReduceSum.Guided.7threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(7);
}

TEST_CASE("ReduceSum.Guided.8threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(8);
}

TEST_CASE("ReduceSum.Guided.9threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(9);
}

TEST_CASE("ReduceSum.Guided.10threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(10);
}

TEST_CASE("ReduceSum.Guided.11threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(11);
}

TEST_CASE("ReduceSum.Guided.12threads" * doctest::timeout(300)) {
  reduce_sum<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("ReduceSum.Dynamic.1thread" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(1);
}

TEST_CASE("ReduceSum.Dynamic.2threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(2);
}

TEST_CASE("ReduceSum.Dynamic.3threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(3);
}

TEST_CASE("ReduceSum.Dynamic.4threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(4);
}

TEST_CASE("ReduceSum.Dynamic.5threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(5);
}

TEST_CASE("ReduceSum.Dynamic.6threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(6);
}

TEST_CASE("ReduceSum.Dynamic.7threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(7);
}

TEST_CASE("ReduceSum.Dynamic.8threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(8);
}

TEST_CASE("ReduceSum.Dynamic.9threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(9);
}

TEST_CASE("ReduceSum.Dynamic.10threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(10);
}

TEST_CASE("ReduceSum.Dynamic.11threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(11);
}

TEST_CASE("ReduceSum.Dynamic.12threads" * doctest::timeout(300)) {
  reduce_sum<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("ReduceSum.Static.1thread" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(1);
}

TEST_CASE("ReduceSum.Static.2threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(2);
}

TEST_CASE("ReduceSum.Static.3threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(3);
}

TEST_CASE("ReduceSum.Static.4threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(4);
}

TEST_CASE("ReduceSum.Static.5threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(5);
}

TEST_CASE("ReduceSum.Static.6threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(6);
}

TEST_CASE("ReduceSum.Static.7threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(7);
}

TEST_CASE("ReduceSum.Static.8threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(8);
}

TEST_CASE("ReduceSum.Static.9threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(9);
}

TEST_CASE("ReduceSum.Static.10threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(10);
}

TEST_CASE("ReduceSum.Static.11threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(11);
}

TEST_CASE("ReduceSum.Static.12threads" * doctest::timeout(300)) {
  reduce_sum<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("ReduceSum.Random.1thread" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(1);
}

TEST_CASE("ReduceSum.Random.2threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(2);
}

TEST_CASE("ReduceSum.Random.3threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(3);
}

TEST_CASE("ReduceSum.Random.4threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(4);
}

TEST_CASE("ReduceSum.Random.5threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(5);
}

TEST_CASE("ReduceSum.Random.6threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(6);
}

TEST_CASE("ReduceSum.Random.7threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(7);
}

TEST_CASE("ReduceSum.Random.8threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(8);
}

TEST_CASE("ReduceSum.Random.9threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(9);
}

TEST_CASE("ReduceSum.Random.10threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(10);
}

TEST_CASE("ReduceSum.Random.11threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(11);
}

TEST_CASE("ReduceSum.Random.12threads" * doctest::timeout(300)) {
  reduce_sum<turbo::RandomPartitioner>(12);
}


// ----------------------------------------------------------------------------
// transform_reduce
// ----------------------------------------------------------------------------

class Data {

  private:

    int _v {::rand() % 100 - 50};

  public:

    int get() const { return _v; }
};

template <typename P>
void transform_reduce(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<Data> vec(1000);

  for(size_t n=1; n<vec.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      int smin = std::numeric_limits<int>::max();
      int pmin = std::numeric_limits<int>::max();
      auto beg = vec.end();
      auto end = vec.end();

      taskflow.clear();
      auto stask = taskflow.emplace([&](){
        beg = vec.begin();
        end = vec.begin() + n;
        for(auto itr = beg; itr != end; itr++) {
          smin = std::min(itr->get(), smin);
        }
      });

      turbo::Task ptask;

      ptask = taskflow.transform_reduce(
        std::ref(beg), std::ref(end), pmin,
        [] (int l, int r)   { return std::min(l, r); },
        [] (const Data& data) { return data.get(); },
        P(c)
      );

      stask.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(smin != std::numeric_limits<int>::max());
      REQUIRE(pmin != std::numeric_limits<int>::max());
      REQUIRE(smin == pmin);
    }
  }
}

// guided
TEST_CASE("TransformReduce.Guided.1thread" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(1);
}

TEST_CASE("TransformReduce.Guided.2threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(2);
}

TEST_CASE("TransformReduce.Guided.3threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(3);
}

TEST_CASE("TransformReduce.Guided.4threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(4);
}

TEST_CASE("TransformReduce.Guided.5threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(5);
}

TEST_CASE("TransformReduce.Guided.6threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(6);
}

TEST_CASE("TransformReduce.Guided.7threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(7);
}

TEST_CASE("TransformReduce.Guided.8threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(8);
}

TEST_CASE("TransformReduce.Guided.9threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(9);
}

TEST_CASE("TransformReduce.Guided.10threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(10);
}

TEST_CASE("TransformReduce.Guided.11threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(11);
}

TEST_CASE("TransformReduce.Guided.12threads" * doctest::timeout(300)) {
  transform_reduce<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("TransformReduce.Dynamic.1thread" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(1);
}

TEST_CASE("TransformReduce.Dynamic.2threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(2);
}

TEST_CASE("TransformReduce.Dynamic.3threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(3);
}

TEST_CASE("TransformReduce.Dynamic.4threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(4);
}

TEST_CASE("TransformReduce.Dynamic.5threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(5);
}

TEST_CASE("TransformReduce.Dynamic.6threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(6);
}

TEST_CASE("TransformReduce.Dynamic.7threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(7);
}

TEST_CASE("TransformReduce.Dynamic.8threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(8);
}

TEST_CASE("TransformReduce.Dynamic.9threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(9);
}

TEST_CASE("TransformReduce.Dynamic.10threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(10);
}

TEST_CASE("TransformReduce.Dynamic.11threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(11);
}

TEST_CASE("TransformReduce.Dynamic.12threads" * doctest::timeout(300)) {
  transform_reduce<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("TransformReduce.Static.1thread" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(1);
}

TEST_CASE("TransformReduce.Static.2threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(2);
}

TEST_CASE("TransformReduce.Static.3threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(3);
}

TEST_CASE("TransformReduce.Static.4threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(4);
}

TEST_CASE("TransformReduce.Static.5threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(5);
}

TEST_CASE("TransformReduce.Static.6threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(6);
}

TEST_CASE("TransformReduce.Static.7threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(7);
}

TEST_CASE("TransformReduce.Static.8threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(8);
}

TEST_CASE("TransformReduce.Static.9threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(9);
}

TEST_CASE("TransformReduce.Static.10threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(10);
}

TEST_CASE("TransformReduce.Static.11threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(11);
}

TEST_CASE("TransformReduce.Static.12threads" * doctest::timeout(300)) {
  transform_reduce<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("TransformReduce.Random.1thread" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(1);
}

TEST_CASE("TransformReduce.Random.2threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(2);
}

TEST_CASE("TransformReduce.Random.3threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(3);
}

TEST_CASE("TransformReduce.Random.4threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(4);
}

TEST_CASE("TransformReduce.Random.5threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(5);
}

TEST_CASE("TransformReduce.Random.6threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(6);
}

TEST_CASE("TransformReduce.Random.7threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(7);
}

TEST_CASE("TransformReduce.Random.8threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(8);
}

TEST_CASE("TransformReduce.Random.9threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(9);
}

TEST_CASE("TransformReduce.Random.10threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(10);
}

TEST_CASE("TransformReduce.Random.11threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(11);
}

TEST_CASE("TransformReduce.Random.12threads" * doctest::timeout(300)) {
  transform_reduce<turbo::RandomPartitioner>(12);
}

// ----------------------------------------------------------------------------
// Transform & Reduce on Movable Data
// ----------------------------------------------------------------------------

template <typename P>
void move_only_transform_reduce(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  for(size_t c : {0, 1, 3, 7, 99}) {

    P partitioner(c);
  
    taskflow.clear();

    const size_t N = 100000;
    std::vector<MoveOnly1> vec(N);

    for(auto& i : vec) i.a = 1;

    MoveOnly2 res;
    res.b = 100;

    taskflow.transform_reduce(vec.begin(), vec.end(), res,
      [](MoveOnly2 m1, MoveOnly2 m2) {
        MoveOnly2 res;
        res.b = m1.b + m2.b;
        return res;
      },
      [](const MoveOnly1& m) {
        MoveOnly2 n;
        n.b = m.a;
        return n;
      },
      partitioner
    );

    executor.run(taskflow).wait();

    REQUIRE(res.b == N + 100);
    
    // change vec data
    taskflow.clear();
    res.b = 0;
    
    taskflow.transform_reduce(vec.begin(), vec.end(), res,
      [](MoveOnly2&& m1, MoveOnly2&& m2) {
        MoveOnly2 res;
        res.b = m1.b + m2.b;
        return res;
      },
      [](MoveOnly1& m) {
        MoveOnly2 n;
        n.b = m.a;
        m.a = -7;
        return n;
      },
      partitioner
    );

    executor.run(taskflow).wait();
    REQUIRE(res.b == N);

    for(const auto& i : vec) {
      REQUIRE(i.a == -7);
    }

    // reduce
    taskflow.clear();
    MoveOnly1 red;
    red.a = 0;

    taskflow.reduce(vec.begin(), vec.end(), red,
      [](MoveOnly1& m1, MoveOnly1& m2){
        MoveOnly1 res;
        res.a = m1.a + m2.a;
        return res;
      },
      partitioner
    );  

    executor.run(taskflow).wait();
    REQUIRE(red.a == -7*N);
    
    taskflow.clear();
    red.a = 0;

    taskflow.reduce(vec.begin(), vec.end(), red,
      [](const MoveOnly1& m1, const MoveOnly1& m2){
        MoveOnly1 res;
        res.a = m1.a + m2.a;
        return res;
      },
      partitioner
    );  

    executor.run(taskflow).wait();
    REQUIRE(red.a == -7*N);
  }
}

// static
TEST_CASE("TransformReduce.MoveOnlyData.Static.1thread" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::StaticPartitioner>(1);
}

TEST_CASE("TransformReduce.MoveOnlyData.Static.2threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::StaticPartitioner>(2);
}

TEST_CASE("TransformReduce.MoveOnlyData.Static.3threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::StaticPartitioner>(3);
}

TEST_CASE("TransformReduce.MoveOnlyData.Static.4threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::StaticPartitioner>(4);
}

// dynamic
TEST_CASE("TransformReduce.MoveOnlyData.Guided.1thread" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(1);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.2threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(2);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.3threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(3);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.4threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(4);
}

// guided
TEST_CASE("TransformReduce.MoveOnlyData.Guided.1thread" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(1);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.2threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(2);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.3threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(3);
}

TEST_CASE("TransformReduce.MoveOnlyData.Guided.4threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::GuidedPartitioner>(4);
}

// random
TEST_CASE("TransformReduce.MoveOnlyData.Random.1thread" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::RandomPartitioner>(1);
}

TEST_CASE("TransformReduce.MoveOnlyData.Random.2threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::RandomPartitioner>(2);
}

TEST_CASE("TransformReduce.MoveOnlyData.Random.3threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::RandomPartitioner>(3);
}

TEST_CASE("TransformReduce.MoveOnlyData.Random.4threads" * doctest::timeout(300)) {
  move_only_transform_reduce<turbo::RandomPartitioner>(4);
}

// ----------------------------------------------------------------------------
// transform_reduce_sum
// ----------------------------------------------------------------------------

template <typename P>
void transform_reduce_sum(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<Data> vec(1000);

  for(size_t n=1; n<vec.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      auto sum = 100;
      auto sol = 100;
      auto beg = vec.end();
      auto end = vec.end();

      taskflow.clear();
      auto stask = taskflow.emplace([&](){
        beg = vec.begin();
        end = vec.begin() + n;
        for(auto itr = beg; itr != end; itr++) {
          sum += itr->get();
        }
      });

      turbo::Task ptask;

      ptask = taskflow.transform_reduce(
        std::ref(beg), std::ref(end), sol,
        [] (int l, int r)   { return  l + r; },
        [] (const Data& data) { return data.get(); },
        P(c)
      );

      stask.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(sol == sum);
    }
  }
}

// guided
TEST_CASE("TransformReduceSum.Guided.1thread" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(1);
}

TEST_CASE("TransformReduceSum.Guided.2threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(2);
}

TEST_CASE("TransformReduceSum.Guided.3threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(3);
}

TEST_CASE("TransformReduceSum.Guided.4threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(4);
}

TEST_CASE("TransformReduceSum.Guided.5threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(5);
}

TEST_CASE("TransformReduceSum.Guided.6threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(6);
}

TEST_CASE("TransformReduceSum.Guided.7threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(7);
}

TEST_CASE("TransformReduceSum.Guided.8threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(8);
}

TEST_CASE("TransformReduceSum.Guided.9threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(9);
}

TEST_CASE("TransformReduceSum.Guided.10threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(10);
}

TEST_CASE("TransformReduceSum.Guided.11threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(11);
}

TEST_CASE("TransformReduceSum.Guided.12threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("TransformReduceSum.Dynamic.1thread" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(1);
}

TEST_CASE("TransformReduceSum.Dynamic.2threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(2);
}

TEST_CASE("TransformReduceSum.Dynamic.3threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(3);
}

TEST_CASE("TransformReduceSum.Dynamic.4threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(4);
}

TEST_CASE("TransformReduceSum.Dynamic.5threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(5);
}

TEST_CASE("TransformReduceSum.Dynamic.6threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(6);
}

TEST_CASE("TransformReduceSum.Dynamic.7threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(7);
}

TEST_CASE("TransformReduceSum.Dynamic.8threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(8);
}

TEST_CASE("TransformReduceSum.Dynamic.9threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(9);
}

TEST_CASE("TransformReduceSum.Dynamic.10threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(10);
}

TEST_CASE("TransformReduceSum.Dynamic.11threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(11);
}

TEST_CASE("TransformReduceSum.Dynamic.12threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("TransformReduceSum.Static.1thread" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(1);
}

TEST_CASE("TransformReduceSum.Static.2threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(2);
}

TEST_CASE("TransformReduceSum.Static.3threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(3);
}

TEST_CASE("TransformReduceSum.Static.4threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(4);
}

TEST_CASE("TransformReduceSum.Static.5threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(5);
}

TEST_CASE("TransformReduceSum.Static.6threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(6);
}

TEST_CASE("TransformReduceSum.Static.7threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(7);
}

TEST_CASE("TransformReduceSum.Static.8threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(8);
}

TEST_CASE("TransformReduceSum.Static.9threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(9);
}

TEST_CASE("TransformReduceSum.Static.10threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(10);
}

TEST_CASE("TransformReduceSum.Static.11threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(11);
}

TEST_CASE("TransformReduceSum.Static.12threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("TransformReduceSum.Random.1thread" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(1);
}

TEST_CASE("TransformReduceSum.Random.2threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(2);
}

TEST_CASE("TransformReduceSum.Random.3threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(3);
}

TEST_CASE("TransformReduceSum.Random.4threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(4);
}

TEST_CASE("TransformReduceSum.Random.5threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(5);
}

TEST_CASE("TransformReduceSum.Random.6threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(6);
}

TEST_CASE("TransformReduceSum.Random.7threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(7);
}

TEST_CASE("TransformReduceSum.Random.8threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(8);
}

TEST_CASE("TransformReduceSum.Random.9threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(9);
}

TEST_CASE("TransformReduceSum.Random.10threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(10);
}

TEST_CASE("TransformReduceSum.Random.11threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(11);
}

TEST_CASE("TransformReduceSum.Random.12threads" * doctest::timeout(300)) {
  transform_reduce_sum<turbo::RandomPartitioner>(12);
}

// ----------------------------------------------------------------------------
// binary_transform_reduce
// ----------------------------------------------------------------------------
template <typename P>
void binary_transform_reduce(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<Data> vec1(1000);
  std::vector<Data> vec2(1000);

  for(size_t n=1; n<vec1.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {
      
      int smin = std::numeric_limits<int>::max();
      int pmin = std::numeric_limits<int>::max();
      auto beg1 = vec1.end();
      auto end1 = vec1.end();
      auto beg2 = vec2.end();
      auto end2 = vec2.end();

      taskflow.clear();
      auto stack = taskflow.emplace([&](){
        beg1 = vec1.begin();
        end1 = vec1.begin() + n;
        beg2 = vec2.begin();
        end2 = vec2.begin() + n;
        for (auto itr1 = beg1, itr2 = beg2; itr1 != end1 && itr2 != end2; itr1++, itr2++) {
          smin = std::min(itr1->get(), smin);
          smin = std::min(itr2->get(), smin);
        }
      });

      turbo::Task ptask;
      
      ptask = taskflow.transform_reduce(
        std::ref(beg1), std::ref(end1), std::ref(beg2), pmin,
        [] (int l, int r) { return std::min(l, r); },
        [] (const Data& data1, const Data& data2) { return std::min(data1.get(), data2.get()); },
        P(c)
      );

      stack.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(smin != std::numeric_limits<int>::max());
      REQUIRE(pmin != std::numeric_limits<int>::max());
      REQUIRE(smin == pmin);
    }
  }
}

// guided
TEST_CASE("BinaryTransformReduce.Guided.1thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(1);
}

TEST_CASE("BinaryTransformReduce.Guided.2thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(2);
}

TEST_CASE("BinaryTransformReduce.Guided.3thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(3);
}

TEST_CASE("BinaryTransformReduce.Guided.4thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(4);
}

TEST_CASE("BinaryTransformReduce.Guided.5thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(5);
}

TEST_CASE("BinaryTransformReduce.Guided.6thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(6);
}

TEST_CASE("BinaryTransformReduce.Guided.7thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(7);
}

TEST_CASE("BinaryTransformReduce.Guided.8thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(8);
}

TEST_CASE("BinaryTransformReduce.Guided.9thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(9);
}

TEST_CASE("BinaryTransformReduce.Guided.10thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(10);
}

TEST_CASE("BinaryTransformReduce.Guided.11thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(11);
}

TEST_CASE("BinaryTransformReduce.Guided.12thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("BinaryTransformReduce.Dynamic.1thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(1);
}

TEST_CASE("BinaryTransformReduce.Dynamic.2thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(2);
}

TEST_CASE("BinaryTransformReduce.Dynamic.3thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(3);
}

TEST_CASE("BinaryTransformReduce.Dynamic.4thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(4);
}

TEST_CASE("BinaryTransformReduce.Dynamic.5thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(5);
}

TEST_CASE("BinaryTransformReduce.Dynamic.6thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(6);
}

TEST_CASE("BinaryTransformReduce.Dynamic.7thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(7);
}

TEST_CASE("BinaryTransformReduce.Dynamic.8thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(8);
}

TEST_CASE("BinaryTransformReduce.Dynamic.9thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(9);
}

TEST_CASE("BinaryTransformReduce.Dynamic.10thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(10);
}

TEST_CASE("BinaryTransformReduce.Dynamic.11thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(11);
}

TEST_CASE("BinaryTransformReduce.Dynamic.12thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("BinaryTransformReduce.Static.1thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(1);
}

TEST_CASE("BinaryTransformReduce.Static.2thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(2);
}

TEST_CASE("BinaryTransformReduce.Static.3thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(3);
}

TEST_CASE("BinaryTransformReduce.Static.4thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(4);
}

TEST_CASE("BinaryTransformReduce.Static.5thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(5);
}

TEST_CASE("BinaryTransformReduce.Static.6thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(6);
}

TEST_CASE("BinaryTransformReduce.Static.7thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(7);
}

TEST_CASE("BinaryTransformReduce.Static.8thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(8);
}

TEST_CASE("BinaryTransformReduce.Static.9thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(9);
}

TEST_CASE("BinaryTransformReduce.Static.10thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(10);
}

TEST_CASE("BinaryTransformReduce.Static.11thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(11);
}

TEST_CASE("BinaryTransformReduce.Static.12thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("BinaryTransformReduce.Random.1thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(1);
}

TEST_CASE("BinaryTransformReduce.Random.2thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(2);
}

TEST_CASE("BinaryTransformReduce.Random.3thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(3);
}

TEST_CASE("BinaryTransformReduce.Random.4thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(4);
}

TEST_CASE("BinaryTransformReduce.Random.5thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(5);
}

TEST_CASE("BinaryTransformReduce.Random.6thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(6);
}

TEST_CASE("BinaryTransformReduce.Random.7thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(7);
}

TEST_CASE("BinaryTransformReduce.Random.8thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(8);
}

TEST_CASE("BinaryTransformReduce.Random.9thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(9);
}

TEST_CASE("BinaryTransformReduce.Random.10thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(10);
}

TEST_CASE("BinaryTransformReduce.Random.11thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(11);
}

TEST_CASE("BinaryTransformReduce.Random.12thread" * doctest::timeout(300)) {
  binary_transform_reduce<turbo::RandomPartitioner>(12);
}
// ----------------------------------------------------------------------------
// binary_transform_reduce_sum
// ----------------------------------------------------------------------------

template <typename P>
void binary_transform_reduce_sum(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<Data> vec1(1000);
  std::vector<Data> vec2(1000);

  for(size_t n=1; n<vec1.size(); n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      auto sum = 100;
      auto sol = 100;
      auto beg1 = vec1.end();
      auto end1 = vec1.end();
      auto beg2 = vec2.end();
      auto end2 = vec2.end();

      taskflow.clear();
      auto stask = taskflow.emplace([&](){
        beg1 = vec1.begin();
        end1 = vec1.begin() + n;
        beg2 = vec2.begin();
        end2 = vec2.begin() + n;
        for(auto itr1 = beg1, itr2 = beg2; itr1 != end1 && itr2 != end2; itr1++, itr2++) {
          sum += (itr1->get() + itr2->get());
        }
      });

      turbo::Task ptask;

      ptask = taskflow.transform_reduce(
        std::ref(beg1), std::ref(end1), std::ref(beg2), sol,
        [] (int l, int r)   { return  l + r; },
        [] (const Data& data1, const Data& data2) { return data1.get() + data2.get(); },
        P(c)
      );

      stask.precede(ptask);

      executor.run(taskflow).wait();

      REQUIRE(sol == sum);
      
    }
  }
}

// guided
TEST_CASE("BinaryTransformReduceSum.Guided.1thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(1);
}

TEST_CASE("BinaryTransformReduceSum.Guided.2thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(2);
}

TEST_CASE("BinaryTransformReduceSum.Guided.3thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(3);
}

TEST_CASE("BinaryTransformReduceSum.Guided.4thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(4);
}

TEST_CASE("BinaryTransformReduceSum.Guided.5thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(5);
}

TEST_CASE("BinaryTransformReduceSum.Guided.6thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(6);
}

TEST_CASE("BinaryTransformReduceSum.Guided.7thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(7);
}

TEST_CASE("BinaryTransformReduceSum.Guided.8thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(8);
}

TEST_CASE("BinaryTransformReduceSum.Guided.9thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(9);
}

TEST_CASE("BinaryTransformReduceSum.Guided.10thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(10);
}

TEST_CASE("BinaryTransformReduceSum.Guided.11thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(11);
}

TEST_CASE("BinaryTransformReduceSum.Guided.12thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("BinaryTransformReduceSum.Dynamic.1thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(1);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.2thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(2);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.3thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(3);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.4thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(4);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.5thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(5);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.6thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(6);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.7thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(7);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.8thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(8);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.9thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(9);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.10thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(10);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.11thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(11);
}

TEST_CASE("BinaryTransformReduceSum.Dynamic.12thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("BinaryTransformReduceSum.Static.1thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(1);
}

TEST_CASE("BinaryTransformReduceSum.Static.2thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(2);
}

TEST_CASE("BinaryTransformReduceSum.Static.3thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(3);
}

TEST_CASE("BinaryTransformReduceSum.Static.4thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(4);
}

TEST_CASE("BinaryTransformReduceSum.Static.5thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(5);
}

TEST_CASE("BinaryTransformReduceSum.Static.6thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(6);
}

TEST_CASE("BinaryTransformReduceSum.Static.7thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(7);
}

TEST_CASE("BinaryTransformReduceSum.Static.8thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(8);
}

TEST_CASE("BinaryTransformReduceSum.Static.9thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(9);
}

TEST_CASE("BinaryTransformReduceSum.Static.10thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(10);
}

TEST_CASE("BinaryTransformReduceSum.Static.11thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(11);
}

TEST_CASE("BinaryTransformReduceSum.Static.12thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("BinaryTransformReduceSum.Random.1thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(1);
}

TEST_CASE("BinaryTransformReduceSum.Random.2thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(2);
}

TEST_CASE("BinaryTransformReduceSum.Random.3thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(3);
}

TEST_CASE("BinaryTransformReduceSum.Random.4thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(4);
}

TEST_CASE("BinaryTransformReduceSum.Random.5thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(5);
}

TEST_CASE("BinaryTransformReduceSum.Random.6thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(6);
}

TEST_CASE("BinaryTransformReduceSum.Random.7thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(7);
}

TEST_CASE("BinaryTransformReduceSum.Random.8thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(8);
}

TEST_CASE("BinaryTransformReduceSum.Random.9thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(9);
}

TEST_CASE("BinaryTransformReduceSum.Random.10thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(10);
}

TEST_CASE("BinaryTransformReduceSum.Random.11thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(11);
}

TEST_CASE("BinaryTransformReduceSum.Random.12thread" * doctest::timeout(300)) {
  binary_transform_reduce_sum<turbo::RandomPartitioner>(12);
}

