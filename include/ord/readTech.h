#include <iostream>
#include <filesystem>
#include <string>
#include <tcl.h>


using std::string;

/**
 * @brief read technology file (tf format)
 *
 * @param interp: tcl interpreter, must be initialized by openroad before calling this function 
 * @param techPath 
 * @param techTempFile: temporary file to store the parsed technology data (lef foramt)
 * @return true: read successfully
 * @return false: read failed
 */
bool readTech(Tcl_Interp* interp, const string& techPath, const string& techTempFile = "techTemp.lef");


int ReadTechCmd(ClientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);