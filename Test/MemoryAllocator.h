#pragma once
#include <vector>
#include <map>
#include <memory>
#include <assert.h>

#define MEMORY_ALIGN_UP(Size,AlignmentSize) (((Size) + ((AlignmentSize) - 1)) & ((AlignmentSize) - 1))
#define MEMORY_ALIGN_DOWN(Size,AlignmentSize) (((Size) + (AlignmentSize)) & (AlignmentSize))
#define IS_POWER_OF_2(Size) (0 == ((Size) & ((Size) - 1)))

enum class MemoryFreeErrorEnum : uint32_t
{
	eNoError,
	eSizeMismatch
};

struct MemoryBlock 
{
	MemoryBlock(MemoryAllocator allocator,void* pBlockPointer, size_t BlockSize)
		: allocator(allocator)
		, pBlockPointer(pBlockPointer)
		, BlockSize(BlockSize)
	{
		
	}

	virtual ~MemoryBlock()
	{
		allocator.Deallocate(*this);
	}
	
	void* pBlockPointer;
	size_t BlockSize;

private:
	MemoryAllocator& allocator;
};


class MemoryAllocator 
{
public:
	MemoryAllocator(size_t blockSize)
	{
		pBlockPointer = std::malloc(blockSize);
		this->blockSize = blockSize;
	}
	virtual ~MemoryAllocator() 
	{
		std::free(pBlockPointer);
	}

	virtual std::unique_ptr<MemoryBlock> Allocate(size_t size) = 0;

	virtual MemoryFreeErrorEnum Deallocate(MemoryBlock& memoryBlock) = 0;

protected:
	void* pBlockPointer = nullptr;
	size_t blockSize = 0;
	

};

class LinearMemoryAllocator : public MemoryAllocator
{
public:
	LinearMemoryAllocator(size_t blockSize)
		:MemoryAllocator(blockSize)
	{
		Reset();
	}

	std::unique_ptr<MemoryBlock> Allocate(size_t size) override
	{
		assert(IS_POWER_OF_2(size));
		size_t actualSize = MEMORY_ALIGN_UP(size, alignmentSize);
		if (actualSize > remainSize)
			return nullptr;
		void* pBlockPointer = pOffset;
		pOffset = reinterpret_cast<char *>(pOffset) + actualSize;
		remainSize -= actualSize;
		return std::make_unique<MemoryBlock>(*this, pBlockPointer, actualSize);
	}

	MemoryFreeErrorEnum Deallocate(MemoryBlock& memoryBlock) override
	{
		return MemoryFreeErrorEnum::eNoError;
	}

	void Reset() 
	{
		pOffset = pBlockPointer;
		remainSize = blockSize;
	}


protected:
	std::vector<std::unique_ptr<MemoryBlock>> blocks;
	//std::map<void*, size_t> memoryRecord;
	void* pOffset = nullptr;
	size_t remainSize;
	size_t alignmentSize = 64;

};















