// CI validation input — each function here must produce analyzable VHDL.
// This file exercises the core supported C subset: arithmetic, conditionals,
// loops (while, for, nested), bitwise ops, break/continue, and return values.

int add(int a, int b) {
    int sum = a + b;
    return sum;
}

int bitwise_ops(int x, int y) {
    int a = x & y;
    int o = x | y;
    int xr = x ^ y;
    return a + o + xr;
}

int negate(int x) {
    return -x;
}

int max_val(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int while_sum(int n) {
    int sum = 0;
    int i = 0;
    while (i < n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

int nested_loops(int outer, int inner) {
    int total = 0;
    int i = 0;
    while (i < outer) {
        int j = 0;
        while (j < inner) {
            total = total + i + j;
            j = j + 1;
        }
        i = i + 1;
    }
    return total;
}

int for_loop(int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s = s + i;
    }
    return s;
}

int break_continue(int n) {
    int total = 0;
    int i = 0;
    while (i < n) {
        if (i == 3) {
            i = i + 1;
            continue;
        }
        if (total > 100) {
            break;
        }
        total = total + i;
        i = i + 1;
    }
    return total;
}

int comparison_return(int a, int b) {
    return a == b;
}
