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

    auto printHeader = [&]() {
        cout << "id\t";
        for (auto& col : cols) cout << col.first << "\t";
        cout << "\n" << string(40,'-') << "\n";
    };

    auto printRecord = [&](const Record<int>& r) {
        cout << r.key << "\t";
        int off = 0;
        for (auto& col : cols) {
            cout << deserializeField(r.data + off, col.second) << "\t";
            off += getTypeSize(col.second);
        }
        cout << "\n";
    };


    auto btreeSearch = [&](const string& col_name, const string& col_tipo,int col_offset, const string& val_str,BinaryOp op) -> vector<RID> {
        string btree_path = "archivos/"+s->table+"_"+col_name+".btree";
        Disk disk(btree_path);
        vector<RID> rids;

        if (col_tipo == "INT") {
            BPlusTree<int> btree(disk);
            int val = stoi(val_str);
            if (op == EQUAL_OP) {
                rids = btree.searchAll(val);
            } else {
                int lo = (op == GER_OP || op == GEQ_OP) ? val : INT_MIN;
                int hi = (op == LES_OP || op == LEQ_OP) ? val : INT_MAX;
                rids = btree.rangeSearch(lo, hi);
            }
        } else if (col_tipo == "FLOAT") {
            BPlusTree<float> btree(disk);
            float val = stof(val_str);
            if (op == EQUAL_OP) {
                rids = btree.searchAll(val);
            } else {
                float lo = (op == GER_OP || op == GEQ_OP) ? val : -FLT_MAX;
                float hi = (op == LES_OP || op == LEQ_OP) ? val :  FLT_MAX;
                rids = btree.rangeSearch(lo, hi);
            }
        } else if (col_tipo.find("CHAR") != string::npos) {
            BPlusTree<FixedString<64>> btree(disk);
            FixedString<64> val(val_str);
            if (op == EQUAL_OP) {
                rids = btree.searchAll(val);
            } else {
                // CHAR range search
                FixedString<64> lo, hi;
                if (op == GER_OP || op == GEQ_OP) lo = val;
                else memset(lo.data, 0, 64);
                if (op == LES_OP || op == LEQ_OP) hi = val;
                else memset(hi.data, 127, 64);
                rids = btree.rangeSearch(lo, hi);
            }
        }
        return rids;
    };

    auto scanFiltrar = [&](const string& col_tipo, int col_offset,const string& val_str, BinaryOp op) {
        auto records = sf.scanAll();
        for (auto& r : records) {
            string val_rec = deserializeField(r.data + col_offset, col_tipo);
            bool cumple = false;

            if (col_tipo == "INT") {
                int a = stoi(val_rec), b2 = stoi(val_str);
                switch(op) {
                    case EQUAL_OP: cumple = a == b2; break;
                    case LEQ_OP:   cumple = a <= b2; break;
                    case LES_OP:   cumple = a <  b2; break;
                    case GEQ_OP:   cumple = a >= b2; break;
                    case GER_OP:   cumple = a >  b2; break;
                    default: break;
                }
            } else if (col_tipo == "FLOAT") {
                float a = stof(val_rec), b2 = stof(val_str);
                switch(op) {
                    case EQUAL_OP: cumple = a == b2; break;
                    case LEQ_OP:   cumple = a <= b2; break;
                    case LES_OP:   cumple = a <  b2; break;
                    case GEQ_OP:   cumple = a >= b2; break;
                    case GER_OP:   cumple = a >  b2; break;
                    default: break;
                }
            } else if (col_tipo.find("CHAR") != string::npos) {
                if (op == EQUAL_OP) cumple = (val_rec == val_str);
                else if (op == LES_OP) cumple = (val_rec <  val_str);
                else if (op == LEQ_OP) cumple = (val_rec <= val_str);
                else if (op == GER_OP) cumple = (val_rec >  val_str);
                else if (op == GEQ_OP) cumple = (val_rec >= val_str);
            }

            if (cumple) printRecord(r);
        }
    };

    //sin condicion
    if (s->where_cond == nullptr) {
        cout << "SequentialFile" << "\n";
        auto records = sf.scanAll();
        printHeader();
        for (auto& r : records) printRecord(r);
        cout << "Total: " << records.size() << " registros\n";

    } else if (BinaryExp* b = dynamic_cast<BinaryExp*>(s->where_cond)) {
        IdExp* campo = dynamic_cast<IdExp*>(b->left);
        if (!campo) { cerr << "WHERE mal formado\n"; return; }

        string val_str = getExpValue(b->right);
        printHeader();

        if (campo->value == "id") {
            cout << "SequentialFile  binary search" << "\n";
            if (b->op == EQUAL_OP) {
                try {
                    auto [rec, ios] = sf.search_key(stoi(val_str));
                    printRecord(rec);
                } catch (...) { cout << "No encontrado\n"; }
            }
        } else {
            int col_offset = 0;
            string col_tipo = "";
            for (auto& col : cols) {
                if (col.first == campo->value) { col_tipo = col.second; break; }
                col_offset += getTypeSize(col.second);
            }
            if (col_tipo.empty()) { cerr << "Columna no existe\n"; return; }

            auto [idx_type, idx_col_tipo] = getIndexInfo(s->table, campo->value);

            if (idx_type == "btree") {
                cout << "BTreegod" << "\n";
                auto rids = btreeSearch(campo->value, col_tipo, col_offset, val_str, b->op);
                for (auto& rid : rids) {
                    RecordPointer ptr(false, rid.page_id, rid.slot);
                    printRecord(sf.readByPointer(ptr));
                }
            } else {
                cout << "Sequential" << "\n";
                scanFiltrar(col_tipo, col_offset, val_str, b->op);
            }
        }

    } else if (BetweenEXp* be = dynamic_cast<BetweenEXp*>(s->where_cond)) {
        IdExp* campo = dynamic_cast<IdExp*>(be->id);
        if (!campo) { cerr << "BETWEEN mal formado\n"; return; }

        string low_str  = getExpValue(be->low);
        string high_str = getExpValue(be->high);
        printHeader();

        if (campo->value == "id") {
            cout << "SequentialFile rangeSearch" << "\n";
            auto records = sf.rangeSearch(stoi(low_str), stoi(high_str));
            for (auto& r : records) printRecord(r);

        } else {
            int col_offset = 0;
            string col_tipo = "";
            for (auto& col : cols) {
                if (col.first == campo->value) { col_tipo = col.second; break; }
                col_offset += getTypeSize(col.second);
            }
            if (col_tipo.empty()) { cerr << "Columna no existe\n"; return; }

            auto [idx_type, idx_col_tipo] = getIndexInfo(s->table, campo->value);

            if (idx_type == "btree") {
                cout << "BTree rangeSearch" << "\n";
                string btree_path = "archivos/"+s->table+"_"+campo->value+".btree";
                Disk disk(btree_path);
                vector<RID> rids;

                if (col_tipo == "INT") {
                    BPlusTree<int> btree(disk);
                    rids = btree.rangeSearch(stoi(low_str), stoi(high_str));
                } else if (col_tipo == "FLOAT") {
                    BPlusTree<float> btree(disk);
                    rids = btree.rangeSearch(stof(low_str), stof(high_str));
                } else if (col_tipo.find("CHAR") != string::npos) {
                    BPlusTree<FixedString<64>> btree(disk);
                    rids = btree.rangeSearch(FixedString<64>(low_str), FixedString<64>(high_str));
                }

                for (auto& rid : rids) {
                    RecordPointer ptr(false, rid.page_id, rid.slot);
                    printRecord(sf.readByPointer(ptr));
                }
            } else {
                cout << "SequentialFile BETWEEN\n";
                auto records = sf.scanAll();
                for (auto& r : records) {
                    string val_rec = deserializeField(r.data + col_offset, col_tipo);
                    bool cumple = false;
                    if (col_tipo == "INT") {
                        int v = stoi(val_rec);
                        cumple = (v >= stoi(low_str) && v <= stoi(high_str));
                    } else if (col_tipo == "FLOAT") {
                        float v = stof(val_rec);
                        cumple = (v >= stof(low_str) && v <= stof(high_str));
                    } else if (col_tipo.find("CHAR") != string::npos) {
                        cumple = (val_rec >= low_str && val_rec <= high_str);
                    }
                    if (cumple) printRecord(r);
                }
            }
        }
    }
}

void EVALVisitor::visit(CreateTableStmt* s) {
    // 1. Guardar schema
    ofstream schema("archivos/"+s->tabla+".schema");
    bool first = true;
    for (auto& col : s->columns) {
        if (!first) schema << ",";
        schema << col.first << ":" << col.second;
        first = false;
    }
    schema << "\n";
    schema.close();

    // 2. Abrir CSV
    ifstream csv(s->path);
    if (!csv.is_open()) {
        cerr << "No se pudo abrir: " << s->path << "\n";
        return;
    }

    // saltar header
    string header;
    getline(csv, header);

    // 3. Calcular total bytes del data (sin columnas INCREMENTAL)
    int total = 0;
    for (auto& col : s->columns) {
        if (col.second.find("INCREMENTAL") != string::npos) continue;
        int sz = getTypeSize(getTipo(col.second));
        if (sz == 0) {
            cerr << "Tipo desconocido: " << col.second << "\n";
            return;
        }
        total += sz;
    }

    if (total > 64) {
        cerr << "Error: schema supera 64 bytes\n";
        return;
    }

    // 4. Crear SF e insertar registros
    SequentialFile<int> sf("archivos/"+s->tabla+".dat","archivos/"+s->tabla+"_aux.dat", 50);

    string line;
    int count = 0;

    while (getline(csv, line)) {
        if (line.empty()) continue;
        line.erase(remove(line.begin(), line.end(), '\r'), line.end());

        // separar valores del CSV
        vector<string> vals;
        stringstream ss(line);
        string v;
        while (getline(ss, v, ',')) {
            v.erase(0, v.find_first_not_of(" \t"));
            v.erase(v.find_last_not_of(" \t") + 1);
            vals.push_back(v);
        }

        // serializar al buffer
        char buffer[64] = {0};
        int offset = 0;
        int csv_i = 0;

        for (auto& col : s->columns) {
            if (col.second.find("INCREMENTAL") != string::npos) continue;
            if (csv_i >= (int)vals.size()) break;

            string tipo = getTipo(col.second);
            serializeField(buffer + offset, vals[csv_i], tipo);
            offset += getTypeSize(tipo);
            csv_i++;
        }

        sf.add(buffer, total);
        count++;
    }

    csv.close();
    cout << "Tabla '" << s->tabla << "' creada con " << count << " registros\n";
}

void EVALVisitor::visit(InsertStmt* s) {
    auto cols = leerSchema("archivos/"+s->table_name+".schema");

    // Calcular tamaño total
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
        else if (IntExp* ne = dynamic_cast<IntExp*>(e))
            val = to_string(ne->value);

        serializeField(buffer + offset, val, cols[i].second);
        offset += getTypeSize(cols[i].second);
        i++;
    }

    SequentialFile<int> sf("archivos/"+s->table_name+".dat","archivos/"+s->table_name+"_aux.dat", 50);

    // Usar add con buffer binario directo
    Record<int> rec;
    // autoincremento lo maneja el SF internamente
    sf.add(buffer, total); // limpio, autoincrementa
    cout << "Insertado en '" << s->table_name << "'\n";

}

void EVALVisitor::visit(CreateIndexStmt* s) {
    if (s->op != BTREE) {
        cerr << "Solo BTREE implementado por ahora\n";
        return;
    }

    // 1. Leer schema y encontrar columna
    auto cols = leerSchema("archivos/"+s->tableName+".schema");
    int col_offset = 0;
    string col_tipo = "";
    for (auto& col : cols) {
        if (col.first == s->indexName) {
            col_tipo = col.second;
            break;
        }
        col_offset += getTypeSize(col.second);
    }

    if (col_tipo.empty()) {
        cerr << "Columna '" << s->indexName << "' no existe\n";
        return;
    }

    // 2. Abrir SF
    SequentialFile<int> sf("archivos/"+s->tableName+".dat",
                           "archivos/"+s->tableName+"_aux.dat", 50);
    auto records = sf.scanAllWithPtr();

    // 3. Crear BTree segun tipo
    string btree_path = "archivos/"+s->tableName+"_"+s->indexName+".btree";
    Disk disk(btree_path);

    if (col_tipo == "INT") {
        // [BTree<int>]
        BPlusTree<int> btree(disk);
        for (auto& [rec, ptr] : records) {
            int val; memcpy(&val, rec.data + col_offset, sizeof(int));
            btree.insert(val, RID{(int)ptr.page_id, ptr.record_idx});
        }
    } else if (col_tipo == "FLOAT") {
        // [BTree<float>]
        BPlusTree<float> btree(disk);
        for (auto& [rec, ptr] : records) {
            float val; memcpy(&val, rec.data + col_offset, sizeof(float));
            btree.insert(val, RID{(int)ptr.page_id, ptr.record_idx});
        }
    } else if (col_tipo.find("CHAR") != string::npos) {
        // [BTree<FixedString<64>>]
        BPlusTree<FixedString<64>> btree(disk);
        for (auto& [rec, ptr] : records) {
            FixedString<64> val(rec.data + col_offset);
            btree.insert(val, RID{(int)ptr.page_id, ptr.record_idx});
        }
    } else {
        cerr << "Tipo '" << col_tipo << "' no soportado para indice\n";
        return;
    }

    // 4. Registrar en .indexes
    ofstream idx("archivos/"+s->tableName+".indexes", ios::app);
    idx << s->indexName << ":btree:" << col_tipo << "\n";
    idx.close();

    cout << "[CREATE INDEX BTREE] sobre '" << s->indexName<< "' tipo=" << col_tipo<< " en tabla '" << s->tableName << "'\n";
}


///////////////////////////////////////////////////////////////////////////////////
//Helpers

int getTypeSize(const string& tipo) {
    if (tipo == "INT")    return 4;
    if (tipo == "FLOAT")  return 4;
    if (tipo == "DOUBLE") return 8;
    if (tipo.find("CHAR(") != string::npos) {
        size_t start = tipo.find('(') + 1;
        size_t end   = tipo.find(')');
        return stoi(tipo.substr(start, end - start));
    }
    return 0;
}

void serializeField(char* buf, const string& val, const string& tipo) {
    if (tipo == "INT") {
        int v = stoi(val);
        memcpy(buf, &v, 4);
    } else if (tipo == "FLOAT") {
        float v = stof(val);
        memcpy(buf, &v, 4);
    } else if (tipo == "DOUBLE") {
        double v = stod(val);
        memcpy(buf, &v, 8);
    } else if (tipo.find("CHAR(") != string::npos) {
        size_t start = tipo.find('(') + 1;
        size_t end   = tipo.find(')');
        int n = stoi(tipo.substr(start, end - start));
        memset(buf, 0, n);
        strncpy(buf, val.c_str(), n-1);
    }
}

string deserializeField(const char* buf, const string& tipo) {
    if (tipo == "INT") {
        int v; memcpy(&v, buf, 4);
        return to_string(v);
    } else if (tipo == "FLOAT") {
        float v; memcpy(&v, buf, 4);
        return to_string(v);
    } else if (tipo == "DOUBLE") {
        double v; memcpy(&v, buf, 8);
        return to_string(v);
    } else if (tipo.find("CHAR(") != string::npos) {
        size_t start = tipo.find('(') + 1;
        size_t end   = tipo.find(')');
        int n = stoi(tipo.substr(start, end - start));
        return string(buf, strnlen(buf, n));
    }
    return "";
}

vector<pair<string,string>> leerSchema(const string& path) {
    vector<pair<string,string>> cols;
    ifstream f(path);
    string line;
    getline(f, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();

    stringstream ss(line);
    string token;
    while (getline(ss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);

        // separar nombre y tipo por ':'
        size_t pos = token.find(':');
        if (pos == string::npos) continue;
        
        string nombre = token.substr(0, pos);
        string resto  = token.substr(pos + 1); // "INT PK INCREMENTAL" o "INT" o "CHAR(10)"

        // quedarse solo con la primera palabra = el tipo
        // pero CHAR(10) tiene parentesis, no espacios dentro
        string tipo = resto.substr(0, resto.find(' '));

        // limpiar espacios
        nombre.erase(0, nombre.find_first_not_of(" \t"));
        nombre.erase(nombre.find_last_not_of(" \t") + 1);
        tipo.erase(0, tipo.find_first_not_of(" \t"));
        tipo.erase(tipo.find_last_not_of(" \t") + 1);

        // saltar columnas incrementales (no van al data buffer)
        if (resto.find("INCREMENTAL") != string::npos) continue;

        cols.push_back({nombre, tipo});
    }
    return cols;
}

string getTipo(const string& raw) {
    string tipo = raw.substr(0, raw.find(' '));
    tipo.erase(0, tipo.find_first_not_of(" \t"));
    tipo.erase(tipo.find_last_not_of(" \t") + 1);
    return tipo;
}

string getIndex(const string& tabla, const string& columna) {
    ifstream f("archivos/"+tabla+".indexes");
    if (!f.is_open()) return "";
    string line;
    while (getline(f, line)) {
        auto pos = line.find(':');
        if (line.substr(0, pos) == columna)
            return line.substr(pos+1);
    }
    return "";
}

pair<string,string> getIndexInfo(const string& tabla, const string& columna) {
    ifstream f("archivos/"+tabla+".indexes");
    if (!f.is_open()) return {"",""};
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        string col, idx, tipo;
        getline(ss, col, ':');
        getline(ss, idx, ':');
        getline(ss, tipo, ':');
        if (col == columna) return {idx, tipo};
    }
    return {"",""};
}

// helper para extraer valor como string desde cualquier Exp
string getExpValue(Exp* e) {
    if (IntExp* ie = dynamic_cast<IntExp*>(e))       return to_string(ie->value);
    if (FloatExp* fe = dynamic_cast<FloatExp*>(e))   return to_string(fe->value);
    if (StringExp* se = dynamic_cast<StringExp*>(e)) return se->value;
    return "";
}