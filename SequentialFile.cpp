//
// Created by ASUS
#include "SequentialFile.h"

// --- Implementación RecordPointer ---
RecordPointer::RecordPointer() : in_aux(false), page_id(-1), record_idx(-1) {}
RecordPointer::RecordPointer(bool aux, long pid, int idx) : in_aux(aux), page_id(pid), record_idx(idx) {}
bool RecordPointer::is_null() const { return page_id == -1; }

// --- Implementación DiskManager ---
DiskManager::DiskManager() {}
DiskManager::DiskManager(const std::string& name) { open(name); }
DiskManager::~DiskManager() { close(); }

void DiskManager::open(const std::string& name) {
    filename = name;
    if (file.is_open()) file.close();
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }
}

void DiskManager::close() {
    if (file.is_open()) file.close();
}

void DiskManager::reset_stats() {
    read_count = 0; write_count = 0;
}

template <typename KeyType>
void DiskManager::read_page(long page_id, Page<KeyType>& page) {
    file.clear();
    file.seekg(page_id * PAGE_SIZE, std::ios::beg);
    file.read(reinterpret_cast<char*>(&page), sizeof(Page<KeyType>));
    read_count++;
}

template <typename KeyType>
void DiskManager::write_page(long page_id, const Page<KeyType>& page) {
    file.clear();
    file.seekp(page_id * PAGE_SIZE, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page<KeyType>));
    write_count++;
}

// --- Implementación SequentialFile ---

template <typename KeyType>
SequentialFile<KeyType>::SequentialFile(const std::string& data_name, const std::string& aux_name, size_t k)
    : data_file(data_name), aux_file(aux_name), aux_record_count(0), total_data_pages(0), total_aux_pages(0), K_LIMIT(k) {}

template <typename KeyType>
void SequentialFile<KeyType>::fetch_page(const RecordPointer& ptr, Page<KeyType>& page) {
    if (ptr.in_aux) aux_file.read_page(ptr.page_id, page);
    else data_file.read_page(ptr.page_id, page);
}

template <typename KeyType>
void SequentialFile<KeyType>::save_page(const RecordPointer& ptr, const Page<KeyType>& page) {
    if (ptr.in_aux) aux_file.write_page(ptr.page_id, page);
    else data_file.write_page(ptr.page_id, page);
}

template <typename KeyType>
RecordPointer SequentialFile<KeyType>::find_predecessor_or_exact(KeyType search_key) {
    if (head_ptr.is_null()) return RecordPointer();

    Page<KeyType> head_page;
    fetch_page(head_ptr, head_page);
    if (search_key < head_page.records[head_ptr.record_idx].key) {
        return RecordPointer();
    }

    long l = 0;
    long u = total_data_pages - 1;
    Page<KeyType> current_page;
    RecordPointer last_seen = head_ptr;

    while (l <= u && total_data_pages > 0) {
        long m = l + (u - l) / 2;
        data_file.read_page(m, current_page);
        if (current_page.record_count == 0) break;

        KeyType first_key = current_page.records[0].key;
        KeyType last_key = current_page.records[current_page.record_count - 1].key;

        if (search_key >= first_key && search_key <= last_key) {
            for (size_t i = 0; i < current_page.record_count; ++i) {
                if (current_page.records[i].key == search_key) return RecordPointer(false, m, i);
                if (current_page.records[i].key < search_key) last_seen = RecordPointer(false, m, i);
                else break;
            }
            break;
        }
        else if (search_key < first_key) {
            u = m - 1;
        }
        else {
            last_seen = RecordPointer(false, m, current_page.record_count - 1);
            l = m + 1;
        }
    }

    RecordPointer current_ptr = last_seen;
    RecordPointer prev_ptr = last_seen;

    while (!current_ptr.is_null()) {
        fetch_page(current_ptr, current_page);
        Record<KeyType>& rec = current_page.records[current_ptr.record_idx];

        if (rec.key == search_key) return current_ptr;
        if (rec.key > search_key) return prev_ptr;

        prev_ptr = current_ptr;
        current_ptr = rec.next_ptr;
    }

    return prev_ptr;
}

template <typename KeyType>
std::pair<Record<KeyType>, int> SequentialFile<KeyType>::search(KeyType search_key) {
    data_file.reset_stats();
    aux_file.reset_stats();

    RecordPointer ptr = find_predecessor_or_exact(search_key);

    if (!ptr.is_null()) {
        Page<KeyType> page;
        fetch_page(ptr, page);
        Record<KeyType>& rec = page.records[ptr.record_idx];

        if (rec.key == search_key && !rec.is_deleted) {
            return {rec, data_file.read_count + aux_file.read_count};
        }
    }
    throw std::runtime_error("Registro no encontrado");
}

template <typename KeyType>
void SequentialFile<KeyType>::add(const Record<KeyType>& new_record) {
    data_file.reset_stats();
    aux_file.reset_stats();

    RecordPointer pred_ptr = find_predecessor_or_exact(new_record.key);

    Page<KeyType> aux_page;
    long target_aux_page = total_aux_pages > 0 ? total_aux_pages - 1 : 0;

    if (total_aux_pages > 0) {
        aux_file.read_page(target_aux_page, aux_page);
    }

    if (aux_page.record_count >= get_blocking_factor<KeyType>()) {
        target_aux_page++;
        aux_page = Page<KeyType>();
        total_aux_pages++;
    } else if (total_aux_pages == 0) {
        total_aux_pages = 1;
    }

    int rec_idx = aux_page.record_count;
    RecordPointer new_rec_ptr(true, target_aux_page, rec_idx);
    Record<KeyType> record_to_insert = new_record;

    if (pred_ptr.is_null()) {
        record_to_insert.next_ptr = head_ptr;
        head_ptr = new_rec_ptr;
    } else {
        if (pred_ptr.in_aux && pred_ptr.page_id == target_aux_page) {
            record_to_insert.next_ptr = aux_page.records[pred_ptr.record_idx].next_ptr;
            aux_page.records[pred_ptr.record_idx].next_ptr = new_rec_ptr;
        } else {
            Page<KeyType> pred_page;
            fetch_page(pred_ptr, pred_page);

            record_to_insert.next_ptr = pred_page.records[pred_ptr.record_idx].next_ptr;
            pred_page.records[pred_ptr.record_idx].next_ptr = new_rec_ptr;

            save_page(pred_ptr, pred_page);
        }
    }

    aux_page.records[rec_idx] = record_to_insert;
    aux_page.record_count++;
    aux_file.write_page(target_aux_page, aux_page);

    aux_record_count++;

    if (aux_record_count >= K_LIMIT) {
        rebuild();
    }
}

template <typename KeyType>
void SequentialFile<KeyType>::remove(KeyType key) {
    RecordPointer ptr = find_predecessor_or_exact(key);
    if (!ptr.is_null()) {
        Page<KeyType> page;
        fetch_page(ptr, page);
        if (page.records[ptr.record_idx].key == key && !page.records[ptr.record_idx].is_deleted) {
            page.records[ptr.record_idx].is_deleted = true;
            save_page(ptr, page);
            return;
        }
    }
    throw std::runtime_error("Registro no existe para eliminar");
}

template <typename KeyType>
std::vector<Record<KeyType>> SequentialFile<KeyType>::rangeSearch(KeyType begin_key, KeyType end_key) {
    std::vector<Record<KeyType>> results;
    RecordPointer current_ptr = find_predecessor_or_exact(begin_key);

    if (!current_ptr.is_null()) {
         Page<KeyType> p;
         fetch_page(current_ptr, p);
         if (p.records[current_ptr.record_idx].key < begin_key) {
             current_ptr = p.records[current_ptr.record_idx].next_ptr;
         }
    } else {
         current_ptr = head_ptr;
    }

    Page<KeyType> current_page;
    while (!current_ptr.is_null()) {
        fetch_page(current_ptr, current_page);
        Record<KeyType>& rec = current_page.records[current_ptr.record_idx];

        if (rec.key > end_key) break;

        if (!rec.is_deleted && rec.key >= begin_key) {
            results.push_back(rec);
        }
        current_ptr = rec.next_ptr;
    }
    return results;
}

template <typename KeyType>
void SequentialFile<KeyType>::rebuild() {
    std::string temp_filename = "temp_datos.dat";
    DiskManager new_data(temp_filename);

    Page<KeyType> current_read_page;
    Page<KeyType> new_page;
    long new_page_id = 0;

    RecordPointer current_read_ptr = head_ptr;

    while (!current_read_ptr.is_null()) {
        if (current_read_ptr.in_aux) {
            aux_file.read_page(current_read_ptr.page_id, current_read_page);
        } else {
            data_file.read_page(current_read_ptr.page_id, current_read_page);
        }

        Record<KeyType>& rec = current_read_page.records[current_read_ptr.record_idx];

        if (!rec.is_deleted) {
            Record<KeyType> new_rec = rec;
            new_rec.next_ptr = RecordPointer(false, new_page_id, new_page.record_count + 1);

            new_page.records[new_page.record_count] = new_rec;
            new_page.record_count++;

            if (new_page.record_count >= get_blocking_factor<KeyType>()) {
                new_page.records[new_page.record_count - 1].next_ptr = RecordPointer(false, new_page_id + 1, 0);
                new_data.write_page(new_page_id, new_page);
                new_page_id++;
                new_page = Page<KeyType>();
            }
        }
        current_read_ptr = rec.next_ptr;
    }

    if (new_page.record_count > 0) {
        new_page.records[new_page.record_count - 1].next_ptr = RecordPointer();
        new_data.write_page(new_page_id, new_page);
        total_data_pages = new_page_id + 1;
    } else {
        if (new_page_id > 0) {
            Page<KeyType> last_page;
            new_data.read_page(new_page_id - 1, last_page);
            last_page.records[get_blocking_factor<KeyType>() - 1].next_ptr = RecordPointer();
            new_data.write_page(new_page_id - 1, last_page);
        }
        total_data_pages = new_page_id;
    }

    new_data.close();
    data_file.close();
    aux_file.close();

    std::remove("datos.dat");
    std::rename(temp_filename.c_str(), "datos.dat");

    std::ofstream ofs("aux.dat", std::ios::trunc | std::ios::binary);
    ofs.close();

    data_file.open("datos.dat");
    aux_file.open("aux.dat");

    aux_record_count = 0;
    total_aux_pages = 0;

    if (total_data_pages > 0) head_ptr = RecordPointer(false, 0, 0);
    else head_ptr = RecordPointer();
}

template <typename KeyType>
std::vector<Record<KeyType>> SequentialFile<KeyType>::scanAll() {
    data_file.reset_stats();
    aux_file.reset_stats();

    std::vector<Record<KeyType>> results;
    RecordPointer current_ptr = head_ptr;
    Page<KeyType> current_page;

    while (!current_ptr.is_null()) {
        fetch_page(current_ptr, current_page);
        Record<KeyType>& rec = current_page.records[current_ptr.record_idx];

        if (!rec.is_deleted) {
            results.push_back(rec);
        }
        current_ptr = rec.next_ptr;
    }

    return results;
}

template class SequentialFile<int>;
template void DiskManager::read_page<int>(long, Page<int>&);
template void DiskManager::write_page<int>(long, const Page<int>&);