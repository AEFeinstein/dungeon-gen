/*
 *   linked_list.c
 *
 *   Created on: Sept 12, 2019
 *       Author: Jonathan Moriarty
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "inttypes.h"

#include "linked_list.h"

#ifdef RUN_LINKED_LIST_TESTS
    #define VALIDATE_LIST(func, line, nl, list, target) validateList(func, line, nl, list, target)
#else
    #define VALIDATE_LIST(func, line, nl, list, target)
#endif

void validateList(const char* func, int line, bool nl, list_t* list, const node_t* target);

#ifdef RUN_LINKED_LIST_TESTS
/**
 * @brief Debug print and validate the list
 *
 * @param func The calling function
 * @param line The calling line
 * @param nl true to print a newline before the list, false to not
 * @param list The list to validate
 * @param target The target for the operation, may be NULL
 */
void validateList(const char* func, int line, bool nl, list_t* list, const node_t* target)
{
    if (nl)
    {
        printf("\n");
    }
    printf("%s::%d, len: %d (%p)", func, line, list->length, target);
    node_t* currentNode = list->first;
    node_t* prev        = NULL;
    int countedLen      = 0;

    if (NULL != list->first && NULL != list->first->prev)
    {
        fprintf(stderr, "Node before first not NULL");
        exit(1);
    }

    if (NULL != list->last && NULL != list->last->next)
    {
        fprintf(stderr, "Node after last not NULL");
        exit(1);
    }

    while (currentNode != NULL)
    {
        printf("%p -> %p -> %p", currentNode->prev, currentNode, currentNode->next);
        if (prev != currentNode->prev)
        {
            fprintf(stderr, "Linkage error %p != %p", currentNode->prev, prev);
            exit(1);
        }
        prev        = currentNode;
        currentNode = currentNode->next;
        countedLen++;
    }

    if (countedLen != list->length)
    {
        fprintf(stderr, "List mismatch, list says %d, counted %d", list->length, countedLen);
        exit(1);
    }
}
#endif

// Add to the end of the list.
void push(list_t* list, void* val)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    node_t* newLast = calloc(1, sizeof(node_t));
    newLast->val    = val;
    newLast->next   = NULL;
    newLast->prev   = list->last;

    if (list->length == 0)
    {
        list->first = newLast;
        list->last  = newLast;
    }
    else
    {
        list->last->next = newLast;
        list->last       = newLast;
    }
    list->length++;
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
}

// Remove from the end of the list.
void* pop(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    void* retval = NULL;

    // Get a direct pointer to the node we're removing.
    node_t* target = list->last;

    // If the list any nodes at all.
    if (target != NULL)
    {
        // Adjust the node before the removed node, then adjust the list's last pointer.
        if (target->prev != NULL)
        {
            target->prev->next = NULL;
            list->last         = target->prev;
        }
        // If the list has only one node, clear the first and last pointers.
        else
        {
            list->first = NULL;
            list->last  = NULL;
        }

        // Get the last node val, then free it and update length.
        retval = target->val;
        free(target);
        list->length--;
    }

    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
    return retval;
}

/**
 * @brief TODO
 *
 * @param list
 * @return void*
 */
void* peekFirst(list_t* list)
{
    node_t* target = list->last;
    if (NULL != target)
    {
        return target->val;
    }
    return NULL;
}

/**
 * @brief TODO
 *
 * @param list
 * @return void*
 */
void* peekLast(list_t* list)
{
    node_t* target = list->last;
    if (NULL != target)
    {
        return target->val;
    }
    return NULL;
}

// Add to the front of the list.
void unshift(list_t* list, void* val)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    node_t* newFirst = calloc(1, sizeof(node_t));
    newFirst->val    = val;
    newFirst->next   = list->first;
    newFirst->prev   = NULL;

    if (list->length == 0)
    {
        list->first = newFirst;
        list->last  = newFirst;
    }
    else
    {
        list->first->prev = newFirst;
        list->first       = newFirst;
    }
    list->length++;
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
}

// Remove from the front of the list.
void* shift(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    void* retval = NULL;

    // Get a direct pointer to the node we're removing.
    node_t* target = list->first;

    // If the list any nodes at all.
    if (target != NULL)
    {
        // Adjust the node after the removed node, then adjust the list's first pointer.
        if (target->next != NULL)
        {
            target->next->prev = NULL;
            list->first        = target->next;
        }
        // If the list has only one node, clear the first and last pointers.
        else
        {
            list->first = NULL;
            list->last  = NULL;
        }

        // Get the first node val, then free it and update length.
        retval = target->val;
        free(target);
        list->length--;
    }

    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
    return retval;
}

// TODO: bool return to check if index is valid?
//  Add at an index in the list.
void add(list_t* list, void* val, int index)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, val);
    // If the index we're trying to add to the start of the list.
    if (index == 0)
    {
        unshift(list, val);
    }
    // Else if the index we're trying to add to is before the end of the list.
    else if (index < list->length - 1)
    {
        node_t* newNode = calloc(1, sizeof(node_t));
        newNode->val    = val;
        newNode->next   = NULL;
        newNode->prev   = NULL;

        node_t* current = list->first;
        while (index--)
        {
            current = current->next;
        }

        // We need to adjust the newNode, and the nodes before and after it.
        // current is set to the node before it.

        current->next->prev = newNode;
        newNode->next       = current->next;

        current->next = newNode;
        newNode->prev = current;

        list->length++;
    }
    // Else just add the node to the end of the list.
    else
    {
        push(list, val);
    }
    VALIDATE_LIST(__func__, __LINE__, false, list, val);
}

// Remove at an index in the list.
void* removeIdx(list_t* list, int index)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, (void*)((intptr_t)index));
    // If the list is null or empty, dont touch it
    if (NULL == list || list->length == 0)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return NULL;
    }
    // If the index we're trying to remove from is the start of the list.
    else if (index == 0)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return shift(list);
    }
    // Can't remove indices that are after the list
    else if (index > (list->length - 1))
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return NULL;
    }
    // Else if the index we're trying to remove from is before the end of the list.
    else if (index < list->length - 1)
    {
        void* retval = NULL;

        node_t* current = list->first;
        while (index--)
        {
            current = current->next;
        }

        // We need to free the removed node, and adjust the nodes before and after it.
        // current is set to the node before it.

        retval = current->val;

        if (current->prev)
        {
            current->prev->next = current->next;
        }
        if (current->next)
        {
            current->next->prev = current->prev;
        }

        free(current);

        list->length--;
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return retval;
    }
    // Else just remove the node at the end of the list.
    else
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, (void*)((intptr_t)index));
        return pop(list);
    }
}

/**
 * Remove a specific entry from the linked list
 * This only removes the first instance of the entry if it is linked multiple
 * times
 *
 * @param list  The list to remove an entry from
 * @param entry The entry to remove
 * @return The void* val associated with the removed entry
 */
void* removeEntry(list_t* list, const node_t* entry)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, entry);
    // If the list is null or empty, dont touch it
    if (NULL == list || list->length == 0)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, entry);
        return NULL;
    }
    // If the entry we're trying to remove is the fist one, shift it
    else if (list->first == entry)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, entry);
        return shift(list);
    }
    // If the entry we're trying to remove is the last one, pop it
    else if (list->last == entry)
    {
        VALIDATE_LIST(__func__, __LINE__, false, list, entry);
        return pop(list);
    }
    // Otherwise it's somewhere in the middle, or doesn't exist
    else
    {
        // Start at list->first->next because we know the entry isn't at the head
        node_t* curr = list->first->next;
        // Iterate!
        while (curr != NULL)
        {
            // Found the node to remove
            if (entry == curr)
            {
                // We need to free the removed node, and adjust the nodes before and after it.
                // current is set to the node before it.

                // Link the previous node to the next node
                if (curr->prev)
                {
                    curr->prev->next = curr->next;
                }
                // Link the next node to the previous node
                if (curr->next)
                {
                    curr->next->prev = curr->prev;
                }

                // Save a value to return
                void* retval = curr->val;

                // Free the unlinked node, decrement the list
                free(curr);
                list->length--;
                VALIDATE_LIST(__func__, __LINE__, false, list, entry);

                // Return the removed value
                return retval;
            }

            // Iterate to the next node
            curr = curr->next;
        }
    }
    // Nothing to be removed
    VALIDATE_LIST(__func__, __LINE__, false, list, entry);
    return NULL;
}

// Remove all items from the list.
void clear(list_t* list)
{
    VALIDATE_LIST(__func__, __LINE__, true, list, NULL);
    while (list->first != NULL)
    {
        pop(list);
    }
    VALIDATE_LIST(__func__, __LINE__, false, list, NULL);
}

#ifdef RUN_LINKED_LIST_TESTS
/**
 * @brief Exercise the linked list code by doing lots of random operations
 */
void listTester(void)
{
    list_t testList = {.first = NULL, .last = NULL, .length = 0};
    list_t* l       = &testList;

    // Seed the list
    for (int i = 0; i < 25; i++)
    {
        push(l, NULL);
    }

    for (int64_t i = 0; i < 100000000; i++)
    {
        if (0 == i % 10000)
        {
            printf("link tester %" PRIu64 "\n", i);
        }
        switch (rand() % 8)
        {
            case 0:
            {
                push(l, NULL);
                break;
            }
            case 1:
            {
                pop(l);
                break;
            }
            case 2:
            {
                unshift(l, NULL);
                break;
            }
            case 3:
            {
                shift(l);
                break;
            }
            case 4:
            {
                // Add to random valid index
                int idx = 0;
                if (l->length)
                {
                    idx = rand() % l->length;
                }
                add(l, NULL, idx);
                break;
            }
            case 5:
            {
                // Remove from random valid index
                int idx = 0;
                if (l->length)
                {
                    idx = rand() % l->length;
                }
                removeIdx(l, idx);
                break;
            }
            case 6:
            {
                // Remove random valid node
                int idx = 0;
                if (l->length)
                {
                    idx = rand() % l->length;
                }
                node_t* node = l->first;
                while (idx--)
                {
                    node = node->next;
                }
                removeEntry(l, node);
                break;
            }
            case 7:
            {
                // clear(l);
                break;
            }
        }
    }
    printf("List validated");
}
#endif