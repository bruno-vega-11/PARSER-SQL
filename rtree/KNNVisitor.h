
#pragma once

#include <vector>
#include <spatialindex/SpatialIndex.h>

class KNNVisitor : public SpatialIndex::IVisitor {
private:
    std::vector<SpatialIndex::id_type> resultados;

public:
    void visitNode(const SpatialIndex::INode& nodo) override {
    }

    void visitData(const SpatialIndex::IData& dato) override {
        resultados.push_back(dato.getIdentifier());
    }

    void visitData(std::vector<const SpatialIndex::IData*>& datos) override {
        for (const SpatialIndex::IData* dato : datos) {
            resultados.push_back(dato->getIdentifier());
        }
    }

    const std::vector<SpatialIndex::id_type>& getResultados() const {
        return resultados;
    }
};