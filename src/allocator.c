#include <stdio.h>
#include <stdlib.h>

#include "../include/allocator.h"
#include "../include/super.h"
#include "../include/internal.h"
#include "../include/types.h"
#include "../include/inode.h"

extern int get_cpuid(void);

void init_allocator_list(allocator_list_t *alloc_list)
{
    alloc_list->head = OFFSET_NULL;
    alloc_list->count = 0;
    pthread_mutex_init(&alloc_list->mutex, NULL);
}

void fill_local_page_list(pagelist_t *local_page_list, int page_cnt)
{
    int i;
    list_node_t *head = NULL;
    list_node_t *next = NULL;
    list_node_t *node_head = NULL;
    list_node_t *node = NULL;
    pagelist_t *global_page_list = get_page_list_base_addr();

    ATFS_DEBUG("fill_local_page_list start");
    
    pthread_mutex_lock(&global_page_list->mutex);
    node = get_nvm_page_node_addr(global_page_list->head);
    node_head = node;
    
    for (i = 0; i < page_cnt - 1; ++i) {
        node = get_nvm_page_node_addr(node->next_offset);
    }
    global_page_list->head = node->next_offset;
    node->next_offset = OFFSET_NULL;

    if (local_page_list->head == OFFSET_NULL) {
        local_page_list->head = node_head->offset;
    } else {
        head = get_nvm_page_node_addr(local_page_list->head);
        next = get_nvm_page_node_addr(head->next_offset);
        head->next_offset = node_head->offset;
        node->next_offset = next->offset;
    }

    global_page_list->count += -page_cnt;
    local_page_list->count += page_cnt;
    pthread_mutex_unlock(&global_page_list->mutex);

    ATFS_DEBUG("fill_local_page_list success");
}

list_node_t *alloc_free_pages(u32 page_num)
{
    u32 i;
    int cpuid = get_cpuid() + 1;
    list_node_t *head = NULL;
    list_node_t *node = NULL;
    pagelist_t *local_page_list = get_local_page_list_addr(cpuid);

    ATFS_DEBUG("alloc_free_pages start");

    pthread_mutex_lock(&local_page_list->mutex);
    while (local_page_list->count == 0 || local_page_list->count < page_num) {
        fill_local_page_list(local_page_list, FILL_LOCAL_PAGE_LIST_MAX);
    }

    head = get_nvm_page_node_addr(local_page_list->head);
    node = head;
    for (i = 0; i < page_num - 1; ++i) {
        node = get_nvm_page_node_addr(node->next_offset);
    }
    local_page_list->head = node->next_offset;
    node->next_offset = OFFSET_NULL;
    
    local_page_list->count -= page_num;

    pthread_mutex_unlock(&local_page_list->mutex);

    ATFS_DEBUG("alloc_free_pages success");

    return head;
}

void free_pages(list_node_t *node, u32 page_num)
{
    u32 i;
    int cpuid = get_cpuid() + 1;
    pagelist_t *page_list = get_local_page_list_addr(cpuid);
    list_node_t *head = NULL;
    list_node_t *tail = node;
    u64 next;

    ATFS_DEBUG("free_pages start");

    if (page_list->count > FILL_LOCAL_PAGE_LIST_MAX) {
        page_list = get_page_list_base_addr();
    }

    for (i = 1; i < page_num; ++i) {
        tail = get_nvm_page_node_addr(tail->next_offset);
    }

    pthread_mutex_lock(&page_list->mutex);
    head = get_nvm_page_node_addr(page_list->head);
    next = head->next_offset;
    head->next_offset = nvm_addr2off((char *)node - PAGE_SIZE);
    tail->next_offset = next;

    pthread_mutex_unlock(&page_list->mutex);

    ATFS_DEBUG("free_pages success");
}

/*
void fill_radixtree_node_list(allocator_list_t *alloc_list)
{
    int i, j;
    int obj_size;
    int obj_num;
    list_node_t *node = NULL;
    list_node_t *prev_node = NULL;
    radixtree_node_t *tree_prev = NULL;
    radixtree_node_t *tree_node = NULL;

    ATFS_DEBUG("fill_radixtree_node_list start");
    
    node = alloc_free_pages(RADIXTREE_NODE_PER_ALLOC);

    obj_size = sizeof(radixtree_node_t);
    obj_num = PAGE_SIZE / obj_size;
    
    for (i = 0;i < RADIXTREE_NODE_PER_ALLOC; ++i) {
        for (j = 0; j < obj_num; ++j) {
            tree_node = nvm_off2addr(node->offset + j * obj_size);
            init_radixtree_node(tree_node);
            tree_node->in_page = node->offset;
            if (tree_prev == NULL) {
                tree_node->next = OFFSET_NULL;
            } else {
                tree_node->next = nvm_addr2off(tree_prev);
            }
            tree_prev = tree_node;
        }
        if (node->next_offset != OFFSET_NULL) {
            prev_node = node;
            node = get_nvm_page_node_addr(node->next_offset);
            prev_node->next_offset = OFFSET_NULL;
        }
    }

    alloc_list->head = nvm_addr2off(tree_node);
    alloc_list->count = RADIXTREE_NODE_PER_ALLOC * obj_num;

    ATFS_DEBUG("fill_radixtree_node_list success");
}

u64 alloc_radixtree_node(void)
{
    int cpuid = get_cpuid();
    radixtree_node_t *node = NULL;
    u64 node_offset;
    allocator_list_t *alloc_list = get_radixtree_table_addr(cpuid);

    ATFS_DEBUG("alloc_radixtree_node start");
    
    pthread_mutex_lock(&alloc_list->mutex);
    if (alloc_list->count == 0) {
        fill_radixtree_node_list(alloc_list);
    }

    node_offset = alloc_list->head;
    node = nvm_off2addr(node_offset);
    alloc_list->head = node->next;
    node->next = OFFSET_NULL;
    alloc_list->count -= 1;
    
    pthread_mutex_unlock(&alloc_list->mutex);

    ATFS_DEBUG("alloc_radixtree_node success");

    return node_offset;
}

void free_radixtree_node(radixtree_node_t* node)
{
    int cpuid = get_cpuid();
    allocator_list_t *alloc_list = get_radixtree_table_addr(cpuid);
    radixtree_node_t *head = nvm_off2addr(alloc_list->head);
    u64 next = head->next;

    ATFS_DEBUG("free_radixtree_node start");

    pthread_mutex_lock(&alloc_list->mutex);
    head->next = nvm_addr2off(node);
    node->next = next;
    
    pthread_mutex_unlock(&alloc_list->mutex);
    
    ATFS_DEBUG("free_radixtree_node success");
}
*/

void fill_atfs_inode_list(allocator_list_t *alloc_list)
{
    int i, j;
    int obj_size;
    int obj_num;
    list_node_t *node = NULL;
    list_node_t *prev_node = NULL;
    struct atfs_inode *inode_prev = NULL;
    struct atfs_inode *inode_node = NULL;

    ATFS_DEBUG("fill_atfs_inode_list start");
    
    node = alloc_free_pages(ATFS_INODE_PER_ALLOC);

    obj_size = sizeof(struct atfs_inode);
    obj_num = PAGE_SIZE / obj_size;
    
    for (i = 0;i < ATFS_INODE_PER_ALLOC; ++i) {
        for (j = 0; j < obj_num; ++j) {
            inode_node = nvm_off2addr(node->offset + j * obj_size);
            init_atfs_inode(inode_node);
            inode_node->in_page = node->offset;
            if (inode_prev == NULL) {
                inode_node->next = OFFSET_NULL;
            } else {
                inode_node->next = nvm_addr2off(inode_prev);
            }
            inode_prev = inode_node;
        }
        if (node->next_offset != OFFSET_NULL) {
            prev_node = node;
            node = get_nvm_page_node_addr(node->next_offset);
            prev_node->next_offset = OFFSET_NULL;
        }
    }

    alloc_list->head = nvm_addr2off(inode_node);
    alloc_list->count = ATFS_INODE_PER_ALLOC * obj_num;

    ATFS_DEBUG("fill_atfs_inode_list success");
}

u64 alloc_atfs_inode(void)
{
    int cpuid = get_cpuid();
    struct atfs_inode *node = NULL;
    u64 node_offset;
    allocator_list_t *alloc_list = get_atfs_inode_table_addr(cpuid);

    ATFS_DEBUG("alloc_atfs_inode start");
    
    pthread_mutex_lock(&alloc_list->mutex);
    if (alloc_list->count == 0) {
        fill_atfs_inode_list(alloc_list);
    }

    node_offset = alloc_list->head;
    node = nvm_off2addr(node_offset);
    alloc_list->head = node->next;
    node->next = OFFSET_NULL;
    alloc_list->count -= 1;
    
    pthread_mutex_unlock(&alloc_list->mutex);

    ATFS_DEBUG("alloc_atfs_inode success");

    return node_offset;
}

void free_atfs_inode(struct atfs_inode *node)
{
    int cpuid = get_cpuid();
    allocator_list_t *alloc_list = get_atfs_inode_table_addr(cpuid);
    struct atfs_inode *head = nvm_off2addr(alloc_list->head);
    u64 next = head->next;

    ATFS_DEBUG("free_atfs_inode start");

    pthread_mutex_lock(&alloc_list->mutex);
    head->next = nvm_addr2off(node);
    node->next = next;
    
    pthread_mutex_unlock(&alloc_list->mutex);

    ATFS_DEBUG("free_atfs_inode success");
}
