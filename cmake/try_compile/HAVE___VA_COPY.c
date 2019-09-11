#include <stdarg.h>

int main() {
  va_list ap1,ap2;
  __va_copy(ap1,ap2);
}
