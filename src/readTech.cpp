#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <variant>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cstring>

#include "ord/readTech.h"

using namespace std;


// Value 可以是 int、double、string，或各種 vector
using Value = variant<
    int,
    double,
    string,
    vector<int>,
    vector<double>,
    vector<string>
>;

// 整個 technology 裡面的所有 key-vale 對
using TechMap = unordered_map<string, Value>;

// Helper #1：trim 前後空白
static string trim(const string &s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// Helper #2：判斷純整數字串
static bool isIntegerString(const string &s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0]=='+'||s[0]=='-') i = 1;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i)
        if (!isdigit(s[i])) return false;
    return true;
}

// Helper #3（更新版）：判斷浮點數字串（含小數點或科學計數法）
static bool isDoubleString(const string &s) {
    if (s.empty()) return false;
    size_t i = 0;
    // optional sign
    if (s[i]=='+' || s[i]=='-') ++i;
    bool hasDigit = false, hasDot = false, hasExp = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (isdigit(c)) {
            hasDigit = true;
            continue;
        }
        if (c == '.' && !hasDot && !hasExp) {
            hasDot = true;
            continue;
        }
        if ((c == 'e' || c == 'E') && hasDigit && !hasExp) {
            hasExp = true;
            // exponent 部分
            ++i;
            if (i < s.size() && (s[i]=='+'||s[i]=='-')) ++i;
            // e 或 E 後至少要有一個 digit
            if (i >= s.size() || !isdigit(s[i])) return false;
            // consume all remaining digits
            while (i < s.size() && isdigit(s[i])) ++i;
            return (i == s.size());
        }
        // 其他都不合法
        return false;
    }
    // 沒有遇到 exp，但是有 digit 且有小數點，就當作 double
    return hasDigit && hasDot;
}

// Helper to extract a value or throw
template<typename T>
static T getValue(const TechMap& m, const string& key) {
    auto it = m.find(key);
    if (it == m.end()) {
        throw runtime_error("Missing technology key: " + key);
    }
    return get<T>(it->second);
}

void writeTechnology(ofstream& techTempFile, const TechMap& techMap) {
    techTempFile << "UNITS\n";

    // Length (DATABASE MICRONS)
    {
        string unit = getValue<string>(techMap, "unitLengthName");
        int precision = getValue<int>(techMap, "lengthPrecision");
        string lefUnit = (unit == "micron" ? "MICRONS" : unit);
        techTempFile << "  DATABASE " << lefUnit << " " << precision << " ;\n";
    }

    // Time (TIME NANOSECONDS)
    {
        string unit = getValue<string>(techMap, "unitTimeName");
        int precision = getValue<int>(techMap, "timePrecision");
        string lefUnit;
        if      (unit == "ns") lefUnit = "NANOSECONDS";
        else if (unit == "us") lefUnit = "MICROSECONDS";
        else if (unit == "ps") lefUnit = "PICOSECONDS";
        else                    lefUnit = unit;
        techTempFile << "  TIME " << lefUnit << " " << precision << " ;\n";
    }

    // Voltage (VOLTS)
    {
        string unit = getValue<string>(techMap, "unitVoltageName");
        int precision = getValue<int>(techMap, "voltagePrecision");
        string lefUnit = (unit == "V" ? "VOLTS" : unit);
        techTempFile << "  VOLTAGE " << lefUnit << " " << precision << " ;\n";
    }

    // Current (MILLIAMPS)
    {
        string unit = getValue<string>(techMap, "unitCurrentName");
        int precision = getValue<int>(techMap, "currentPrecision");
        int factor = precision;
        if (unit == "ua" || unit == "uA") {
            // 1 mA = 1000 µA
            factor = precision * 1000;
        }
        techTempFile << "  CURRENT MILLIAMPS " << factor << " ;\n";
    }

    // Power (MILLIWATTS)
    {
        string unit = getValue<string>(techMap, "unitPowerName");
        int precision = getValue<int>(techMap, "powerPrecision");
        string lefUnit = (unit == "mw" ? "MILLIWATTS" : unit);
        techTempFile << "  POWER " << lefUnit << " " << precision << " ;\n";
    }

    // Resistance (OHMS)
    {
        string unit = getValue<string>(techMap, "unitResistanceName");
        int precision = getValue<int>(techMap, "resistancePrecision");
        int factor = precision;
        if (unit == "Mohm" || unit == "MOhm" || unit == "MOHM") {
            // 1 MΩ = 1e6 Ω
            factor = precision / 1000000;
        }
        techTempFile << "  RESISTANCE OHMS " << factor << " ;\n";
    }

    // Capacitance (PICOFARADS)
    {
        string unit = getValue<string>(techMap, "unitCapacitanceName");
        int precision = getValue<int>(techMap, "capacitancePrecision");
        string lefUnit = (unit == "pf" ? "PICOFARADS" : unit);
        techTempFile << "  CAPACITANCE " << lefUnit << " " << precision << " ;\n";
    }

    techTempFile << "END UNITS\n";
    techTempFile << endl;
}

void writeTile(ofstream& techTempFile, const TechMap& techMap) {
    // Tile name
    string name = getValue<string>(techMap, "blockName");
    // Dimensions
    double w = getValue<double>(techMap, "width");
    double h = getValue<double>(techMap, "height");

    techTempFile << "SITE " << name << "\n";
    techTempFile << "  CLASS CORE ;\n";
    techTempFile << "  SIZE " << w << " BY " << h << " ;\n";
    techTempFile << "END " << name << "\n";
    techTempFile << endl;
}

// Helper to check if key exists
static bool hasKey(const TechMap& m, const string& key) {
    return m.find(key) != m.end();
}

// Helper to get numeric value as double, whether int or double
static double getNumeric(const TechMap& m, const string& key) {
    auto it = m.find(key);
    if (it == m.end()) throw runtime_error("Missing numeric key: " + key);
    if (holds_alternative<double>(it->second)) {
        return get<double>(it->second);
    } else if (holds_alternative<int>(it->second)) {
        return static_cast<double>(get<int>(it->second));
    } else {
        throw runtime_error("Key '" + key + "' is not numeric");
    }
}

void writeLayer(ofstream& techTempFile, const TechMap& techMap) {
    string name = getValue<string>(techMap, "blockName");
    // Determine type
    // Determine masterslice layers (exact names or prefixes)
    static const vector<string> mastersExact = {"NWELL","PWELL","FIN","PO","PAD","ESD", "DNW"};
    static const vector<string> mastersPrefix = {"DIFF"};
    bool isCut     = hasKey(techMap, "cutTblSize");
    bool isImplant = (name == "PIMP" || name == "NIMP");
    // master if exact match or matches a prefix
    bool isMaster  = (find(mastersExact.begin(), mastersExact.end(), name) != mastersExact.end());
    for (const auto &pref : mastersPrefix) {
        if (!isMaster && name.rfind(pref, 0) == 0) {
            isMaster = true;
            break;
        }
    }
    bool isRouting = !isCut && !isImplant && !isMaster && !isCut && !isImplant && !isMaster;
    string lefType = isCut     ? "CUT"
                        : isImplant? "IMPLANT"
                        : isMaster ? "MASTERSLICE"
                        :            "ROUTING";

    techTempFile << "LAYER " << name << "\n";
    techTempFile << "  TYPE "  << lefType << " ;\n";

    // Pitch if exists
    if (hasKey(techMap, "pitch")) {
        double p = getNumeric(techMap, "pitch");
        techTempFile << "  PITCH " << p << " ;\n";
    }
    // Width or rect
    if (hasKey(techMap, "defaultWidth")) {
        double w = getNumeric(techMap, "defaultWidth");
        techTempFile << "  WIDTH " << w << " ;\n";
    }
    // Min width
    if (lefType == "ROUTING" && hasKey(techMap, "minWidth")) {
        double mw = getNumeric(techMap, "minWidth");
        techTempFile << "  MINWIDTH " << mw << " ;\n";
    }
    // Spacing as generic
    if (hasKey(techMap, "minSpacing") && !isMaster) {
        double sp = getNumeric(techMap, "minSpacing");
        techTempFile << "  SPACING " << sp << " ;\n";
    }

    techTempFile << "END " << name << "\n\n";
    techTempFile << endl;
}


// Write a LEF VIA block from ContactCode TechMap
void writeContactCode(ofstream& techTempFile, const TechMap& techMap) {
    // Name and layers
    string name       = getValue<string>(techMap, "blockName");
    string lowerLayer = getValue<string>(techMap, "lowerLayer");
    string cutLayer   = getValue<string>(techMap, "cutLayer");
    string upperLayer = getValue<string>(techMap, "upperLayer");

    // Dimensions and counts
    double cutW       = getNumeric(techMap, "cutWidth");
    double cutH       = getNumeric(techMap, "cutHeight");
    double minSpacing = getNumeric(techMap, "minCutSpacing");
    int rows          = static_cast<int>(getNumeric(techMap, "minNumRows"));
    int cols          = static_cast<int>(getNumeric(techMap, "minNumCols"));

    // Enclosure and resistance
    double encW_low  = getNumeric(techMap, "lowerLayerEncWidth");
    double encH_low  = getNumeric(techMap, "lowerLayerEncHeight");
    double encW_high = getNumeric(techMap, "upperLayerEncWidth");
    double encH_high = getNumeric(techMap, "upperLayerEncHeight");
    double nomRes    = getNumeric(techMap, "unitNomResistance");

    techTempFile << "VIA " << name << "\n";
    techTempFile << "  LAYER " << lowerLayer << " ;\n";
    techTempFile << "    RECT "
                 << -(encW_low + cutW/2) << " " << -(encH_low + cutH/2) << " "
                 <<  (encW_low + cutW/2) << " " <<  (encH_low + cutH/2) << " ;\n";
    techTempFile << "  LAYER " << cutLayer << " ;\n";
    techTempFile << "    RECT "
                 << -cutW/2 << " " << -cutH/2 << " "
                 <<  cutW/2 << " " <<  cutH/2 << " ;\n";
    techTempFile << "  LAYER " << upperLayer << " ;\n";
    techTempFile << "    RECT "
                 << -(encW_high + cutW/2) << " " << -(encH_high + cutH/2) << " "
                 <<  (encW_high + cutW/2) << " " <<  (encH_high + cutH/2) << " ;\n";
    techTempFile << "  RESISTANCE " << nomRes << " ;\n";
    techTempFile << "END " << name << "\n\n";
    techTempFile << endl;
}



// 處理一整串 "(...)" 變成對應的 vector<int/double/string>
static void parseVector(const string &key, const string &raw, TechMap &techMap) {
    auto p1 = raw.find('(');
    auto p2 = raw.rfind(')');
    string inner = raw.substr(p1+1, p2-p1-1);
    vector<string> items;
    string tok;
    stringstream ss(inner);
    while (getline(ss, tok, ',')) {
        items.push_back(trim(tok));
    }
    bool allNum = true;
    for (auto &it : items) {
        if (it.size()>=2 && it.front()=='"' && it.back()=='"') {
            allNum = false;
            break;
        }
        string cp = it;
        if (cp[0]=='+'||cp[0]=='-') cp = cp.substr(1);
        bool localNum = true, dot=false;
        for (char c : cp) {
            if (c=='.') dot = true;
            else if (!isdigit(c)) { localNum = false; break; }
        }
        if (!localNum) { allNum = false; break; }
    }
    if (allNum) {
        bool anyDot = false;
        for (auto &it : items)
            if (it.find('.') != string::npos) { anyDot = true; break; }
        if (anyDot) {
            vector<double> vd;
            for (auto &it : items) vd.push_back(stod(it));
            techMap[key] = move(vd);
        } else {
            vector<int> vi;
            for (auto &it : items) vi.push_back(stoi(it));
            techMap[key] = move(vi);
        }
    } else {
        vector<string> vs;
        for (auto &it : items) {
            if (it.size()>=2 && it.front()=='"' && it.back()=='"')
                vs.push_back(it.substr(1, it.size()-2));
            else
                vs.push_back(it);
        }
        techMap[key] = move(vs);
    }
}

bool readTech(Tcl_Interp* interp, const string& techPath, const string& techTempFile) {
    if (!interp) {
        cerr << "Tcl interpreter is not initialized." << endl;
        return false;
    }
    ifstream file(techPath);
    if (!file.is_open()) {
        cerr << "Failed to open technology file: " << techPath << endl;
        return false;
    }

    // rewrite techTempFile
    ofstream tempFile(techTempFile);
    if (!tempFile.is_open()) {
        cerr << "Failed to open temporary file: " << techTempFile << endl;
        return false;
    }
    tempFile << "VERSION 5.8 ;" << endl;
    tempFile << "BUSBITCHARS \"[]\" ;" << endl;

    enum class State { technology, tile, layer, contactCode, designRule, densityRule, none };
    State state = State::none;

    TechMap techMap;
    string line;

    bool inVector = false;
    string vecKey, vecRaw;

    while (getline(file, line)) {
        if (inVector) {
            string t = trim(line);
            vecRaw += " " + t;
            if (t.find(')') != string::npos) {
                parseVector(vecKey, vecRaw, techMap);
                inVector = false;
            }
            continue;
        }

        if (state == State::none) {
            if (line.find("Technology") != string::npos) {
                state = State::technology;
            } 
            else if (line.find("Tile") != string::npos) {
                state = State::tile;
                auto p1 = line.find_first_of('"');
                auto p2 = line.find_last_of('"');
                techMap["blockName"] = line.substr(p1+1, p2-p1-1);
            }
            else if (line.find("Layer") != string::npos) {
                state = State::layer;
                auto p1 = line.find_first_of('"');
                auto p2 = line.find_last_of('"');
                techMap["blockName"] = line.substr(p1+1, p2-p1-1);
            } 
            else if (line.find("ContactCode") != string::npos) {
                state = State::contactCode;
                auto p1 = line.find_first_of('"');
                auto p2 = line.find_last_of('"');
                techMap["blockName"] = line.substr(p1+1, p2-p1-1);
            } 
            else if (line.find("DesignRule") != string::npos) {
                state = State::designRule;
            } 
            else if (line.find("DensityRule") != string::npos) {
                state = State::densityRule;
            }
        }
        else if (line.find('}') != string::npos) {
            if (state == State::technology) {
                writeTechnology(tempFile, techMap);
            }
            else if (state == State::tile) {
                writeTile(tempFile, techMap);
            }
            else if (state == State::layer) {
                writeLayer(tempFile, techMap);
            }
            else if (state == State::contactCode) {
                writeContactCode(tempFile, techMap);
            }
            else if (state == State::designRule) {

            }
            else if (state == State::densityRule) {

            }

            state = State::none;
            techMap.clear();
        }
        else {
            auto pos = line.find('=');
            if (pos == string::npos) continue;

            string key = trim(line.substr(0, pos));
            string raw = trim(line.substr(pos+1));

            if (!raw.empty() && raw.front()=='(') {
                vecKey = key;
                vecRaw = raw;
                if (raw.back()==')') {
                    parseVector(vecKey, vecRaw, techMap);
                } 
                else {
                    inVector = true;
                }
                continue;
            }

            if (raw.size()>=2 && raw.front()=='"' && raw.back()=='"') {
                techMap[key] = raw.substr(1, raw.size()-2);
            }
            else if (isIntegerString(raw)) {
                techMap[key] = stoi(raw);
            }
            else if (isDoubleString(raw)) {
                techMap[key] = stod(raw);
            }
            else {
                techMap[key] = raw;
            }
        }

    }

    string readLefCmd = "read_lef " + techTempFile;
    if (Tcl_Eval(interp, readLefCmd.c_str()) != TCL_OK) {
        cerr << "Error executing read_lef command: " << Tcl_GetStringResult(interp) << endl;
        return false;
    }

    return true;
}

// Tcl_Obj 版的指令實作
int ReadTechCmd(ClientData /*clientData*/,
                       Tcl_Interp *interp,
                       int objc,
                       Tcl_Obj *const objv[])
{
    // 參數檢查：要么 2 個參數 (命令 + techPath)，要么 4 個參數 (命令 + techPath + "-t" + tempFile)
    if (!(objc == 2 || (objc == 4 &&
                       strcmp(Tcl_GetString(objv[2]), "-t") == 0)))
    {
        Tcl_WrongNumArgs(interp, 1, objv, "technology_file.tf ?-t tempFile.lef?");
        return TCL_ERROR;
    }

    // 取出 technology file 路徑
    std::string techPath = Tcl_GetString(objv[1]);

    // 預設 tempFile
    std::string tempFile = "techTemp.lef";
    if (objc == 4) {
        tempFile = Tcl_GetString(objv[3]);
    }

    // 呼叫你原本的函式
    bool ok = readTech(interp, techPath, tempFile);
    if (!ok) {
        // readTech 已經把錯誤訊息放到 Tcl result 了
        return TCL_ERROR;
    }

    // 回傳成功
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        ("Loaded technology -> " + tempFile).c_str(), -1));
    return TCL_OK;
}