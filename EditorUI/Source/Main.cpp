#include <stdio.h>

int ExportFn();

int main(){
    printf("Exported function returned %d!", ExportFn());
    return 0;
}
