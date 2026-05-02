#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <sstream>
#include "ast.h"
#include "visitor.h"


using namespace std;
// unordered_map<std::string, int> memoria;
///////////////////////////////////////////////////////////////////////////////////
// int BinaryExp::accept(Visitor* visitor) {
//     return visitor->visit(this);
// }

// int NumberExp::accept(Visitor* visitor) {
//     return visitor->visit(this);
// }

// int SqrtExp::accept(Visitor* visitor) {
//     return visitor->visit(this);
// }

void SelectStmt::accept(Visitor* visitor) {
    return visitor->visit(this);
}
void InsertStmt::accept(Visitor* visitor) {
    return visitor->visit(this);
}
void CreateIndexStmt::accept(Visitor* visitor) {
    return visitor->visit(this);
}
void DeleteStmt::accept(Visitor* visitor) {
    return visitor->visit(this);
}
void CreateTableStmt::accept(Visitor* visitor) {
    return visitor->visit(this);
}

///////////////////////////////////////////////////////////////////////////////////
void EVALVisitor::visit(SelectStmt* s) {
    SequentialFile<int> sf("archivos/"+s->table + ".dat", "archivos/"+s->table + "_aux.dat", 4);

    if (s->where_cond == nullptr) {
        auto records = sf.scanAll();
        cout << "Scan exitoso";
        for (auto& r : records) {
            std::cout << r.key << " ... \n";
        }
    }
    
}
void EVALVisitor::visit(CreateTableStmt* s) {

    std::ifstream csv(s->path);
    if (!csv.is_open()) {
        std::cerr << "No se pudo abrir: " << s->path << "\n";
        return;
    }


    std::string header_line;
    std::getline(csv, header_line);

    std::vector<std::string> csv_headers;
    std::stringstream ssh(header_line);
    std::string h;
    while (std::getline(ssh, h, ',')) {

        h.erase(0, h.find_first_not_of(" \t\r\n"));
        h.erase(h.find_last_not_of(" \t\r\n") + 1);
        csv_headers.push_back(h);
    }
    
    //if (csv_headers.size() != s->values.size()) {         //Esto es si, si o si necestiamos k esten todas las columnas, tambien podrias ingresar con valor default
    //    std::cerr << "Se pasaron ms columnas que las definidas"<< "\n";
    //    return;
    //    
    //}
    
    
    //for (size_t i = 0; i < csv_headers.size(); i++) {
    //    if (csv_headers[i] != s->values[i].first) {   //Si importa el nombre, podriamos cambiarlo con que solo importe el tipo nc
    //       std::cerr << Se esperaba otro nombre de columna << "'\n";
    //       return;
    //    }
    //}


    //std::ofstream schema("archivos/" + s->tabla + ".schema");
    //schema << "id:int,";      auto incremento manejado x el sf, no seguro si se mantedra asi
    //for (size_t i = 0; i < s->values.size(); i++) {
    //    if (i > 0) schema << ",";
    //    schema << s->values[i].first << ":" << s->values[i].second;
    //}
    //schema << "\n";
    //schema.close();

    
    SequentialFile<int> sf("archivos/"+s->tabla+".dat","archivos/"+s->tabla+"_aux.dat", 50);

    std::string line;
    int count = 0;

    while (std::getline(csv, line)) {
        if (line.empty()) continue;

        std::vector<std::string> cols;
        std::stringstream ss(line);
        std::string col;
        while (std::getline(ss, col, ',')) {
            col.erase(0, col.find_first_not_of(" \t\r\n"));
            col.erase(col.find_last_not_of(" \t\r\n") + 1);
            cols.push_back(col);
        }

        
        //if (cols.size() != s->values.size()) {
        //    std::cerr << "Fila incorrecta\n";  X si hay filas faltantes, solo se salta
        //    continue;
        //}

        
        //std::string data_str = "";
        //for (size_t i = 0; i < cols.size(); i++) {
        //    if (i > 0) data_str += "|";
        //    data_str += cols[i];
        //}

        //sf.add_auto(data_str);
        count++;
    }

    csv.close();
    std::cout << "Tabla '" << s->tabla << "' creada con " << count << " registros\n";
}
 

///////////////////////////////////////////////////////////////////////////////////

