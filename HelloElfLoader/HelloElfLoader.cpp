#include "stdafx.h"
#include "ElfDefine.h"
#include "LibraryFuncs.h"
#include "HelloElfLoader.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <memory>

namespace HelloElfLoader {
	Loader::Loader(const std::string& path) :
		Loader(std::ifstream(path, std::ios::in | std::ios::binary)) {}

	Loader::Loader(std::ifstream&& fileStream) :
		fileStream_(std::move(fileStream)) {
		if (!fileStream_) {
			throw std::runtime_error("open file failed");
		}
	}

	void Loader::execute() {
		std::cout << "====== start loading elf ======" << std::endl;

		// ��鵱ǰ���г����Ƿ�64λ
		if (sizeof(intptr_t) != sizeof(std::int64_t)) {
			throw std::runtime_error("please use x64 compile and run this program");
		}

		// ��ȡELFͷ
		Elf64_External_Ehdr elfHeader = {};
		fileStream_.seekg(0);
		fileStream_.read(reinterpret_cast<char*>(&elfHeader), sizeof(elfHeader));

		// ���ELFͷ��ֻ֧��64λ��byte order��little endian�ĳ���
		if (std::string(reinterpret_cast<char*>(elfHeader.e_ident), 4) != "\x7f\x45\x4c\x46") {
			throw std::runtime_error("magic not match");
		}
		else if (elfHeader.e_ident[EI_CLASS] != ELFCLASS64) {
			throw std::runtime_error("only support ELF64");
		}
		else if (elfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
			throw std::runtime_error("only support little endian");
		}

		// ��ȡprogram table����Ϣ
		std::uint32_t programTableOffset = *reinterpret_cast<std::uint32_t*>(elfHeader.e_phoff);
		std::uint16_t programTableEntrySize = *reinterpret_cast<std::uint16_t*>(elfHeader.e_phentsize);
		std::uint16_t programTableEntryNum = *reinterpret_cast<std::uint16_t*>(elfHeader.e_phnum);
		std::cout << "program table at: " << programTableOffset << ", "
			<< programTableEntryNum << " x " << programTableEntrySize << std::endl;

		// ��ȡsection table����Ϣ
		// section tableֻ��linker�ã�loader����ʵ����Ҫ����section table
		std::uint32_t sectionTableOffset = *reinterpret_cast<std::uint32_t*>(elfHeader.e_shoff);
		std::uint16_t sectionTableEntrySize = *reinterpret_cast<std::uint16_t*>(elfHeader.e_shentsize);
		std::uint16_t sectionTableEntryNum = *reinterpret_cast<std::uint16_t*>(elfHeader.e_shentsize);
		std::cout << "section table at: " << sectionTableOffset << ", "
			<< sectionTableEntryNum << " x " << sectionTableEntrySize << std::endl;

		// ׼����̬���ӵ���Ϣ
		std::uint64_t jmpRelAddr = 0; // �ض�λ��¼�Ŀ�ʼ��ַ
		std::uint64_t pltRelType = 0; // �ض�λ��¼������ RELA��REL
		std::uint64_t pltRelSize = 0; // �ض�λ��¼���ܴ�С
		std::uint64_t symTabAddr = 0; // ��̬���ű�Ŀ�ʼ��ַ
		std::uint64_t strTabAddr = 0; // ��̬�������Ʊ�Ŀ�ʼ��ַ
		std::uint64_t strTabSize = 0; // ��̬�������Ʊ���ܴ�С

		// ����program hedaer
		std::vector<Elf64_External_Phdr> programHeaders;
		programHeaders.resize(programTableEntryNum);
		fileStream_.read(reinterpret_cast<char*>(programHeaders.data()), programTableEntryNum * programTableEntrySize);
		std::vector<std::shared_ptr<void>> loadedSegments;
		for (const auto& programHeader : programHeaders) {
			std::uint32_t type = *reinterpret_cast<const std::uint32_t*>(programHeader.p_type);
			if (type == PT_LOAD) {
				// ���ļ�����(����������������)���ص������ڴ棬���ʾ�������ǵ�ַ��ͻ
				std::uint64_t fileOffset = *reinterpret_cast<const std::uint64_t*>(programHeader.p_offset);
				std::uint64_t fileSize = *reinterpret_cast<const std::uint64_t*>(programHeader.p_filesz);
				std::uint64_t virtAddr = *reinterpret_cast<const std::uint64_t*>(programHeader.p_vaddr);
				std::uint64_t memSize = *reinterpret_cast<const std::uint64_t*>(programHeader.p_memsz);
				if (memSize < fileSize) {
					throw std::runtime_error("invalid memsz in program header, it shouldn't less than filesz");
				}
				// ��ָ���������ַ�����ڴ�
				std::cout << std::hex << "allocate address at: 0x" << virtAddr <<
					" size: 0x" << memSize << std::dec << std::endl;
				void* addr = ::VirtualAlloc((void*)virtAddr, memSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (addr == nullptr) {
					throw std::runtime_error("allocate memory at specific address failed");
				}
				loadedSegments.emplace_back(addr, [](void* ptr) { ::VirtualFree(ptr, 0, MEM_RELEASE); });
				// �����ļ����ݵ������ڴ�
				fileStream_.seekg(fileOffset);
				if (!fileStream_.read(reinterpret_cast<char*>(addr), fileSize)) {
					throw std::runtime_error("read contents into memory from LOAD program header failed");
				}
			}
			else if (type == PT_DYNAMIC) {
				// ������̬��
				std::uint64_t fileOffset = *reinterpret_cast<const std::uint64_t*>(programHeader.p_offset);
				fileStream_.seekg(fileOffset);
				Elf64_External_Dyn dynSection = {};
				std::uint64_t dynSectionTag = 0;
				std::uint64_t dynSectionVal = 0;
				do {
					if (!fileStream_.read(reinterpret_cast<char*>(&dynSection), sizeof(dynSection))) {
						throw std::runtime_error("read dynamic section failed");
					}
					dynSectionTag = *reinterpret_cast<const std::uint64_t*>(dynSection.d_tag);
					dynSectionVal = *reinterpret_cast<const std::uint64_t*>(dynSection.d_un.d_val);
					if (dynSectionTag == DT_JMPREL) {
						jmpRelAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_PLTREL) {
						pltRelType = dynSectionVal;
					}
					else if (dynSectionTag == DT_PLTRELSZ) {
						pltRelSize = dynSectionVal;
					}
					else if (dynSectionTag == DT_SYMTAB) {
						symTabAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_STRTAB) {
						strTabAddr = dynSectionVal;
					}
					else if (dynSectionTag == DT_STRSZ) {
						strTabSize = dynSectionVal;
					}
				} while (dynSectionTag != 0);
			}
		}

		// ��ȡ��̬���ӷ��ű�
		std::string dynamicSymbolNames(reinterpret_cast<char*>(strTabAddr), strTabSize);
		Elf64_External_Sym* dynamicSymbols = reinterpret_cast<Elf64_External_Sym*>(symTabAddr);

		// ���ö�̬���ӵĺ�����ַ
		std::cout << std::hex << "read dynamic entires at: 0x" << jmpRelAddr <<
			" size: 0x" << pltRelSize << std::dec << std::endl;
		if (jmpRelAddr == 0 || pltRelType != DT_RELA || pltRelSize % sizeof(Elf64_External_Rela) != 0) {
			throw std::runtime_error("invalid dynamic entry info, rel type should be rela");
		}
		std::vector<std::shared_ptr<void>> libraryFuncs;
		for (std::uint64_t offset = 0; offset < pltRelSize; offset += sizeof(Elf64_External_Rela)) {
			Elf64_External_Rela* rela = (Elf64_External_Rela*)(jmpRelAddr + offset);
			std::uint64_t relaOffset = *reinterpret_cast<const std::uint64_t*>(rela->r_offset);
			std::uint64_t relaInfo = *reinterpret_cast<const std::uint64_t*>(rela->r_info);
			std::uint64_t relaSym = relaInfo >> 32; // ELF64_R_SYM
			std::uint64_t relaType = relaInfo & 0xffffffff; // ELF64_R_TYPE
			// ��ȡ����
			Elf64_External_Sym* symbol = dynamicSymbols + relaSym;
			std::uint32_t symbolNameOffset = *reinterpret_cast<std::uint32_t*>(symbol->st_name);
			std::string symbolName(dynamicSymbolNames.data() + symbolNameOffset);
			std::cout << "relocate symbol: " << symbolName << std::endl;
			// �滻������ַ
			// ԭ��Ӧ���ӳٽ��������ͼ�򵥾�ֱ�Ӹ�����
			void** relaPtr = reinterpret_cast<void**>(relaOffset);
			std::shared_ptr<void> func = resolveLibraryFunc(symbolName);
			if (func == nullptr) {
				throw std::runtime_error("unsupport symbol name");
			}
			libraryFuncs.emplace_back(func);
			*relaPtr = func.get();
		}

		// ��ȡ��ڵ�
		std::uint64_t entryPointAddress = *reinterpret_cast<const std::uint64_t*>(elfHeader.e_entry);
		void(*entryPointFunc)() = reinterpret_cast<void(*)()>(entryPointAddress);
		std::cout << "entry point: " << entryPointFunc << std::endl;
		std::cout << "====== finish loading elf ======" << std::endl;

		// ִ��������
		// ���ȵ���__libc_start_main, Ȼ���ٵ���main
		// ����__libc_start_main���ָ����hlt�����Ա�����__libc_start_main���˳�ִ��
		entryPointFunc();
	}
}
