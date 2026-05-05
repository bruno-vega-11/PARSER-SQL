//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <vector>
#include <bits/stdc++.h> // fstream

using namespace std;

struct RID {int page_id ; int slot;};
static constexpr RID NULLRID  = {-1,-1};
inline bool isNullRID(const RID& r) {return r.page_id == -1 && r.slot == -1;}

static constexpr int PAGE_SIZE = 4096;
using PageID = int;
static constexpr PageID NULL_PAGE = -1;
struct Page { char data[PAGE_SIZE]; };

static constexpr int DLIMIT       = 16;
//-----------------------------------DISK---------------------------------------------

//Página 0  →  meta (nextPage + dirPageId + D)
//Página 1  →  primer bucket
//Página 2  →  segundo bucket

class Diske {
private:
    fstream file;
    int nextPage = -1; // pagina 0
    static constexpr int META_OFFSET_NEXT = 0;
    static constexpr int META_OFFSET_DIRPAGE = sizeof(int);
    static constexpr int META_OFFSET_D = sizeof(int)  + sizeof(PageID);

    int readCount = 0;
    int writeCount = 0;

    void loadMeta() {
        file.seekg(0,ios::end);
        streamsize sz = file.tellg();

        if (sz < PAGE_SIZE) {
            nextPage = 1;
            saveMeta(NULL_PAGE,1);
            return;
        }
        Page p{};
        file.seekg(0);
        file.read(p.data, PAGE_SIZE);
        memcpy(&nextPage, p.data + META_OFFSET_NEXT,sizeof(int));
    }

public:
    explicit Diske(const string& filename) {
        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
        }
        loadMeta();
    }

    Page read(PageID id) {
        ++readCount;
        Page p{};
        file.seekg((long long)id * PAGE_SIZE);
        file.read(p.data, PAGE_SIZE);
        return p;
    }

    void write(PageID id, const Page& p) {
        ++writeCount;
        file.seekp((long long)id * PAGE_SIZE);
        file.write(p.data, PAGE_SIZE);
        file.flush();
    }

    PageID alloc() {
        PageID id = nextPage++;
        Page p{};
        write(id, p);       // inicializar la página en disco
        return id;
    }

    void saveMeta(PageID dirPageId, int D) {
        Page p{};
        file.seekg(0);
        file.read(p.data, PAGE_SIZE);

        memcpy(p.data + META_OFFSET_NEXT,    &nextPage,  sizeof(int));
        memcpy(p.data + META_OFFSET_DIRPAGE, &dirPageId, sizeof(PageID));
        memcpy(p.data + META_OFFSET_D,       &D,         sizeof(int));

        file.seekp(0);
        file.write(p.data, PAGE_SIZE);
        file.flush();
    }

    pair<PageID, int> loadMetaValues() {
        Page p{};
        file.seekg(0);
        file.read(p.data, PAGE_SIZE);

        PageID dirPageId = NULL_PAGE;
        int    D         = 1;
        memcpy(&dirPageId, p.data + META_OFFSET_DIRPAGE, sizeof(PageID));
        memcpy(&D,         p.data + META_OFFSET_D,       sizeof(int));

        return { dirPageId, D };
    }

    void resetCounters()      { readCount = writeCount = 0; }
    int  totalReads()   const { return readCount;  }
    int  totalWrites()  const { return writeCount; }
    int  totalAccesses()const { return readCount + writeCount; }
    int  pageCount()    const { return nextPage; }
};
// -------------------------------------------------------------------------------------

template<typename TKey>
static constexpr int computeBucketCapacity() {
    // Overhead: count(int) + localDepth(int) + next(ptr) + useChaining(bool)
    constexpr int overhead = static_cast<int>(sizeof(int) + sizeof(int) + sizeof(PageID) + sizeof(bool));
    return (PAGE_SIZE - overhead) / sizeof(TKey);
}


template<typename TKey>
struct BucketPage {
    int count = 0;
    int  localDepth = 0;
    PageID nextPage = NULL_PAGE;
    bool useChaining = false;

    static constexpr int BUCKET_SIZE = computeBucketCapacity<TKey>();
    TKey keys[BUCKET_SIZE];

    explicit BucketPage(int depth): localDepth(depth){}

    /*


    TKey* searchKey(TKey find) {
        for (int i = 0 ; i < count ; i++) {
            if (keys[i] == find) {
                return &keys[i];
            }
        }
        if (next != nullptr) {
            return this->nextPage->searchKey(find);
        }
        return nullptr;
    }
    */ // toda esta logica se pasa al extendible hashing
};

//--------------------------------------------------------------------------------------------------

template<typename TKey>
class ExtendibleHashing {
private:
    using BucketT = BucketPage<TKey>;

    Diske& disk;
    int D = 1; // global depth [1-16]
    vector<PageID> directory;  // en memoria mientras corre
    PageID dirPageId;          // pagina donde se serializa el vector (disco)

public:

    int getIndex(TKey key) {
        size_t h = hash<TKey>{}(key);  // cambiar por un hash implementado por mi
        return h & ((1 << D) - 1);     // se queda con los ultimos bits, para mapearlo en mi directorio
    }

    size_t getHash(TKey key) {
        return hash<TKey>{}(key);     // aplica el hash nada mas, sin hayar su lugar en directory
    }

    void add_hash(TKey key) {
        int index = getIndex(key);
        PageID targetId = directory[index];

        Page p = disk.read(targetId);  // lees pagina - bucket
        BucketT* b = asBucket(p);   // cast: ->

        // si esta lleno y no hay chaining
        if (isFull(b) && !b->useChaining) {
            split(index); // falta arreglar
            add_hash(key);
            return;
        }
        // si esta lleno y hay chaining:
        while (isFull(b) && b->useChaining) {
            if (b->nextPage == NULL_PAGE) {
                b->nextPage = disk.alloc();  // crear siguiente bucket
                disk.write(targetId,p);      //

                PageID newId = b->nextPage;
                Page   np    = disk.read(newId);
                BucketT* nb  = asBucket(np);
                nb->localDepth  = b->localDepth;
                nb->count       = 0;
                nb->nextPage    = NULL_PAGE;
                nb->useChaining = true;
                disk.write(newId, np);
                return;
            }
            PageID nextId = b->nextPage;
            p = disk.read(nextId);
            b = asBucket(p);
            targetId = nextId;
        }
        b->keys[b->count++] = key;
        disk.write(targetId,p);
    }


    // helpers:
    BucketT* asBucket(Page& p) {
        return reinterpret_cast<BucketT*>(p.data);
    }
    bool isFull(BucketT* b) const { return count>=b->BUCKET_SIZE;}

    /*

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

        int buddyIndex = index ^(1 << (bucket->localDepth - 1));
        Bucket<TKey>* buddy = directory[buddyIndex];
        if (buddy != bucket && buddy->localDepth == bucket->localDepth) {
            if (bucket->count + buddy->count <= BUCKET_SIZE) {
                for (int i = 0; i < buddy->count; i++) {
                    bucket->addKey(buddy->keys[i]);
                }
                for (int i = 0; i < directory.size(); i++) {
                    if (directory[i] == buddy) {
                        directory[i] = bucket;
                    }
                }
                bucket->localDepth--;
                delete buddy;

                //shrinkDirectory();
            }
        }

    }

    explicit ExtendibleHashing(){
        int size = 1 << D;
        directory.resize(size);

        Bucket<TKey>* firstBucket = new Bucket<TKey>(0);

        for (int i = 0; i < size ; i ++) {
            directory[i] = firstBucket;
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
    */
};


#endif //EXTENDIBLEHASHING_H
