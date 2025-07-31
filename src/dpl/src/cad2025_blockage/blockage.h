#include <iostream>
#include "../infrastructure/Coordinates.h"
#include "dpl/Opendp.h"

namespace dpl {
class Node;
class Architecture;
class DetailedMgr;
class Network;
class dbBox;
class dbBTerm;
class dbInst;
class dbMaster;
class dbOrientType;
class dbSite;

class blockage
{
public:
    Node* parentNode;
    DbuX left_;
    DbuY bottom_;
    dbOrientType orient_;
    // Width and height.
    DbuX width_{0};
    DbuY height_{0};
    blockage(Node*);
};


}