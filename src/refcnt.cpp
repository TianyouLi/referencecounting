#include <barrier>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>
#include <memory>
#include "CLI11.hpp"
#include "refcnt.hpp"

typedef std::vector<std::thread> Threads;
ReferenceCount<int64_t> rc;

class ThreadCtx
{
public:
  unsigned int index;
  unsigned long nops;
  std::shared_ptr<std::chrono::nanoseconds::rep[]> time_elapsed_per_thread;
  std::shared_ptr<std::barrier<>> start_barrier;
};

void thread_func(std::shared_ptr<ThreadCtx> ctx)
{
  char thdname[32];
  snprintf(thdname, sizeof(thdname), "thread_%d", ctx->index);
  pthread_setname_np(pthread_self(), thdname);
  ctx->start_barrier->arrive_and_wait();

  const auto begin = std::chrono::high_resolution_clock::now();

  for (unsigned long i = 0; i < ctx->nops; i++)
  {
    rc.get();
    rc.put();
  }

  const auto end = std::chrono::high_resolution_clock::now();

  std::chrono::nanoseconds duration = end - begin;

  ctx->time_elapsed_per_thread[ctx->index] = duration.count();
}

int main(const int argc, const char *argv[])
{
  CLI::App app{"Simple reference counting performance test"};

  unsigned long nops = 100000;
  app.add_option("-n,--number-of-ops", nops, "The number of operations during the test.");

  unsigned int nthreads = 80;
  app.add_option("-t,--thread-count", nthreads, "The number of threads during the test.");

  CLI11_PARSE(app, argc, argv);

  // array for holding the time duration
  std::shared_ptr<std::chrono::nanoseconds::rep[]> time_elapsed_per_thread(new std::chrono::nanoseconds::rep[nthreads]);
  // barrier for trigger start
  std::shared_ptr<std::barrier<>> start_barrier(new std::barrier(nthreads));

  Threads threads;
  for (unsigned int i = 0; i < nthreads; i++)
  {
    std::shared_ptr<ThreadCtx> ctx(new ThreadCtx);
    ctx->index = i;
    ctx->nops = nops;
    ctx->time_elapsed_per_thread = time_elapsed_per_thread;
    ctx->start_barrier = start_barrier;
    threads.push_back(std::thread(::thread_func, ctx));
  }

  for (auto &t : threads)
  {
    t.join();
  }

  std::chrono::nanoseconds::rep total_nanos = 0;
  for (unsigned int i = 0; i < nthreads; i++)
  {
    total_nanos += time_elapsed_per_thread[i];
  }

  unsigned long long total_counts = nops * nthreads;

  std::cout << "Total executed ops : " << total_counts << std::endl;
  std::cout << "Total executed time: " << total_nanos << std::endl;
  std::cout << "Nanoseconds per ops: " << total_nanos / total_counts << std::endl;

  return 0;
}
