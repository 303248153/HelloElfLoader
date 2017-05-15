#include "stdafx.h"
#include "LibraryFuncs.h"
#include <Windows.h>
#include <iostream>
#include <cstdarg>

namespace HelloElfLoader {
	namespace {
		// 原始的返回地址
		thread_local void* originalReturnAddress = nullptr;

		void* getOriginalReturnAddress() {
			return originalReturnAddress;
		}

		void setOriginalReturnAddress(void* address) {
			originalReturnAddress = address;
		}

		// 模拟libc调用main的函数，目前不支持传入argc和argv
		void __libc_start_main(int(*main)()) {
			std::cout << "call main: " << main << std::endl;
			int ret = main();
			std::cout << "result: " << ret << std::endl;
			std::exit(0);
		}

		// 模拟printf函数
		int printf(const char* fmt, ...) {
			int ret;
			va_list myargs;
			va_start(myargs, fmt);
			ret = ::vprintf(fmt, myargs);
			va_end(myargs);
			return ret;
		}

		// 把System V AMD64 ABI转换为Microsoft x64 calling convention
		// 因为vc++不支持inline asm，只能直接写hex
		// 这个函数支持任意长度的参数，但是性能会有损耗，如果参数数量已知可以编写更快的loader代码	
		const char generic_func_loader[]{
			// 让参数连续排列在栈上
			// [第一个参数] [第二个参数] [第三个参数] ...
			0x58, // pop %rax 暂存原返回地址
			0x41, 0x51, // push %r9 入栈第六个参数，之后的参数都在后续的栈上
			0x41, 0x50, // push %r8 入栈第五个参数
			0x51, // push %rcx 入栈第四个参数
			0x52, // push %rdx 入栈第三个参数
			0x56, // push %rsi 入栈第二个参数
			0x57, // push %rdi 入栈第一个参数

			// 调用setOriginalReturnAddress保存原返回地址
			0x48, 0x89, 0xc1, // mov %rax, %rcx 第一个参数是原返回地址
			0x48, 0x83, 0xec, 0x20, // sub $0x20, %rsp 预留32位的影子空间
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
			0xff, 0xd0, // callq *%rax 调用setOriginalReturnAddress
			0x48, 0x83, 0xc4, 0x20, // add %0x20, %rsp 释放影子空间

			// 转换到Microsoft x64 calling convention
			0x59, // pop %rcx 出栈第一个参数
			0x5a, // pop %rdx 出栈第二个参数
			0x41, 0x58, // pop %r8 // 出栈第三个参数
			0x41, 0x59, // pop %r9 // 出栈第四个参数

			// 调用目标函数
			0x48, 0x83, 0xec, 0x20, // sub $0x20, %esp 预留32位的影子空间
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs 0, %rax
			0xff, 0xd0, // callq *%rax 调用模拟的函数
			0x48, 0x83, 0xc4, 0x30, // add $0x30, %rsp 释放影子空间和参数(影子空间32 + 参数8*2)
			0x50, // push %rax 保存返回值

			// 调用getOriginalReturnAddress获取原返回地址
			0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs $0, %rax
			0xff, 0xd0, // callq *%rax 调用getOriginalReturnAddress
			0x48, 0x89, 0xc1, // mov %rax, %rcx 原返回地址存到rcx
			0x58, // 恢复返回值
			0x51, // 原返回地址入栈顶
			0xc3 // 返回
		};
		const int generic_func_loader_set_addr_offset = 18;
		const int generic_func_loader_target_offset = 44;
		const int generic_func_loader_get_addr_offset = 61;
	}

	// 获取动态链接函数的调用地址
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
