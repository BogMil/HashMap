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

int _get_bucket_index_for_key(HashMap *hm, char* key) {
    return hm->hash(key) % hm->key_space;
}

void _put_bucket(HashMap * hm, Bucket *bucket) {
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
    
    bucket->key = (char*)calloc(strlen(key), sizeof(char));
    strcpy(bucket->key, key);

    bucket->data = data_to_store;
    bucket->next = NULL;
    _put_bucket(hm, bucket);
}

void iterate(HashMap* hm, void (*callback)(char* key, void* data)) {
    for (int i = 0; i < hm->key_space; i++) {
        if (hm->buckets[i] == NULL)
            continue;

        Bucket* temp = hm->buckets[i];

        while (temp->next != NULL)
        {
            callback(temp->key,temp->data);
            temp = temp->next;
        }

        callback(temp->key, temp->data);
    }
}

void remove_data(HashMap* hm, char* key, void (*destroy_data)(void* data)) {
    if (get_data(hm, key) == NULL)
        return;
    int index = _get_bucket_index_for_key(hm, key);
    
    if (hm->buckets[index] == NULL)
        return;
    
    if (destroy_data != NULL) 
    {
        destroy_data(get_data(hm, key));
    }

    Bucket* temp = hm->buckets[index];

    if (strcmp(temp->key, key) == 0 && temp->next == NULL)
    {
        free(temp->key);
        free(temp);
        hm->buckets[index] = NULL;
        return;
    }

    Bucket* previous = temp;
    Bucket* current= temp -> next;

    if (strcmp(previous->key, key)==0) 
    {
        hm->buckets[index] = previous->next;
        free(previous->key);
        free(previous);
        return;
    }
    
    while (current->next != NULL) 
    {
        if (strcmp(current->key, key) == 0)
        {
            previous->next=current->next;
            free(previous->key);
            free(previous);
            return;
        }
        previous = previous->next;
        current = current->next;
    }
    
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
    char* x = hm->buckets[0]->key;
    assert_ptr_not_equal(key, x);

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
    int data2 = 20;
    char* key1 = "AC";
    char* key2 = "CA";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);
    insert_data(hm, key2, &data2, &resolve_collision);

    assert_ptr_equal(get_data(hm, key1), &data1);
    assert_ptr_equal(get_data(hm, key2), &data2);
}

void insert_two_buckets_with_same_keys(void** state)
{
    int data1 = 10;
    int data2 = 20;
    char* key1 = "ACC";
    char* key2 = "ACC";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);
    insert_data(hm, key2, &data2, &resolve_collision);

    assert_ptr_equal(get_data(hm, key1), &data2);
    assert_ptr_equal(get_data(hm, key2), &data2);
}



void insert_3_buckets_with_same_indexes_and_diferent_keys(void** state)
{
    int data1 = 10;
    int data2 = 20;
    int data3 = 30;
    char* key1 = "ACA";
    char* key2 = "CAA";
    char* key3 = "AAC";
    HashMap* hm = create_hashmap(10);
    insert_data(hm, key1, &data1, &resolve_collision);
    insert_data(hm, key2, &data2, &resolve_collision);
    insert_data(hm, key3, &data3, &resolve_collision);
}
int c = 0;
void myCallback1(char* k, void* d) {
    ++c;
    //return d;
}

void test_iterator() {

     int  value1 = 1;
     char *key1 = "AC";
     int  value2 = 2;
     char *key2 = "CA";

     HashMap * hashmap = create_hashmap(2);

     insert_data(hashmap,key1,&value1, &resolve_collision);
     insert_data(hashmap,key2,&value2, &resolve_collision);


     iterate(hashmap,&myCallback1);
     assert_int_equal(c,2);
}


#pragma region LAB TESTS
void test_get_data_keeps_multiple_values_for_same_key() {
    int  value1 = 1;
    char* key1 = "AC";
    int  value2 = 2;
    char* key2 = "CA";
    HashMap* hashmap = create_hashmap(2);
    insert_data(hashmap, key1, &value1, &resolve_collision);
    insert_data(hashmap, key2, &value2, &resolve_collision);

    assert_ptr_equal(&value1, get_data(hashmap, key1));
}

void test_insert_data_works() {
    int  a = 20;
    char* key = "AC";
    HashMap* hashmap = create_hashmap(10);
    insert_data(hashmap, key, &a, &resolve_collision);

    assert_ptr_equal(&a, get_data(hashmap, key));
}

void test_get_data_returns_null_for_invalid_key() {
    int  a = 20;
    char* key = "AC";
    char* invalidKey = "C";
    HashMap* hashmap = create_hashmap(2);
    insert_data(hashmap, key, &a, &resolve_collision);

    assert_ptr_equal(NULL, get_data(hashmap, invalidKey));
}

void test_nothing_is_changed_when_invalid_key_provided() {
     int  value1 = 1;
     char *key1 = "AC";
     int  value2 = 2;
     char *key2 = "A";
     HashMap * hashmap = create_hashmap(2);
     insert_data(hashmap,key1,&value1, &resolve_collision);
     insert_data(hashmap,key2,&value2, &resolve_collision);

     remove_data(hashmap, "S",NULL);
     
     assert_string_equal(hashmap->buckets[0]->key,key1);
     assert_string_equal(hashmap->buckets[1]->key,key2);
}

void test_remove_data_works() {
        int  a = 20;
         char *key = "AC";

         HashMap * hashmap = create_hashmap(10);
         insert_data(hashmap,key,&a, &resolve_collision);
         remove_data(hashmap,key,NULL);
         assert_ptr_equal(NULL,get_data(hashmap,key));
}
#pragma endregion
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
        cmocka_unit_test(insert_two_buckets_with_same_keys),
        cmocka_unit_test(insert_3_buckets_with_same_indexes_and_diferent_keys),








        /// <summary>
        /// from lab
        /// </summary>
        /// <returns></returns>
        cmocka_unit_test(test_get_data_keeps_multiple_values_for_same_key),
        cmocka_unit_test(test_insert_data_works),
        cmocka_unit_test(test_get_data_returns_null_for_invalid_key),
        cmocka_unit_test(test_iterator),
        cmocka_unit_test(test_nothing_is_changed_when_invalid_key_provided),
        cmocka_unit_test(test_remove_data_works),
    };

    //HashMap* hashmap = create_hashmap(3);
    /* If setup and teardown functions are not
       needed, then NULL may be passed instead */

    int count_fail_tests =
        cmocka_run_group_tests(tests, setup, teardown);

    return count_fail_tests;
}


