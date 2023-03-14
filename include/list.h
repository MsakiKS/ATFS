#ifndef _LIST_H
#define _LIST_H

#include <stdio.h>

struct list_head {
  struct list_head *next, *prev;
};

#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#define LIST_HEAD_INIT(name) \
  { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/**
 *@brief 初始化链表
 *
 * @param list
 */
static inline void INIT_LIST_HEAD(struct list_head *list) {
  list->next = list;
  list->prev = list;
}

/**
 *@brief 根据结构体type中成员member的指针ptr获取容器结构体type的地址
 *
 */
#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

/**
 *@brief 从一个结构的成员指针找到其容器的指针
 *
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 *@brief 用于获得包含链表第一个成员的结构体指针
 *
 */
#define list_first_entry(ptr, type, member) \
  list_entry((ptr)->next, type, member)

/**
 *@brief 用于获得包含链表最后一个成员的结构体指针
 *
 */
#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

/**
 * @brief 用于获得包含链表下一个成员的结构体指针
 *
 */
#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * @brief 用于获得包含链表前一个成员的结构体指针
 *
 */
#define list_prev_entry(pos, member) \
  list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * @brief 遍历链表
 *
 */
#define list_for_each_entry(pos, head, member)             \
  for (pos = list_first_entry(head, typeof(*pos), member); \
       &pos->member != (head); pos = list_next_entry(pos, member))

/**
 * @brief 逆向遍历链表
 *
 */
#define list_for_each_entry_reverse(pos, head, member)    \
  for (pos = list_last_entry(head, typeof(*pos), member); \
       &pos->member != (head); pos = list_prev_entry(pos, member))

/**
 * @brief 安全遍历链表
 *
 */
#define list_for_each_entry_safe(pos, n, head, member)     \
  for (pos = list_first_entry(head, typeof(*pos), member), \
      n = list_next_entry(pos, member);                    \
       &pos->member != (head); pos = n, n = list_next_entry(n, member))

/**
 * @brief 安全逆向遍历链表
 *
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member) \
  for (pos = list_last_entry(head, typeof(*pos), member),      \
      n = list_prev_entry(pos, member);                        \
       &pos->member != (head); pos = n, n = list_prev_entry(n, member))

/**
 * @brief 添加链表
 *
 */
static inline void __list_add(struct list_head *new_node,
                              struct list_head *prev, struct list_head *next) {
  next->prev = new_node;
  new_node->next = next;
  new_node->prev = prev;
  prev->next = new_node;
}
static inline void list_add(struct list_head *new_node,
                            struct list_head *head) {
  __list_add(new_node, head, head->next);
}

/**
 * @brief 尾插/增加链表
 *
 */
static inline void list_add_tail(struct list_head *new_node,
                                 struct list_head *head) {
  __list_add(new_node, head->prev, head);
}

/**
 * @brief 删除链表
 *
 */
static inline void __list_del(struct list_head *prev, struct list_head *next) {
  next->prev = prev;
  prev->next = next;
}
static inline void list_del(struct list_head *entry) {
  __list_del(entry->prev, entry->next);
  entry->next = NULL;
  entry->prev = NULL;
}

/**
 * @brief 删除并重新初始化
 *
 * @param entry
 */
static inline void list_del_init(struct list_head *entry) {
  __list_del(entry->prev, entry->next);
  INIT_LIST_HEAD(entry);
}

/**
 * @brief 元素移动到链表头
 *
 * @param list
 * @param head
 */
static inline void list_move(struct list_head *list, struct list_head *head) {
  __list_del(list->prev, list->next);
  list_add(list, head);
}

/**
 * @brief 元素移动到链表尾
 *
 * @param list
 * @param head
 */
static inline void list_move_tail(struct list_head *list,
                                  struct list_head *head) {
  __list_del(list->prev, list->next);
  list_add_tail(list, head);
}

/**
 * @brief 清空
 *
 * @param head
 * @return int
 */
static inline int list_empty(const struct list_head *head) {
  return head->next == head;
}

/**
 * @brief 链接两个链表
 *
 * @param list
 * @param prev
 * @param next
 */
static inline void __list_splice(const struct list_head *list,
                                 struct list_head *prev,
                                 struct list_head *next) {
  struct list_head *first = list->next;
  struct list_head *last = list->prev;

  first->prev = prev;
  prev->next = first;

  last->next = next;
  next->prev = last;
}
static inline void list_splice(const struct list_head *list,
                               struct list_head *head) {
  if (!list_empty(list)) __list_splice(list, head, head->next);
}

/**
 * @brief 头尾链接
 *
 * @param list
 * @param head
 */
static inline void list_splice_tail(struct list_head *list,
                                    struct list_head *head) {
  if (!list_empty(list)) __list_splice(list, head->prev, head);
}

/**
 * @brief 链接链表并重新初始化
 *
 * @param list
 * @param head
 */
static inline void list_splice_init(struct list_head *list,
                                    struct list_head *head) {
  if (!list_empty(list)) {
    __list_splice(list, head, head->next);
    INIT_LIST_HEAD(list);
  }
}

/**
 * @brief 头尾相链接并初始化
 *
 * @param list
 * @param head
 */
static inline void list_splice_tail_init(struct list_head *list,
                                         struct list_head *head) {
  if (!list_empty(list)) {
    __list_splice(list, head->prev, head);
    INIT_LIST_HEAD(list);
  }
}
#endif
