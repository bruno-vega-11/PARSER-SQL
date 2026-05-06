//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <vector>
#include <bits/stdc++.h>

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
        file.clear();
        file.seekg(0,ios::end);
        streamsize sz = file.tellg();

        if (sz < PAGE_SIZE) {
            nextPage = 1;
            saveMeta(NULL_PAGE,1);
            return;
        }
        Page p{};
        file.clear();
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
        file.clear();
        file.seekg((long long)id * PAGE_SIZE);
        file.read(p.data, PAGE_SIZE);
        if (!file) {
            cerr << "ERROR: read fallo en página " << id << "\n";
        }
        return p;
    }

    void write(PageID id, const Page& p) {
        ++writeCount;
        file.clear();
        file.seekp((long long)id * PAGE_SIZE);
        file.write(p.data, PAGE_SIZE);
        file.flush();
        if (!file) {
            cerr << "ERROR: write fallo en página " << id << "\n";
        }
    }

    PageID alloc() {
        PageID id = nextPage++;
        Page p{};
        write(id, p);       // inicializar la página en disco
        return id;
    }

    void saveMeta(PageID dirPageId, int D) {
        Page p{};

        file.clear();
        file.seekg(0, ios::end);
        streamsize sz = file.tellg();

        if (sz >= PAGE_SIZE) {
            file.clear();
            file.seekg(0);
            file.read(p.data, PAGE_SIZE);
        }
        memcpy(p.data + META_OFFSET_NEXT,    &nextPage,  sizeof(int));
        memcpy(p.data + META_OFFSET_DIRPAGE, &dirPageId, sizeof(PageID));
        memcpy(p.data + META_OFFSET_D,       &D,         sizeof(int));

        file.clear();
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

    void saveDirectory(PageID& dirPageId,int D,const vector<PageID>& directory) {
        if (dirPageId == NULL_PAGE)
            dirPageId = alloc();

        Page p{};
        int size = (int)directory.size();
        memcpy(p.data, &size, sizeof(int));
        for (int i = 0; i < size; i++)
            memcpy(p.data + sizeof(int) + i * sizeof(PageID),
                   &directory[i], sizeof(PageID));

        write(dirPageId, p);
        saveMeta(dirPageId, D);
    }

    vector<PageID> loadDirectory(PageID dirPageId) {
        vector<PageID> directory;
        if (dirPageId == NULL_PAGE) return directory;

        Page p   = read(dirPageId);
        int size = 0;
        memcpy(&size, p.data, sizeof(int));

        directory.resize(size);
        for (int i = 0; i < size; i++)
            memcpy(&directory[i],
                   p.data + sizeof(int) + i * sizeof(PageID),
                   sizeof(PageID));

        return directory;
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

};

//--------------------------------------------------------------------------------------------------

template<typename TKey>
class ExtendibleHashing {
private:
    using BucketT = BucketPage<TKey>;

    Diske& disk;
    int D = 1; // global depth [1-16]
    vector<PageID> directory;  // en memoria mientras corre
    PageID dirPageId = NULL_PAGE;          // pagina donde se serializa el vector (disco)

    int getIndex(TKey key) {
        size_t h = hash<TKey>{}(key);
        return h & ((1 << D) - 1);     // se queda con los ultimos bits, para mapearlo en mi directorio
    }

    size_t getHash(TKey key) {
        return hash<TKey>{}(key);     // aplica el hash nada mas, sin hayar su lugar en directory
    }

public:

    explicit ExtendibleHashing(Diske& d): disk(d) {
        auto [dp, loadedD] = disk.loadMetaValues();
        if (dp == NULL_PAGE) {
            // primera vez — archivo nuevo
            D = 1;
            int size = 1 << D;
            directory.resize(size);

            // crear el único bucket inicial
            PageID firstBucket = disk.alloc();
            Page   p           = disk.read(firstBucket);
            BucketT* b         = asBucket(p);
            b->count           = 0;
            b->localDepth      = 0;
            b->nextPage        = NULL_PAGE;
            b->useChaining     = false;
            disk.write(firstBucket, p);

            // todas las entradas del directorio apuntan a ese bucket
            for (int i = 0; i < size; i++)
                directory[i] = firstBucket;
            saveDirectory();
        } else {
            // ya existía — reconstruir desde disco
            dirPageId = dp;
            D         = loadedD;
            directory = disk.loadDirectory(dirPageId);
        }
    }

    void expandDirectory() {
        int oldsize = 1 << D;
        D++;
        int newsize = 1 << D;

        directory.resize(newsize);

        for (int i=0; i < oldsize; i++) {
            directory[i + oldsize] = directory[i];
        }
        saveDirectory();
    }

    void split(int index) {
        PageID oldId = directory[index];
        Page p = disk.read(oldId);
        BucketT* oldBucket = asBucket(p);

        if (oldBucket->localDepth == D) {
            if (D>= DLIMIT) {
                oldBucket->useChaining = true;
                disk.write(oldId,p);
                return;
            }
            expandDirectory();
            p = disk.read(oldId);
            oldBucket = asBucket(p);
        }
        // Modificar local depth y definit splitbit
        int newLocalDepth = oldBucket->localDepth+1;
        int splitBit = 1 << (newLocalDepth - 1);

        // se guardar keys del old bucket
        vector<TKey> tempKeys;
        for (int i = 0 ; i < oldBucket->count; i++) {
            tempKeys.push_back(oldBucket->keys[i]);
        }
        oldBucket->count = 0;

        // borrando old bucket
        oldBucket->localDepth = newLocalDepth;
        oldBucket->count = 0;
        disk.write(oldId,p);

        // creando nuevo bucket
        PageID newId = disk.alloc();
        Page np = disk.read(newId);
        BucketT* newBucket = asBucket(np);
        newBucket->localDepth = newLocalDepth;
        newBucket->count = 0;
        newBucket->nextPage = NULL_PAGE;
        newBucket->useChaining = false;
        disk.write(newId,np);

        // reasignar entradas del directorio
        for (int i = 0; i < (int)directory.size(); i++) {
            if (directory[i] == oldId && (i & splitBit)) {
                directory[i] = newId;
            }
        }

        for (TKey k:tempKeys) {
            size_t h = getHash(k);
            PageID destId = (h & splitBit) ? newId : oldId;

            Page dp = disk.read(destId);
            BucketT* db = asBucket(dp);
            db->keys[db->count++] = k;
            disk.write(destId,dp);
        }
        // persistir
        saveDirectory();
    }

    void add_hash(TKey key) {
        int index = getIndex(key);
        PageID targetId = directory[index];

        Page p = disk.read(targetId);  // lees pagina
        BucketT* b = asBucket(p);   // cast:pagina -> bucket

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
            }
            PageID nextId = b->nextPage;
            p = disk.read(nextId);
            b = asBucket(p);
            targetId = nextId;
        }
        b->keys[b->count++] = key;
        disk.write(targetId,p);
    }

    vector<RID> search_hash(const TKey& key) {
        vector<RID> result;
        int index = getIndex(key);
        PageID target = directory[index];

        while (target != NULL_PAGE) {
            Page p = disk.read(target);
            BucketT* current = asBucket(p);

            for (int i = 0 ; i < current->count ; i++) {
                if (key == current->keys[i]) {
                    result.push_back({target,i});
                }
            }
            target = current->nextPage;
        }
        return result;
    }

    void delete_hash(const TKey& key){
        int index = getIndex(key);
        PageID target = directory[index];

        bool borrado = delete_aux(key,target);

        if (!borrado) {
            cout << "No se encontro la llave a borrar";
        }
        //try_merge(bucket,index);
    }

    bool delete_aux(const TKey& key,PageID cubetaId) {
        while (cubetaId != NULL_PAGE) {
            Page p = disk.read(cubetaId);
            BucketT* cubeta = asBucket(p);

            for (int i = 0; i < cubeta->count; i++) {
                if (cubeta->keys[i] == key) {
                    for (int j = i; j < cubeta->count - 1; j++) {
                        cubeta->keys[j] = cubeta->keys[j + 1];
                    }
                    cubeta->count  = cubeta->count - 1;
                    disk.write(cubetaId,p);
                    return true;
                }
            }
            cubetaId = cubeta->nextPage;
        }
        return false;
    }

    // helpers:
    BucketT* asBucket(Page& p) {
        return reinterpret_cast<BucketT*>(p.data);
    }
    bool isFull(BucketT* b) const { return b->count >= BucketT::BUCKET_SIZE;}

    void saveDirectory() {
        disk.saveDirectory(dirPageId, D, directory);
    }

};

/*
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
*/


#endif //EXTENDIBLEHASHING_H
