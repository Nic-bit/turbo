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
#ifndef TURBO_TASKFLOW_CORE_GRAPH_H_
#define TURBO_TASKFLOW_CORE_GRAPH_H_

#include <atomic>
#include <vector>
#include <string>
#include <utility>
#include "turbo/meta/type_traits.h"
#include "turbo/meta/algorithm.h"
#include "turbo/memory/atomic_object_pool.h"
#include "turbo/base/check_math.h"
#include "turbo/container/small_vector.h"
#include "turbo/container/serializer.h"
#include "turbo/taskflow/core/error.h"
#include "turbo/taskflow/core/declarations.h"
#include "turbo/taskflow/core/semaphore.h"
#include "turbo/taskflow/core/environment.h"
#include "turbo/taskflow/core/topology.h"
#include "turbo/taskflow/core/tsq.h"

/**
@file graph.h
@brief graph include file
*/

namespace turbo {

    // ----------------------------------------------------------------------------
    // Class: Graph
    // ----------------------------------------------------------------------------

    /**
    @class Graph

    @brief class to create a graph object

    A graph is the ultimate storage for a task dependency graph and is the main
    gateway to interact with an executor.
    A graph manages a set of nodes in a global object pool that animates and
    recycles node objects efficiently without going through repetitive and
    expensive memory allocations and deallocations.
    This class is mainly used for creating an opaque graph object in a custom
    class to interact with the executor through taskflow composition.

    A graph object is move-only.
    */
    class Graph {

        friend class Node;

        friend class FlowBuilder;

        friend class Subflow;

        friend class Taskflow;

        friend class Executor;

    public:

        /**
        @brief constructs a graph object
        */
        Graph() = default;

        /**
        @brief disabled copy constructor
        */
        Graph(const Graph & rhs) = delete;

        /**
        @brief constructs a graph using move semantics
        */
        Graph(Graph &&rhs) noexcept;

        /**
        @brief destructs the graph object
        */
        ~Graph();

        /**
        @brief disabled copy assignment operator
        */
        Graph &operator=(const Graph &) = delete;

        /**
        @brief assigns a graph using move semantics
        */
        Graph &operator=(Graph && rhs) noexcept;

        /**
        @brief queries if the graph is empty
        */
        [[nodiscard]] bool empty() const;

        /**
        @brief queries the number of nodes in the graph
        */
        [[nodiscard]] size_t size() const;

        /**
        @brief clears the graph
        */
        void clear();

    private:

        std::vector<Node *> _nodes;

        void clear_impl();

        void clear_detached();

        void merge(Graph && rhs);

        void erase(Node *);

        /**
        @private
        */
        template<typename ...ArgsT>
        Node *emplace_back(ArgsT &&...);
    };

    // ----------------------------------------------------------------------------

    /**
    @class Runtime

    @brief class to include a runtime object in a task

    A runtime object allows users to interact with the
    scheduling runtime inside a task, such as scheduling an active task,
    spawning a subflow, and so on.

    @code{.cpp}
    turbo::Task A, B, C, D;
    std::tie(A, B, C, D) = taskflow.emplace(
      [] () { return 0; },
      [&C] (turbo::Runtime& rt) {  // C must be captured by reference
        std::cout << "B\n";
        rt.schedule(C);
      },
      [] () { std::cout << "C\n"; },
      [] () { std::cout << "D\n"; }
    );
    A.precede(B, C, D);
    executor.run(taskflow).wait();
    @endcode

    A runtime object is associated with the worker and the executor
    that runs the task.

    */
    class Runtime {

        friend class Executor;

        friend class FlowBuilder;

    public:

        /**
        @brief destroys the runtime object

        Issues a turbo::Runtime::corun_all to finish all spawned asynchronous tasks
        and then destroys the runtime object.
        */
        ~Runtime();

        /**
        @brief obtains the running executor

        The running executor of a runtime task is the executor that runs
        the parent taskflow of that runtime task.

        @code{.cpp}
        turbo::Executor executor;
        turbo::Taskflow taskflow;
        taskflow.emplace([&](turbo::Runtime& rt){
          assert(&(rt.executor()) == &executor);
        });
        executor.run(taskflow).wait();
        @endcode
        */
        Executor &executor();

        /**
        @brief schedules an active task immediately to the worker's queue

        @param task the given active task to schedule immediately

        This member function immediately schedules an active task to the
        task queue of the associated worker in the runtime task.
        An active task is a task in a running taskflow.
        The task may or may not be running, and scheduling that task
        will immediately put the task into the task queue of the worker
        that is running the runtime task.
        Consider the following example:

        @code{.cpp}
        turbo::Task A, B, C, D;
        std::tie(A, B, C, D) = taskflow.emplace(
          [] () { return 0; },
          [&C] (turbo::Runtime& rt) {  // C must be captured by reference
            std::cout << "B\n";
            rt.schedule(C);
          },
          [] () { std::cout << "C\n"; },
          [] () { std::cout << "D\n"; }
        );
        A.precede(B, C, D);
        executor.run(taskflow).wait();
        @endcode

        The executor will first run the condition task @c A which returns @c 0
        to inform the scheduler to go to the runtime task @c B.
        During the execution of @c B, it directly schedules task @c C without
        going through the normal taskflow graph scheduling process.
        At this moment, task @c C is active because its parent taskflow is running.
        When the taskflow finishes, we will see both @c B and @c C in the output.
        */
        void schedule(Task task);

        /**
        @brief runs the given callable asynchronously

        @tparam F callable type
        @param f callable object

        The method creates an asynchronous task to launch the given
        function on the given arguments.
        The difference to turbo::Executor::async is that the created asynchronous task
        pertains to the runtime object.
        Applications can explicitly issue turbo::Runtime::corun_all
        to wait for all spawned asynchronous tasks to finish.
        For example:

        @code{.cpp}
        std::atomic<int> counter(0);
        taskflow.emplace([&](turbo::Runtime& rt){
          auto fu1 = rt.async([&](){ counter++; });
          auto fu2 = rt.async([&](){ counter++; });
          fu1.get();
          fu2.get();
          assert(counter == 2);

          // spawn 100 asynchronous tasks from the worker of the runtime
          for(int i=0; i<100; i++) {
            rt.async([&](){ counter++; });
          }

          // wait for the 100 asynchronous tasks to finish
          rt.corun_all();
          assert(counter == 102);
        });
        @endcode

        This method is thread-safe and can be called by multiple workers
        that hold the reference to the runtime.
        For example, the code below spawns 100 tasks from the worker of
        a runtime, and each of the 100 tasks spawns another task
        that will be run by another worker.

        @code{.cpp}
        std::atomic<int> counter(0);
        taskflow.emplace([&](turbo::Runtime& rt){
          // worker of the runtime spawns 100 tasks each spawning another task
          // that will be run by another worker
          for(int i=0; i<100; i++) {
            rt.async([&](){
              counter++;
              rt.async([](){ counter++; });
            });
          }

          // wait for the 200 asynchronous tasks to finish
          rt.corun_all();
          assert(counter == 200);
        });
        @endcode
        */
        template<typename F>
        auto async(F &&f);

        /**
        @brief similar to turbo::Runtime::async but assigns the task a name

        @tparam F callable type

        @param name assigned name to the task
        @param f callable

        @code{.cpp}
        taskflow.emplace([&](turbo::Runtime& rt){
          auto future = rt.async("my task", [](){});
          future.get();
        });
        @endcode

        */
        template<typename F>
        auto async(const std::string &name, F &&f);

        /**
        @brief runs the given function asynchronously without returning any future object

        @tparam F callable type
        @param f callable

        This member function is more efficient than turbo::Runtime::async
        and is encouraged to use when there is no data returned.

        @code{.cpp}
        std::atomic<int> counter(0);
        taskflow.emplace([&](turbo::Runtime& rt){
          for(int i=0; i<100; i++) {
            rt.silent_async([&](){ counter++; });
          }
          rt.corun_all();
          assert(counter == 100);
        });
        @endcode

        This member function is thread-safe.
        */
        template<typename F>
        void silent_async(F &&f);

        /**
        @brief similar to turbo::Runtime::silent_async but assigns the task a name

        @tparam F callable type
        @param name assigned name to the task
        @param f callable

        @code{.cpp}
        taskflow.emplace([&](turbo::Runtime& rt){
          rt.silent_async("my task", [](){});
          rt.corun_all();
        });
        @endcode
        */
        template<typename F>
        void silent_async(const std::string &name, F &&f);

        /**
        @brief similar to turbo::Runtime::silent_async but the caller must be the worker of the runtime

        @tparam F callable type

        @param name assigned name to the task
        @param f callable

        The method bypass the check of the caller worker from the executor
        and thus can only called by the worker of this runtime.

        @code{.cpp}
        taskflow.emplace([&](turbo::Runtime& rt){
          // running by the worker of this runtime
          rt.silent_async_unchecked("my task", [](){});
          rt.corun_all();
        });
        @endcode
        */
        template<typename F>
        void silent_async_unchecked(const std::string &name, F &&f);

        /**
        @brief co-runs the given target and waits until it completes

        A target can be one of the following forms:
          + a dynamic task to spawn a subflow or
          + a composable graph object with `turbo::Graph& T::graph()` defined

        @code{.cpp}
        // co-run a subflow and wait until all tasks complete
        taskflow.emplace([](turbo::Runtime& rt){
          rt.corun([](turbo::Subflow& sf){
            turbo::Task A = sf.emplace([](){});
            turbo::Task B = sf.emplace([](){});
          });
        });

        // co-run a taskflow and wait until all tasks complete
        turbo::Taskflow taskflow1, taskflow2;
        taskflow1.emplace([](){ std::cout << "running taskflow1\n"; });
        taskflow2.emplace([&](turbo::Runtime& rt){
          std::cout << "running taskflow2\n";
          rt.corun(taskflow1);
        });
        executor.run(taskflow2).wait();
        @endcode

        Although turbo::Runtime::corun blocks until the operation completes,
        the caller thread (worker) is not blocked (e.g., sleeping or holding any lock).
        Instead, the caller thread joins the work-stealing loop of the executor
        and returns when all tasks in the target completes.

        @attention
        Only the worker of this turbo::Runtime can issue corun.
        */
        template<typename T>
        void corun(T &&target);

        /**
        @brief keeps running the work-stealing loop until the predicate becomes true

        @tparam P predicate type
        @param predicate a boolean predicate to indicate when to stop the loop

        The method keeps the caller worker running in the work-stealing loop
        until the stop predicate becomes true.

        @attention
        Only the worker of this turbo::Runtime can issue corun.
        */
        template<typename P>
        void corun_until(P &&predicate);

        /**
        @brief corun all asynchronous tasks spawned by this runtime with other workers

        Coruns all asynchronous tasks (turbo::Runtime::async,
        turbo::Runtime::silent_async) with other workers until all those
        asynchronous tasks finish.

        @code{.cpp}
        std::atomic<size_t> counter{0};
        taskflow.emplace([&](turbo::Runtime& rt){
          // spawn 100 async tasks and wait
          for(int i=0; i<100; i++) {
            rt.silent_async([&](){ counter++; });
          }
          rt.corun_all();
          assert(counter == 100);

          // spawn another 100 async tasks and wait
          for(int i=0; i<100; i++) {
            rt.silent_async([&](){ counter++; });
          }
          rt.corun_all();
          assert(counter == 200);
        });
        @endcode

        @attention
        Only the worker of this turbo::Runtime can issue turbo::Runtime::corun_all.
        */
        inline void corun_all();

        /**
        @brief acquire a reference to the underlying worker
        */
        inline Worker &worker();

    protected:

        /**
        @private
        */
        explicit Runtime(Executor &, Worker &, Node *);

        /**
        @private
        */
        Executor &_executor;

        /**
        @private
        */
        Worker &_worker;

        /**
        @private
        */
        Node *_parent;

        /**
        @private
        */
        template<typename F>
        auto async_impl(Worker &w, const std::string &name, F &&f);

        /**
        @private
        */
        template<typename F>
        void silent_async(Worker &w, const std::string &name, F &&f);
    };

    // constructor
    inline Runtime::Runtime(Executor &e, Worker &w, Node *p) :
            _executor{e},
            _worker{w},
            _parent{p} {
    }

    // Function: executor
    inline Executor &Runtime::executor() {
        return _executor;
    }

    // Function: worker
    inline Worker &Runtime::worker() {
        return _worker;
    }

    // ----------------------------------------------------------------------------
    // Node
    // ----------------------------------------------------------------------------

    /**
    @private
    */
    class Node {

        friend class Graph;

        friend class Task;

        friend class AsyncTask;

        friend class TaskView;

        friend class Taskflow;

        friend class Executor;

        friend class FlowBuilder;

        friend class Subflow;

        friend class Runtime;

        enum class AsyncState : int {
            UNFINISHED = 0,
            LOCKED = 1,
            FINISHED = 2
        };

        TURBO_ENABLE_POOLABLE_ON_THIS;

        // state bit flag
        constexpr static int CONDITIONED = 1;
        constexpr static int DETACHED = 2;
        constexpr static int ACQUIRED = 4;
        constexpr static int READY = 8;

        using Placeholder = std::monostate;

        // static work handle
        struct Static {

            template<typename C>
            Static(C &&);

            std::variant<
                    std::function<void()>, std::function<void(Runtime &)>
            > work;
        };

        // dynamic work handle
        struct Dynamic {

            template<typename C>
            Dynamic(C &&);

            std::function<void(Subflow &)> work;
            Graph subgraph;
        };

        // condition work handle
        struct Condition {

            template<typename C>
            Condition(C &&);

            std::variant<
                    std::function<int()>, std::function<int(Runtime &)>
            > work;
        };

        // multi-condition work handle
        struct MultiCondition {

            template<typename C>
            MultiCondition(C &&);

            std::variant<
                    std::function<turbo::SmallVector<int>()>, std::function<turbo::SmallVector<int>(Runtime &)>
            > work;
        };

        // module work handle
        struct Module {

            template<typename T>
            Module(T &);

            Graph &graph;
        };

        // Async work
        struct Async {

            template<typename T>
            Async(T &&);

            std::variant<
                    std::function<void()>, std::function<void(Runtime &)>
            > work;
        };

        // silent dependent async
        struct DependentAsync {

            template<typename C>
            DependentAsync(C &&);

            std::variant<
                    std::function<void()>, std::function<void(Runtime &)>
            > work;

            std::atomic<size_t> use_count{1};
            std::atomic<AsyncState> state{AsyncState::UNFINISHED};
        };

        using handle_t = std::variant<
                Placeholder,      // placeholder
                Static,           // static tasking
                Dynamic,          // dynamic tasking
                Condition,        // conditional tasking
                MultiCondition,   // multi-conditional tasking
                Module,           // composable tasking
                Async,            // async tasking
                DependentAsync    // dependent async tasking
        >;

        struct Semaphores {
            turbo::SmallVector<Semaphore *> to_acquire;
            turbo::SmallVector<Semaphore *> to_release;
        };

    public:

        // variant index
        constexpr static auto PLACEHOLDER = get_index_v<Placeholder, handle_t>;
        constexpr static auto STATIC = get_index_v<Static, handle_t>;
        constexpr static auto DYNAMIC = get_index_v<Dynamic, handle_t>;
        constexpr static auto CONDITION = get_index_v<Condition, handle_t>;
        constexpr static auto MULTI_CONDITION = get_index_v<MultiCondition, handle_t>;
        constexpr static auto MODULE = get_index_v<Module, handle_t>;
        constexpr static auto ASYNC = get_index_v<Async, handle_t>;
        constexpr static auto DEPENDENT_ASYNC = get_index_v<DependentAsync, handle_t>;

        Node() = default;

        template<typename... Args>
        Node(const std::string &, unsigned, Topology *, Node *, size_t, Args &&... args);

        ~Node();

        size_t num_successors() const;

        size_t num_dependents() const;

        size_t num_strong_dependents() const;

        size_t num_weak_dependents() const;

        const std::string &name() const;

    private:

        std::string _name;

        unsigned _priority{0};

        Topology *_topology{nullptr};
        Node *_parent{nullptr};

        void *_data{nullptr};

        turbo::SmallVector<Node *> _successors;
        turbo::SmallVector<Node *> _dependents;

        std::atomic<int> _state{0};
        std::atomic<size_t> _join_counter{0};

        std::unique_ptr<Semaphores> _semaphores;

        handle_t _handle;

        void precede(Node *);

        void set_up_join_counter();

        [[nodiscard]] bool is_cancelled() const;

        [[nodiscard]] bool is_conditioner() const;

        [[nodiscard]]  bool acquire_all(turbo::SmallVector<Node *> &);

        turbo::SmallVector<Node *> release_all();
    };

    // ----------------------------------------------------------------------------
    // Node Object Pool
    // ----------------------------------------------------------------------------

    /**
    @private
    */
    inline AtomicObjectPool<Node> node_pool;

    // ----------------------------------------------------------------------------
    // Definition for Node::Static
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::Static::Static(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::Dynamic
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::Dynamic::Dynamic(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::Condition
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::Condition::Condition(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::MultiCondition
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::MultiCondition::MultiCondition(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::Module
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename T>
    inline Node::Module::Module(T &obj) : graph{obj.graph()} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::Async
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::Async::Async(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node::DependentAsync
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename C>
    Node::DependentAsync::DependentAsync(C &&c) : work{std::forward<C>(c)} {
    }

    // ----------------------------------------------------------------------------
    // Definition for Node
    // ----------------------------------------------------------------------------

    // Constructor
    template<typename... Args>
    Node::Node(
            const std::string &name,
            unsigned priority,
            Topology *topology,
            Node *parent,
            size_t join_counter,
            Args &&... args
    ) :
            _name{name},
            _priority{priority},
            _topology{topology},
            _parent{parent},
            _join_counter{join_counter},
            _handle{std::forward<Args>(args)...} {
    }

    // Destructor
    inline Node::~Node() {
        // this is to avoid stack overflow

        if (_handle.index() == DYNAMIC) {
            // using std::get_if instead of std::get makes this compatible
            // with older macOS versions
            // the result of std::get_if is guaranteed to be non-null
            // due to the index check above
            auto &subgraph = std::get_if<Dynamic>(&_handle)->subgraph;
            std::vector<Node *> nodes;
            nodes.reserve(subgraph.size());

            std::move(
                    subgraph._nodes.begin(), subgraph._nodes.end(), std::back_inserter(nodes)
            );
            subgraph._nodes.clear();

            size_t i = 0;

            while (i < nodes.size()) {

                if (nodes[i]->_handle.index() == DYNAMIC) {
                    auto &sbg = std::get_if<Dynamic>(&(nodes[i]->_handle))->subgraph;
                    std::move(
                            sbg._nodes.begin(), sbg._nodes.end(), std::back_inserter(nodes)
                    );
                    sbg._nodes.clear();
                }

                ++i;
            }

            //auto& np = Graph::_node_pool();
            for (i = 0; i < nodes.size(); ++i) {
                node_pool.recycle(nodes[i]);
            }
        }
    }

    // Procedure: _precede
    inline void Node::precede(Node *v) {
        _successors.push_back(v);
        v->_dependents.push_back(this);
    }

    // Function: num_successors
    inline size_t Node::num_successors() const {
        return _successors.size();
    }

    // Function: dependents
    inline size_t Node::num_dependents() const {
        return _dependents.size();
    }

    // Function: num_weak_dependents
    inline size_t Node::num_weak_dependents() const {
        size_t n = 0;
        for (size_t i = 0; i < _dependents.size(); i++) {
            //if(_dependents[i]->_handle.index() == Node::CONDITION) {
            if (_dependents[i]->is_conditioner()) {
                n++;
            }
        }
        return n;
    }

    // Function: num_strong_dependents
    inline size_t Node::num_strong_dependents() const {
        size_t n = 0;
        for (size_t i = 0; i < _dependents.size(); i++) {
            //if(_dependents[i]->_handle.index() != Node::CONDITION) {
            if (!_dependents[i]->is_conditioner()) {
                n++;
            }
        }
        return n;
    }

    // Function: name
    inline const std::string &Node::name() const {
        return _name;
    }

    // Function: is_conditioner
    inline bool Node::is_conditioner() const {
        return _handle.index() == Node::CONDITION ||
               _handle.index() == Node::MULTI_CONDITION;
    }

    // Function: is_cancelled
    // we currently only support cancellation of taskflow (no async task)
    inline bool Node::is_cancelled() const {
        //return _topology && _topology->is_cancelled.load(std::memory_order_relaxed);
        return _topology &&
               (_topology->_state.load(std::memory_order_relaxed) & Topology::CANCELLED);
    }

    // Procedure: set_up_join_counter
    inline void Node::set_up_join_counter() {
        size_t cnt = 0;
        for (auto *ptr: _dependents) {
            //if(p->_handle.index() == Node::CONDITION) {
            if (ptr->is_conditioner()) {
                _state.fetch_or(Node::CONDITIONED, std::memory_order_relaxed);
            } else {
                cnt++;
            }
        }
        _join_counter.store(cnt, std::memory_order_relaxed);
    }


    // Function: _acquire_all
    inline bool Node::acquire_all(turbo::SmallVector<Node *> &nodes) {

        auto &to_acquire = _semaphores->to_acquire;

        for (size_t i = 0; i < to_acquire.size(); ++i) {
            if (!to_acquire[i]->_try_acquire_or_wait(this)) {
                for (size_t j = 1; j <= i; ++j) {
                    auto r = to_acquire[i - j]->_release();
                    nodes.insert(std::end(nodes), std::begin(r), std::end(r));
                }
                return false;
            }
        }
        return true;
    }

    // Function: release_all
    inline turbo::SmallVector<Node *> Node::release_all() {

        auto &to_release = _semaphores->to_release;

        turbo::SmallVector<Node *> nodes;
        for (const auto &sem: to_release) {
            auto r = sem->_release();
            nodes.insert(std::end(nodes), std::begin(r), std::end(r));
        }

        return nodes;
    }

    // ----------------------------------------------------------------------------
    // Node Deleter
    // ----------------------------------------------------------------------------

    /**
    @private
    */
    struct NodeDeleter {
        void operator()(Node *ptr) {
            node_pool.recycle(ptr);
        }
    };

    // ----------------------------------------------------------------------------
    // Graph definition
    // ----------------------------------------------------------------------------

    // Destructor
    inline Graph::~Graph() {
        clear_impl();
    }

    // Move constructor
    inline Graph::Graph(Graph &&rhs) noexcept :
            _nodes{std::move(rhs._nodes)} {
    }

    // Move assignment
    inline Graph &Graph::operator=(Graph &&rhs) noexcept {
        clear_impl();
        _nodes = std::move(rhs._nodes);
        return *this;
    }

    // Procedure: clear
    inline void Graph::clear() {
        clear_impl();
    }

    // Procedure: clear
    inline void Graph::clear_impl() {
        for (auto node: _nodes) {
            node_pool.recycle(node);
        }
        _nodes.clear();
    }

    // Procedure: clear_detached
    inline void Graph::clear_detached() {

        auto mid = std::partition(_nodes.begin(), _nodes.end(), [](Node *node) {
            return !(node->_state.load(std::memory_order_relaxed) & Node::DETACHED);
        });

        for (auto itr = mid; itr != _nodes.end(); ++itr) {
            node_pool.recycle(*itr);
        }
        _nodes.resize(std::distance(_nodes.begin(), mid));
    }

    // Procedure: merge
    inline void Graph::merge(Graph &&rhs) {
        for (auto n: rhs._nodes) {
            _nodes.push_back(n);
        }
        rhs._nodes.clear();
    }

    // Function: erase
    inline void Graph::erase(Node *node) {
        if (auto I = std::find(_nodes.begin(), _nodes.end(), node); I != _nodes.end()) {
            _nodes.erase(I);
            node_pool.recycle(node);
        }
    }

    // Function: size
    inline size_t Graph::size() const {
        return _nodes.size();
    }

    // Function: empty
    inline bool Graph::empty() const {
        return _nodes.empty();
    }

    /**
    @private
    */
    template<typename ...ArgsT>
    Node *Graph::emplace_back(ArgsT &&... args) {
        _nodes.push_back(node_pool.animate(std::forward<ArgsT>(args)...));
        return _nodes.back();
    }

}  // namespace turbo
#endif  // TURBO_TASKFLOW_CORE_GRAPH_H_