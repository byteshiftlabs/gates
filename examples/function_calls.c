// Test file for function call support

// Simple function to add two numbers
int add(int a, int b) {
    return a + b;
}

// Function to multiply two numbers
int multiply(int x, int y) {
    return x * y;
}

// Function using function calls in expressions
int calculate(int a, int b, int c) {
    int result = add(a, b);
    return result;
}

// Function with nested function calls
int complex_calc(int x, int y) {
    return add(multiply(x, 2), multiply(y, 3));
}

// Function with function call in return statement
int direct_return(int a, int b) {
    return add(a, b);
}

// Function with function call in if condition
int conditional_call(int a, int b) {
    if (add(a, b) > 10) {
        return 1;
    }
    return 0;
}

// Function with standalone function call statement
int standalone_call_test(int x) {
    add(x, 5);  // Call without using return value
    return x;
}

// Main function demonstrating various scenarios
int main() {
    int x = 5;
    int y = 10;
    int z = 0;
    
    // Scenario 1: Assignment from function call
    z = add(x, y);
    
    // Scenario 2: Function call in expression
    z = add(x, y) + 3;
    
    // Scenario 3: Nested function calls
    z = add(add(x, 2), add(y, 3));
    
    // Scenario 4: Function call in while condition
    while (add(x, y) < 100) {
        x = x + 1;
    }
    
    // Scenario 5: Function call in for loop
    int i = 0;
    for (i = 0; i < add(5, 5); i = i + 1) {
        z = z + 1;
    }
    
    // Scenario 6: Standalone function call
    multiply(x, y);
    
    return z;
}
