#include "stdafx.h"
#include "MemoryAllocator.h"

const int MSLOT_SIZE = 8;

int locateFreeMemorySlot(int requiredSlots, struct MemoryBlock *mem);
void setMemorySlotStates(int size, int pad, int requiredSlots, struct MemoryBlock *mem, int initMemSlot);

void setMemorySlotStates(int size, int pad, int requiredSlots, struct MemoryBlock *mem, int initMemSlot) {
	for (int i = initMemSlot; i < initMemSlot + requiredSlots; i++) {
		if (i == initMemSlot) {
			mem->memorySlots[i].state = MSLOT_POINTERBASE;
			mem->memorySlots[i].size = (size + pad) / MSLOT_SIZE;
		}
		else {
			mem->memorySlots[i].state = MSLOT_USED;
		}
	}
}

int locateFreeMemorySlot(int requiredSlots, struct MemoryBlock *mem) {
	if (requiredSlots > 511)
		// out of memory
		return -1;

	for (int i = 0; i < 512; i++) {
		if (i == 511)
			// out of memory
			break;
		else {
			if (mem->memorySlots[i].state == MSLOT_POINTERBASE) {
				i += mem->memorySlots[i].size - 1;
				continue;
			}
			else if (mem->memorySlots[i].state == MSLOT_FREE) {
				char flag = 1;
				int skipCount = 0;
				for (int j = 0; j < requiredSlots; j++, skipCount++) {
					if (mem->memorySlots[i + j].state != MSLOT_FREE) {
						flag = 0;
						break;
					}
				}
				if (flag == 1) {
					return i;
				}
				else {
					// small optimization:
					// if we know there's not enough slots between
					// i and i+requiredSlots, we also know that there
					// isn't enough slots between i and (i+requiredSlots) - n
					// this can potentially save us some precious iterations
					i += skipCount;
					continue;
				}
			}
			else {
				continue;
			}
		}
	}
	return -1;
}

int allocateMemory(int size, struct MemoryBlock *mem) {
	int pad = 0;

	// pad until size is a multiple of MSLOT_SIZE
	if (size % MSLOT_SIZE != 0)
		while ((size + ++pad) % MSLOT_SIZE != 0);

	int requiredSlots = (size + pad) / MSLOT_SIZE;
	int initMemorySlot = locateFreeMemorySlot(requiredSlots, mem);
	if (initMemorySlot == -1) // couldn't find any free memory slots
		return -1;

	setMemorySlotStates(size, pad, requiredSlots, mem, initMemorySlot);
#ifdef _DEBUG
	assert(mem->memorySlots[initMemorySlot].state == MSLOT_POINTERBASE);
	assert(mem->memorySlots[initMemorySlot].size == requiredSlots);
#endif
	return initMemorySlot * MSLOT_SIZE;
}

int deallocateMemory(int ptr, struct MemoryBlock *mem) {
	if (ptr % MSLOT_SIZE != 0)
		// invalid pointer
		return -1;	
	
	int slotIdx = ptr / MSLOT_SIZE;
	struct MemorySlot memSlot = mem->memorySlots[slotIdx];
	if (memSlot.state != MSLOT_POINTERBASE)
		// invalid pointer
		return -1;

	for (int i = 0; i < memSlot.size; i++) 
		mem->memorySlots[slotIdx + i].state = MSLOT_FREE;

	return 0;
}

int getPaddedPointerDataSize(int ptr, struct MemoryBlock *mem) {
	if (ptr % MSLOT_SIZE != 0)
		// invalid pointer
		return -1;

	int slotIdx = ptr / MSLOT_SIZE;
	struct MemorySlot memSlot = mem->memorySlots[slotIdx];
	if (memSlot.state != MSLOT_POINTERBASE)
		// invalid pointer
		return -1;

	return memSlot.size * MSLOT_SIZE;
}