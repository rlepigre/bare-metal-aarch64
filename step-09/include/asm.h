#pragma once

#define read_sysreg(r, var) { asm volatile("mrs %[reg], " #r : [reg] "=r" (var)); }