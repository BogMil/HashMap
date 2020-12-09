#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* resolve_collision(void* old_data, void* new_data) {
    return new_data;
}

typedef struct bucket {
    char* key;
    void* data;
    struct bucket* next;
} Bucket;

typedef struct hashMap {
    Bucket** buckets;
    size_t key_space;
    unsigned int(*hash)(char*);
} HashMap;

unsigned int hash(char* key) {
    int res = 0;
    for (char* c = key; *c; ++c) {
        res += (int)*c;
    }
    return res;
}

void _allocate_and_zeroify_buckets_memory(HashMap* hm, size_t key_space) {
    hm->buckets = calloc(key_space, sizeof(Bucket*));

    if (!hm->buckets) {
        perror("Error allocating buckets");
        abort();
    }
    memset(hm->buckets, 0, sizeof(Bucket) * key_space);
}

unsigned int _get_bucket_index_for_key(HashMap *hm, char* key) {
    return hm->hash(key) % hm->key_space;
}

char* _copy_key(char* key) {
    char* copiedKey = (char*)malloc(strlen(key) + 1);
    strcpy_s(copiedKey,sizeof(copiedKey),key);
    return copiedKey;
}

void  _put_bucket(HashMap * hm, Bucket *bucket) {
    int index = _get_bucket_index_for_key(hm, bucket->key);
    Bucket* tmp_bucket = hm->buckets[index];

    if (tmp_bucket == NULL) {
        hm->buckets[index] = bucket;
        return;
    }
        
    while (tmp_bucket->next != NULL) {
        if (strcmp(tmp_bucket->key, bucket->key) == 0) {
            tmp_bucket->data = bucket->data;
            return;
        }
        tmp_bucket = tmp_bucket->next;
    }

    if (strcmp(tmp_bucket->key, bucket->key) == 0) {
        tmp_bucket->data = bucket->data;
        return;
    }

    tmp_bucket->next = bucket;
}

HashMap* create_hashmap(size_t key_space) {
    HashMap* hm = calloc(1,sizeof(HashMap));
    //hm->key_space = calloc(1, sizeof(size_t));

    hm->key_space = key_space;
    _allocate_and_zeroify_buckets_memory(hm, key_space);
    hm->hash = hash;
    return hm;
}

void* get_data(HashMap* hm, char* key) {
    int index = _get_bucket_index_for_key(hm, key);
    Bucket* bucket = hm->buckets[index];

    if (bucket == NULL)
        return NULL;

    while (bucket != NULL && bucket->next != NULL) {
        if (strcmp(bucket->key, key) == 0) {
            return bucket->data;
        }
        bucket = bucket->next;
    }

    if (strcmp(bucket->key, key) == 0) {
        return bucket->data;
    }

    return NULL;
}

void insert_data(HashMap * hm, char* key, void* data, void* (*resolve_collision)(void* old_data, void* new_data)) {
    int index = _get_bucket_index_for_key(hm, key);
    void* data_to_store = data;
    if (get_data(hm, key) != NULL) {
        data_to_store = resolve_collision(get_data(hm, key), data);
    }

    Bucket* bucket =(Bucket*) calloc(1, sizeof(Bucket));
    /*bucket->key= malloc(sizeof(char) * strlen(key)+1);*/
    bucket->key = _copy_key(key);
    bucket->data = data_to_store;
    bucket->next = NULL;
    _put_bucket(hm, bucket);
}



////////////////////////////////////TESTS
int setup(void** state)
{
    return 0;
}

int teardown(void** state)
{
    return 0;
}

void hash_returns_expected_int(void** state)
{
    unsigned int ex1 = hash("AC");
    unsigned int ex2 = hash("AAA");

    assert_int_equal(ex1, 132);
    assert_int_equal(ex2, 65 * 3);
}

void created_hashmap_has_exact_key_space(void** state)
{
    HashMap* hm = create_hashmap(10);
    assert_int_equal(hm->key_space, 10);
}

void insert_data_copies_key(void** state)
{
    int data = 10;
    char* key = "A";
    HashMap* hm = create_hashmap(1);
    insert_data(hm, key, &data, &resolve_collision);

    assert_ptr_not_equal(key, hm->buckets[0]->key);

}

void insert_one_bucket(void** state)
{
    int data1 = 10;
    char* key1 = "AC";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);

    assert_ptr_equal(get_data(hm,key1), &data1);
}

void insert_two_buckets_with_different_index(void** state)
{
    int data1 = 10;
    int data2 = 10;
    char* key1 = "A";
    char* key2 = "C";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);
    insert_data(hm, key2, &data2, &resolve_collision);

    assert_ptr_equal(get_data(hm, key1), &data1);
    assert_ptr_equal(get_data(hm, key2), &data2);
}

void insert_two_buckets_with_same_index_and_different_keys(void** state)
{
    int data1 = 10;
    int data2 = 10;
    char* key1 = "AC";
    char* key2 = "CA";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);
    insert_data(hm, key2, &data2, &resolve_collision);

    assert_ptr_equal(get_data(hm, key1), &data1);
    assert_ptr_equal(get_data(hm, key2), &data2);
}



//void insert_data_one_bucket(void** state)
//{
//    int data1 = 10;
//    int data2 = 20;
//    int data3 = 30;
//    char* key1 = "AC";
//    char* key2 = "CA";
//    char* key3 = "A";
//    HashMap* hm = create_hashmap(10);
//    insert_data(hm, key1, &data1, &resolve_collision);
//    insert_data(hm, key2, &data2, &resolve_collision);
//    insert_data(hm, key2, &data2, &resolve_collision);
//    insert_data(hm, key3, &data3, &resolve_collision);
//
//    Bucket* x = hm->buckets[2];
//
//}


   /////////////////////////////////////////

int main() {
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(hash_returns_expected_int),
        cmocka_unit_test(created_hashmap_has_exact_key_space),
        cmocka_unit_test(insert_data_copies_key),

        cmocka_unit_test(insert_one_bucket),
        cmocka_unit_test(insert_two_buckets_with_different_index),
        cmocka_unit_test(insert_two_buckets_with_same_index_and_different_keys),
        
        
    };

    //HashMap* hashmap = create_hashmap(3);
    /* If setup and teardown functions are not
       needed, then NULL may be passed instead */

    int count_fail_tests =
        cmocka_run_group_tests(tests, setup, teardown);

    return count_fail_tests;
}


