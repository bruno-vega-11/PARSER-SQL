//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <list>
#include <functional> // quitar

using namespace std;

static constexpr int PAGE_SIZE = 4096;

static constexpr int BUCKET_SIZE  = 3;


template<typename TKey>
class Bucket {
public:
    TKey keys[BUCKET_SIZE];
    int count = 0;
    int  localDepth = 0;
    Bucket* next = nullptr;

    explicit Bucket(int depth): localDepth(depth){}

    [[nodiscard]] bool isFull() const { return count>= BUCKET_SIZE;}

    void addKey(TKey key) {
        if (!isFull()) {
            keys[count++] = key;
        }
    }

    TKey* searchKey(TKey find) {
        for (int i = 0 ; i < count ; i++) {
            if (keys[i] == find) {
                return &keys[i];
            }
        }
        if (next != nullptr) {
            return this->next->searchKey(find);
        }
        return nullptr;
    }
};



template<typename TKey>
class ExtendibleHashing {
private:
    int D = 1; // global depth
    list<Bucket<TKey>*> directory;

    int getIndex(TKey key) {
        size_t h = hash<TKey>{}(key);
        return h & ((1 << D) - 1);
    }

    void split(int index) {
        Bucket<TKey>* oldBucket = directory[index];

        if (oldBucket->localDepth == D) {
            //has cositas
        }
        int newLocalDepth = oldBucket->localDepth+1;
        oldBucket->localDepth = newLocalDepth;
        Bucket<TKey>* newBucket = new Bucket<TKey>(newLocalDepth);

        int splitBit = 1 << (newLocalDepth - 1);

        vector<TKey> tempKeys;
        for (int i = 0 ; i < oldBucket->count; i++) {
            tempKeys.push_back(oldBucket->keys[i]);
        }
        oldBucket->count = 0;

        for (int i = 0; i < directory.size(); i++) {
            if (directory[i] == oldBucket && (i & splitBit)) {
                directory[i] = newBucket;
            }
        }
        for (const auto&k : tempKeys) {
            add(k);
        }


    }
public:
    // digamos que tienes implementaod la funcion hash y que retorna un binario
    size_t do_hash(TKey key) {
        hash<TKey> hasher; // quitar

        size_t valorHash = hasher(key);

        return valorHash & ((1 << D) - 1);

    }
    // quita esto pls ⬆️

    explicit ExtendibleHashing(){
        int size = 1 << D;
        directory.resize(size);

        Bucket<TKey>* firstBucket = new Bucket<TKey>(0);

        for (int i = 0; i < size ; i ++) {
            directory[i] = firstBucket;
        }
    }

    void add(TKey key) {
        int index = getIndex(key);
        Bucket<TKey>* target = directory[index];

        if (!target->isFull()) {
            target->addKey(key);
        } else {
            split(index);
            add(key);
        }
    }
};


#endif //EXTENDIBLEHASHING_H
