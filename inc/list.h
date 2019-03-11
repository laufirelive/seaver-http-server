/* 侵入式链表 */
#ifndef __LIST__
#define __LIST__    1


// 链表结构
typedef struct __list_head {
    struct __list_head *next, *prev;
} list_head;

// 链表创建
#define LIST_HEAD_CREATE(name) \
    list_head name = {&(name), &(name)}

// 链表初始化
#define LIST_HEAD_INIT(name) \
    (name)->next = (name)->prev = (name);

// 链表判空
static inline int list_is_empty(const list_head *L) {
    return L->next == L;
}

// 头插
static inline void list_add(list_head *L, list_head *newNode) {
    newNode->next = L->next;
    L->next->prev = newNode;
    newNode->prev = L;
    L->next = newNode;
}

// 尾插
static inline void list_add_tail(list_head *L, list_head *newNode) {
    newNode->prev = L->prev;
    L->prev->next = newNode;
    newNode->next = L;
    L->prev = newNode;
}

// 删除
static inline void list_del(list_head *delNode) {
    delNode->next->prev = delNode->prev;
    delNode->prev->next = delNode->next;
}


/* (unsigned long)(&((type *)0))->member)
将 0 转换为 type 类型的指针，间接访问其对应 member 后取地址，
即是从该 type 类型的首地址到 type 类型的 member 成员的偏移量。
ptr - 偏移量 结果即是该 结构体变量的地址 */

// 访问成员
// ptr 链表节点地址
// type 链表所在的结构体类型
// member 链表作为结构体中的成员变量名
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *) 0)->member)))

// 遍历成员
#define list_for_each(index, head) \
    for (index = (head)->next; index != (head); index = index->next)

#endif