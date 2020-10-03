// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2020, v0.8
// A sipmle allocator that counts the consumed memory

#ifndef COUNTINGALLOCATOR_H
#define COUNTINGALLOCATOR_H

////////////////////////////////////////////////////////////////////
// A special class for counting memory consumption

struct CMemoryCounter {
	void RegisterAllocation(size_t n)
	  {memoryConsumption += n;}
	void UnregisterAllocation(size_t n)
	  { assert( memoryConsumption >= n); memoryConsumption -= n;}

	size_t GetMemoryConsumption()
	  {return memoryConsumption;}
private:
	size_t memoryConsumption;
};

////////////////////////////////////////////////////////////////////
// A special wrap up of the standard allocation procedure for counting the memory consumption
// https://stackoverflow.com/questions/15940978/what-is-the-size-of-each-element-in-stdlist

template <typename T>
struct CountingAllocator {
   typedef T value_type;
   typedef T* pointer;
   typedef T& reference;
   typedef T const* const_pointer;
   typedef T const& const_reference;
   typedef std::size_t size_type;
   typedef std::ptrdiff_t difference_type;

   template <typename U>
   struct rebind {
      typedef CountingAllocator<U> other;
   };

   CountingAllocator( CMemoryCounter& cnt)
	{ counter = cnt; };
   CountingAllocator(CountingAllocator&&) = default;
   CountingAllocator(CountingAllocator const&) = default;
   CountingAllocator& operator=(CountingAllocator&&) = default;
   CountingAllocator& operator=(CountingAllocator const&) = default;

   template <typename U>
   CountingAllocator(CountingAllocator<U> const&) {}

   pointer address(reference x) const { return &x; }
   const_pointer address(const_reference x) const { return &x; }

   pointer allocate(size_type n, void const* = 0) {
      pointer p = reinterpret_cast<pointer>(malloc(n * sizeof(value_type)));
      cnt.RegisterAllocation(n);
      return p;
   }

   void deallocate(pointer p, size_type n) {
      cnt.UnregisterAllocation(n);
      free(p);
   }

   size_type max_size() const throw() { return std::numeric_limits<size_type>::max() / sizeof(value_type); }

   template <typename U, typename... Args>
   void construct(U* p, Args&&... args) { ::new ((void*)p) U (std::forward<Args>(args)...); }

   template <typename U>
   void destroy(U* p) { p->~U(); }

private:
	CMemoryCounter& cnt;
};

#endif // COUNTINGALLOCATOR_H
