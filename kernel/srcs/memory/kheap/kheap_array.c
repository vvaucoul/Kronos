/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kheap_array.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/19 16:47:00 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/07/30 23:22:25 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <memory/kheap.h>
#include <assert.h>

// Heap predicate function
bool heap_predicate(data_t a, data_t b) {
    return (a < b);
}

// Create a heap array
heap_array_t heap_array_create(void *addr, uint32_t max_size, heap_node_predicate_t predicate) {
    heap_array_t heap_array;
    heap_array.array = (data_t *)addr;
    memset(heap_array.array, 0, max_size * sizeof(data_t));
    heap_array.size = 0;
    heap_array.max_size = max_size;
    heap_array.predicate = predicate;
    return heap_array;
}

// Insert an element into the heap array
void heap_array_insert_element(data_t data, heap_array_t *array) {
    assert(array->predicate != NULL);

    uint32_t index = 0;
    while (index < array->size && array->predicate(array->array[index], data)) {
        index++;
    }

    if (index == array->size) {
        array->array[array->size++] = data;
    } else {
        data_t tmp = array->array[index];
        array->array[index] = data;

        while (index < array->size) {
            index++;
            data_t tmp2 = array->array[index];
            array->array[index] = tmp;
            tmp = tmp2;
        }
        array->size++;
    }
}

// Get an element from the heap array
data_t heap_array_get_element(uint32_t index, heap_array_t *array) {
    assert(index <= array->size);
    return array->array[index];
}

// Remove an element from the heap array
void heap_array_remove_element(uint32_t index, heap_array_t *array) {
    assert(index <= array->size);

    while (index < array->size) {
        array->array[index] = array->array[index + 1];
        index++;
    }
    array->size--;
}

// Destroy the heap array
void heap_destroy(heap_array_t *array) {
    kfree(array->array);
}