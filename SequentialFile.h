//
// Created by ASUS on 29/04/2026.
//
#ifndef SEQUENTIAL_FILE_H
#define SEQUENTIAL_FILE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdio>

constexpr size_t PAGE_SIZE = 4096;

// 1. Puntero Lógico
struct RecordPointer {
    bool in_aux;
    long page_id;
    int record_idx;

    RecordPointer();
    RecordPointer(bool aux, long pid, int idx);
    bool is_null() const;
};

// 2. Estructura de Registro
template <typename KeyType>
struct Record {
    KeyType key;
    char data[64];
    RecordPointer next_ptr;
    bool is_deleted;

    Record() : is_deleted(false) {
        std::memset(data, 0, sizeof(data));
    }
};

// Cálculo de factor de bloqueo
template <typename KeyType>
constexpr size_t get_blocking_factor() {
    return (PAGE_SIZE - sizeof(size_t)) / sizeof(Record<KeyType>);
}

// 3. Estructura de Página
template <typename KeyType>
struct Page {
    size_t record_count = 0;
    Record<KeyType> records[get_blocking_factor<KeyType>()];
};

// 4. Gestor de Disco
class DiskManager {
private:
    std::fstream file;
    std::string filename;
public:
    int read_count = 0;
    int write_count = 0;

    DiskManager();
    DiskManager(const std::string& name);
    ~DiskManager();

    void open(const std::string& name);
    void close();
    void reset_stats();

    template <typename KeyType>
    void read_page(long page_id, Page<KeyType>& page);

    template <typename KeyType>
    void write_page(long page_id, const Page<KeyType>& page);
};

// 5. Motor Sequential File
template <typename KeyType>
class SequentialFile {
private:
    DiskManager data_file;
    DiskManager aux_file;

    size_t aux_record_count;
    size_t K_LIMIT;
    long total_data_pages;
    long total_aux_pages;
    RecordPointer head_ptr;

    void fetch_page(const RecordPointer& ptr, Page<KeyType>& page);
    void save_page(const RecordPointer& ptr, const Page<KeyType>& page);
    RecordPointer find_predecessor_or_exact(KeyType search_key);

public:
    SequentialFile(const std::string& data_name, const std::string& aux_name, size_t k = 50);
    std::pair<Record<KeyType>, int> search(KeyType search_key);
    void add(const Record<KeyType>& new_record);
    void remove(KeyType key);
    std::vector<Record<KeyType>> rangeSearch(KeyType begin_key, KeyType end_key);
    void rebuild();
    std::vector<Record<KeyType>> scanAll();
};

#endif // SEQUENTIAL_FILE_H
