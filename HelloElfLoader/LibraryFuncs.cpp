#include "stdafx.h"
#include "LibraryFuncs.h"
#include <Windows.h>
#include <iostream>
#include <cstdarg>

namespace HelloElfLoader {
	namespace {
		// ԭʼ�ķ��ص�ַ
		thread_local void* originalReturnAddress = nullptr;

		void* getOriginalReturnAddress() {
			return originalReturnAddress;
		}

		void setOriginalReturnAddress(void* address) {
			originalReturnAddress = address;
		}

		// ģ��libc����main�ĺ�����Ŀǰ��֧�ִ���argc��argv
		void __libc_start_main(int(*main)()) {
			std::cout << "call main: " << main << std::endl;
			int ret = main();
			std::cout << "result: " << ret << std::endl;
			std::exit(0);
		}

		// ģ��printf����
		int printf(const char* fmt, ...) {
			int ret;
			va_list myargs;
			va_start(myargs, fmt);
			ret = ::vprintf(fmt, myargs);
			va_end(myargs);
			return ret;
		}

		// ��System V AMD64 ABIת��ΪMicrosoft x64 calling convention
		// ��Ϊvc++��֧��inline asm��ֻ��ֱ��дhex
		// �������֧�����ⳤ�ȵĲ������������ܻ�����ģ��������������֪���Ա�д�����loader����	
		const char generic_func_loader[]{
			// �ò�������������ջ��
			// [��һ������] [�ڶ�������] [����������] ...
			0x58, // pop %rax �ݴ�ԭ���ص�ַ
			0x41, 0x51, // push %r9 ��ջ������������֮��Ĳ������ں�����ջ��
			0x41, 0x50, // push %r8 ��ջ���������
			0x51, // push %rcx ��ջ���ĸ�����
			0x52, // push %rdx ��ջ����������
			0x56, // push %rsi ��ջ�ڶ�������
			0x57, // push %rdi ��ջ��һ������

			// ����setOriginalReturnAddress����ԭ���ص�ַ
			0x48, 0x89, 0xc1, // mov %rax, %rcx ��һ��������ԭ���ص�ַ
			0x48, 0x83, 0xec, 0x20, // sub $0x20, %rsp Ԥ��32λ��Ӱ�ӿռ�
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
			0xff, 0xd0, // callq *%rax ����setOriginalReturnAddress
			0x48, 0x83, 0xc4, 0x20, // add %0x20, %rsp �ͷ�Ӱ�ӿռ�

			// ת����Microsoft x64 calling convention
			0x59, // pop %rcx ��ջ��һ������
			0x5a, // pop %rdx ��ջ�ڶ�������
			0x41, 0x58, // pop %r8 // ��ջ����������
			0x41, 0x59, // pop %r9 // ��ջ���ĸ�����

			// ����Ŀ�꺯��
			0x48, 0x83, 0xec, 0x20, // sub $0x20, %esp Ԥ��32λ��Ӱ�ӿռ�
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs 0, %rax
			0xff, 0xd0, // callq *%rax ����ģ��ĺ���
			0x48, 0x83, 0xc4, 0x30, // add $0x30, %rsp �ͷ�Ӱ�ӿռ�Ͳ���(Ӱ�ӿռ�32 + ����8*2)
			0x50, // push %rax ���淵��ֵ

			// ����getOriginalReturnAddress��ȡԭ���ص�ַ
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
			0xff, 0xd0, // callq *%rax ����getOriginalReturnAddress
			0x48, 0x89, 0xc1, // mov %rax, %rcx ԭ���ص�ַ�浽rcx
			0x58, // �ָ�����ֵ
			0x51, // ԭ���ص�ַ��ջ��
			0xc3 // ����
		};
		const int generic_func_loader_set_addr_offset = 18;
		const int generic_func_loader_target_offset = 44;
		const int generic_func_loader_get_addr_offset = 61;
	}

	// ��ȡ��̬���Ӻ����ĵ��õ�ַ
	std::shared_ptr<void> resolveLibraryFunc(const std::string& name) {
		void* funcPtr = nullptr;
		if (name == "__libc_start_main") {
			funcPtr = __libc_start_main;
		}
		else if (name == "printf") {
			funcPtr = printf;
		}
		else {
			return nullptr;
		}
		void* addr = ::VirtualAlloc(nullptr,
			sizeof(generic_func_loader), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (addr == nullptr) {
			throw std::runtime_error("allocate memory for _libc_start_main_loader failed");
		}
		std::shared_ptr<void> result(addr, [](void* ptr) { ::VirtualFree(ptr, 0, MEM_RELEASE); });
		std::memcpy(addr, generic_func_loader, sizeof(generic_func_loader));
		char* addr_c = reinterpret_cast<char*>(addr);
		*reinterpret_cast<void**>(addr_c + generic_func_loader_set_addr_offset) = setOriginalReturnAddress;
		*reinterpret_cast<void**>(addr_c + generic_func_loader_target_offset) = funcPtr;
		*reinterpret_cast<void**>(addr_c + generic_func_loader_get_addr_offset) = getOriginalReturnAddress;
		return result;
	}
}
