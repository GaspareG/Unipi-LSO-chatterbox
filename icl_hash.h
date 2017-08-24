/**
 * @file
 *
 * Header file for icl_hash routines.
 *
 */
/* $Id$ */
/* $UTK_Copyright: $ */

#ifndef icl_hash_h
#define icl_hash_h

#include <stdio.h>
#include <string.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef struct icl_entry_s {
    void* key;
    void *data;
    struct icl_entry_s* next;
} icl_entry_t;

typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    icl_entry_t **buckets;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
} icl_hash_t;

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) );

void
* icl_hash_find(icl_hash_t *, void* );

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *);

int
icl_hash_destroy(icl_hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(FILE *, icl_hash_t *);

int icl_hash_delete( icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*) );


/* funzioni hash per per l'hashing di stringhe */
static inline unsigned int string_hash_function( void *key ) {

    unsigned int hash = 5381;
    int c;
    unsigned char *str = (unsigned char*) key;
    while ( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static inline int string_key_compare( void *key1, void *key2  ) {
    return strcmp( (char*) key1, (char*) key2 ) == 0;
}

static inline unsigned int fnv_hash_function( void *key, int len ) {
    unsigned char *p = (unsigned char*)key;
    unsigned int h = 2166136261u;
    int i;
    for ( i = 0; i < len; i++ )
        h = ( h * 16777619 ) ^ p[i];
    return h;
}

/* funzioni hash per per l'hashing di interi */
static inline unsigned int ulong_hash_function( void *key ) {
    unsigned int x = *(unsigned long*)key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
    //int len = sizeof(unsigned long);
    //unsigned int hashval = fnv_hash_function( key, len );
    //return hashval;
}

static inline int ulong_key_compare( void *key1, void *key2  ) {
    //unsigned int x = *(unsigned long*)key1;
    //unsigned int y = *(unsigned long*)key2;
    return ( *(unsigned long*)key1 == *(unsigned long*)key2 );
}
#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next)


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
