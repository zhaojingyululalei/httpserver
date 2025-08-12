#ifndef __ZHAO_MEMDEF_HPP__
#define __ZHAO_MEMDEF_HPP__
#include "utils/utils.hpp"
namespace zhao
{
#define GET_PAGE_SIZE() Utils::Memory::getPageSize()       // 获取操作系统的页大小
#define PAGE_SHIFT Utils::Math::log2_pow2(GET_PAGE_SIZE()) // 操作系统页大小的对数
#define EXPAND_SIZE (128 * 1024)                           // 剩余空间不够，就开辟128KB空间
#define TC_FREELIST_SIZE 208                               // 多少个桶
#define TC_SPANLIST_SIZE TC_FREELIST_SIZE                  // spanlist和freelist桶数量一样
#define TC_ALLOC_BYTES_MAX_SIZE (256 * 1024)               // 最多一次申请多少空间
#define TC_REQUEST_FROM_CC_MAX_SIZE 512                    // TC向CC申请空闲块，一次性最多块数
#define PC_SPANLIST_SIZE 129                               // PC spanlist桶数量,下标代表Span管理的页数(0~128)
} // namespace zhao

#endif