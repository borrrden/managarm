
#include <frigg/types.hpp>
#include <frigg/traits.hpp>
#include <frigg/debug.hpp>
#include <frigg/initializer.hpp>
#include <frigg/support.hpp>
#include <frigg/algorithm.hpp>
#include <frigg/atomic.hpp>
#include <frigg/memory.hpp>
#include <frigg/libc.hpp>
#include <frigg/elf.hpp>

#include <frigg/optional.hpp>
#include <frigg/tuple.hpp>
#include <frigg/string.hpp>
#include <frigg/vector.hpp>
#include <frigg/hashmap.hpp>
#include <frigg/linked.hpp>

#include <hel.h>
#include <hel-syscalls.h>
#include <helx.hpp>

#include <frigg/glue-hel.hpp>
#include <frigg/protobuf.hpp>

#include "ld-server.frigg_pb.hpp"

#include "linker.hpp"

uintptr_t libraryBase = 0x41000000;

bool verbose = false;
bool eagerBinding = true;

// --------------------------------------------------------
// SharedObject
// --------------------------------------------------------

SharedObject::SharedObject(const char *name, bool is_main_object)
		: name(name), isMainObject(is_main_object), baseAddress(0), loadScope(nullptr),
		dynamic(nullptr), globalOffsetTable(nullptr), entry(nullptr),
		hashTableOffset(0), symbolTableOffset(0), stringTableOffset(0),
		lazyRelocTableOffset(0), lazyTableSize(0),
		lazyExplicitAddend(false),
		dependencies(*allocator), onInitStack(false), wasInitialized(false) { }

void processCopyRela(SharedObject *object, Elf64_Rela *reloc) {
	Elf64_Xword type = ELF64_R_TYPE(reloc->r_info);
	Elf64_Xword symbol_index = ELF64_R_SYM(reloc->r_info);
	assert(type == R_X86_64_COPY);
	
	uintptr_t rel_addr = object->baseAddress + reloc->r_offset;
	
	auto symbol = (Elf64_Sym *)(object->baseAddress + object->symbolTableOffset
			+ symbol_index * sizeof(Elf64_Sym));
	assert(symbol->st_name != 0);

	const char *symbol_str = (const char *)(object->baseAddress
			+ object->stringTableOffset + symbol->st_name);
	uintptr_t copy_addr = (uintptr_t)object->loadScope->resolveSymbol(symbol_str,
			object, Scope::kResolveCopy);
	assert(copy_addr != 0);

	memcpy((void *)rel_addr, (void *)copy_addr, symbol->st_size);
}

void processCopyRelocations(SharedObject *object) {
	bool has_rela_offset = false, has_rela_length = false;
	uintptr_t rela_offset;
	size_t rela_length;

	for(size_t i = 0; object->dynamic[i].d_tag != DT_NULL; i++) {
		Elf64_Dyn *dynamic = &object->dynamic[i];
		
		switch(dynamic->d_tag) {
		case DT_RELA:
			rela_offset = dynamic->d_ptr;
			has_rela_offset = true;
			break;
		case DT_RELASZ:
			rela_length = dynamic->d_val;
			has_rela_length = true;
			break;
		case DT_RELAENT:
			assert(dynamic->d_val == sizeof(Elf64_Rela));
			break;
		}
	}

	if(has_rela_offset && has_rela_length) {
		for(size_t offset = 0; offset < rela_length; offset += sizeof(Elf64_Rela)) {
			auto reloc = (Elf64_Rela *)(object->baseAddress + rela_offset + offset);
			processCopyRela(object, reloc);
		}
	}else{
		assert(!has_rela_offset && !has_rela_length);
	}
}

void doInitialize(SharedObject *object) {
	assert(!object->wasInitialized);
	assert(!object->onInitStack);

	// make sure we detect cyclic dependencies
	object->onInitStack = true;

	// if the object has dependencies we initialize them first
	for(size_t i = 0; i < object->dependencies.size(); i++) {
		if(!object->dependencies[i]->wasInitialized)
			doInitialize(object->dependencies[i]);
		assert(object->dependencies[i]->wasInitialized);
	}

	if(verbose)
		infoLogger->log() << "Initialize " << object->name << frigg::EndLog();
	
	// now initialize the actual object
	typedef void (*InitFuncPtr) ();

	InitFuncPtr init_ptr = nullptr;
	InitFuncPtr *init_array = nullptr;
	size_t array_size = 0;

	for(size_t i = 0; object->dynamic[i].d_tag != DT_NULL; i++) {
		Elf64_Dyn *dynamic = &object->dynamic[i];
		
		switch(dynamic->d_tag) {
		case DT_INIT:
			if(dynamic->d_ptr != 0)
				init_ptr = (InitFuncPtr)(object->baseAddress + dynamic->d_ptr);
			break;
		case DT_INIT_ARRAY:
			if(dynamic->d_ptr != 0)
				init_array = (InitFuncPtr *)(object->baseAddress + dynamic->d_ptr);
			break;
		case DT_INIT_ARRAYSZ:
			array_size = dynamic->d_val;
			break;
		}
	}

	if(init_ptr != nullptr)
		init_ptr();
	
	assert((array_size % sizeof(InitFuncPtr)) == 0);
	for(size_t i = 0; i < array_size / sizeof(InitFuncPtr); i++)
		init_array[i]();
	
	object->wasInitialized = true;
}

// --------------------------------------------------------
// Scope
// --------------------------------------------------------

Scope::Scope() : objects(*allocator) { }

bool strEquals(const char *str1, const char *str2) {
	while(*str1 != 0 && *str2 != 0) {
		if(*str1++ != *str2++)
			return false;
	}
	if(*str1 != 0 || *str2 != 0)
		return false;
	return true;
}

uint32_t elf64Hash(const char *name) {
	uint32_t h = 0, g;

	while(*name) {
		h = (h << 4) + (uint32_t)(*name++);
		g = h & 0xF0000000;
		if(g)
			h ^= g >> 24;
		h &= 0x0FFFFFFF;
	}

	return h;
}

bool symbolMatches(SharedObject *object, Elf64_Sym *symbol, const char *resolve_str) {
	uint8_t bind = ELF64_ST_BIND(symbol->st_info);
	assert(bind == STB_GLOBAL || bind == STB_WEAK);
	if(symbol->st_shndx == SHN_UNDEF)
		return false;
	assert(symbol->st_name != 0);

	const char *symbol_str = (const char *)(object->baseAddress
			+ object->stringTableOffset + symbol->st_name);
	return strEquals(symbol_str, resolve_str);
}

// TODO: move this to some namespace or class?
void *resolveInObject(SharedObject *object, const char *resolve_str) {
	auto hash_table = (Elf64_Word *)(object->baseAddress + object->hashTableOffset);
	
	Elf64_Word num_buckets = hash_table[0];
	auto bucket = elf64Hash(resolve_str) % num_buckets;

	auto index = hash_table[2 + bucket];
	while(index != 0) {
		auto *symbol = (Elf64_Sym *)(object->baseAddress
				+ object->symbolTableOffset + index * sizeof(Elf64_Sym));
		
		if(symbolMatches(object, symbol, resolve_str))
			return (void *)(object->baseAddress + symbol->st_value);

		index = hash_table[2 + num_buckets + index];
	}

	return nullptr;
}

// TODO: let this return uintptr_t
void *Scope::resolveSymbol(const char *resolve_str,
		SharedObject *from_object, uint32_t flags) {
	for(size_t i = 0; i < objects.size(); i++) {
		if((flags & kResolveCopy) != 0 && objects[i] == from_object)
			continue;

		void *resolved = resolveInObject(objects[i], resolve_str);
		if(resolved != nullptr)
			return resolved;
	}

	return nullptr;
}


// --------------------------------------------------------
// Loader
// --------------------------------------------------------

struct Tcb {
	Tcb *selfPointer;
};

Loader::Loader(Scope *scope)
: p_scope(scope), p_processQueue(*allocator), p_initQueue(*allocator),
		p_allObjects(frigg::DefaultHasher<frigg::String<Allocator>>(), *allocator) { }

void Loader::loadFromPhdr(SharedObject *object, void *phdr_pointer,
		size_t phdr_entry_size, size_t phdr_count, void *entry_pointer) {
	assert(object->isMainObject);
	if(verbose)
		infoLogger->log() << "Loading " << object->name << frigg::EndLog();
	
	object->entry = entry_pointer;

	// segments are already mapped, so we just have to find the dynamic section
	for(size_t i = 0; i < phdr_count; i++) {
		auto phdr = (Elf64_Phdr *)((uintptr_t)phdr_pointer + i * phdr_entry_size);
		switch(phdr->p_type) {
		case PT_DYNAMIC:
			object->dynamic = (Elf64_Dyn *)phdr->p_vaddr;
			break;
		case PT_TLS: {
			size_t tls_size = phdr->p_memsz + sizeof(Tcb);
			void *tls_buffer = allocator->allocate(tls_size);
			memset(tls_buffer, 0, tls_size);
			memcpy(tls_buffer, (void *)(object->baseAddress + phdr->p_vaddr), phdr->p_filesz);

			auto tcb_ptr = (Tcb *)((uintptr_t)tls_buffer + phdr->p_memsz);
			tcb_ptr->selfPointer = tcb_ptr;
			HEL_CHECK(helWriteFsBase(tcb_ptr));
		} break;
		default:
			//FIXME warn about unknown phdrs
			break;
		}
	}
	
	p_allObjects.insert(frigg::String<Allocator>(*allocator, object->name), object);
	parseDynamic(object);
	p_processQueue.addBack(object);
	p_initQueue.addBack(object);
	p_scope->objects.push(object);
}

void Loader::loadFromFile(SharedObject *object, const char *file) {
	assert(!object->isMainObject);
	if(verbose)
		infoLogger->log() << "Loading " << object->name << frigg::EndLog();

	managarm::ld_server::ClientRequest<Allocator> request(*allocator);
	request.set_identifier(frigg::String<Allocator>(*allocator, file));
	request.set_base_address(object->baseAddress);

	frigg::String<Allocator> serialized(*allocator);
	request.SerializeToString(&serialized);
	serverPipe->sendStringReq(serialized.data(), serialized.size(), 1, 0);
	
	uint8_t buffer[128];
	int64_t async_id;
	HEL_CHECK(helSubmitRecvString(serverPipe->getHandle(), eventHub->getHandle(),
			buffer, 128, 1, 0, kHelNoFunction, kHelNoObject,
			kHelResponse, &async_id));
	HelError response_error;
	size_t length;
	eventHub->waitForRecvString(async_id, response_error, length);
	HEL_CHECK(response_error);

	managarm::ld_server::ServerResponse<Allocator> response(*allocator);
	response.ParseFromArray(buffer, length);

	object->entry = (void *)response.entry();
	object->dynamic = (Elf64_Dyn *)response.dynamic();

	for(size_t i = 0; i < response.segments_size(); i++) {
		auto &segment = response.segments(i);

		uint32_t map_flags = 0;
		if(segment.access() == managarm::ld_server::Access::READ_WRITE) {
			map_flags |= kHelMapReadWrite;
		}else{
			assert(segment.access() == managarm::ld_server::Access::READ_EXECUTE);
			map_flags |= kHelMapReadExecute;
		}
		
		int64_t async_id;
		HEL_CHECK(helSubmitRecvDescriptor(serverPipe->getHandle(), eventHub->getHandle(),
				1, 1 + i, kHelNoFunction, kHelNoObject, kHelResponse, &async_id));
		HelError memory_error;
		HelHandle memory_handle;
		eventHub->waitForRecvDescriptor(async_id, memory_error, memory_handle);
		HEL_CHECK(memory_error);

		void *actual_pointer;
		HEL_CHECK(helMapMemory(memory_handle, kHelNullHandle, (void *)segment.virt_address(),
				segment.virt_length(), map_flags, &actual_pointer));
		HEL_CHECK(helCloseDescriptor(memory_handle));
	}

	p_allObjects.insert(frigg::String<Allocator>(*allocator, object->name), object);
	parseDynamic(object);
	p_processQueue.addBack(object);
	p_initQueue.addBack(object);
	p_scope->objects.push(object);
}

void Loader::process() {
	while(!p_processQueue.empty()) {
		SharedObject *object = p_processQueue.front();
		object->loadScope = p_scope;

		processDependencies(object);
		processStaticRelocations(object);
		processLazyRelocations(object);

		p_processQueue.removeFront();
	}
}

void Loader::initialize() {
	while(!p_initQueue.empty()) {
		SharedObject *object = p_initQueue.front();
		if(!object->wasInitialized)
			doInitialize(object);

		p_initQueue.removeFront();
	}
}

void Loader::parseDynamic(SharedObject *object) {
	assert(object->dynamic != nullptr);

	for(size_t i = 0; object->dynamic[i].d_tag != DT_NULL; i++) {
		Elf64_Dyn *dynamic = &object->dynamic[i];
		switch(dynamic->d_tag) {
		// handle hash table, symbol table and string table
		case DT_HASH:
			object->hashTableOffset = dynamic->d_ptr;
			break;
		case DT_STRTAB:
			object->stringTableOffset = dynamic->d_ptr;
			break;
		case DT_STRSZ:
			break; // we don't need the size of the string table
		case DT_SYMTAB:
			object->symbolTableOffset = dynamic->d_ptr;
			break;
		case DT_SYMENT:
			assert(dynamic->d_val == sizeof(Elf64_Sym));
			break;
		// handle lazy relocation table
		case DT_PLTGOT:
			object->globalOffsetTable = (void **)(object->baseAddress
					+ dynamic->d_ptr);
			break;
		case DT_JMPREL:
			object->lazyRelocTableOffset = dynamic->d_ptr;
			break;
		case DT_PLTRELSZ:
			object->lazyTableSize = dynamic->d_val;
			break;
		case DT_PLTREL:
			if(dynamic->d_val == DT_RELA) {
				object->lazyExplicitAddend = true;
			}else{
				assert(dynamic->d_val == DT_REL);
			}
			break;
		// ignore unimportant tags
		case DT_SONAME: case DT_NEEDED: case DT_RPATH: // we handle this later
		case DT_INIT: case DT_FINI:
		case DT_INIT_ARRAY: case DT_INIT_ARRAYSZ:
		case DT_FINI_ARRAY: case DT_FINI_ARRAYSZ:
		case DT_DEBUG:
		case DT_RELA: case DT_RELASZ: case DT_RELAENT: case DT_RELACOUNT:
		case DT_VERSYM:
		case DT_VERDEF: case DT_VERDEFNUM:
		case DT_VERNEED: case DT_VERNEEDNUM:
			break;
		default:
			frigg::panicLogger.log() << "Unexpected dynamic entry "
					<< (void *)dynamic->d_tag << " in object" << frigg::EndLog();
		}
	}
}

void Loader::processDependencies(SharedObject *object) {
	// load required dynamic libraries
	for(size_t i = 0; object->dynamic[i].d_tag != DT_NULL; i++) {
		Elf64_Dyn *dynamic = &object->dynamic[i];
		if(dynamic->d_tag != DT_NEEDED)
			continue;

		const char *library_str = (const char *)(object->baseAddress
				+ object->stringTableOffset + dynamic->d_val);

		// TODO: cache libraries
		SharedObject **existing
				= p_allObjects.get(frigg::String<Allocator>(*allocator, library_str));
		if(existing) {
			object->dependencies.push(*existing);
		}else{
			auto library = frigg::construct<SharedObject>(*allocator, library_str, false);
			library->baseAddress = libraryBase;
			// TODO: handle this dynamically
			libraryBase += 0x1000000; // assume 16 MiB per library
			loadFromFile(library, library_str);

			object->dependencies.push(library);
		}
	}
}

void Loader::processRela(SharedObject *object, Elf64_Rela *reloc) {
	Elf64_Xword type = ELF64_R_TYPE(reloc->r_info);
	Elf64_Xword symbol_index = ELF64_R_SYM(reloc->r_info);
	
	if(type == R_X86_64_COPY)
		return; // TODO: make sure this only occurs in executables

	uintptr_t rel_addr = object->baseAddress + reloc->r_offset;
	
	// resolve the symbol if there is a symbol
	uintptr_t symbol_addr = 0;
	if(symbol_index != 0) {
		auto symbol = (Elf64_Sym *)(object->baseAddress + object->symbolTableOffset
				+ symbol_index * sizeof(Elf64_Sym));
		assert(symbol->st_name != 0);

		const char *symbol_str = (const char *)(object->baseAddress
				+ object->stringTableOffset + symbol->st_name);
		//FIXME infoLogger->log() << "Looking up " << symbol_str << frigg::EndLog();
		symbol_addr = (uintptr_t)object->loadScope->resolveSymbol(symbol_str, object, 0);
		if(symbol_addr == 0) {
			if(ELF64_ST_BIND(symbol->st_info) == STB_WEAK) {
				if(verbose)
					frigg::infoLogger.log() << "Unresolved weak load-time symbol "
							<< (const char *)symbol_str
							<< " in object " << object->name << frigg::EndLog();
			}else{
				frigg::panicLogger.log() << "Unresolved load-time symbol "
						<< (const char *)symbol_str
						<< " in object " << object->name << frigg::EndLog();
			}
		}
	}

	switch(type) {
	case R_X86_64_64:
		*((uint64_t *)rel_addr) = symbol_addr + reloc->r_addend;
		//FIXME infoLogger->log() << "R_X86_64_64 at " << (void *)rel_addr
		//		<< " resolved to " << (void *)(symbol_addr + reloc->r_addend)
		//		<< frigg::EndLog();
		break;
	case R_X86_64_GLOB_DAT:
		*((uint64_t *)rel_addr) = symbol_addr;
		//FIXME infoLogger->log() << "R_X86_64_GLOB_DAT at " << (void *)rel_addr
		//		<< " resolved to " << (void *)(symbol_addr)
		//		<< frigg::EndLog();
		break;
	case R_X86_64_RELATIVE:
		*((uint64_t *)rel_addr) = object->baseAddress + reloc->r_addend;
		//FIXME infoLogger->log() << "R_X86_64_RELATIVE at " << (void *)rel_addr
		//		<< " resolved to " << (void *)(object->baseAddress + reloc->r_addend)
		//		<< frigg::EndLog();
		break;
	case R_X86_64_DTPMOD64:
	case R_X86_64_DTPOFF64:
		// TODO: implement TLS
		break;
	default:
		frigg::panicLogger.log() << "Unexpected relocation type "
				<< (void *)type << frigg::EndLog();
	}
}

void Loader::processStaticRelocations(SharedObject *object) {
	bool has_rela_offset = false, has_rela_length = false;
	uintptr_t rela_offset;
	size_t rela_length;

	for(size_t i = 0; object->dynamic[i].d_tag != DT_NULL; i++) {
		Elf64_Dyn *dynamic = &object->dynamic[i];
		
		switch(dynamic->d_tag) {
		case DT_RELA:
			rela_offset = dynamic->d_ptr;
			has_rela_offset = true;
			break;
		case DT_RELASZ:
			rela_length = dynamic->d_val;
			has_rela_length = true;
			break;
		case DT_RELAENT:
			assert(dynamic->d_val == sizeof(Elf64_Rela));
			break;
		}
	}

	if(has_rela_offset && has_rela_length) {
		for(size_t offset = 0; offset < rela_length; offset += sizeof(Elf64_Rela)) {
			auto reloc = (Elf64_Rela *)(object->baseAddress + rela_offset + offset);
			processRela(object, reloc);
		}
	}else{
		assert(!has_rela_offset && !has_rela_length);
	}
}

void Loader::processLazyRelocations(SharedObject *object) {
	if(object->globalOffsetTable == nullptr) {
		assert(object->lazyRelocTableOffset == 0);
		return;
	}

	object->globalOffsetTable[1] = object;
	object->globalOffsetTable[2] = (void *)&pltRelocateStub;
	
	// adjust the addresses of JUMP_SLOT relocations
	assert(object->lazyExplicitAddend);
	for(size_t offset = 0; offset < object->lazyTableSize; offset += sizeof(Elf64_Rela)) {
		auto reloc = (Elf64_Rela *)(object->baseAddress + object->lazyRelocTableOffset + offset);
		Elf64_Xword type = ELF64_R_TYPE(reloc->r_info);
		Elf64_Xword symbol_index = ELF64_R_SYM(reloc->r_info);
		uintptr_t rel_addr = object->baseAddress + reloc->r_offset;

		assert(type == R_X86_64_JUMP_SLOT);
		if(eagerBinding) {
			auto symbol = (Elf64_Sym *)(object->baseAddress + object->symbolTableOffset
					+ symbol_index * sizeof(Elf64_Sym));
			assert(symbol->st_name != 0);

			const char *symbol_str = (const char *)(object->baseAddress
					+ object->stringTableOffset + symbol->st_name);

			void *pointer = object->loadScope->resolveSymbol(symbol_str, object, 0);
			if(pointer == nullptr) {
				if(ELF64_ST_BIND(symbol->st_info) == STB_WEAK) {
					if(verbose)
						frigg::infoLogger.log() << "Unresolved weak JUMP_SLOT symbol "
								<< symbol_str << " in object " << object->name << frigg::EndLog();
				}else{
					frigg::panicLogger.log() << "Unresolved JUMP_SLOT symbol "
							<< symbol_str << " in object " << object->name << frigg::EndLog();
				}
			}
			*((void **)rel_addr) = pointer;
		}else{
			*((uint64_t *)rel_addr) += object->baseAddress;
		}
	}
}

