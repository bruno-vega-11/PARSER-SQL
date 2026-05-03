#include "BPTree.h"

void Disk::loadMeta() {
    file.seekg(0, ios::end);
    streamsize sz = file.tellg();
    if (sz < PAGE_SIZE) {
        nextPage = 1; cachedRoot = NULL_PAGE; saveMeta();
    } else {
        Page p{}; file.seekg(0); file.read(p.data, PAGE_SIZE);
        memcpy(&nextPage,   p.data + META_OFFSET_NEXT, sizeof(int));
        memcpy(&cachedRoot, p.data + META_OFFSET_ROOT, sizeof(PageID));
    }
}

void Disk::saveMeta() {
    Page p{};
    memcpy(p.data + META_OFFSET_NEXT, &nextPage,   sizeof(int));
    memcpy(p.data + META_OFFSET_ROOT, &cachedRoot, sizeof(PageID));
    file.seekp(0); file.write(p.data, PAGE_SIZE); file.flush();
}

Disk::Disk(const string& filename) {
    file.open(filename, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        file.open(filename, ios::out | ios::binary);
        file.close();
        file.open(filename, ios::in | ios::out | ios::binary);
    }
    loadMeta();
}

Page Disk::read(PageID id) {
    ++readCount; Page p{};
    file.seekg((long long)id * PAGE_SIZE);
    file.read(p.data, PAGE_SIZE); return p;
}

void Disk::write(PageID id, const Page& p) {
    ++writeCount;
    file.seekp((long long)id * PAGE_SIZE);
    file.write(p.data, PAGE_SIZE); file.flush(); saveMeta();
}

PageID Disk::alloc() {
    PageID id = nextPage++; Page p{}; write(id, p); return id;
}

void Disk::saveRoot(PageID root) { cachedRoot = root; saveMeta(); }
PageID Disk::loadRoot() const { return cachedRoot; }
void Disk::resetCounters()     { readCount = writeCount = 0; }
int Disk::totalReads()   const { return readCount;  }
int Disk::totalWrites()  const { return writeCount; }
int Disk::totalAccesses()const { return readCount + writeCount; }
int Disk::pageCount()    const { return nextPage; }