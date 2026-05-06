
#pragma once
#include <spatialindex/SpatialIndex.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdint>

class MiPagedDiskStorageManager : public SpatialIndex::IStorageManager {
private:
    static constexpr uint32_t PAGE_SIZE = 4096;
    static constexpr uint32_t NEXT_SIZE = sizeof(SpatialIndex::id_type);
    static constexpr uint32_t USED_SIZE = sizeof(uint32_t);
    static constexpr uint32_t HEADER_SIZE = NEXT_SIZE + USED_SIZE;
    static constexpr uint32_t PAYLOAD_SIZE = PAGE_SIZE - HEADER_SIZE;

    static constexpr SpatialIndex::id_type NO_PAGE =
        SpatialIndex::StorageManager::EmptyPage;

    std::string nombreArchivo;
    std::fstream archivo;

    SpatialIndex::id_type siguientePagina;
    uint64_t contadorLecturas;
    uint64_t contadorEscrituras;

private:
    std::streampos offsetPagina(SpatialIndex::id_type id) const {
        return static_cast<std::streampos>(id * PAGE_SIZE);
    }

    void abrirArchivo() {
        archivo.open(nombreArchivo, std::ios::in | std::ios::out | std::ios::binary);

        if (!archivo.is_open()) {
            std::ofstream crear(nombreArchivo, std::ios::binary);
            crear.close();

            archivo.open(nombreArchivo, std::ios::in | std::ios::out | std::ios::binary);
        }

        if (!archivo.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo de paginas.");
        }

        archivo.seekg(0, std::ios::end);
        std::streamoff tamanio = archivo.tellg();

        if (tamanio < 0) {
            siguientePagina = 0;
        } else {
            siguientePagina = static_cast<SpatialIndex::id_type>(
                tamanio / PAGE_SIZE
            );
        }

        archivo.clear();
    }

    void validarPagina(SpatialIndex::id_type id) const {
        if (id < 0 || id >= siguientePagina) {
            throw SpatialIndex::InvalidPageException(id);
        }
    }

    SpatialIndex::id_type reservarPaginaNueva() {
        SpatialIndex::id_type id = siguientePagina;
        siguientePagina++;
        return id;
    }

    std::vector<uint8_t> leerPaginaCompleta(SpatialIndex::id_type id) {
        validarPagina(id);

        std::vector<uint8_t> pagina(PAGE_SIZE, 0);

        archivo.seekg(offsetPagina(id), std::ios::beg);
        archivo.read(reinterpret_cast<char*>(pagina.data()), PAGE_SIZE);

        if (!archivo) {
            archivo.clear();
            throw SpatialIndex::InvalidPageException(id);
        }

        contadorLecturas++;
        return pagina;
    }

    void escribirPaginaCompleta(
        SpatialIndex::id_type id,
        const std::vector<uint8_t>& pagina
    ) {
        if (pagina.size() != PAGE_SIZE) {
            throw std::runtime_error("La pagina debe tener exactamente 4096 bytes.");
        }

        archivo.seekp(offsetPagina(id), std::ios::beg);
        archivo.write(reinterpret_cast<const char*>(pagina.data()), PAGE_SIZE);

        if (!archivo) {
            archivo.clear();
            throw std::runtime_error("Error escribiendo pagina en disco.");
        }

        contadorEscrituras++;
    }

    SpatialIndex::id_type leerNextPage(const std::vector<uint8_t>& pagina) const {
        SpatialIndex::id_type next = NO_PAGE;
        std::memcpy(&next, pagina.data(), NEXT_SIZE);
        return next;
    }

    uint32_t leerUsedBytes(const std::vector<uint8_t>& pagina) const {
        uint32_t used = 0;
        std::memcpy(&used, pagina.data() + NEXT_SIZE, USED_SIZE);
        return used;
    }

    void escribirHeader(
        std::vector<uint8_t>& pagina,
        SpatialIndex::id_type next,
        uint32_t used
    ) {
        std::memcpy(pagina.data(), &next, NEXT_SIZE);
        std::memcpy(pagina.data() + NEXT_SIZE, &used, USED_SIZE);
    }

    void limpiarCadena(SpatialIndex::id_type primeraPagina) {
        if (primeraPagina == NO_PAGE) {
            return;
        }

        validarPagina(primeraPagina);

        SpatialIndex::id_type actual = primeraPagina;

        while (actual != NO_PAGE) {
            std::vector<uint8_t> pagina = leerPaginaCompleta(actual);
            SpatialIndex::id_type siguiente = leerNextPage(pagina);

            std::vector<uint8_t> paginaVacia(PAGE_SIZE, 0);
            escribirPaginaCompleta(actual, paginaVacia);

            actual = siguiente;
        }
    }

public:

    void dumpPages(uint32_t bytesPayloadAMostrar = 64) {
    std::cout << "\n================ DUMP DE PAGINAS ================\n";
    std::cout << "Archivo: " << nombreArchivo << std::endl;
    std::cout << "PAGE_SIZE: " << PAGE_SIZE << " bytes" << std::endl;
    std::cout << "HEADER_SIZE: " << HEADER_SIZE << " bytes" << std::endl;
    std::cout << "PAYLOAD_SIZE: " << PAYLOAD_SIZE << " bytes" << std::endl;
    std::cout << "Total paginas reservadas: " << siguientePagina << std::endl;

    for (SpatialIndex::id_type id = 0; id < siguientePagina; ++id) {
        std::vector<uint8_t> pagina(PAGE_SIZE, 0);

        archivo.seekg(offsetPagina(id), std::ios::beg);
        archivo.read(reinterpret_cast<char*>(pagina.data()), PAGE_SIZE);

        if (!archivo) {
            archivo.clear();
            std::cout << "\n[PAGE " << id << "] Error leyendo pagina." << std::endl;
            continue;
        }

        SpatialIndex::id_type next = leerNextPage(pagina);
        uint32_t used = leerUsedBytes(pagina);

        std::cout << "\n---------------- PAGE " << id << " ----------------\n";
        std::cout << "nextPage  : " << next << std::endl;
        std::cout << "usedBytes : " << used << std::endl;

        if (used == 0) {
            std::cout << "estado    : vacia o eliminada" << std::endl;
            continue;
        }

        if (used > PAYLOAD_SIZE) {
            std::cout << "estado    : corrupta, usedBytes excede PAYLOAD_SIZE" << std::endl;
            continue;
        }

        uint32_t limite = std::min(bytesPayloadAMostrar, used);

        std::cout << "payload hex primeros " << limite << " bytes:\n";

        for (uint32_t i = 0; i < limite; ++i) {
            if (i % 16 == 0) {
                std::cout << "\n";
            }

            uint8_t byte = pagina[HEADER_SIZE + i];

            std::cout << std::hex
                      << std::setw(2)
                      << std::setfill('0')
                      << static_cast<int>(byte)
                      << " ";
        }

        std::cout << std::dec << std::setfill(' ') << "\n";
    }

    std::cout << "\n============== FIN DUMP DE PAGINAS ==============\n";
}

    explicit MiPagedDiskStorageManager(const std::string& rutaArchivo)
        : nombreArchivo(rutaArchivo),
          siguientePagina(0),
          contadorLecturas(0),
          contadorEscrituras(0) {
        abrirArchivo();
    }

    ~MiPagedDiskStorageManager() override {
        flush();

        if (archivo.is_open()) {
            archivo.close();
        }
    }

    void resetCounters() {
        contadorLecturas = 0;
        contadorEscrituras = 0;
    }

    uint64_t getReadCount() const {
        return contadorLecturas;
    }

    uint64_t getWriteCount() const {
        return contadorEscrituras;
    }

    void loadByteArray(
        const SpatialIndex::id_type id,
        uint32_t& len,
        uint8_t** data
    ) override {
        validarPagina(id);

        std::vector<uint8_t> bytes;
        SpatialIndex::id_type actual = id;

        while (actual != NO_PAGE) {
            std::vector<uint8_t> pagina = leerPaginaCompleta(actual);

            SpatialIndex::id_type siguiente = leerNextPage(pagina);
            uint32_t used = leerUsedBytes(pagina);

            if (used > PAYLOAD_SIZE) {
                throw std::runtime_error("Pagina corrupta: usedBytes excede PAYLOAD_SIZE.");
            }

            bytes.insert(
                bytes.end(),
                pagina.data() + HEADER_SIZE,
                pagina.data() + HEADER_SIZE + used
            );

            actual = siguiente;
        }

        len = static_cast<uint32_t>(bytes.size());

        if (len == 0) {
            *data = nullptr;
            return;
        }

        *data = new uint8_t[len];
        std::memcpy(*data, bytes.data(), len);
    }

    void storeByteArray(
        SpatialIndex::id_type& id,
        const uint32_t len,
        const uint8_t* const data
    ) override {
        bool esNuevaPagina = false;

        if (id == SpatialIndex::StorageManager::NewPage) {
            id = reservarPaginaNueva();
            esNuevaPagina = true;
        } else {
            validarPagina(id);
        }

        if (!esNuevaPagina) {
            limpiarCadena(id);
        }

        uint32_t bytesRestantes = len;
        uint32_t posicion = 0;

        SpatialIndex::id_type paginaActual = id;

        while (true) {
            uint32_t bytesEnEstaPagina =
                bytesRestantes > PAYLOAD_SIZE ? PAYLOAD_SIZE : bytesRestantes;

            bytesRestantes -= bytesEnEstaPagina;

            SpatialIndex::id_type siguiente =
                bytesRestantes > 0 ? reservarPaginaNueva() : NO_PAGE;

            std::vector<uint8_t> pagina(PAGE_SIZE, 0);

            escribirHeader(pagina, siguiente, bytesEnEstaPagina);

            if (bytesEnEstaPagina > 0) {
                std::memcpy(
                    pagina.data() + HEADER_SIZE,
                    data + posicion,
                    bytesEnEstaPagina
                );
            }

            escribirPaginaCompleta(paginaActual, pagina);

            posicion += bytesEnEstaPagina;

            if (siguiente == NO_PAGE) {
                break;
            }

            paginaActual = siguiente;
        }
    }

    void deleteByteArray(const SpatialIndex::id_type id) override {
        validarPagina(id);
        limpiarCadena(id);
    }

    void flush() override {
        if (archivo.is_open()) {
            archivo.flush();
        }
    }
};