#include <stdarg.h>

void a(va_list * ap) {}

int main() {
  va_list ap1, ap2;
  a(&ap1);
  ap2 = (va_list) ap1;
}
