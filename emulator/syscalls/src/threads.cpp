#include <include/threads.hpp>
#include <cassert>
#include <cstdio>
#include <sched.h>
using namespace riscv;

template <int W>
thread<W>::thread(multithreading<W>& mt, int ttid, thread* p,
				address_t tls, address_t stack)
	: threading(mt), parent(p), tid(ttid), my_tls(tls), my_stack(stack)   {}

template <int W>
void thread<W>::activate()
{
	threading.m_current = this;
	auto& cpu = threading.machine.cpu;
	cpu.reg(RISCV::REG_SP) = this->my_stack;
	cpu.reg(RISCV::REG_TP) = this->my_tls;
}

template <int W>
void thread<W>::suspend()
{
	this->stored_regs = threading.machine.cpu.registers();
	// add to suspended (NB: can throw)
	threading.suspended.push_back(this);
}

template <int W>
void thread<W>::exit()
{
	const bool exiting_myself = (threading.get_thread() == this);
	assert(this->parent != nullptr);
	// temporary copy of thread manager
	auto& thr  = this->threading;
	// CLONE_CHILD_CLEARTID: set userspace TID value to zero
	if (this->clear_tid) {
		THPRINT("Clearing thread value for tid=%d at 0x%X\n",
				this->tid, this->clear_tid);
		threading.machine.memory.template write<uint32_t> (this->clear_tid, 0);
	}
	// delete this thread
	threading.erase_thread(this->tid);
	// free thread resources
	delete this;

	if (exiting_myself)
	{
		// resume next thread in suspended list
		thr.wakeup_next();
	}
}

template <int W>
void thread<W>::resume()
{
	THPRINT("Returning to tid=%ld tls=%p stack=%p\n",
			this->tid, (void*) this->my_tls, (void*) this->my_stack);

	threading.m_current = this;
	auto& m = threading.machine;
	// preserve some registers
	auto counter = m.cpu.registers().counter;
	// restore registers
	m.cpu.registers() = this->stored_regs;
	m.cpu.registers().counter = counter;
}

template <int W>
thread<W>* multithreading<W>::create(
			thread_t* parent, int flags, address_t ctid, address_t ptid,
			address_t stack, address_t tls)
{
	const int tid = ++thread_counter;
	auto* thread = new thread_t(*this, tid, parent, tls, stack);

	// flag for write child TID
	if (flags & CLONE_CHILD_SETTID) {
		machine.memory.template write<uint32_t> (ctid, thread->tid);
	}
	if (flags & CLONE_PARENT_SETTID) {
		machine.memory.template write<uint32_t> (ptid, thread->tid);
	}
	if (flags & CLONE_CHILD_CLEARTID) {
		thread->clear_tid = ctid;
	}

	threads.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(tid),
		std::forward_as_tuple(thread));
	return thread;
}

template <int W>
multithreading<W>::multithreading(Machine<W>& mach)
	: machine(mach), main_thread(*this, 0, nullptr, 0x0, 0x0)
{
	main_thread.my_stack = machine.cpu.reg(RISCV::REG_SP);
	m_current = &main_thread;
}

template <int W>
thread<W>* multithreading<W>::get_thread()
{
	return this->m_current;
}

template <int W>
thread<W>* multithreading<W>::get_thread(int tid)
{
	auto it = threads.find(tid);
	if (it == threads.end()) return nullptr;
	return it->second;
}

template <int W>
bool multithreading<W>::suspend_and_yield()
{
	auto* thread = get_thread();
	// don't go through the ardous yielding process when alone
	if (suspended.empty()) {
		// set the return value for sched_yield
		thread->stored_regs.get(RISCV::REG_ARG0) = 0;
		return false;
	}
	// suspend current thread
	thread->suspend();
	// set the return value for sched_yield
	thread->stored_regs.get(RISCV::REG_ARG0) = 0;
	// resume some other thread
	this->wakeup_next();
	return true;
}

template <int W>
void multithreading<W>::yield_to(int tid)
{
	auto* thread = get_thread();
	auto* next   = get_thread(tid);
	if (next == nullptr) {
		machine.cpu.reg(RISCV::REG_ARG0) = -1;
		return;
	}
	// return value for yield_to
	machine.cpu.reg(RISCV::REG_ARG0) = 0;
	if (thread == next) return;
	// suspend current thread
	thread->suspend();
	// remove the next thread from suspension
	for (auto it = suspended.begin(); it != suspended.end(); ++it) {
		if (*it == next) {
			suspended.erase(it);
			break;
		}
	}
	// resume next thread
	next->resume();
}

template <int W>
void multithreading<W>::wakeup_next()
{
  // resume a waiting thread
  assert(!suspended.empty());
  auto* next = suspended.front();
  suspended.pop_front();
  // resume next thread
  next->resume();
}

template <int W>
void multithreading<W>::erase_thread(int tid)
{
	auto it = threads.find(tid);
	assert(it != threads.end());
	threads.erase(it);
}