// src/main.cpp
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm> // for std::find

// OpenROAD headers
#include <tcl.h>
#include "ord/InitOpenRoad.hh"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "ord/Design.h"

// 使用 using-declarations 來簡化程式碼
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::ofstream;

// 函式：用於顯示如何執行程式
void print_usage(const char* prog_name) {
    cerr << "Usage: " << prog_name << " \\\n"
         << "    -weight <weightFile> \\\n"
         << "    -lib <libFile1> <libFile2> ... \\\n"
         << "    -lef <lefFile1> <lefFile2> ... \\\n"
         << "    -db <dbFile1> <dbFile2> ... \\\n"
         << "    -tf <tfFile1> <tfFile2> ... \\\n"
         << "    -sdc <sdcFile1> <sdcFile2> ... \\\n"
         << "    -v <verilogFile1> <verilogFile2> ... \\\n"
         << "    -def <defFile1> <defFile2> ... \\\n"
         << "    -out <outputName>" << endl;
}


int main(int argc, char **argv) {
    // 1. 參數解析 (Argument Parsing)
    // =================================================================
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    map<string, vector<string>> params;
    string current_flag;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg[0] == '-') { // 這是一個 flag (例如 -lib, -lef)
            current_flag = arg;
            params[current_flag] = {}; // 建立一個空的檔案列表
        } else { // 這是一個檔案或值
            if (current_flag.empty()) {
                cerr << "Error: File '" << arg << "' provided without a preceding flag." << endl;
                print_usage(argv[0]);
                return 1;
            }
            params[current_flag].push_back(arg);
        }
    }
    
    // =================================================================
    // ** 新增：檢查所有必要的 flag 是否都存在 **
    // =================================================================
    const vector<string> required_flags = {
        "-weight", "-lib", "-lef", "-tf", 
        "-sdc", "-v", "-def", "-out"
    };
    vector<string> missing_flags;

    for (const auto& flag : required_flags) {
        if (params.find(flag) == params.end()) {
            missing_flags.push_back(flag);
        }
    }

    if (!missing_flags.empty()) {
        cerr << "Error: The following mandatory parameters are missing:" << endl;
        for (const auto& flag : missing_flags) {
            cerr << "  " << flag << endl;
        }
        cerr << endl;
        print_usage(argv[0]);
        return 1;
    }
    
    // 檢查 -out 參數是否只有一個值
    if (params["-out"].size() != 1) {
        cerr << "Error: The '-out' parameter must be followed by exactly one output name." << endl;
        print_usage(argv[0]);
        return 1;
    }
    const string outputName = params["-out"][0];


    // 2. OpenROAD 初始化 (OpenROAD Initialization)
    // =================================================================
    Tcl_Interp* interp = Tcl_CreateInterp();
    if (Tcl_Init(interp) != TCL_OK) {
        cerr << "Tcl_Init error: " << Tcl_GetStringResult(interp) << endl;
        return 1;
    }

    cout << "Initializing OpenROAD..." << endl;
    auto tech   = std::make_unique<ord::Tech>(interp);
    auto design = std::make_unique<ord::Design>(tech.get());
    ord::OpenRoad::setOpenRoad(design->getOpenRoad());
    ord::initOpenRoad(interp, /*log=*/nullptr, /*metrics=*/nullptr, /*batch_mode=*/true);
    cout << "OpenROAD initialized successfully." << endl;

    auto eval = [&](const string &cmd) {
        cout << "[CMD] " << cmd << endl;
        if (Tcl_Eval(interp, cmd.c_str()) != TCL_OK) {
            cerr << "TCL Error: " << Tcl_GetStringResult(interp)
                 << "\nFailed to execute command: " << cmd << endl;
            return false;
        }
        return true;
    };
    
    eval("suppress_message STA 1257");


    // 3. 讀取輸入檔案 (Reading Input Files)
    // =================================================================
    
    auto process_files = [&](const string& flag, const string& tcl_command_base) {
        if (params.count(flag)) {
            for (const auto& file : params[flag]) {
                eval(tcl_command_base + " " + file);
            }
        }
    };
    
    process_files("-tf", "read_tech");
    process_files("-lef", "read_lef");
    process_files("-lib", "read_liberty");
    process_files("-v", "read_verilog");

    if (params.count("-v")) {
        eval("link_design top"); 
    }
    
    if (params.count("-def")) {
        const auto& def_files = params["-def"];
        if (!def_files.empty()) {
            eval("read_def -floorplan_initialize " + def_files[0]);
            for (size_t i = 1; i < def_files.size(); ++i) {
                eval("read_def " + def_files[i]);
            }
        }
    }
    
    process_files("-sdc", "read_sdc");

    if (params.count("-weight")) {
        cout << "Note: -weight parameter processing is not yet implemented in this flow." << endl;
    }
    if (params.count("-db")) {
        cout << "Note: Reading .db files via 'read_db' might conflict with building from source files." << endl;
        process_files("-db", "read_db");
    }

    cout << "All input files have been loaded." << endl;


    // 4. 執行設計流程 & 產生報告 (Run Flow & Generate Reports)
    // =================================================================
    
    // cout << "Generating reports..." << endl;
    // string report_command = "redirect " + outputName + ".list {";
    // report_command += "report_tns; ";
    // report_command += "report_power; ";
    // report_command += "report_design_area";
    // report_command += "}";
    // eval(report_command);


    // detailed placement
    cout << "Running detailed placement..." << endl;
    eval("detailed_placement");

    // 6. 寫出輸出檔案 (Writing Output Files)
    // =================================================================
    cout << "Writing output files..." << endl;
    
    eval("write_def " + outputName + ".def");
    eval("write_verilog " + outputName + ".v");

    ofstream list_file(outputName + ".list");
    if (!list_file.is_open()) {
        cerr << "Error: Unable to open output file: " << outputName << ".list" << endl;
        return 1;
    }
    list_file.close();

    cout << "\nFlow completed successfully!" << endl;
    cout << "Outputs generated: " << endl;
    cout << " -> " << outputName << ".list" << endl;
    cout << " -> " << outputName << ".def" << endl;
    cout << " -> " << outputName << ".v" << endl;

    return 0;
}