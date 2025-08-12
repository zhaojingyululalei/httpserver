# ThreadCache内存池

# 要解决的问题

- 设计内存池的两个问题
  - 效率问题
  - 碎片问题
- 解决方法
  - 效率问题
    - 减少锁竞争，每个线程有一个ThreadCache，互不影响
    - 设计合理的缓存策略，避免频繁向系统申请内存
  - 设计合理的分配策略，避免碎片

# 整体框架

- 分为三层(TC CC PC)
  - ObjectPool（定长内存池，new的底层是malloc，我要取代malloc，因此需要避免使用new，因此先设计一个内存池，为TC,CC,PC等分配结构体用）
  - ThreadCache层（TC）
  - CentralCache层（CC）
  - PageCacehe层（PC）
- 每个线程向各自的TC申请所需要的内存块 (无需锁)
- TC不够，向CC申请(需要锁)
  - CC维护Span(来自PC)，Span包含若干页，并且做好切分，切分成小块，方便提供给TC
- CC不够，向PC申请(需要锁)
  - PC也维护Span，方便提供给CC使用
  - PC向OS申请以页为单位的内存，并将之维护成Span的形式

# 定长内存池(ObjectPool)

## 支持

- **高效内存管理**：通过内存池减少系统调用次数
- **线程安全**：使用互斥锁保护并发访问
- **对象复用**：维护一个空闲对象链表，优先复用已释放的对象
- **动态扩容**：当内存不足时自动扩展内存空间
- **支持数组分配**：提供 `_new_array` 方法支持批量对象分配

## 工作原理

### 内存布局

对象池使用 `mmap` 系统调用分配大块虚拟内存。每个对象的前8字节用于存储下一个空闲对象的地址，形成一个单向链表。

### 对象分配流程

1. **检查空闲列表**：首先检查 [m_freelist]是否有可复用的对象
2. 复用或分配：
   - 如果有空闲对象，则直接复用
   - 如果没有空闲对象，则从内存池中分配新内存
3. **内存不足处理**：如果内存池剩余空间不足，使用 `mmap` 扩展内存
4. **对象构造**：使用定位 `new` 操作符在预分配内存中构造对象

### 对象释放流程

1. **析构对象**：调用对象的析构函数
2. **加入空闲列表**：将释放的对象添加到 [m_freelist] 中供后续复用

# ThreadCache

​	[ThreadCache] 是一个线程本地缓存层，用于高性能内存分配。它是内存分配器架构中的第二层，位于线程和 [CentralCache] 之间，通过减少锁竞争和系统调用，提供高效的内存分配服务。

## 作用

- **线程本地性**：每个线程拥有独立的 [ThreadCache] 实例，避免多线程竞争
- **快速分配**：大部分内存分配请求可以直接从本地缓存满足
- **智能调节**：根据使用情况动态调节与 [CentralCache] 的交互频率
- **降低锁开销**：减少对全局锁的依赖，提高并发性能

## 核心组件

### FreeList 数组

[ThreadCache] 包含一个大小为 [TC_FREELIST_SIZE]的 [FreeList] 数组，每个元素对应一个特定大小类的空闲内存块链表。

~~~c++
FreeList m_freelists[TC_FREELIST_SIZE];
~~~

### hash桶策略

| 区间        | 对齐大小 | 下标范围 | 内存块大小范围 | 计算式               |
| ----------- | -------- | -------- | -------------- | -------------------- |
| `[0,16)`    | 8B       | 0~15     | 8B~128B        | `(i+1)*8`            |
| `[16,72)`   | 16B      | 16~71    | 144B~1024B     | `128 + (i-15)*16`    |
| `[72,128)`  | 128B     | 72~127   | 1152B~8KB      | `1024 + (i-71)*128`  |
| `[128,184)` | 1024B    | 128~183  | 9KB~64KB       | `8KB + (i-127)*1024` |
| `[184,208)` | 8KB      | 184~207  | 72KB~256KB     | `64KB + (i-183)*8KB` |

此设计内存碎片率能保持在10％

#### 示例1: 请求7字节

- 请求大小: 7B
- 对齐到: 8B (区间[0,16)内)
- 实际分配: 8B
- 浪费空间: 8B - 7B = 1B
- 碎片率: (1/7) × 100% ≈ 14.3%

#### 示例2: 请求65字节

- 请求大小: 65B
- 对齐到: 72B (因为72B是8的倍数且≥65B，对应索引8)
- 实际分配: 72B
- 浪费空间: 72B - 65B = 7B
- 碎片率: (7/65) × 100% ≈ 10.8%

其他以此类推

## 主要方法

### malloc(size_t size)

分配指定大小的内存块。

**处理流程**：

1. 计算请求大小对应的桶索引和对齐大小
2. 检查对应 [FreeList] 是否有空闲块
3. 如果有空闲块，直接返回
4. 如果没有空闲块，调用 [requestFromCentralCache] 向 [CentralCache]申请

### free(void *obj, size_t size)

释放内存块到本地缓存。

**处理流程**：

1. 根据大小计算桶索引
2. 将对象插入对应 [FreeList](javascript:void(0)) 的空闲链表中

### requestFromCentralCache(size_t align_size, size_t index)

当本地缓存不足时，向 [CentralCache](javascript:void(0)) 请求更多内存块。

**处理流程**：

1. 使用调节算法计算合适的请求数量
2. 从 [CentralCache](javascript:void(0)) 获取一批内存块
3. 返回一个块给调用者，其余块加入本地缓存

### releaseToCentralCache(int index, size_t align_size)

将多余的内存块归还给 [CentralCache](javascript:void(0))。

**处理流程**：

1. 使用令牌桶算法计算合适的归还数量
2. 从本地缓存取出指定数量的块
3. 将这些块归还给 [CentralCache](javascript:void(0))

## 调节机制

### 批量申请算法

通过 [req_num_from_cc](javascript:void(0)) 方法计算合适的申请数量，平衡申请频率和内存使用效率。

### 令牌桶算法

使用令牌桶机制控制内存归还策略：

```c++
size_t release_num = m_freelists[index].get_token_bucket().calculateRelease(
    token_count, max_tokens, free_blocks, used_blocks, min_retain);
```

根据以下因素决定归还数量：

- 当前令牌数量
- 最大令牌数量
- 空闲块数量
- 已使用块数量
- 最小保留数量

## 线程本地存储

使用 `thread_local` 关键字确保每个线程拥有独立的 `ThreadCache` 实例：

```c++

thread_local ThreadCache *t_thread_cache = nullptr;
```

## 使用限制

[ThreadCache](javascript:void(0)) 只处理小于等于 [TC_ALLOC_BYTES_MAX_SIZE](javascript:void(0)) 的内存请求。超过此大小的请求会直接转发到 [PageCache](javascript:void(0))。

## 性能优化

1. **无锁设计**：线程本地缓存避免了锁竞争
2. **批量操作**：与 [CentralCache](javascript:void(0)) 的交互采用批量方式，减少交互频率
3. **智能调节**：根据使用模式动态调整申请和归还策略
4. **缓存友好**：局部性好的内存分配模式

## 典型工作流程

1. 线程首次需要 [ThreadCache](javascript:void(0)) 时创建线程本地实例
2. 线程调用 [malloc](javascript:void(0)) 分配内存
3. 首先检查本地缓存是否有合适大小的空闲块
4. 如果有，直接返回；如果没有，向 [CentralCache](javascript:void(0)) 批量申请
5. 线程调用 [free](javascript:void(0)) 释放内存
6. 内存块返回到本地缓存
7. 当本地缓存过大时，根据策略向 [CentralCache](javascript:void(0)) 归还部分内存



# CentralCache

​	[CentralCache](javascript:void(0)) 是内存分配器架构中的核心组件之一，作为 [ThreadCache](javascript:void(0)) 和 [PageCache](javascript:void(0)) 之间的中间层。它负责管理和分配大块内存，协调多个线程之间的内存共享，并通过批量操作减少锁竞争。

## 核心组件

### SpanList 数组

[CentralCache](javascript:void(0)) 维护一个大小为 [TC_SPANLIST_SIZE](javascript:void(0)) 的 [SpanList](javascript:void(0)) 数组，每个元素对应一个特定大小类的 [Span](javascript:void(0)) 链表：

- 每个span会切分成对应块的大小，组织成链表到span中的freelist
- span的对应方式和TC中的一样

## 主要功能

### offerRangeObj 方法

从 [CentralCache](javascript:void(0)) 中批量申请内存块。

**处理流程**：

1. 根据对齐大小获取对应的桶索引
2. 锁定对应的 [SpanList](javascript:void(0))
3. 调用 [getSpan](javascript:void(0)) 获取包含空闲内存块的 [Span](javascript:void(0))
4. 从 [Span](javascript:void(0)) 的空闲链表中取出指定数量的内存块
5. 更新 [Span](javascript:void(0)) 的使用计数
6. 解锁并返回实际分配的内存块

### getSpan 方法

获取包含空闲内存块的 [Span](javascript:void(0)) 对象。

**处理流程**：

1. 在对应大小类的 [SpanList](javascript:void(0)) 中查找包含空闲块的 [Span](javascript:void(0))
2. 如果找不到合适的Span：
   - 计算所需页数
   - 向 [PageCache](javascript:void(0)) 申请新的 [Span](javascript:void(0))
   - 将 [Span](javascript:void(0)) 按照指定大小切分成内存块链表
   - 将新 [Span](javascript:void(0)) 加入对应的 [SpanList](javascript:void(0))

### collectRangeObj 方法

回收 [ThreadCache](javascript:void(0)) 释放的内存块。

**处理流程**：

1. 根据大小计算桶索引并锁定对应 [SpanList](javascript:void(0))
2. 遍历回收的内存块链表：
   - 通过 [mapObjToSpan](javascript:void(0)) 找到内存块对应的 [Span](javascript:void(0))
   - 将内存块加入 [Span](javascript:void(0)) 的空闲链表
   - 减少 [Span](javascript:void(0)) 的使用计数
   - 如果 [Span](javascript:void(0)) 的所有块都被回收，将其归还给 [PageCache](javascript:void(0))
3. 解锁 [SpanList](javascript:void(0))

### 使用计数跟踪

每个 [Span](javascript:void(0)) 维护一个 [out_count](javascript:void(0)) 计数器，跟踪已分配给 [ThreadCache](javascript:void(0)) 的内存块数量。当计数归零时，表示该 [Span](javascript:void(0)) 可以被回收。

### 线程安全

[CentralCache](javascript:void(0)) 使用细粒度锁机制，每个 [SpanList](javascript:void(0)) 都有自己的互斥锁，减少锁竞争：

### 单例模式

[CentralCache](javascript:void(0)) 采用单例模式设计，确保全局只有一个实例：

## 与其它组件的交互

### 与 ThreadCache 的交互

1. [ThreadCache](javascript:void(0)) 通过 [offerRangeObj](javascript:void(0)) 批量申请内存块
2. [ThreadCache](javascript:void(0)) 通过 [collectRangeObj](javascript:void(0)) 批量归还内存块

### 与 PageCache 的交互

1. 通过 `PageCache::offerSpan` 申请新的 [Span](javascript:void(0))
2. 通过 `PageCache::collectSpan` 归还完全空闲的 [Span](javascript:void(0))
3. 通过 `PageCache::mapObjToSpan` 查找内存块对应的 [Span](javascript:void(0))

## 性能优化策略

1. **批量操作**：减少与 [ThreadCache](javascript:void(0)) 的交互次数
2. **细粒度锁**：每个大小类使用独立的锁，减少锁竞争
3. **内存池化**：通过 [Span](javascript:void(0)) 管理大块内存，避免频繁系统调用
4. **智能回收**：只有当 [Span](javascript:void(0)) 完全空闲时才归还给 [PageCache](javascript:void(0))

## 典型工作流程

1. 多个 [ThreadCache](javascript:void(0)) 实例向 [CentralCache](javascript:void(0)) 请求内存块
2. [CentralCache](javascript:void(0)) 从对应大小类的 [SpanList](javascript:void(0)) 中获取包含空闲块的 [Span](javascript:void(0))
3. 如果没有合适的 [Span](javascript:void(0))，向 [PageCache](javascript:void(0)) 申请新的内存页并切分成 [Span](javascript:void(0))
4. 从 [Span](javascript:void(0)) 中取出指定数量的内存块返回给 [ThreadCache](javascript:void(0))
5. [ThreadCache](javascript:void(0)) 归还内存块时，[CentralCache](javascript:void(0)) 将其加入对应 [Span](javascript:void(0)) 的空闲链表
6. 当 [Span](javascript:void(0)) 完全空闲时，将其归还给 [PageCache](javascript:void(0))

# PageCache

## 概述

[PageCache](javascript:void(0)) 是内存分配器架构中的底层组件，负责管理以页为单位的大块内存。它作为 [CentralCache](javascript:void(0)) 和操作系统之间的桥梁，通过页级内存管理和合并策略，最大化内存利用率并减少系统调用次数。

## 设计目标

- **页级内存管理**：以页为单位管理大块内存
- **内存合并**：通过相邻页合并减少内存碎片
- **高效查找**：通过哈希映射快速定位对象所属的内存页
- **减少系统调用**：批量申请和释放内存页，减少 `mmap`/`munmap` 调用

## 架构位置

```
应用程序
    ↓
ThreadCache (线程本地，无锁)
    ↓
CentralCache (全局，有锁)
    ↓
PageCache (全局，有锁)
    ↓
系统内存 (mmap/sbrk)
```

## 核心数据结构

### SpanList 数组

[PageCache](javascript:void(0)) 维护一个大小为 [PC_SPANLIST_SIZE](javascript:void(0)) (129) 的 [SpanList](javascript:void(0)) 数组，每个桶对应特定页数的 [Span](javascript:void(0))：

```c++

SpanList m_spanlists[PC_SPANLIST_SIZE];
```

### 页号到 Span 的映射

使用哈希表维护页号到 [Span](javascript:void(0)) 的映射关系，用于快速查找对象所属的 [Span](javascript:void(0))：

```
cpp

HashMap<pageid_t, Span*> m_spanmap;
```

## 主要功能

### offerSpan 方法

为 [CentralCache](javascript:void(0)) 提供指定页数的 [Span](javascript:void(0))。

**处理流程**：

1. 如果请求页数超过 128 页，直接向操作系统申请内存
2. 在对应的桶中查找是否有合适的 [Span](javascript:void(0))
3. 如果没有，在更大的桶中查找并进行分割
4. 如果所有桶都为空，向操作系统批量申请 128 页内存并重新尝试分配

### mapObjToSpan 方法

根据对象指针查找其所属的 [Span](javascript:void(0))。

**处理流程**：

1. 通过右移操作计算对象所在页号
2. 在哈希表中查找页号对应的 [Span](javascript:void(0))
3. 返回找到的 [Span](javascript:void(0))

### collectSpan 方法

回收 [CentralCache](javascript:void(0)) 归还的 [Span](javascript:void(0))，并尝试与相邻页合并。

**处理流程**：

1. 如果 [Span](javascript:void(0)) 超过 128 页，直接归还给操作系统
2. 向左合并相邻的空闲页
3. 向右合并相邻的空闲页
4. 如果合并后仍不超过 128 页，将其加入对应的桶中
5. 如果合并后超过 128 页，归还给操作系统

## 内存管理策略

### 页数管理范围

[PageCache](javascript:void(0)) 管理 1 到 128 页大小的内存块：

- 1 页 = 4KB (默认页大小)
- 128 页 = 512KB

超过 128 页的内存请求直接由操作系统处理。

### 内存合并机制

通过向左和向右合并相邻的空闲页来减少碎片：

```
cpp// 向左合并示例
span->page_id = leftSpan->page_id;
span->n_pages += leftSpan->n_pages;

// 向右合并示例
span->n_pages += rightSpan->n_pages;
```

合并遵循以下规则：

1. 相邻页必须是空闲状态（`inUse == false`）
2. 合并后的总页数不能超过 128 页
3. 合并过程中更新哈希映射关系

### 批量内存申请

当需要向操作系统申请内存时，一次性申请 128 页（512KB）：

```
cppsize_t req_size = (PC_SPANLIST_SIZE - 1) << PAGE_SHIFT; // 128*pagesize
void *memory = (uint8_t *)mmap(nullptr, req_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
```

## 线程安全

[PageCache](javascript:void(0)) 使用全局互斥锁保护所有操作：

```
cpp

Mutex::MutexLockGuardType lock(m_mutex);
```

## 单例模式

[PageCache](javascript:void(0)) 采用单例模式设计，确保全局只有一个实例：

```
cppstatic PageCache* getInstance()
{
    return Singleton<PageCache>::getInstance();
}
```

## 与其它组件的交互

### 与 CentralCache 的交互

1. [CentralCache](javascript:void(0)) 通过 [offerSpan](javascript:void(0)) 申请页级内存
2. [CentralCache](javascript:void(0)) 通过 [mapObjToSpan](javascript:void(0)) 查找对象所属的 [Span](javascript:void(0))
3. [CentralCache](javascript:void(0)) 通过 [collectSpan](javascript:void(0)) 归还完全空闲的 [Span](javascript:void(0))

### 与 ObjectPool 的交互

使用 [ObjectPool](javascript:void(0)) 管理 [Span](javascript:void(0)) 对象的分配和回收：

```
cppSpan *span = span_pool._new();
span_pool._delete(span);
```

## 性能优化策略

1. **批量申请**：减少系统调用频率
2. **内存合并**：减少内存碎片，提高内存利用率
3. **哈希查找**：快速定位对象所属的内存页
4. **分级管理**：不同大小的内存块分类管理
5. **延迟释放**：尽可能合并内存而不是立即归还操作系统

## 典型工作流程

1. [CentralCache](javascript:void(0)) 请求特定页数的内存 [Span](javascript:void(0))
2. [PageCache](javascript:void(0)) 在对应桶中查找或向操作系统申请内存
3. 将申请到的内存组织成 [Span](javascript:void(0)) 并返回给 [CentralCache](javascript:void(0))
4. [CentralCache](javascript:void(0)) 将内存块分配给 [ThreadCache](javascript:void(0))
5. 当 [ThreadCache](javascript:void(0)) 归还内存时，最终 [CentralCache](javascript:void(0)) 将完全空闲的 [Span](javascript:void(0)) 归还给 [PageCache](javascript:void(0))
6. [PageCache](javascript:void(0)) 尝试合并相邻的空闲页，必要时归还给操作系统

这种设计使得内存分配系统能够在保证高性能的同时，最大化内存利用率并减少系统调用开销。

