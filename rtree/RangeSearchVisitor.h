

#pragma once

#include <vector>
#include <cmath>
#include <spatialindex/SpatialIndex.h>

class RangeSearchVisitor : public SpatialIndex::IVisitor {
private:
    SpatialIndex::Point centro;
    double radio;
    double radioCuadrado;
    std::vector<SpatialIndex::id_type> resultados;

public:
    RangeSearchVisitor(const SpatialIndex::Point& centroBusqueda, double radioBusqueda)
        : centro(centroBusqueda),
          radio(radioBusqueda),
          radioCuadrado(radioBusqueda * radioBusqueda) {}

    void visitNode(const SpatialIndex::INode& nodo) override {
        // No necesitamos hacer nada con los nodos.
        // La librería ya se encarga de recorrer solo los MBR candidatos.
    }

    void visitData(const SpatialIndex::IData& dato) override {
        SpatialIndex::IShape* forma = nullptr;
        dato.getShape(&forma);

        SpatialIndex::Point puntoDato;
        forma->getCenter(puntoDato);

        double distanciaCuadrada = 0.0;

        for (uint32_t i = 0; i < centro.getDimension(); ++i) {
            double diferencia = centro.getCoordinate(i) - puntoDato.getCoordinate(i);
            distanciaCuadrada += diferencia * diferencia;
        }

        if (distanciaCuadrada <= radioCuadrado) {
            resultados.push_back(dato.getIdentifier());
        }

        delete forma;
    }

    void visitData(std::vector<const SpatialIndex::IData*>& datos) override {
        for (const SpatialIndex::IData* dato : datos) {
            visitData(*dato);
        }
    }

    const std::vector<SpatialIndex::id_type>& getResultados() const {
        return resultados;
    }
};