#pragma once

// Sustituye operator new/delete para contar las reservas de memoria mientras `AllocGuard::counting`
// está activo en el hilo actual. Se incluye en UN solo archivo de pruebas por ejecutable.

#include <atomic>
#include <cstdlib>
#include <new>

namespace allocguard
{
inline std::atomic<long> count { 0 };
inline thread_local bool counting = false;

/** Cuenta las reservas hechas en el hilo actual mientras vive. */
struct Scope
{
    Scope() { before = count.load(); counting = true; }
    ~Scope() { counting = false; }
    long allocations() const { return count.load() - before; }
    long before;
};
} // namespace allocguard

void* operator new (std::size_t n)
{
    if (allocguard::counting)
        ++allocguard::count;
    if (void* p = std::malloc (n ? n : 1))
        return p;
    throw std::bad_alloc();
}

void* operator new[] (std::size_t n) { return operator new (n); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
