//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <vector>
#include <cstdint>

using namespace std;

struct RID_h {int page_id ; int slot;};
static constexpr RID_h NULLRID  = {-1,-1};
inline bool isNullRID(const RID_h& r) {return r.page_id == -1 && r.slot == -1;}

static constexpr int PAGE_SIZE_H = 4096;
using PageID = int;
static constexpr PageID NULL_PAGE_H = -1;
struct Page_h { char data[PAGE_SIZE_H]; };

static constexpr int DLIMIT       = 9;

// -------------------------------MurmurHash3--------------------------------------
static uint32_t murmur3_32(const void* key, int len, uint32_t seed = 42) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];
        k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2;
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2; h1 ^= k1;
    }

    h1 ^= len;
    h1 ^= h1 >> 16; h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13; h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}
//--------------------------------FixedString---------------------------------------
template<int N>
struct FixedString_H {
    char data[N];

    FixedString_H() {
        memset(data, 0, N);
    }

    FixedString_H(const char* s) {
        memset(data, 0, N);
        memcpy(data, s, min((int)strlen(s), N));
    }

    FixedString_H(const std::string& s) {
        memset(data, 0, N);
        memcpy(data, s.c_str(), min((int)s.size(), N));
    }

    bool operator<(const FixedString_H& other) const {
        return strncmp(data, other.data, N) < 0;
    }

    bool operator>(const FixedString_H& other) const {
        return strncmp(data, other.data, N) > 0;
    }

    bool operator==(const FixedString_H& other) const {
        return strncmp(data, other.data, N) == 0;
    }

    bool operator<=(const FixedString_H& other) const {
        return !(*this > other);
    }

    bool operator>=(const FixedString_H& other) const {
        return !(*this < other);
    }

    friend std::ostream& operator<<(std::ostream& os, const FixedString_H<N>& fs) {
        os << fs.data;
        return os;
    }
};
namespace std {
    template<int N>
    struct hash<FixedString_H<N>> {
        size_t operator()(const FixedString_H<N>& fs) const {
            size_t h = 0;
            for (int i = 0; i < N && fs.data[i] != '\0'; i++) {
                h = h * 31 + static_cast<unsigned char>(fs.data[i]);
            }
            return h;
        }
    };
}
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

        if (sz < PAGE_SIZE_H) {
            nextPage = 1;
            saveMeta(NULL_PAGE_H,1);
            return;
        }
        Page_h p{};
        file.clear();
        file.seekg(0);
        file.read(p.data, PAGE_SIZE_H);
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

    Page_h read(PageID id) {
        ++readCount;
        Page_h p{};
        file.clear();
        file.seekg((long long)id * PAGE_SIZE_H);
        file.read(p.data, PAGE_SIZE_H);
        if (!file) {
            cerr << "ERROR: read fallo en página " << id << "\n";
        }
        return p;
    }

    void write(PageID id, const Page_h& p) {
        ++writeCount;
        file.clear();
        file.seekp((long long)id * PAGE_SIZE_H);
        file.write(p.data, PAGE_SIZE_H);
        file.flush();
        if (!file) {
            cerr << "ERROR: write fallo en página " << id << "\n";
        }
    }

    PageID alloc() {
        PageID id = nextPage++;
        Page_h p{};
        write(id, p);       // inicializar la página en disco
        return id;
    }

    void saveMeta(PageID dirPageId, int D) {
        Page_h p{};

        file.clear();
        file.seekg(0, ios::end);
        streamsize sz = file.tellg();

        if (sz >= PAGE_SIZE_H) {
            file.clear();
            file.seekg(0);
            file.read(p.data, PAGE_SIZE_H);
        }
        memcpy(p.data + META_OFFSET_NEXT,    &nextPage,  sizeof(int));
        memcpy(p.data + META_OFFSET_DIRPAGE, &dirPageId, sizeof(PageID));
        memcpy(p.data + META_OFFSET_D,       &D,         sizeof(int));

        file.clear();
        file.seekp(0);
        file.write(p.data, PAGE_SIZE_H);
        file.flush();
    }

    pair<PageID, int> loadMetaValues() {
        Page_h p{};
        file.seekg(0);
        file.read(p.data, PAGE_SIZE_H);

        PageID dirPageId = NULL_PAGE_H;
        int    D         = 1;
        memcpy(&dirPageId, p.data + META_OFFSET_DIRPAGE, sizeof(PageID));
        memcpy(&D,         p.data + META_OFFSET_D,       sizeof(int));

        return { dirPageId, D };
    }

    void saveDirectory(PageID& dirPageId,int D,const vector<PageID>& directory) {
        if (dirPageId == NULL_PAGE_H)
            dirPageId = alloc();

        Page_h p{};
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
        if (dirPageId == NULL_PAGE_H) return directory;

        Page_h p   = read(dirPageId);
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
    constexpr int leafEntry= static_cast<int>(sizeof(TKey) + sizeof(RID_h));
    return (PAGE_SIZE_H - overhead) / leafEntry;
}


template<typename TKey>
struct BucketPage {
    int count ;
    int  localDepth;
    PageID nextPage;
    bool useChaining;

    static constexpr int BUCKET_SIZE = computeBucketCapacity<TKey>();
    TKey keys[BUCKET_SIZE];
    RID_h  values[BUCKET_SIZE];

};

//--------------------------------------------------------------------------------------------------
template<typename TKey>
class ExtendibleHashing {
private:
    using BucketT = BucketPage<TKey>;

    Diske& disk;
    int D = 1;
    vector<PageID> directory;
    PageID dirPageId = NULL_PAGE_H;

    int getIndex(const TKey& key) {
        return (int)(getHash(key) & ((1 << D) - 1));;
    }

    size_t getHash(const TKey& key) {
        return murmur3_32(&key,sizeof(TKey));
    }

    BucketT* asBucket(Page_h& p) {
        return reinterpret_cast<BucketT*>(p.data);
    }

    bool isFull(BucketT* b) const {
        return b->count >= BucketT::BUCKET_SIZE;
    }

    void saveDirectory() {
        disk.saveDirectory(dirPageId, D, directory);
    }

public:

    explicit ExtendibleHashing(Diske& d) : disk(d) {
        auto [dp, loadedD] = disk.loadMetaValues();
        if (dp == NULL_PAGE_H) {
            D = 1;
            int size = 1 << D;
            directory.resize(size);

            PageID firstBucket = disk.alloc();
            Page_h p = disk.read(firstBucket);
            BucketT* b     = asBucket(p);
            b->count       = 0;
            b->localDepth  = 1;
            b->nextPage    = NULL_PAGE_H;
            b->useChaining = false;
            disk.write(firstBucket, p);

            for (int i = 0; i < size; i++)
                directory[i] = firstBucket;
            saveDirectory();
        } else {
            dirPageId = dp;
            D         = loadedD;
            directory = disk.loadDirectory(dirPageId);
        }
    }

    void expandDirectory() {
        int oldSize = 1 << D;
        D++;
        directory.resize(1 << D);
        for (int i = 0; i < oldSize; i++)
            directory[i + oldSize] = directory[i];
        saveDirectory();
    }

    void split(int index) {
    PageID oldId = directory[index];
    Page_h oldPage = disk.read(oldId);
    BucketT* oldBucket = asBucket(oldPage);

    cerr << "[SPLIT] index=" << index << " oldId=" << oldId
         << " localDepth=" << oldBucket->localDepth
         << " D=" << D << " count=" << oldBucket->count << "\n";

    if (oldBucket->localDepth == D) {
        if (D >= DLIMIT) {
            cerr << "[SPLIT] DLIMIT alcanzado — activando chaining en bucket " << oldId << "\n";
            oldBucket->useChaining = true;
            disk.write(oldId, oldPage);
            return;
        }
        cerr << "[SPLIT] expandiendo directorio D=" << D << " -> " << D+1 << "\n";
        expandDirectory();
        oldPage   = disk.read(oldId);
        oldBucket = asBucket(oldPage);
    }

    int newLocalDepth = oldBucket->localDepth + 1;
    int splitBit      = 1 << (newLocalDepth - 1);

    // ✅ DIAGNÓSTICO antes de vaciar el bucket
    {
        int goOld = 0, goNew = 0;
        for (int i = 0; i < oldBucket->count; i++) {
            size_t h       = getHash(oldBucket->keys[i]);
            int    fullIdx = (int)(h & ((1 << newLocalDepth) - 1));
            if (fullIdx & splitBit) goNew++;
            else                    goOld++;
        }
        cerr << "[SPLIT] pre-redistribucion: newLocalDepth=" << newLocalDepth
             << " splitBit=" << splitBit
             << " → OLD=" << goOld << " NEW=" << goNew << "\n";

        // si todos van al mismo lado, el hash es identidad
        if (goOld == 0 || goNew == 0) {
            // mostrar los primeros 5 hashes para confirmarlo
            cerr << "[SPLIT] ⚠ TODOS AL MISMO LADO — primeros 5 hashes:\n";
            for (int i = 0; i < min(5, (int)oldBucket->count); i++) {
                size_t h       = getHash(oldBucket->keys[i]);
                int    fullIdx = (int)(h & ((1 << newLocalDepth) - 1));
                cerr << "  key=" << oldBucket->keys[i]
                     << " hash=" << h
                     << " fullIdx=" << fullIdx
                     << " bit=" << ((fullIdx & splitBit) ? 1 : 0) << "\n";
            }
        }
    }

    vector<TKey>  tempKeys(oldBucket->keys,   oldBucket->keys   + oldBucket->count);
    vector<RID_h> tempVals(oldBucket->values, oldBucket->values + oldBucket->count);

    oldBucket->localDepth = newLocalDepth;
    oldBucket->count      = 0;
    disk.write(oldId, oldPage);

    PageID newId = disk.alloc();
    Page_h newPage = disk.read(newId);
    BucketT* newBucket = asBucket(newPage);
    newBucket->localDepth  = newLocalDepth;
    newBucket->count       = 0;
    newBucket->nextPage    = NULL_PAGE_H;
    newBucket->useChaining = false;
    disk.write(newId, newPage);

    for (int i = 0; i < (int)directory.size(); i++)
        if (directory[i] == oldId && (i & splitBit))
            directory[i] = newId;

    for (int i = 0; i < (int)tempKeys.size(); i++) {
        size_t h       = getHash(tempKeys[i]);
        int    fullIdx = (int)(h & ((1 << newLocalDepth) - 1));
        PageID destId  = (fullIdx & splitBit) ? newId : oldId;

        Page_h destPage = disk.read(destId);
        BucketT* db     = asBucket(destPage);

        if (db->count >= BucketT::BUCKET_SIZE) {
            cerr << "[SPLIT] overflow en redistribucion — activando chaining bucket " << destId << "\n";
            db->useChaining = true;
            disk.write(destId, destPage);

            PageID chainId = disk.alloc();
            Page_h chainPage = disk.read(chainId);
            BucketT* cb      = asBucket(chainPage);
            cb->count        = 0;
            cb->localDepth   = newLocalDepth;
            cb->nextPage     = NULL_PAGE_H;
            cb->useChaining  = true;
            cb->keys[0]      = tempKeys[i];
            cb->values[0]    = tempVals[i];
            cb->count        = 1;
            disk.write(chainId, chainPage);

            db->nextPage = chainId;
            disk.write(destId, destPage);
            continue;
        }

        db->keys  [db->count] = tempKeys[i];
        db->values[db->count] = tempVals[i];
        db->count++;
        disk.write(destId, destPage);
    }

    // ✅ DIAGNÓSTICO resultado final
    {
        Page_h op2 = disk.read(oldId);
        BucketT* ob2 = asBucket(op2);
        Page_h np2 = disk.read(newId);
        BucketT* nb2 = asBucket(np2);
        cerr << "[SPLIT] resultado: oldId=" << oldId << " count=" << ob2->count
             << " | newId=" << newId << " count=" << nb2->count << "\n";
    }

    saveDirectory();
    cerr << "[SPLIT] completado\n\n";
    }

    PageID findOrMakeBucket(const TKey& key) {
        int iteraciones = 0;
        while (true) {
            if (++iteraciones > 1000) {
                cerr << "LOOP INFINITO en findOrMakeBucket\n";
                return NULL_PAGE_H;
            }

            int    index    = getIndex(key);
            PageID currId   = directory[index];
            PageID prevId   = NULL_PAGE_H;
            bool   needSplit = false;

            while (currId != NULL_PAGE_H) {
                Page_h   p       = disk.read(currId);
                BucketT* current = asBucket(p);

                if (!isFull(current))
                    return currId;

                if (!current->useChaining)
                    needSplit = true;

                prevId = currId;
                currId = current->nextPage;
            }

            if (needSplit) {
                split(index);
                continue;   // reintentar — el directorio cambió
            }

            // Todos los buckets tienen useChaining=true y están llenos → extender cadena
            Page_h   prevPage   = disk.read(prevId);
            BucketT* prevBucket = asBucket(prevPage);

            PageID newId = disk.alloc();
            Page_h   newPage   = disk.read(newId);
            BucketT* newBucket = asBucket(newPage);
            newBucket->count       = 0;
            newBucket->localDepth  = prevBucket->localDepth;
            newBucket->nextPage    = NULL_PAGE_H;
            newBucket->useChaining = true;
            disk.write(newId, newPage);

            prevBucket->nextPage = newId;
            disk.write(prevId, prevPage);

            return newId;
        }
    }

    void insertIntoBucket(PageID page_id, const TKey& key, const RID_h& rid) {
        Page_h   p = disk.read(page_id);
        BucketT* b = asBucket(p);
        b->keys  [b->count] = key;
        b->values[b->count] = rid;
        b->count++;
        disk.write(page_id, p);
    }

    void insert_hash(const TKey& key, const RID_h& rid) {
        insertIntoBucket(findOrMakeBucket(key), key, rid);
    }

    vector<RID_h> search_hash(const TKey& key) {
        vector<RID_h> result;
        int    index  = getIndex(key);
        PageID currId = directory[index];

        cerr << "[SEARCH] key hash=" << getHash(key)
         << " index=" << index << "\n";

        while (currId != NULL_PAGE_H) {
            Page_h   p       = disk.read(currId);
            BucketT* current = asBucket(p);

            cerr << "[SEARCH] bucket=" <<currId
             << " count=" << current->count << "\n";

            for (int i = 0; i < current->count; i++) {
                cerr << "[SEARCH]   stored key hash=" << getHash(current->keys[i])
                 << " match=" << (key == current->keys[i]) << "\n";
                if (key == current->keys[i])
                    result.push_back(current->values[i]);
            }
            currId = current->nextPage;
        }
        return result;
    }

    void delete_hash(const TKey& key, const RID_h& target) {
        int    index  = getIndex(key);
        PageID currId = directory[index];

        while (currId != NULL_PAGE_H) {
            Page_h   p      = disk.read(currId);
            BucketT* bucket = asBucket(p);

            for (int i = 0; i < bucket->count; i++) {
                if (bucket->keys[i]           == key          &&
                    bucket->values[i].page_id == target.page_id &&
                    bucket->values[i].slot    == target.slot)
                {
                    for (int j = i; j + 1 < bucket->count; j++) {
                        bucket->keys  [j] = bucket->keys  [j+1];
                        bucket->values[j] = bucket->values[j+1];
                    }
                    bucket->count--;
                    disk.write(currId, p);
                    return;
                }
            }
            currId = bucket->nextPage;
        }
        cout << "No se encontro la entrada a borrar\n";
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
