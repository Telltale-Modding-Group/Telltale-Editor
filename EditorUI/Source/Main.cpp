#include "Config.hpp"

U32 ExportFn();

int main(){
    TTE_LOG("Exported function returned %d! Running TTE Version %s\n", ExportFn(), TTE_VERSION);
    return 0;
}
