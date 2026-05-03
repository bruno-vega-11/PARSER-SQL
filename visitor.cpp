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
    auto cols = leerSchema("archivos/"+s->table+".schema");

    vector<int> offsets;
    int offset = 0;
    for (auto& col : cols) {
        offsets.push_back(offset);
        offset += getTypeSize(col.second);
    }

    SequentialFile<int> sf("archivos/"+s->table+".dat","archivos/"+s->table+"_aux.dat", 50);

    if (s->where_cond == nullptr) {
        auto records = sf.scanAll();

        cout << "id\t";
        for (auto& col : cols) cout << col.first << "\t";
        cout << "\n";
        cout << string(40, '-') << "\n";

        for (auto& r : records) {
            cout << r.key << "\t";
            for (int i = 0; i < cols.size(); i++) {
                string val = deserializeField(r.data + offsets[i], cols[i].second);
                cout << val << "\t";
            }
            cout << "\n";
        }
        cout << "Total: " << records.size() << " registros\n";
    } else {
        // WHERE
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

        //sf.add(data_str);
        count++;
    }

    csv.close();
    std::cout << "Tabla '" << s->tabla << "' creada con " << count << " registros\n";
}
 
void EVALVisitor::visit(InsertStmt* s) {
    auto cols = leerSchema("archivos/"+s->table_name+".schema");

    // Calcular tamaño total del data
    int total = 0;
    for (auto& col : cols) total += getTypeSize(col.second);

    if (total > 64) {
        cerr << "Error: schema supera 64 bytes\n";
        return;
    }

    // Serializar valores al buffer
    char buffer[64] = {0};
    int offset = 0;
    int i = 0;
    for (Exp* e : s->values) {
        string val = "";
        if (StringExp* se = dynamic_cast<StringExp*>(e))
            val = se->value;
        else if (NumberExp* ne = dynamic_cast<NumberExp*>(e))
            val = to_string(ne->value);

        serializeField(buffer + offset, val, cols[i].second);
        offset += getTypeSize(cols[i].second);
        i++;
    }

    SequentialFile<int> sf("archivos/"+s->table_name+".dat",
                           "archivos/"+s->table_name+"_aux.dat", 50);

    // add con buffer directo
    Record<int> rec;
    rec.key = 0; // el sf lo autoincrementa
    memcpy(rec.data, buffer, 64);
    sf.add(rec); // usa el add(string payload)? no...
}


///////////////////////////////////////////////////////////////////////////////////
//Helpers

int getTypeSize(const string& tipo) {
    if (tipo == "int")    return 4;
    if (tipo == "float")  return 4;
    if (tipo == "double") return 8;
    if (tipo.find("char[") != string::npos) {
        // "char[10]" → substr desde pos 5 hasta ']'
        size_t start = tipo.find('[') + 1;
        size_t end   = tipo.find(']');
        string num = tipo.substr(start, end - start);
        cout << "DEBUG tipo='" << tipo << "' num='" << num << "'\n"; // ver qué llega
        return stoi(num);
    }
    return 0;
}

void serializeField(char* buf, const string& val, const string& tipo) {
    if (tipo == "int") {
        int v = stoi(val);
        memcpy(buf, &v, 4);
    } else if (tipo == "float") {
        float v = stof(val);
        memcpy(buf, &v, 4);
    } else if (tipo == "double") {
        double v = stod(val);
        memcpy(buf, &v, 8);
    } else if (tipo.find("char[") != string::npos) {
        int n = stoi(tipo.substr(5, tipo.size()-6));
        memset(buf, 0, n);
        strncpy(buf, val.c_str(), n-1);
    }
}

string deserializeField(const char* buf, const string& tipo) {
    if (tipo == "int") {
        int v; memcpy(&v, buf, 4);
        return to_string(v);
    } else if (tipo == "float") {
        float v; memcpy(&v, buf, 4);
        return to_string(v);
    } else if (tipo == "double") {
        double v; memcpy(&v, buf, 8);
        return to_string(v);
    } else if (tipo.find("char[") != string::npos) {
        int n = stoi(tipo.substr(5, tipo.size()-6));
        return string(buf, strnlen(buf, n));
    }
    return "";
}

vector<pair<string,string>> leerSchema(const string& path) {
    vector<pair<string,string>> cols;
    ifstream f(path);
    string line;
    getline(f, line);
    stringstream ss(line);
    string token;
    while (getline(ss, token, ',')) {
        auto pos = token.find(':');
        string nombre = token.substr(0, pos);
        string tipo   = token.substr(pos+1);
        cols.push_back({nombre, tipo});
    }
    return cols;
}