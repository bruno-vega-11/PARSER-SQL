//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <list>
#include <functional> // quitar
#include <stdexcept>

using namespace std;

static constexpr int PAGE_SIZE = 4096;

static constexpr int BUCKET_SIZE  = 3; // arreglar estitoxd
static constexpr int DLIMIT       = 16;

template<typename TKey>
class Bucket {
public:
    TKey keys[BUCKET_SIZE];
    int count = 0;
    int  localDepth = 0;
    Bucket* next = nullptr;
    bool useChaining = false;

    explicit Bucket(int depth): localDepth(depth){}

    [[nodiscard]] bool isFull() const { return count>= BUCKET_SIZE;}

    void addKey(TKey key) {
        if (!isFull()) {
            keys[count++] = key;
            return;
        }
        if (useChaining) {
            if (next == nullptr) next = new Bucket<TKey>(localDepth);
            next->addKey(key);
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
    int D = 1; // global depth [1-16]
    list<Bucket<TKey>*> directory;

    int getIndex(TKey key) {
        size_t h = hash<TKey>{}(key);  // cambiar por un hash implementado por mi
        return h & ((1 << D) - 1); // se queda con los ultimos bits, para mapearlo en mi directorio
    }

    size_t getHash(TKey key) {
        return hash<TKey>{}(key);
    }

    void expandDirectory() {
        int oldsize = 1 << D;
        D++;
        int newsize = 1 << D;

        directory.resize(newsize);

        for (int i=0; i < oldsize; i++) {
            directory[i + oldsize] = directory[i];
        }
    }

    void split(int index) {
        Bucket<TKey>* oldBucket = directory[index];

        if (oldBucket->localDepth == D) {
            if (D>= DLIMIT) {
                oldBucket->useChaining = true;
                return;
            }
            expandDirectory();
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
        for (TKey k:tempKeys) {
            size_t h = getHash(k);
            if (h & splitBit) {
                newBucket->addKey(k);
            } else {
                oldBucket->addKey(k);
            }
        }

    }

    bool delete_aux(TKey key,Bucket<TKey>* cubeta) {
        for (int i = 0; i < cubeta->count; i++) {
            if (cubeta->keys[i] == key) {
                for (int j = i; j < cubeta->count - 1; j++) {
                    cubeta->keys[j] = cubeta->keys[j + 1];
                }
                cubeta->count --;
                return true;
            }
        }
        if (cubeta->next != nullptr) {
            return delete_aux(key,cubeta->next);
        }
        return false;
    }

    void try_merge(Bucket<TKey>* bucket,int index) {
        if (bucket->localDepth == 0 || bucket->useChaining) return;

        int buddyIndex = index ^(   1 << (bucket->localDepth - 1));
        Bucket<TKey>* buddy = directory[buddyIndex];


    }
public:

    explicit ExtendibleHashing(){
        int size = 1 << D;
        directory.resize(size);

        Bucket<TKey>* firstBucket = new Bucket<TKey>(0);

        for (int i = 0; i < size ; i ++) {
            directory[i] = firstBucket;
        }
    }

    void add_hash(TKey key) {
        int index = getIndex(key);
        Bucket<TKey>* target = directory[index];

        if (target->isFull() && !target->useChaining) {
            split(index);
            add_hash(key);
        } else {
            target->addKey(key);
        }
    }

    list<int> search_hash(TKey key) {
        list<int> result;
        int index = getIndex(key);

        Bucket<TKey>* current = directory[index];

        while (current != nullptr) {
            for (int i = 0 ; i < current->count ; i++) {
                if (key == current->keys[i]) {
                    result.push_back(i);
                }
            }
            current = current->next;
        }
        return result;
    }

    void delete_hash(TKey key){
        int index = getIndex(key);
        Bucket<TKey>* bucket = directory[index];
        bool borrado = delete_aux(key,bucket);

        if (!borrado) {
            throw runtime_error("XDDD que webon");
        }
        try_merge(bucket,index);
    }
};


#endif //EXTENDIBLEHASHING_H
