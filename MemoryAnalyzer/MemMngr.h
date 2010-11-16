/** @file MemMngr.h
@brief Internal implementation -- do not include this file directly.  Please include MemoryAnalyzer.h instead.
*/

#ifndef MEMMNGR_H
#define MEMMNGR_H


//#ifndef MEMORYANALYZER_H
//#error Do not include this file directly.  Please include MemoryAnalyzer.h instead.
//#endif

#include <fstream>
#include <typeinfo>

/** @enum AllocationType
This is used to differentiate between memory allocated through either new or new[]
*/
enum AllocationType
{
	ALLOC_NEW,			/**< Normal memory allocation */
	ALLOC_NEW_ARRAY		/**< Array memory allocation */
};


class SourcePacket
{
private:

	SourcePacket(const SourcePacket&);
	SourcePacket& operator=(const SourcePacket&);

public:

	SourcePacket(const char *file, int line) : file(file), line(line)
	{}
	~SourcePacket()
	{}

	const char *file;
	int line;
};


template<typename T>
T* operator*(const SourcePacket& packet, T* p);


/** @class MemoryManager
@brief Internal implementation of the MemoryAnalyzer tool.

This class intercepts and handles all allocations and deallocations. It can	display info such as peak memory, 
number of allocations, and address lists for allocations, to name a few pieces of information it stores. It is 
implemented as a singleton, has very few dependencies (all of which are standard), and is portable.
*/
class MemoryManager
{
private:

	/** @struct AllocationHeader
	Information object placed directly before all memory upon allocation.
	*/
	struct AllocationHeader
	{
		//! Size of the the object in memory (not including the header)
		size_t rawSize;
		AllocationType type;
	};

	/** @struct AddrListNode
	Internal information container. Used to keep track of current memory allocations.
	*/
	struct AddrListNode
	{
		//! Allocation address
		void *address;
		//! Object type
		const char *type;
		//! Source file
		const char *file;
		//! Line number
		int line;
		AddrListNode *next;
	};

	/** @struct MemInfoNode
	Internal information container. Used to help organize individual memory information objects (AddrListNode's).
	*/
	struct MemInfoNode
	{
		//! Size of the allocations referenced within
		size_t size;
		//! Number of objects allocated with this object's size
		int numberOfAllocations;
		//! Linked list of current memory allocations with this object's size
		AddrListNode *addresses;
		MemInfoNode *next;
	};

	//! Linked list of normal allocations & information
	MemInfoNode *head_new;
	//! Linked list of array allocations & information
	MemInfoNode *head_new_array;
	//! Pointer to last allocated block of memory.  The detailing function checks this variable first to save time.  If it
	//! the correct node, it will use it; otherwise, it will search for the node using RetrieveAddrNode.
	AddrListNode *mostRecentAllocAddrNode;

	size_t currentMemory;
	size_t peakMemory;
	long long currentBlocks;
	long long peakBlocks;

	const char *unknown;

	std::ofstream dumpFile;

	MemoryManager();
	~MemoryManager();
	MemoryManager(const MemoryManager&);
	MemoryManager& operator=(const MemoryManager&);

	/**	@brief Adds information for a single allocation to the internal list
	@param size Size of the allocation requested by the program
	@param type Allocation type
	@param ptr Pointer to the allocated memory which needs to be added to the internal list
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	*/
	void AddAllocationToList(size_t size, AllocationType type, void *ptr);

	/** @brief Adds context information to the most recent allocation
	@param ptr Pointer to the allocated memory
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	@param type Type of the allocation (i.e., int, Complex, Vector, etc.) determined using RTTI
	@param objectSize Size of the object; used to decrease lookup times if the node matching the ptr address is not the same
	as the one in mostRecentAllocAddrNode
	*/
	void AddAllocationDetails(void *ptr, const char *file, int line, const char *type, size_t objectSize);

	/** @brief Allocates memory upon request from the overloaded new operator
	@param size Requested allocation size
	@param type Allocation type
	@param file Source filename from which the allocation was requested
	@param line Line number of the source file on which the allocation request occurred
	@param throwEx Indicates whether or not an exception should be thrown if memory couldn't be allocated (default: false)
	@return Pointer to allocated memory
	*/
	void* Allocate(size_t size, AllocationType type, bool throwEx = false);

	/** @brief Frees memory upon request from the overloaded delete operator
	@param ptr Pointer to memory which should be freed
	@param type Allocation type
	@param throwEx Indicates whether or not an exception should be thrown if memory couldn't be allocated (default: false)
	*/
	void Deallocate(void *ptr, AllocationType type, bool throwEx = false);

	/**	@brief Returns string version of allocation type enum
	@param type Allocation type to convert to a string
	@return Allocation type in string form
	*/
	const char* GetAllocTypeAsString(AllocationType type);

	/**	@brief Returns the head of the linked list for a desired allocation type
	@param type Allocation type of head of internal list (i.e., ALLOC_NEW for non-array allocation list, etc.)
	@return Pointer to head of allocation info list for desired type
	*/
	MemInfoNode* GetListHead(AllocationType type);

	/**	@brief Removes information for a single allocation from the internal list
	@param ptr Pointer to the freed memory which needs to be deleted from the internal list
	@param type Allocation type of ptr
	*/
	void RemoveAllocationFromList(void *ptr, AllocationType type);

	/** @brief Finds the information node associated with the address.
		@param ptr Address of the desired node
		@param objectSize Size of the object pointed to by ptr.  If the size is not known, this parameter can be ignored.
		@return Pointer to the AddrListNode containing information about the allocation
	*/
	AddrListNode* RetrieveAddrNode(void *ptr, size_t objectSize = -1);

public:

	/** Set to true to display information about every allocation in the console (default: false). Warning: Depending on
	program size/number of allocations, this may create a lot of messages.
	*/
	bool showAllAllocs;
	/** Set to true to display information about every deallocation in the console (default: false). Warning: Depending on
	program size/number of allocations, this may create a lot of messages.
	*/
	bool showAllDeallocs;
	/** Set to true to save all memory leaks and related information in a generated file, memleaks.log (default: false).
	*/
	bool dumpLeaksToFile;

	/** @brief Singleton access
	@return Reference to singleton object
	*/
	static MemoryManager& Get();

		/** @brief Displays current memory allocations in the console according to criteria
	@param displayNumberOfAllocsFirst Set to true to display the list according to the number of allocations
	of a certain size (i.e., 10 allocs of size 2); otherwise, the list will be displayed according to
	the size of the allocations, followed by the number of allocations of that size (i.e., Size: 2, 10 allocs).
	(default: true)
	@param displayDetail Set to true to display extra detail about every allocation (address, file, line).
	Warning: Depending on program size/number of allocations, this may create a lot of messages. (default: false)
	*/
	void DisplayAllocations(bool displayNumberOfAllocsFirst = true, bool displayDetail = false);

	/** @brief Retrieves the number of blocks allocated at present
		@return Number of allocated blocks
	*/
	long long GetCurrentBlocks();

	/** @brief Retrieves the current allocated memory
	@return Currently allocated memory in bytes
	*/
	size_t GetCurrentMemory();

	/** @brief Retrieves the peak number of allocated blocks
		@return Largest number of blocks allocated at any time during program execution
	*/
	long long GetPeakBlocks();

	/** @brief Retrieves the peak allocated memory
	@return Largest amount of memory allocated at any time during program execution in bytes
	*/
	size_t GetPeakMemory();	

#ifdef _WIN32
	/** @brief Calls Windows-specific function to check the state of the heap and display a message in the console 
	indicating said state
	*/
	void HeapCheck();
#endif

	friend void* operator new(size_t size);
	friend void* operator new(size_t size, const std::nothrow_t&);
	friend void operator delete(void *ptr);
	friend void operator delete(void *ptr, const std::nothrow_t&);

	friend void* operator new[](size_t size);
	friend void* operator new[](size_t size, const std::nothrow_t&);
	friend void operator delete[](void *ptr);
	friend void operator delete[](void *ptr, const std::nothrow_t&);
	
	template<typename T>
	friend T* operator*(const SourcePacket& packet, T* p);
};


template<typename T>
T* operator*(const SourcePacket& packet, T* p)
{
	if(p)
	{
		const char *type = typeid(*p).name();
		MemoryManager::Get().AddAllocationDetails(p, packet.file, packet.line, type, sizeof(*p));
		if(MemoryManager::Get().showAllAllocs)
		{
			cout << "\tObject Type: " << type << "\n\tFile: " << packet.file << "\n\tLine: " << packet.line << "\n\n";
		}
	}
	return p;
}

#endif