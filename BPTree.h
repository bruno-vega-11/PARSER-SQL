#pragma once
#include <bits/stdc++.h>
using namespace std;

static constexpr int PAGE_SIZE = 4096;

struct RID { int page_id; int slot; };
static constexpr RID NULL_RID = {-1, -1};
inline bool isNullRID(const RID& r) { return r.page_id == -1 && r.slot == -1; }

using PageID = int;
static constexpr PageID NULL_PAGE = -1;

template<int N>
struct FixedString {
    char data[N];

    FixedString() {
        memset(data, 0, N);
    }

    FixedString(const char* s) {
        memset(data, 0, N);
        memcpy(data, s, min((int)strlen(s), N));
    }

    FixedString(const std::string& s) {
        memset(data, 0, N);
        memcpy(data, s.c_str(), min((int)s.size(), N));
    }

    bool operator<(const FixedString& other) const {
        return strncmp(data, other.data, N) < 0;
    }

    bool operator>(const FixedString& other) const {
        return strncmp(data, other.data, N) > 0;
    }

    bool operator==(const FixedString& other) const {
        return strncmp(data, other.data, N) == 0;
    }

    bool operator<=(const FixedString& other) const {
        return !(*this > other);
    }

    bool operator>=(const FixedString& other) const {
        return !(*this < other);
    }
};

template<int N>
ostream& operator<<(ostream& os, const FixedString<N>& fs) {
    return os.write(fs.data, strnlen(fs.data, N));
}

template<typename TKey>
static constexpr int computeMaxKeys() {
    int overhead  = static_cast<int>(sizeof(bool) + 3*sizeof(int) + sizeof(PageID));
    int leafEntry = static_cast<int>(sizeof(TKey) + sizeof(RID));
    return (PAGE_SIZE - overhead) / leafEntry - 1;
}

template<typename TKey>
struct Node {
    bool   isLeaf;
    int    numKeys;
    PageID parent;
    PageID nextLeaf;
    static constexpr int MAX_KEYS = computeMaxKeys<TKey>();
    TKey   keys[MAX_KEYS + 1];
    union {
        PageID children[MAX_KEYS + 2];
        RID    values[MAX_KEYS + 1];
    };
};

struct Page { char data[PAGE_SIZE]; };

class Disk {
private:
    fstream file;
    int     nextPage;
    static constexpr int META_OFFSET_NEXT = 0;
    static constexpr int META_OFFSET_ROOT = sizeof(int);
    PageID cachedRoot = NULL_PAGE;

    void loadMeta();
    void saveMeta();

    mutable int readCount = 0, writeCount = 0;

public:
    explicit Disk(const string& filename);
    Page read(PageID id);
    void write(PageID id, const Page& p);
    PageID alloc();
    void saveRoot(PageID root);
    PageID loadRoot() const;
    void resetCounters();
    int totalReads()   const;
    int totalWrites()  const;
    int totalAccesses()const;
    int pageCount()    const;
};

template<typename TKey>
class BPlusTree {
private:
    using NodeT = Node<TKey>;
    static constexpr int MAX_KEYS = NodeT::MAX_KEYS;
    static constexpr int MIN_KEYS = MAX_KEYS / 2;

    Disk&  disk;
    PageID root;

    NodeT* asNode(Page& p)             { return reinterpret_cast<NodeT*>(p.data); }
    const NodeT* asNode(const Page& p) const { return reinterpret_cast<const NodeT*>(p.data); }

    pair<PageID,Page> newNode(bool isLeaf, PageID parent) {
        PageID id = disk.alloc();
        Page   p  = disk.read(id);
        NodeT* n  = asNode(p);
        n->isLeaf = isLeaf; n->numKeys = 0;
        n->parent = parent; n->nextLeaf = NULL_PAGE;
        return {id, p};
    }

    PageID findLeaf(const TKey& k) const {
        PageID curr = root;
        while (true) {
            Page p = disk.read(curr); const NodeT* n = asNode(p);
            if (n->isLeaf) return curr;
            int i = 0;
            while (i < n->numKeys && k >= n->keys[i]) i++; // cambie
            curr = n->children[i];
        }
    }

    void insertIntoLeaf(PageID leafId, const TKey& k, const RID& rid) {
        Page p = disk.read(leafId); NodeT* n = asNode(p);
        int i = n->numKeys - 1;
        while (i >= 0 && n->keys[i] > k) {
            n->keys[i+1] = n->keys[i]; n->values[i+1] = n->values[i]; i--;
        }
        n->keys[i+1] = k; n->values[i+1] = rid; n->numKeys++;
        disk.write(leafId, p);
        if (n->numKeys > MAX_KEYS) splitLeaf(leafId);
    }

    void splitLeaf(PageID leafId) {
        Page lp = disk.read(leafId); NodeT* ln = asNode(lp);
        auto [newId, np] = newNode(true, ln->parent); NodeT* nn = asNode(np);
        int mid = ln->numKeys / 2;
        nn->numKeys = ln->numKeys - mid;
        for (int i = 0; i < nn->numKeys; i++) {
            nn->keys[i] = ln->keys[mid+i]; nn->values[i] = ln->values[mid+i];
        }
        nn->nextLeaf = ln->nextLeaf; ln->nextLeaf = newId; ln->numKeys = mid;
        disk.write(leafId, lp); disk.write(newId, np);
        insertInParent(leafId, nn->keys[0], newId);
    }

    void insertInParent(PageID left, const TKey& key, PageID right) {
        if (left == root) {
            auto [nr, rp] = newNode(false, NULL_PAGE); NodeT* r = asNode(rp);
            r->numKeys = 1; r->keys[0] = key;
            r->children[0] = left; r->children[1] = right;
            disk.write(nr, rp); setParent(left, nr); setParent(right, nr);
            root = nr; disk.saveRoot(root); return;
        }
        Page lp = disk.read(left); PageID pid = asNode(lp)->parent;
        Page pp = disk.read(pid); NodeT* par = asNode(pp);
        int i = par->numKeys - 1;
        while (i >= 0 && par->keys[i] > key) {
            par->keys[i+1] = par->keys[i]; par->children[i+2] = par->children[i+1]; i--;
        }
        par->keys[i+1] = key; par->children[i+2] = right; par->numKeys++;
        disk.write(pid, pp); setParent(right, pid);
        if (par->numKeys > MAX_KEYS) splitInternal(pid);
    }

    void splitInternal(PageID id) {
        Page lp = disk.read(id); NodeT* ln = asNode(lp);
        auto [newId, np] = newNode(false, ln->parent); NodeT* nn = asNode(np);
        int mid = ln->numKeys / 2; TKey upKey = ln->keys[mid];
        nn->numKeys = ln->numKeys - mid - 1;
        for (int i = 0; i < nn->numKeys; i++) {
            nn->keys[i] = ln->keys[mid+1+i]; nn->children[i] = ln->children[mid+1+i];
        }
        nn->children[nn->numKeys] = ln->children[ln->numKeys];
        ln->numKeys = mid;
        disk.write(id, lp); disk.write(newId, np);
        for (int i = 0; i <= nn->numKeys; i++) setParent(nn->children[i], newId);
        insertInParent(id, upKey, newId);
    }

    void setParent(PageID child, PageID parent) {
        Page p = disk.read(child); asNode(p)->parent = parent; disk.write(child, p);
    }

    // ── delete helpers ────────────────────────────────────────────

    struct SiblingInfo { PageID siblingId; int separatorIdx; bool isLeft; };

    SiblingInfo findSibling(PageID childId, const NodeT* par) {
        int pos = 0;
        while (pos <= par->numKeys && par->children[pos] != childId) ++pos;
        if (pos > 0) return { par->children[pos-1], pos-1, true  };
        else         return { par->children[pos+1], pos,   false };
    }

    void redistributeLeaves(PageID nodeId, PageID sibId, bool sibIsLeft,
                            int sepIdx, PageID parentId)
    {
        Page np = disk.read(nodeId); NodeT* n   = asNode(np);
        Page sp = disk.read(sibId);  NodeT* sib = asNode(sp);
        Page pp = disk.read(parentId); NodeT* par = asNode(pp);
        if (sibIsLeft) {
            for (int i = n->numKeys; i > 0; --i) {
                n->keys[i] = n->keys[i-1]; n->values[i] = n->values[i-1];
            }
            n->keys[0]   = sib->keys[sib->numKeys-1];
            n->values[0] = sib->values[sib->numKeys-1];
            n->numKeys++; sib->numKeys--;
            par->keys[sepIdx] = n->keys[0];
        } else {
            n->keys[n->numKeys]   = sib->keys[0];
            n->values[n->numKeys] = sib->values[0];
            n->numKeys++;
            for (int i = 0; i+1 < sib->numKeys; ++i) {
                sib->keys[i] = sib->keys[i+1]; sib->values[i] = sib->values[i+1];
            }
            sib->numKeys--;
            par->keys[sepIdx] = sib->keys[0];
        }
        disk.write(nodeId, np); disk.write(sibId, sp); disk.write(parentId, pp);
    }

    void mergeLeaves(PageID nodeId, PageID sibId, bool sibIsLeft,
                     int sepIdx, PageID parentId)
    {
        PageID leftId  = sibIsLeft ? sibId  : nodeId;
        PageID rightId = sibIsLeft ? nodeId : sibId;
        Page lp = disk.read(leftId);  NodeT* left  = asNode(lp);
        Page rp = disk.read(rightId); NodeT* right = asNode(rp);
        Page pp = disk.read(parentId); NodeT* par  = asNode(pp);
        for (int i = 0; i < right->numKeys; ++i) {
            left->keys  [left->numKeys+i] = right->keys[i];
            left->values[left->numKeys+i] = right->values[i];
        }
        left->numKeys += right->numKeys;
        left->nextLeaf = right->nextLeaf;
        for (int i = sepIdx; i < par->numKeys-1; ++i) {
            par->keys[i]       = par->keys[i+1];
            par->children[i+1] = par->children[i+2];
        }
        par->numKeys--;
        disk.write(leftId, lp); disk.write(parentId, pp);
    }

    void redistributeInternals(PageID nodeId, PageID sibId, bool sibIsLeft,
                               int sepIdx, PageID parentId)
    {
        Page np = disk.read(nodeId); NodeT* n   = asNode(np);
        Page sp = disk.read(sibId);  NodeT* sib = asNode(sp);
        Page pp = disk.read(parentId); NodeT* par = asNode(pp);
        if (sibIsLeft) {
            for (int i = n->numKeys; i > 0; --i) {
                n->keys[i] = n->keys[i-1]; n->children[i+1] = n->children[i];
            }
            n->children[1] = n->children[0];
            n->keys[0] = par->keys[sepIdx]; n->numKeys++;
            n->children[0] = sib->children[sib->numKeys];
            setParent(n->children[0], nodeId);
            par->keys[sepIdx] = sib->keys[sib->numKeys-1];
            sib->numKeys--;
        } else {
            n->keys[n->numKeys]       = par->keys[sepIdx];
            n->children[n->numKeys+1] = sib->children[0];
            setParent(sib->children[0], nodeId); n->numKeys++;
            par->keys[sepIdx] = sib->keys[0];
            for (int i = 0; i+1 < sib->numKeys; ++i) {
                sib->keys[i] = sib->keys[i+1]; sib->children[i] = sib->children[i+1];
            }
            sib->children[sib->numKeys-1] = sib->children[sib->numKeys];
            sib->numKeys--;
        }
        disk.write(nodeId, np); disk.write(sibId, sp); disk.write(parentId, pp);
    }

    void mergeInternals(PageID nodeId, PageID sibId, bool sibIsLeft,
                        int sepIdx, PageID parentId)
    {
        PageID leftId  = sibIsLeft ? sibId  : nodeId;
        PageID rightId = sibIsLeft ? nodeId : sibId;
        Page lp = disk.read(leftId);  NodeT* left  = asNode(lp);
        Page rp = disk.read(rightId); NodeT* right = asNode(rp);
        Page pp = disk.read(parentId); NodeT* par  = asNode(pp);
        left->keys[left->numKeys] = par->keys[sepIdx];
        left->numKeys++;
        for (int i = 0; i < right->numKeys; ++i) {
            left->keys    [left->numKeys+i] = right->keys[i];
            left->children[left->numKeys+i] = right->children[i];
            setParent(right->children[i], leftId);
        }
        left->children[left->numKeys+right->numKeys] = right->children[right->numKeys];
        setParent(right->children[right->numKeys], leftId);
        left->numKeys += right->numKeys;
        for (int i = sepIdx; i < par->numKeys-1; ++i) {
            par->keys[i]       = par->keys[i+1];
            par->children[i+1] = par->children[i+2];
        }
        par->numKeys--;
        disk.write(leftId, lp); disk.write(parentId, pp);
    }

    void fixAfterDelete(PageID nodeId) {
        Page np = disk.read(nodeId); NodeT* n = asNode(np);
        if (nodeId == root) {
            if (n->numKeys == 0 && !n->isLeaf) {
                PageID newRoot = n->children[0];
                root = newRoot; disk.saveRoot(root);
                Page rp = disk.read(newRoot);
                asNode(rp)->parent = NULL_PAGE;
                disk.write(newRoot, rp);
            }
            return;
        }
        if (n->numKeys >= MIN_KEYS) return;
        PageID parentId = n->parent;
        Page pp = disk.read(parentId); NodeT* par = asNode(pp);
        SiblingInfo si = findSibling(nodeId, par);
        Page sp = disk.read(si.siblingId); NodeT* sib = asNode(sp);
        if (sib->numKeys > MIN_KEYS) {
            if (n->isLeaf) redistributeLeaves   (nodeId, si.siblingId, si.isLeft, si.separatorIdx, parentId);
            else           redistributeInternals(nodeId, si.siblingId, si.isLeft, si.separatorIdx, parentId);
        } else {
            if (n->isLeaf) mergeLeaves   (nodeId, si.siblingId, si.isLeft, si.separatorIdx, parentId);
            else           mergeInternals(nodeId, si.siblingId, si.isLeft, si.separatorIdx, parentId);
            fixAfterDelete(parentId);
        }
    }

    bool deleteFirstOccurrence(const TKey& key) {
        // Bajar desde la raíz — búsqueda fresca cada vez
        PageID leafId = findLeaf(key);

        // Buscar la primera entrada con key==key en esta hoja
        // (y en hojas siguientes si la primera no la tiene)
        PageID curr = leafId;
        while (curr != NULL_PAGE) {
            Page   p = disk.read(curr);
            NodeT* n = asNode(p);

            for (int i = 0; i < n->numKeys; i++) {
                if (n->keys[i] == key) {
                    // Encontrada: borrar esta sola posición
                    for (int j = i; j+1 < n->numKeys; j++) {
                        n->keys[j]   = n->keys[j+1];
                        n->values[j] = n->values[j+1];
                    }
                    n->numKeys--;
                    disk.write(curr, p);
                    // Ahora el árbol tiene un nodo modificado,
                    // corregir underflow/balance desde curr hacia arriba
                    fixAfterDelete(curr);
                    return true;   // borrado exitoso
                }
                if (key < n->keys[i]) return false;  // ya pasamos, no hay más
            }
            Page tmp = disk.read(curr);
            curr = asNode(tmp)->nextLeaf;
        }
        return false;  // no encontrado
    }

public:
    explicit BPlusTree(Disk& d) : disk(d) {
        PageID persisted = disk.loadRoot();
        if (persisted != NULL_PAGE) { root = persisted; return; }
        auto [id, p] = newNode(true, NULL_PAGE);
        disk.write(id, p); root = id; disk.saveRoot(root);
    }

    PageID getRoot() const { return root; }

    // ================= SEARCH =================
    RID search(const TKey& key) const {
        PageID curr = root;
        while (true) {
            Page p = disk.read(curr); const NodeT* n = asNode(p);
            if (n->isLeaf) {
                int lo = 0, hi = n->numKeys-1;
                while (lo <= hi) {
                    int mid = (lo+hi)/2;
                    if      (n->keys[mid] == key) return n->values[mid];
                    else if (n->keys[mid] <  key) lo = mid+1;
                    else                          hi = mid-1;
                }
                return NULL_RID;
            }
            int lo = 0, hi = n->numKeys-1, idx = n->numKeys;
            while (lo <= hi) {
                int mid = (lo+hi)/2;
                if (key < n->keys[mid]) { idx = mid; hi = mid-1; }
                else                    lo = mid+1;
            }
            curr = n->children[idx];
        }
    }

    // ================= SEARCH ALL =================
    vector<RID> searchAll(const TKey& key) const { // mas a la izquierda de la llave q queremos
        vector<RID> res; 

        // bajar desde la raíz (tipo lower_bound)
        PageID curr = root;

        while (true) {
            Page p = disk.read(curr);
            const NodeT* n = asNode(p);

            if (n->isLeaf) break;

            int i = 0;
            while (i < n->numKeys && n->keys[i] < key) i++;

            curr = n->children[i];
        }

        // recorrer hojas hacia la derecha
        while (curr != NULL_PAGE) {
            Page p = disk.read(curr);
            const NodeT* n = asNode(p);

            bool past = false;

            for (int i = 0; i < n->numKeys; i++) {
                if (n->keys[i] == key) {
                    res.push_back(n->values[i]);
                } else if (key < n->keys[i]) {
                    past = true;
                    break;
                }
            }

            if (past) break;

            curr = n->nextLeaf;
        }

        return res;
    }

    // ================= RANGE SEARCH =================
    vector<RID> rangeSearch(const TKey& a, const TKey& b) const { // antes podria ir medio con duplicados, qremos mas izq
        if (b < a) return {};

        vector<RID> res;

        // (lower_bound manual)
        PageID curr = root;

        while (true) {
            Page p = disk.read(curr);
            const NodeT* n = asNode(p);

            if (n->isLeaf) break;

            int i = 0;
            while (i < n->numKeys && n->keys[i] < a) i++;

            curr = n->children[i];
        }

        // recorrer hojas hacia la derecha
        while (curr != NULL_PAGE) {
            Page p = disk.read(curr);
            const NodeT* n = asNode(p);

            bool stop = false;

            for (int i = 0; i < n->numKeys; i++) {
                if (n->keys[i] > b) {
                    stop = true;
                    break;
                }
                if (!(n->keys[i] < a)) { // >= a
                    res.push_back(n->values[i]);
                }
            }

            if (stop) break;
            curr = n->nextLeaf;
        }

        return res;
    }

    // ================= INSERT =================
    void insert(const TKey& key, const RID& rid) {
        insertIntoLeaf(findLeaf(key), key, rid);
    }

    void remove(const TKey& key) {
        while (deleteFirstOccurrence(key)) {}
    }

    void removeByRID(const TKey& key, const RID& target) {
        // Búsqueda fresca desde la raíz
        PageID curr = findLeaf(key);
        while (curr != NULL_PAGE) {
            Page   p = disk.read(curr);
            NodeT* n = asNode(p);
            for (int i = 0; i < n->numKeys; i++) {
                if (n->keys[i] == key &&
                    n->values[i].page_id == target.page_id &&
                    n->values[i].slot    == target.slot)
                {
                    // Borrar esta entrada exacta
                    for (int j = i; j+1 < n->numKeys; j++) {
                        n->keys[j]   = n->keys[j+1];
                        n->values[j] = n->values[j+1];
                    }
                    n->numKeys--;
                    disk.write(curr, p);
                    fixAfterDelete(curr);
                    return;
                }
                if (key < n->keys[i]) return;  // pasamos la clave
            }
            Page tmp = disk.read(curr);
            curr = asNode(tmp)->nextLeaf;
        }
    }

    // ================= DEBUG =================
    void printLeaves() const {
        PageID curr = root;
        while (true) {
            Page        p = disk.read(curr);
            const NodeT* n = asNode(p);
            if (n->isLeaf) break;
            curr = n->children[0];
        }

        cout << "[Leaves] ";
        while (curr != NULL_PAGE) {
            Page        p = disk.read(curr);
            const NodeT* n = asNode(p);
            cout << "| ";
            for (int i = 0; i < n->numKeys; i++)
                cout << n->keys[i] << " ";
            curr = n->nextLeaf;
        }
        cout << "|\n";
    }
};